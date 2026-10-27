#include "core/AppSettings.h"
#include "core/FolderSyncService.h"

#include "core/ImmichClient.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QImageReader>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QStandardPaths>
#include <QSaveFile>
#include <QTimer>

#include <algorithm>
#include <iterator>

namespace Aurora {
namespace {

constexpr qint64 kStabilityMs = 1500;
constexpr int kScanIntervalMs = 8000;

} // namespace

FolderSyncService::FolderSyncService(ImmichClient *client, QObject *parent)
    : QObject(parent)
    , m_client(client)
    , m_stateFilePath(AppSettings::profileDataDirectory() + QStringLiteral("/folder-sync-state.json"))
    , m_settings(m_store.loadFolderSync())
    , m_watcher(new QFileSystemWatcher(this))
    , m_scanTimer(new QTimer(this))
    , m_stabilityTimer(new QTimer(this))
{
    m_scanTimer->setInterval(kScanIntervalMs);
    connect(m_scanTimer, &QTimer::timeout, this, &FolderSyncService::periodicScan);

    m_stabilityTimer->setInterval(500);
    connect(m_stabilityTimer, &QTimer::timeout, this, &FolderSyncService::processStableFiles);

    connect(m_watcher, &QFileSystemWatcher::directoryChanged, this,
            &FolderSyncService::handleDirectoryChanged);
    connect(m_watcher, &QFileSystemWatcher::fileChanged, this,
            &FolderSyncService::handleDirectoryChanged);

    connect(m_client, &ImmichClient::uploadProgress, this,
            &FolderSyncService::handleUploadProgress);
    connect(m_client, &ImmichClient::assetUploaded, this,
            &FolderSyncService::handleAssetUploaded);
    connect(m_client, &ImmichClient::assetUploadFailed, this,
            &FolderSyncService::handleUploadFailed);
    connect(m_client, &ImmichClient::onlineChanged, this,
            &FolderSyncService::handleOnlineChanged);
    connect(m_client, &ImmichClient::configurationChanged, this,
            &FolderSyncService::handleConfigurationChanged);
    connect(m_client, &ImmichClient::activeEndpointChanged, this,
            &FolderSyncService::handleActiveEndpointChanged);
    connect(m_client, &ImmichClient::uploadQueueChanged, this, [this](int) {
        refreshStatus();
        publishLibraryPending();
    });

    loadSyncedState();
    rebuildWatcher();
    refreshStatus();
    publishLibraryPending();
    if (m_settings.enabled)
        QTimer::singleShot(1500, this, &FolderSyncService::scanNow);
}

FolderSyncSettings FolderSyncService::settings() const
{
    return m_settings;
}

void FolderSyncService::setSettings(const FolderSyncSettings &settings)
{
    FolderSyncSettings normalized = settings;
    normalized.folderPaths.clear();
    for (const auto &folder : settings.folderPaths) {
        if (folder.trimmed().isEmpty())
            continue;
        const QFileInfo info(QDir::cleanPath(folder.trimmed()));
        const QString path = info.canonicalFilePath().isEmpty() ? info.absoluteFilePath() : info.canonicalFilePath();
#ifdef Q_OS_WIN
        const bool duplicate = normalized.folderPaths.contains(path, Qt::CaseInsensitive);
#else
        const bool duplicate = normalized.folderPaths.contains(path);
#endif
        if (!duplicate)
            normalized.folderPaths.append(path);
    }

    const bool changed = normalized.enabled != m_settings.enabled ||
                         normalized.localNetworkOnly != m_settings.localNetworkOnly ||
                         normalized.folderPaths != m_settings.folderPaths;
    m_settings = normalized;
    m_store.saveFolderSync(m_settings);
    if (changed) {
        for (auto it = m_pending.begin(); it != m_pending.end();)
            it = isUnderSyncFolder(it.key()) ? std::next(it) : m_pending.erase(it);
        for (auto it = m_queued.begin(); it != m_queued.end();)
            it = isUnderSyncFolder(*it) ? std::next(it) : m_queued.erase(it);
        for (auto it = m_errors.begin(); it != m_errors.end();)
            it = isUnderSyncFolder(it.key()) ? std::next(it) : m_errors.erase(it);
        rebuildWatcher();
        emit settingsChanged(m_settings);
        refreshStatus();
        publishLibraryPending();
        if (m_settings.enabled)
            scanNow();
    } else {
        refreshStatus();
        publishLibraryPending();
    }
}

FolderSyncUiStatus FolderSyncService::uiStatus() const
{
    return m_uiStatus;
}

QString FolderSyncService::statusMessage() const
{
    return m_statusMessage;
}

QString FolderSyncService::statusIconPath() const
{
    switch (m_uiStatus) {
    case FolderSyncUiStatus::Saved:
        return QStringLiteral(":/icons/cloud-check.svg");
    case FolderSyncUiStatus::Saving:
        return QStringLiteral(":/icons/cloud-upload.svg");
    case FolderSyncUiStatus::Error:
        return QStringLiteral(":/icons/cloud-alert.svg");
    case FolderSyncUiStatus::Unsaved:
        return QStringLiteral(":/icons/cloud-off.svg");
    case FolderSyncUiStatus::Disabled:
    default:
        return QStringLiteral(":/icons/folder-sync.svg");
    }
}

QList<ImmichAsset> FolderSyncService::pendingLibraryAssets() const
{
    return m_libraryPending;
}

void FolderSyncService::scanNow()
{
    if (!m_settings.enabled || m_settings.folderPaths.isEmpty()) {
        refreshStatus();
        publishLibraryPending();
        return;
    }

    const QStringList files = collectMediaFiles();
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    for (const QString &path : files) {
        if (alreadySynced(path) || m_queued.contains(path))
            continue;
        const QFileInfo info(path);
        if (!info.exists() || !info.isFile() || info.size() <= 0)
            continue;

        PendingFile &pending = m_pending[path];
        if (pending.size != info.size()) {
            pending.size = info.size();
            pending.stableSinceMs = now;
            m_errors.remove(path);
        }
    }

    for (auto it = m_pending.begin(); it != m_pending.end();) {
        if (!QFileInfo::exists(it.key()))
            it = m_pending.erase(it);
        else
            ++it;
    }

    if (!m_pending.isEmpty() && !m_stabilityTimer->isActive())
        m_stabilityTimer->start();
    processStableFiles();
    refreshStatus();
    publishLibraryPending();
}

void FolderSyncService::handleDirectoryChanged(const QString &)
{
    QTimer::singleShot(300, this, [this] {
        rebuildWatcher();
        scanNow();
    });
}

void FolderSyncService::handleUploadProgress(const QString &filePath, qint64, qint64)
{
    if (!isUnderSyncFolder(filePath))
        return;
    m_lastUploadName = QFileInfo(filePath).fileName();
    refreshStatus();
    publishLibraryPending();
}

void FolderSyncService::handleAssetUploaded(const QString &filePath, const QString &,
                                            bool)
{
    if (!isUnderSyncFolder(filePath))
        return;
    m_queued.remove(filePath);
    m_pending.remove(filePath);
    m_errors.remove(filePath);
    markSynced(filePath);
    m_lastUploadName = QFileInfo(filePath).fileName();
    refreshStatus();
    publishLibraryPending();
}

void FolderSyncService::handleUploadFailed(const QString &filePath, const QString &message)
{
    if (!isUnderSyncFolder(filePath))
        return;
    m_queued.remove(filePath);
    markError(filePath, message);
    refreshStatus();
    publishLibraryPending();
}

void FolderSyncService::handleOnlineChanged(bool)
{
    refreshStatus();
    if (canUploadNow())
        scanNow();
    else
        publishLibraryPending();
}

void FolderSyncService::handleConfigurationChanged(bool)
{
    refreshStatus();
    if (canUploadNow())
        scanNow();
    else
        publishLibraryPending();
}

void FolderSyncService::handleActiveEndpointChanged(bool, const QString &)
{
    refreshStatus();
    if (canUploadNow())
        scanNow();
    else
        publishLibraryPending();
}

void FolderSyncService::processStableFiles()
{
    if (m_pending.isEmpty()) {
        m_stabilityTimer->stop();
        refreshStatus();
        publishLibraryPending();
        return;
    }

    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    QStringList ready;
    for (auto it = m_pending.begin(); it != m_pending.end();) {
        const QString path = it.key();
        const QFileInfo info(path);
        if (!info.exists() || !info.isFile() || info.size() <= 0) {
            it = m_pending.erase(it);
            continue;
        }
        if (info.size() != it->size) {
            it->size = info.size();
            it->stableSinceMs = now;
            ++it;
            continue;
        }
        if (now - it->stableSinceMs < kStabilityMs) {
            ++it;
            continue;
        }
        if (alreadySynced(path) || m_queued.contains(path)) {
            it = m_pending.erase(it);
            continue;
        }
        ready.append(path);
        it = m_pending.erase(it);
    }

    if (!ready.isEmpty())
        enqueueReadyFiles(ready);
    if (m_pending.isEmpty())
        m_stabilityTimer->stop();
    refreshStatus();
    publishLibraryPending();
}

void FolderSyncService::periodicScan()
{
    rebuildWatcher();
    scanNow();
}

bool FolderSyncService::isMediaFile(const QString &path)
{
    static const QSet<QString> extensions = {
        QStringLiteral("jpg"),  QStringLiteral("jpeg"), QStringLiteral("png"),
        QStringLiteral("gif"),  QStringLiteral("webp"), QStringLiteral("heic"),
        QStringLiteral("heif"), QStringLiteral("tif"),  QStringLiteral("tiff"),
        QStringLiteral("bmp"),  QStringLiteral("raw"),  QStringLiteral("dng"),
        QStringLiteral("cr2"),  QStringLiteral("nef"),  QStringLiteral("arw"),
        QStringLiteral("mp4"),  QStringLiteral("mov"),  QStringLiteral("m4v"),
        QStringLiteral("avi"),  QStringLiteral("mkv"),  QStringLiteral("webm"),
        QStringLiteral("3gp"),
    };
    const QFileInfo info(path);
    if (!info.isFile())
        return false;
    if (info.fileName().startsWith(u'.'))
        return false;
    return extensions.contains(info.suffix().toLower());
}

bool FolderSyncService::isVideoFile(const QString &path)
{
    static const QSet<QString> extensions = {
        QStringLiteral("mp4"), QStringLiteral("mov"), QStringLiteral("m4v"),
        QStringLiteral("avi"), QStringLiteral("mkv"), QStringLiteral("webm"),
        QStringLiteral("3gp"),
    };
    return extensions.contains(QFileInfo(path).suffix().toLower());
}

QString FolderSyncService::fingerprint(const QString &path)
{
    const QFileInfo info(path);
    return QStringLiteral("%1:%2")
        .arg(info.size())
        .arg(info.lastModified().toMSecsSinceEpoch());
}

QString FolderSyncService::localAssetId(const QString &path)
{
    const QByteArray hash =
        QCryptographicHash::hash(QFileInfo(path).absoluteFilePath().toUtf8(),
                                 QCryptographicHash::Sha1)
            .toHex();
    return QStringLiteral("local:") + QString::fromLatin1(hash);
}

ImmichAsset FolderSyncService::assetFromLocalFile(const QString &path)
{
    ImmichAsset asset;
    const QFileInfo info(path);
    asset.id = localAssetId(path);
    asset.localPath = info.absoluteFilePath();
    asset.fileName = info.fileName();
    asset.type = isVideoFile(path) ? QStringLiteral("VIDEO") : QStringLiteral("IMAGE");
    asset.takenAt = info.birthTime().isValid() ? info.birthTime() : info.lastModified();
    asset.localSyncState = LocalSyncState::Unsaved;

    QImageReader reader(path);
    const QSize size = reader.size();
    if (size.isValid() && size.height() > 0)
        asset.aspectRatio = qBound(0.2, qreal(size.width()) / qreal(size.height()), 8.0);
    return asset;
}

bool FolderSyncService::isUnderSyncFolder(const QString &path) const
{
    const QString absolute = QDir::fromNativeSeparators(QFileInfo(path).absoluteFilePath());
    for (const QString &folder : m_settings.folderPaths) {
        QString prefix = QDir::fromNativeSeparators(QFileInfo(folder).absoluteFilePath());
        if (!prefix.endsWith(u'/'))
            prefix += u'/';
#ifdef Q_OS_WIN
        if (absolute.startsWith(prefix, Qt::CaseInsensitive))
#else
        if (absolute.startsWith(prefix))
#endif
            return true;
    }
    return false;
}

bool FolderSyncService::alreadySynced(const QString &path) const
{
    const QString key = QFileInfo(path).absoluteFilePath();
    const auto it = m_syncedFingerprints.constFind(key);
    return it != m_syncedFingerprints.cend() && it.value() == fingerprint(path);
}

bool FolderSyncService::canUploadNow() const
{
    if (!m_settings.enabled || !m_client->isConfigured() || !m_client->isOnline())
        return false;
    if (m_settings.localNetworkOnly && !m_client->usingLocalEndpoint())
        return false;
    return true;
}

void FolderSyncService::markSynced(const QString &path)
{
    const QString key = QFileInfo(path).absoluteFilePath();
    m_syncedFingerprints.insert(key, fingerprint(path));
    saveSyncedState();
}

void FolderSyncService::markError(const QString &path, const QString &message)
{
    m_errors.insert(QFileInfo(path).absoluteFilePath(), message);
}

void FolderSyncService::loadSyncedState()
{
    m_syncedFingerprints.clear();
    QFile file(m_stateFilePath);
    if (!file.open(QIODevice::ReadOnly))
        return;
    const QJsonObject root = QJsonDocument::fromJson(file.readAll()).object();
    // Fingerprints are keyed by absolute file path. Keep them when adding,
    // removing, or reordering roots, including the legacy single-folder state.
    const QJsonObject files = root.value(QStringLiteral("files")).toObject();
    for (auto it = files.begin(); it != files.end(); ++it)
        m_syncedFingerprints.insert(it.key(), it.value().toString());
}

void FolderSyncService::saveSyncedState() const
{
    QJsonObject files;
    for (auto it = m_syncedFingerprints.cbegin(); it != m_syncedFingerprints.cend(); ++it)
        files.insert(it.key(), it.value());

    QJsonObject root;
    root.insert(QStringLiteral("folders"), QJsonArray::fromStringList(m_settings.folderPaths));
    root.insert(QStringLiteral("files"), files);

    const QString path = m_stateFilePath;
    QDir().mkpath(QFileInfo(path).absolutePath());
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return;
    file.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
    file.commit();
}

void FolderSyncService::rebuildWatcher()
{
    const QStringList dirs = m_watcher->directories();
    if (!dirs.isEmpty())
        m_watcher->removePaths(dirs);
    const QStringList files = m_watcher->files();
    if (!files.isEmpty())
        m_watcher->removePaths(files);

    m_scanTimer->stop();
    if (!m_settings.enabled || m_settings.folderPaths.isEmpty())
        return;
    QSet<QString> watched;
    for (const QString &folder : m_settings.folderPaths) {
        if (!QDir(folder).exists())
            continue;
        watched.insert(folder);
        QDirIterator it(folder, QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
        while (it.hasNext())
            watched.insert(it.next());
    }
    if (!watched.isEmpty())
        m_watcher->addPaths(watched.values());
    m_scanTimer->start();
}

void FolderSyncService::enqueueReadyFiles(const QStringList &paths)
{
    if (!canUploadNow() || paths.isEmpty())
        return;

    QStringList toUpload;
    toUpload.reserve(paths.size());
    for (const QString &path : paths) {
        if (m_queued.contains(path) || alreadySynced(path))
            continue;
        m_queued.insert(path);
        m_errors.remove(path);
        toUpload.append(path);
    }
    if (toUpload.isEmpty())
        return;
    m_client->uploadAssets(toUpload);
}

void FolderSyncService::refreshStatus()
{
    if (!m_settings.enabled) {
        setStatus(FolderSyncUiStatus::Disabled,
                  tr("Folder sync is turned off."));
        return;
    }
    if (m_settings.folderPaths.isEmpty()) {
        setStatus(FolderSyncUiStatus::Unsaved,
                  tr("Choose a folder to watch for photos and videos."));
        return;
    }
    for (const auto &folder : m_settings.folderPaths) {
        if (!QDir(folder).exists()) {
            setStatus(FolderSyncUiStatus::Error,
                      tr("Sync folder does not exist: %1").arg(folder));
            return;
        }
    }
    if (!m_client->isConfigured()) {
        setStatus(FolderSyncUiStatus::Unsaved,
                  tr("Connect to Immich in Settings → Immich Server first."));
        return;
    }
    if (m_settings.localNetworkOnly && !m_client->usingLocalEndpoint()) {
        setStatus(FolderSyncUiStatus::Unsaved,
                  tr("Waiting for Immich on the local network. "
                     "Files already show in Library with an unsaved badge."));
        return;
    }
    if (!m_queued.isEmpty()) {
        setStatus(FolderSyncUiStatus::Saving,
                  m_lastUploadName.isEmpty()
                      ? tr("Saving to Immich…")
                      : tr("Saving %1…").arg(m_lastUploadName));
        return;
    }
    if (!m_pending.isEmpty()) {
        setStatus(FolderSyncUiStatus::Unsaved,
                  tr("Waiting for %n file(s) to finish copying…", nullptr,
                     m_pending.size()));
        return;
    }
    if (!m_errors.isEmpty()) {
        setStatus(FolderSyncUiStatus::Error,
                  tr("Could not sync %1: %2")
                      .arg(QFileInfo(m_errors.cbegin().key()).fileName(),
                           m_errors.cbegin().value()));
        return;
    }
    if (!m_client->isOnline()) {
        setStatus(FolderSyncUiStatus::Unsaved,
                  tr("Offline - new files will upload when Immich is reachable."));
        return;
    }

    int syncedCount = 0;
    for (auto it = m_syncedFingerprints.cbegin(); it != m_syncedFingerprints.cend(); ++it)
        if (isUnderSyncFolder(it.key()))
            ++syncedCount;
    setStatus(FolderSyncUiStatus::Saved,
              tr("Up to date with Immich (%n synced file(s)).", nullptr,
                 syncedCount));
}

void FolderSyncService::setStatus(FolderSyncUiStatus status, const QString &message)
{
    if (m_uiStatus == status && m_statusMessage == message)
        return;
    m_uiStatus = status;
    m_statusMessage = message;
    emit statusChanged();
}

void FolderSyncService::publishLibraryPending()
{
    QList<ImmichAsset> pending;
    if (m_settings.enabled && !m_settings.folderPaths.isEmpty()) {
        const QStringList files = collectMediaFiles();
        pending.reserve(files.size());
        for (const QString &path : files) {
            if (alreadySynced(path))
                continue;
            ImmichAsset asset = assetFromLocalFile(path);
            if (m_queued.contains(path))
                asset.localSyncState = LocalSyncState::Saving;
            else if (m_errors.contains(path))
                asset.localSyncState = LocalSyncState::Error;
            else
                asset.localSyncState = LocalSyncState::Unsaved;
            pending.append(asset);
        }
        std::sort(pending.begin(), pending.end(),
                  [](const ImmichAsset &a, const ImmichAsset &b) {
                      return a.takenAt > b.takenAt;
                  });
    }

    if (pending.size() == m_libraryPending.size()) {
        bool same = true;
        for (int i = 0; i < pending.size(); ++i) {
            const ImmichAsset &a = pending.at(i);
            const ImmichAsset &b = m_libraryPending.at(i);
            if (a.id != b.id || a.localSyncState != b.localSyncState ||
                a.localPath != b.localPath) {
                same = false;
                break;
            }
        }
        if (same)
            return;
    }

    m_libraryPending = pending;
    emit pendingLibraryAssetsChanged();
}

QStringList FolderSyncService::collectMediaFiles() const
{
    QStringList result;
    QSet<QString> seen;
    for (const auto &folder : m_settings.folderPaths) {
        QDirIterator it(folder,
                    QDir::Files | QDir::Readable | QDir::NoDotAndDotDot,
                    QDirIterator::Subdirectories);
        while (it.hasNext()) {
            const QString path = QFileInfo(it.next()).absoluteFilePath();
#ifdef Q_OS_WIN
            const QString key = path.toCaseFolded();
#else
            const QString key = path;
#endif
            if (isMediaFile(path) && !seen.contains(key)) {
                seen.insert(key);
                result.append(path);
            }
        }
    }
    return result;
}

} // namespace Aurora
