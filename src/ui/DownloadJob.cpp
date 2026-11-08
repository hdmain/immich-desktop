#include "ui/DownloadJob.h"

#include "core/ImmichClient.h"
#include "ui/widgets/Toast.h"

#include <QCheckBox>
#include <QDir>
#include <QFileInfo>
#include <QLocale>
#include <QMessageBox>
#include <QPushButton>
#include <QStringList>
#include <QTimer>

namespace Aurora {

DownloadJob::DownloadJob(ImmichClient *client, QWidget *window, QObject *parent)
    : QObject(parent)
    , m_client(client)
    , m_window(window)
{
    connect(m_client, &ImmichClient::assetInfoLoaded, this, [this](const ImmichAsset &info) {
        if (info.id.isEmpty() || info.id != m_waitingForInfo || !isRunning())
            return;
        m_waitingForInfo.clear();
        Item &item = m_items[m_current];
        if (!info.fileName.isEmpty())
            item.asset.fileName = info.fileName;
        item.asset.fileSize = info.fileSize;
        download(item);
    });
    connect(m_client, &ImmichClient::assetInfoFailed, this,
            [this](const QString &assetId, const QString &) {
                if (assetId != m_waitingForInfo || !isRunning())
                    return;
                m_waitingForInfo.clear();
                download(m_items[m_current]); // Falls back to an id-based name.
            });
    connect(m_client, &ImmichClient::downloadProgress, this, &DownloadJob::handleProgress);
    connect(m_client, &ImmichClient::assetDownloaded, this,
            [this](const QString &assetId, const QString &) { handleFinished(assetId, true, {}); });
    connect(m_client, &ImmichClient::downloadFailed, this,
            [this](const QString &assetId, const QString &message) {
                handleFinished(assetId, false, message);
            });
}

void DownloadJob::downloadTo(const ImmichAsset &asset, const QString &path)
{
    start({Item{asset, path, {}, false}});
}

void DownloadJob::downloadInto(const QList<ImmichAsset> &assets, const QString &folder)
{
    QList<Item> items;
    for (const ImmichAsset &asset : assets)
        items.append(Item{asset, {}, folder, false});
    start(items);
}

void DownloadJob::start(const QList<Item> &items)
{
    if (items.isEmpty())
        return;
    const bool idle = !isRunning();
    m_items.append(items);
    if (!idle) {
        Toast::setProgressText(m_window, progressText(0));
        return;
    }
    m_saved = m_skipped = m_failed = 0;
    m_cancelled = false;
    m_clashPolicy = ClashPolicy::Ask;
    m_usedPaths.clear();
    m_lastError.clear();
    Toast::showProgress(m_window, progressText(0), tr("Cancel"), [this] { cancel(); });
    next();
}

void DownloadJob::next()
{
    ++m_current;
    if (m_cancelled || m_current >= m_items.size()) {
        finish();
        return;
    }
    Toast::setProgressText(m_window, progressText(0));
    download(m_items[m_current]);
}

void DownloadJob::download(Item &item)
{
    QString path = item.path;
    if (path.isEmpty()) {
        // Timeline photos come without a file name (and often a size, which the
        // clash question shows); look them up when it's their turn.
        const bool needsSize = item.asset.fileSize <= 0 && m_clashPolicy == ClashPolicy::Ask &&
                               isTaken(wantedPath(item));
        if ((item.asset.fileName.isEmpty() || needsSize) && !item.lookedUp) {
            item.lookedUp = true;
            m_waitingForInfo = item.asset.id;
            m_client->fetchAssetInfo(item.asset.id);
            return;
        }
        bool skip = false;
        path = resolvePath(item, &skip);
        if (!isRunning()) // Cancelled from the clash question.
            return;
        if (skip) {
            ++m_skipped;
            QTimer::singleShot(0, this, &DownloadJob::next);
            return;
        }
    }
    m_usedPaths.insert(path);
    m_currentId = item.asset.id;
    m_currentPath = path;
    m_client->downloadAsset(item.asset.id, path, QFileInfo(path).fileName());
}

QString DownloadJob::wantedPath(const Item &item) const
{
    const QString name = item.asset.fileName.isEmpty()
                             ? QStringLiteral("immich-%1").arg(item.asset.id)
                             : item.asset.fileName;
    return QDir(item.folder).filePath(name);
}

bool DownloadJob::isTaken(const QString &path) const
{
    return QFileInfo::exists(path) || m_usedPaths.contains(path);
}

// Where a photo from a selection goes; asks once (or for each) when the name is taken.
QString DownloadJob::resolvePath(const Item &item, bool *skip)
{
    const QString wanted = wantedPath(item);
    const QString name = QFileInfo(wanted).fileName();
    const QDir folder(item.folder);
    if (!isTaken(wanted))
        return wanted;

    ClashPolicy policy = m_clashPolicy;
    if (policy == ClashPolicy::Ask) {
        QMessageBox box(m_window);
        box.setIcon(QMessageBox::Question);
        box.setWindowTitle(tr("File already exists"));
        box.setText(tr("“%1” already exists in %2.").arg(name, folder.dirName()));
        box.setInformativeText(clashDetails(wanted, item.asset.fileSize));
        auto *keepBoth = box.addButton(tr("Keep both"), QMessageBox::AcceptRole);
        auto *skipButton = box.addButton(tr("Skip"), QMessageBox::RejectRole);
        auto *cancelButton = box.addButton(tr("Cancel download"), QMessageBox::DestructiveRole);
        auto *applyToAll = new QCheckBox(tr("Apply to all"), &box);
        box.setCheckBox(applyToAll);
        box.setDefaultButton(keepBoth);
        box.exec();
        if (box.clickedButton() == cancelButton) {
            cancel();
            *skip = true;
            return {};
        }
        policy = box.clickedButton() == skipButton ? ClashPolicy::Skip : ClashPolicy::Rename;
        if (applyToAll->isChecked())
            m_clashPolicy = policy;
    }
    if (policy == ClashPolicy::Skip) {
        *skip = true;
        return {};
    }
    // Keep both: name_1.jpg, name_2.jpg… (underscores rather than "(1)").
    const QFileInfo info(name);
    const QString base = info.completeBaseName();
    const QString suffix = info.suffix().isEmpty() ? QString() : u'.' + info.suffix();
    for (int n = 1;; ++n) {
        const QString candidate = folder.filePath(QStringLiteral("%1_%2%3").arg(base).arg(n).arg(suffix));
        if (!isTaken(candidate))
            return candidate;
    }
}

// The two sizes, so the user can tell a copy they already have from a different photo.
QString DownloadJob::clashDetails(const QString &path, qint64 incomingSize) const
{
    const QLocale locale;
    const QFileInfo existing(path);
    const QString incoming = incomingSize > 0 ? locale.formattedDataSize(incomingSize) : tr("unknown size");
    if (!existing.exists()) // Another photo in this download has the same name.
        return tr("Another photo in this download has the same name.\nThis one: %1").arg(incoming);
    return tr("Existing file: %1\nFrom Immich: %2")
        .arg(locale.formattedDataSize(existing.size()), incoming);
}

void DownloadJob::handleProgress(const QString &assetId, qint64 received, qint64 total)
{
    if (assetId != m_currentId || total <= 0)
        return;
    Toast::setProgressText(m_window, progressText(int(received * 100 / total)));
}

void DownloadJob::handleFinished(const QString &assetId, bool ok, const QString &error)
{
    if (assetId.isEmpty() || assetId != m_currentId)
        return;
    m_currentId.clear();
    if (ok) {
        ++m_saved;
    } else if (!error.isEmpty()) {
        ++m_failed;
        m_lastError = error;
    }
    if (m_cancelled)
        finish();
    else // Not from inside the reply's signal: the next one may ask about a name clash.
        QTimer::singleShot(0, this, &DownloadJob::next);
}

void DownloadJob::cancel()
{
    if (!isRunning() || m_cancelled)
        return;
    m_cancelled = true;
    // The in-flight download reports back once aborted (and its partial file is gone).
    if (!m_currentId.isEmpty())
        m_client->cancelDownload(m_currentId);
    else
        finish();
}

void DownloadJob::finish()
{
    Toast::finishProgress(m_window);
    const int total = int(m_items.size());
    if (m_cancelled) {
        Toast::showInfo(m_window, m_saved > 0
                                      ? tr("Download cancelled, %n saved", nullptr, m_saved)
                                      : tr("Download cancelled"));
    } else if (total == 1) {
        if (m_saved == 1)
            Toast::showInfo(m_window, tr("Saved %1").arg(QFileInfo(m_currentPath).fileName()));
        else if (m_failed == 1)
            Toast::showMessage(m_window, tr("Download failed: %1").arg(m_lastError));
    } else {
        QStringList parts{tr("%n saved", nullptr, m_saved)};
        if (m_skipped > 0)
            parts << tr("%n skipped", nullptr, m_skipped);
        if (m_failed > 0)
            parts << tr("%n failed", nullptr, m_failed);
        const QString summary = parts.join(QStringLiteral(", "));
        if (m_failed > 0)
            Toast::showMessage(m_window, tr("%1 (%2)").arg(summary, m_lastError));
        else
            Toast::showInfo(m_window, summary);
    }
    m_items.clear();
    m_current = -1;
    m_currentId.clear();
    m_waitingForInfo.clear();
    m_cancelled = false;
}

QString DownloadJob::progressText(int percent) const
{
    const int total = int(m_items.size());
    if (total == 1) {
        const QString name = QFileInfo(m_items.first().path).fileName();
        return tr("Downloading %1… %2%").arg(name.isEmpty() ? tr("photo") : name).arg(percent);
    }
    return tr("Downloading %1 of %2… %3%").arg(qMax(1, m_current + 1)).arg(total).arg(percent);
}

} // namespace Aurora
