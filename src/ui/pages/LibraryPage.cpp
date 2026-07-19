#include "ui/pages/LibraryPage.h"

#include "ui/widgets/MediaTile.h"
#include "ui/widgets/VideoPlayerDialog.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QHideEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QMessageBox>
#include <QMimeData>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QSet>
#include <QShowEvent>
#include <QStandardPaths>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>

#include <utility>

namespace Aurora {
namespace {

constexpr int kGap = 3;
constexpr int kSidePad = 12;
constexpr int kTargetRowHeight = 132;
constexpr int kMaxRowHeight = 148;
constexpr int kMinRowHeight = 96;
constexpr int kAutoCheckIntervalMs = 30 * 1000;
constexpr int kSearchDebounceMs = 350;

QString mediaFileFilter()
{
    return QObject::tr(
        "Media files (*.jpg *.jpeg *.png *.gif *.webp *.heic *.heif *.tif *.tiff "
        "*.bmp *.raw *.dng *.cr2 *.nef *.arw *.mp4 *.mov *.m4v *.avi *.mkv *.webm "
        "*.3gp);;All files (*.*)");
}

} // namespace

LibraryPage::LibraryPage(ImmichClient *client, QWidget *parent)
    : QWidget(parent)
    , m_client(client)
    , m_scrollArea(new QScrollArea(this))
    , m_timelineHost(new QWidget)
    , m_status(new QLabel(this))
    , m_emptyState(new QLabel(this))
    , m_dropOverlay(new QLabel(this))
    , m_searchField(new QLineEdit(this))
    , m_uploadButton(new QPushButton(tr("Upload"), this))
    , m_refreshButton(new QPushButton(tr("Refresh"), this))
    , m_layoutTimer(new QTimer(this))
    , m_visibilityTimer(new QTimer(this))
    , m_autoCheckTimer(new QTimer(this))
    , m_searchDebounce(new QTimer(this))
{
    m_layoutTimer->setSingleShot(true);
    m_layoutTimer->setInterval(40);
    connect(m_layoutTimer, &QTimer::timeout, this, &LibraryPage::layoutTimeline);
    m_visibilityTimer->setSingleShot(true);
    m_visibilityTimer->setInterval(60);
    connect(m_visibilityTimer, &QTimer::timeout,
            this, &LibraryPage::updateVisibleMedia);
    m_autoCheckTimer->setInterval(kAutoCheckIntervalMs);
    connect(m_autoCheckTimer, &QTimer::timeout, this, &LibraryPage::checkForNewPhotos);
    m_searchDebounce->setSingleShot(true);
    m_searchDebounce->setInterval(kSearchDebounceMs);
    connect(m_searchDebounce, &QTimer::timeout, this, &LibraryPage::applySearch);
    setObjectName(QStringLiteral("libraryPage"));
    setAcceptDrops(true);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto *toolbar = new QWidget(this);
    toolbar->setObjectName(QStringLiteral("libraryToolbar"));
    auto *toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(16, 12, 16, 10);
    toolbarLayout->setSpacing(12);

    auto *headingColumn = new QVBoxLayout;
    headingColumn->setSpacing(2);
    auto *heading = new QLabel(tr("Photos & videos"), toolbar);
    heading->setProperty("heading", true);
    m_status->setProperty("subheading", true);
    m_status->setText(tr("Your Immich library"));
    headingColumn->addWidget(heading);
    headingColumn->addWidget(m_status);
    toolbarLayout->addLayout(headingColumn, 1);

    m_searchField->setPlaceholderText(tr("Search photos and videos…"));
    m_searchField->setClearButtonEnabled(true);
    m_searchField->setMinimumWidth(220);
    m_searchField->setMaximumWidth(360);
    toolbarLayout->addWidget(m_searchField);
    toolbarLayout->addWidget(m_uploadButton);
    toolbarLayout->addWidget(m_refreshButton);

    m_timelineHost->setObjectName(QStringLiteral("timelineHost"));
    m_timelineHost->setAcceptDrops(true);
    m_timelineHost->installEventFilter(this);
    m_scrollArea->setObjectName(QStringLiteral("libraryScroll"));
    m_scrollArea->setWidgetResizable(false);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setWidget(m_timelineHost);
    m_scrollArea->setAcceptDrops(true);
    m_scrollArea->viewport()->setAcceptDrops(true);
    m_scrollArea->installEventFilter(this);
    m_scrollArea->viewport()->installEventFilter(this);
    connect(m_scrollArea->verticalScrollBar(), &QScrollBar::valueChanged,
            this, &LibraryPage::scheduleVisibleMediaUpdate);
    connect(m_scrollArea->verticalScrollBar(), &QScrollBar::valueChanged,
            this, &LibraryPage::maybeLoadMore);
    connect(m_scrollArea->verticalScrollBar(), &QScrollBar::rangeChanged,
            this, [this](int, int) { maybeLoadMore(); });

    m_emptyState->setAlignment(Qt::AlignCenter);
    m_emptyState->setWordWrap(true);
    m_emptyState->setProperty("subheading", true);
    m_emptyState->setMinimumHeight(180);
    m_emptyState->setAcceptDrops(true);
    m_emptyState->installEventFilter(this);

    m_dropOverlay->setObjectName(QStringLiteral("libraryDropOverlay"));
    m_dropOverlay->setAlignment(Qt::AlignCenter);
    m_dropOverlay->setText(tr("Drop photos or videos to upload"));
    m_dropOverlay->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_dropOverlay->hide();
    m_dropOverlay->raise();

    root->addWidget(toolbar);
    root->addWidget(m_emptyState);
    root->addWidget(m_scrollArea, 1);

    connect(m_uploadButton, &QPushButton::clicked, this, &LibraryPage::chooseFilesToUpload);
    connect(m_refreshButton, &QPushButton::clicked, this, &LibraryPage::refresh);
    connect(m_searchField, &QLineEdit::textChanged, this, [this] {
        m_searchDebounce->start();
    });
    connect(m_searchField, &QLineEdit::returnPressed, this, [this] {
        m_searchDebounce->stop();
        applySearch();
    });
    connect(m_client, &ImmichClient::assetsLoaded, this, &LibraryPage::showAssets);
    connect(m_client, &ImmichClient::newestAssetsPolled,
            this, &LibraryPage::handleNewestAssetsPolled);
    connect(m_client, &ImmichClient::thumbnailLoaded, this, &LibraryPage::showThumbnail);
    connect(m_client, &ImmichClient::imageLoadFailed,
            this, &LibraryPage::showThumbnailError);
    connect(m_client, &ImmichClient::requestFailed, this, &LibraryPage::showRequestError);
    connect(m_client, &ImmichClient::uploadProgress, this, &LibraryPage::handleUploadProgress);
    connect(m_client, &ImmichClient::assetUploaded, this, &LibraryPage::handleAssetUploaded);
    connect(m_client, &ImmichClient::downloadProgress,
            this, &LibraryPage::handleDownloadProgress);
    connect(m_client, &ImmichClient::assetDownloaded,
            this, &LibraryPage::handleAssetDownloaded);
    connect(m_client, &ImmichClient::assetsDeleted, this, &LibraryPage::handleAssetsDeleted);
    connect(m_client, &ImmichClient::activeEndpointChanged,
            this, &LibraryPage::handleActiveEndpointChanged);
    connect(m_client, &ImmichClient::configurationChanged, this, [this](bool configured) {
        m_uploadButton->setEnabled(configured);
        m_searchField->setEnabled(configured);
        if (configured)
            refresh();
        else {
            clearTimeline();
            updateEmptyState();
        }
        updateAutoCheckTimer();
        updateEndpointHint();
    });

    m_uploadButton->setEnabled(m_client->isConfigured());
    m_searchField->setEnabled(m_client->isConfigured());
    updateEmptyState();
    updateAutoCheckTimer();
    updateEndpointHint();
    QTimer::singleShot(0, this, [this] {
        if (m_client->isConfigured())
            refresh();
    });
}

void LibraryPage::refresh()
{
    if (m_loading)
        return;
    requestPage(1, false);
}

void LibraryPage::loadMore()
{
    bool ok = false;
    const int page = m_nextPage.toInt(&ok);
    if (!m_loading && ok && page > 0)
        requestPage(page, true);
}

void LibraryPage::maybeLoadMore()
{
    if (m_loading || m_nextPage.isEmpty() || !m_scrollArea->isVisible())
        return;

    const QScrollBar *scrollBar = m_scrollArea->verticalScrollBar();
    const int preloadDistance = qMax(500, m_scrollArea->viewport()->height());
    if (scrollBar->maximum() - scrollBar->value() <= preloadDistance)
        loadMore();
}

void LibraryPage::checkForNewPhotos()
{
    if (!isVisible() || m_loading || m_autoRefreshPending || !m_client->isConfigured() ||
        !m_searchQuery.isEmpty())
        return;
    m_client->pollNewestAssets(24);
}

void LibraryPage::applySearch()
{
    const QString query = m_searchField->text().trimmed();
    if (query == m_searchQuery)
        return;
    m_searchQuery = query;
    updateAutoCheckTimer();
    requestPage(1, false);
}

void LibraryPage::handleNewestAssetsPolled(const QList<ImmichAsset> &assets)
{
    if (m_loading || m_autoRefreshPending || !m_searchQuery.isEmpty())
        return;

    int newCount = 0;
    for (const ImmichAsset &asset : assets) {
        if (!m_tilesById.contains(asset.id))
            ++newCount;
    }
    if (newCount == 0)
        return;

    m_autoRefreshPending = true;
    m_status->setText(tr("New photo%1 found — refreshing…")
                          .arg(newCount == 1 ? QString() : QStringLiteral("s")));
    refresh();
}

void LibraryPage::requestPage(int page, bool append)
{
    if (!m_client->isConfigured()) {
        updateEmptyState();
        return;
    }
    m_loading = true;
    m_appendRequest = append;
    m_refreshButton->setEnabled(false);
    if (!m_autoRefreshPending) {
        if (!m_searchQuery.isEmpty()) {
            m_status->setText(append ? tr("Loading more results…")
                                     : tr("Searching for “%1”…").arg(m_searchQuery));
        } else {
            m_status->setText(append ? tr("Loading more media…")
                                     : tr("Loading your library…"));
        }
    }
    m_client->loadAssets(page, 80, m_searchQuery);
}

LibraryPage::DaySection *LibraryPage::sectionForDate(const QDate &date)
{
    for (DaySection &section : m_sections) {
        if (section.date == date)
            return &section;
    }

    DaySection section;
    section.date = date;
    section.header = new QLabel(formatDayHeader(date), m_timelineHost);
    section.header->setObjectName(QStringLiteral("timelineDayHeader"));
    section.header->setProperty("section", true);
    m_sections.append(section);
    return &m_sections.last();
}

QString LibraryPage::formatDayHeader(const QDate &date) const
{
    if (!date.isValid())
        return tr("Unknown date");

    const QDate today = QDate::currentDate();
    if (date == today)
        return tr("Today");
    if (date == today.addDays(-1))
        return tr("Yesterday");

    const QLocale locale;
    if (date.year() == today.year())
        return locale.toString(date, QStringLiteral("dddd, MMMM d"));
    return locale.toString(date, QStringLiteral("dddd, MMMM d, yyyy"));
}

void LibraryPage::showAssets(const QList<ImmichAsset> &assets, const QString &nextPage,
                             const QString &query)
{
    if (query != m_searchQuery)
        return;

    if (!m_appendRequest)
        clearTimeline();

    for (const ImmichAsset &asset : assets) {
        if (m_tilesById.contains(asset.id))
            continue;

        m_assets.append(asset);
        const QDate date = asset.takenAt.isValid() ? asset.takenAt.date() : QDate();
        DaySection *section = sectionForDate(date);
        auto *tile = new MediaTile(asset, m_timelineHost);
        section->tiles.append(tile);
        m_tilesById.insert(asset.id, tile);
        connect(tile, &MediaTile::activated, this, &LibraryPage::openAsset);
        connect(tile, &MediaTile::downloadRequested, this, &LibraryPage::downloadAsset);
        connect(tile, &MediaTile::trashRequested, this, &LibraryPage::trashAsset);
        connect(tile, &MediaTile::deleteRequested, this, &LibraryPage::deleteAssetPermanently);
    }

    if (!m_appendRequest && !m_assets.isEmpty())
        m_newestAssetId = m_assets.first().id;

    m_nextPage = nextPage;
    m_loading = false;
    m_autoRefreshPending = false;
    m_refreshButton->setEnabled(true);
    if (!m_searchQuery.isEmpty()) {
        m_status->setText(tr("%n result(s) for “%1”", nullptr, m_assets.size())
                              .arg(m_searchQuery));
    } else {
        m_status->setText(tr("%n item(s)", nullptr, m_assets.size()));
        updateEndpointHint();
    }
    scheduleLayout();
    updateEmptyState();
    updateAutoCheckTimer();
}

void LibraryPage::showThumbnail(const QString &assetId, const QPixmap &thumbnail)
{
    m_requestedThumbnails.remove(assetId);
    if (auto *tile = m_tilesById.value(assetId)) {
        if (isTileNearViewport(tile)) {
            tile->setThumbnail(thumbnail);
            scheduleLayout();
        }
    }
    scheduleVisibleMediaUpdate();
}

void LibraryPage::scheduleLayout()
{
    if (!m_layoutTimer->isActive())
        m_layoutTimer->start();
}

void LibraryPage::showThumbnailError(const QString &assetId, const QString &resultSize,
                                     const QString &message)
{
    if (resultSize != QStringLiteral("thumbnail"))
        return;
    m_requestedThumbnails.remove(assetId);
    if (auto *tile = m_tilesById.value(assetId)) {
        if (isTileNearViewport(tile))
            tile->setThumbnailError(message);
    }
}

void LibraryPage::showRequestError(const QString &operation, const QString &message)
{
    if (operation == tr("Load image"))
        return;
    if (operation == tr("Upload")) {
        ++m_uploadsFailed;
        const int remaining = m_client->pendingUploadCount();
        if (remaining == 0) {
            const int completed = m_uploadsCompleted;
            const int failed = m_uploadsFailed;
            m_status->setText(
                tr("Upload finished: %1 ok, %2 failed").arg(completed).arg(failed));
            m_uploadsCompleted = 0;
            m_uploadsFailed = 0;
            m_uploadsTotal = 0;
            if (completed > 0)
                QTimer::singleShot(600, this, &LibraryPage::refresh);
        } else {
            m_status->setText(tr("Upload failed: %1 (%2 left)")
                                  .arg(message, QString::number(remaining)));
        }
        return;
    }
    m_loading = false;
    m_autoRefreshPending = false;
    m_refreshButton->setEnabled(true);
    m_status->setText(tr("%1 failed: %2").arg(operation, message));
    updateEmptyState();
}

void LibraryPage::clearTimeline()
{
    for (DaySection &section : m_sections) {
        if (section.header)
            section.header->deleteLater();
        for (MediaTile *tile : section.tiles)
            tile->deleteLater();
    }
    m_sections.clear();
    m_tilesById.clear();
    m_requestedThumbnails.clear();
    m_assets.clear();
    m_nextPage.clear();
    m_newestAssetId.clear();
}

void LibraryPage::layoutTimeline()
{
    const int viewportWidth = m_scrollArea->viewport()->width();
    const int availableWidth = qMax(240, viewportWidth - 2 * kSidePad);
    int y = 8;

    for (int sectionIndex = 0; sectionIndex < m_sections.size();) {
        DaySection &section = m_sections[sectionIndex];

        // Consecutive one-item days share one gallery row. Each item keeps its
        // own date heading, so chronology remains clear without wasting space.
        if (section.tiles.size() == 1) {
            struct SparseCell {
                DaySection *section;
                int width;
            };
            QList<SparseCell> cells;
            int occupiedWidth = 0;

            while (sectionIndex < m_sections.size() &&
                   m_sections[sectionIndex].tiles.size() == 1) {
                DaySection &candidate = m_sections[sectionIndex];
                const qreal ratio = candidate.tiles.first()->aspectRatio();
                const bool hasSingletonNeighbor =
                    !cells.isEmpty() ||
                    (sectionIndex + 1 < m_sections.size() &&
                     m_sections[sectionIndex + 1].tiles.size() == 1);
                const int maximumCellWidth = hasSingletonNeighbor
                    ? qMax(96, (availableWidth - 14) / 2)
                    : qMin(320, availableWidth);
                const int cellWidth = qBound(
                    96, qRound(ratio * kTargetRowHeight),
                    qMin(320, maximumCellWidth));
                const int required = cellWidth + (cells.isEmpty() ? 0 : 14);
                if (!cells.isEmpty() && occupiedWidth + required > availableWidth)
                    break;

                cells.append({&candidate, cellWidth});
                occupiedWidth += required;
                ++sectionIndex;
            }

            int x = kSidePad;
            for (const SparseCell &cell : std::as_const(cells)) {
                cell.section->header->setGeometry(x, y, cell.width, 26);
                cell.section->header->show();
                MediaTile *tile = cell.section->tiles.first();
                tile->setGeometry(x, y + 30, cell.width, kTargetRowHeight);
                tile->show();
                x += cell.width + 14;
            }
            y += 30 + kTargetRowHeight + 13;
            continue;
        }

        if (section.header) {
            section.header->setGeometry(kSidePad, y, availableWidth, 26);
            section.header->show();
            y += 32;
        }

        QList<MediaTile *> row;
        qreal rowAspectSum = 0.0;

        auto flushRow = [&](bool justifyToWidth) {
            if (row.isEmpty())
                return;

            const int gaps = kGap * qMax(0, row.size() - 1);
            qreal height = kTargetRowHeight;
            const qreal naturalWidth = rowAspectSum * height + gaps;
            if (justifyToWidth || naturalWidth > availableWidth) {
                height = (availableWidth - gaps) / qMax(0.01, rowAspectSum);
                height = qBound(qreal(kMinRowHeight), height, qreal(kMaxRowHeight));
            } else {
                height = qBound(qreal(kMinRowHeight), height, qreal(kMaxRowHeight));
            }

            const bool fillWidth = justifyToWidth || naturalWidth > availableWidth;
            int x = kSidePad;
            int used = 0;
            for (int i = 0; i < row.size(); ++i) {
                MediaTile *tile = row.at(i);
                int width = qRound(tile->aspectRatio() * height);
                if (fillWidth && i == row.size() - 1)
                    width = availableWidth - used;
                width = qMax(40, width);
                tile->setGeometry(x, y, width, qRound(height));
                tile->show();
                x += width + kGap;
                used += width + (i == row.size() - 1 ? 0 : kGap);
            }
            y += qRound(height) + kGap;
            row.clear();
            rowAspectSum = 0.0;
        };

        for (MediaTile *tile : section.tiles) {
            const qreal ratio = tile->aspectRatio();
            const qreal projected =
                (rowAspectSum + ratio) * kTargetRowHeight + kGap * row.size();
            if (!row.isEmpty() && projected > availableWidth)
                flushRow(true);
            row.append(tile);
            rowAspectSum += ratio;
        }
        // Short trailing rows (e.g. a single photo) keep a capped natural size.
        flushRow(false);
        y += 10;
        ++sectionIndex;
    }

    m_timelineHost->resize(viewportWidth, y + 16);
    scheduleVisibleMediaUpdate();
    QTimer::singleShot(0, this, &LibraryPage::maybeLoadMore);
}

void LibraryPage::scheduleVisibleMediaUpdate()
{
    if (!m_visibilityTimer->isActive())
        m_visibilityTimer->start();
}

bool LibraryPage::isTileNearViewport(const MediaTile *tile) const
{
    if (!tile || !m_scrollArea->isVisible())
        return false;
    const int viewportHeight = m_scrollArea->viewport()->height();
    const int scrollTop = m_scrollArea->verticalScrollBar()->value();
    const QRect bufferedViewport(
        0, qMax(0, scrollTop - viewportHeight),
        m_timelineHost->width(), viewportHeight * 3);
    return bufferedViewport.intersects(tile->geometry());
}

void LibraryPage::updateVisibleMedia()
{
    for (auto it = m_tilesById.cbegin(); it != m_tilesById.cend(); ++it) {
        MediaTile *tile = it.value();
        const QString assetId = it.key();
        if (isTileNearViewport(tile)) {
            if (!tile->hasThumbnail() && !tile->hasThumbnailError() &&
                !m_requestedThumbnails.contains(assetId)) {
                m_requestedThumbnails.insert(assetId);
                m_client->loadThumbnail(assetId);
            }
        } else {
            tile->clearThumbnail();
        }
    }
}

void LibraryPage::updateEmptyState()
{
    const bool empty = m_assets.isEmpty();
    m_emptyState->setVisible(empty);
    m_scrollArea->setVisible(!empty);
    if (!empty)
        return;
    if (!m_client->isConfigured()) {
        m_emptyState->setText(
            tr("Connect to your Immich server in Settings → Immich Server\n"
               "to browse photos and videos."));
        return;
    }
    if (!m_searchQuery.isEmpty()) {
        m_emptyState->setText(
            tr("No results for “%1”.\nTry a different search.").arg(m_searchQuery));
        return;
    }
    m_emptyState->setText(
        tr("No photos or videos yet.\n"
           "Drop files here or use Upload to add media."));
}

void LibraryPage::updateAutoCheckTimer()
{
    if (m_client->isConfigured() && isVisible() && m_searchQuery.isEmpty()) {
        if (!m_autoCheckTimer->isActive())
            m_autoCheckTimer->start();
    } else {
        m_autoCheckTimer->stop();
    }
}

void LibraryPage::updateEndpointHint()
{
    if (!m_client->isConfigured() || m_loading || !m_searchQuery.isEmpty())
        return;
    if (!m_client->connection().hasLocalServerUrl())
        return;
    if (m_assets.isEmpty())
        return;

    const QString count = tr("%n item(s)", nullptr, m_assets.size());
    m_status->setText(
        m_client->usingLocalEndpoint()
            ? tr("%1 · local").arg(count)
            : tr("%1 · remote").arg(count));
}

void LibraryPage::handleActiveEndpointChanged(bool usingLocal, const QString &activeUrl)
{
    Q_UNUSED(usingLocal);
    Q_UNUSED(activeUrl);
    updateEndpointHint();
}

void LibraryPage::openAsset(const ImmichAsset &asset)
{
    if (asset.isVideo()) {
        auto *player = new VideoPlayerDialog(m_client, asset, this);
        connect(player, &VideoPlayerDialog::downloadRequested, this,
                &LibraryPage::downloadAsset);
        connect(player, &VideoPlayerDialog::trashRequested, this, &LibraryPage::trashAsset);
        connect(player, &VideoPlayerDialog::deleteRequested, this,
                &LibraryPage::deleteAssetPermanently);
        connect(m_client, &ImmichClient::assetsDeleted, player,
                [player, id = asset.id](const QStringList &ids, bool) {
                    if (ids.contains(id))
                        player->close();
                });
        player->show();
        return;
    }

    auto *dialog = new QDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowTitle(asset.fileName.isEmpty() ? tr("Media preview") : asset.fileName);
    dialog->resize(960, 700);

    auto *layout = new QVBoxLayout(dialog);
    auto *preview = new QLabel(tr("Loading preview…"), dialog);
    preview->setAlignment(Qt::AlignCenter);
    preview->setMinimumSize(480, 320);
    preview->setProperty("mediaPreview", true);
    layout->addWidget(preview, 1);

    auto *buttons = new QDialogButtonBox(dialog);
    auto *downloadButton = buttons->addButton(tr("Download"), QDialogButtonBox::ActionRole);
    auto *trashButton = buttons->addButton(tr("Move to trash"), QDialogButtonBox::ActionRole);
    auto *deleteButton =
        buttons->addButton(tr("Delete permanently"), QDialogButtonBox::DestructiveRole);
    buttons->addButton(QDialogButtonBox::Close);
    connect(downloadButton, &QPushButton::clicked, this, [this, asset] {
        downloadAsset(asset);
    });
    connect(trashButton, &QPushButton::clicked, this, [this, asset] {
        trashAsset(asset);
    });
    connect(deleteButton, &QPushButton::clicked, this, [this, asset] {
        deleteAssetPermanently(asset);
    });
    connect(buttons, &QDialogButtonBox::rejected, dialog, &QDialog::reject);
    connect(m_client, &ImmichClient::assetsDeleted, dialog,
            [dialog, id = asset.id](const QStringList &ids, bool) {
                if (ids.contains(id))
                    dialog->close();
            });
    layout->addWidget(buttons);

    connect(m_client, &ImmichClient::previewLoaded, dialog,
            [preview, id = asset.id](const QString &assetId, const QPixmap &pixmap) {
                if (assetId != id)
                    return;
                preview->setPixmap(pixmap.scaled(
                    preview->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
            });
    connect(m_client, &ImmichClient::imageLoadFailed, dialog,
            [preview, id = asset.id](const QString &assetId, const QString &resultSize,
                                     const QString &message) {
                if (assetId == id && resultSize == QStringLiteral("preview"))
                    preview->setText(tr("Preview unavailable\n%1").arg(message));
            });
    m_client->loadPreview(asset.id);
    dialog->show();
}

void LibraryPage::downloadAsset(const ImmichAsset &asset)
{
    if (!m_client->isConfigured() || asset.id.isEmpty())
        return;

    const QString suggestedName =
        asset.fileName.isEmpty() ? QStringLiteral("immich-%1").arg(asset.id) : asset.fileName;
    const QString downloads =
        QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    const QString path = QFileDialog::getSaveFileName(
        this, tr("Download media"), QStringLiteral("%1/%2").arg(downloads, suggestedName),
        mediaFileFilter());
    if (path.isEmpty())
        return;

    m_status->setText(tr("Downloading %1…").arg(suggestedName));
    m_client->downloadAsset(asset.id, path, suggestedName);
}

void LibraryPage::trashAsset(const ImmichAsset &asset)
{
    confirmAndDelete(asset, false);
}

void LibraryPage::deleteAssetPermanently(const ImmichAsset &asset)
{
    confirmAndDelete(asset, true);
}

void LibraryPage::confirmAndDelete(const ImmichAsset &asset, bool permanent)
{
    if (!m_client->isConfigured() || asset.id.isEmpty())
        return;

    const QString name =
        asset.fileName.isEmpty() ? tr("this item") : QStringLiteral("“%1”").arg(asset.fileName);
    const QString title = permanent ? tr("Delete permanently") : tr("Move to trash");
    const QString text =
        permanent ? tr("Permanently delete %1? This cannot be undone.").arg(name)
                  : tr("Move %1 to trash? You can restore it later from Immich.").arg(name);

    QMessageBox box(this);
    box.setIcon(permanent ? QMessageBox::Warning : QMessageBox::Question);
    box.setWindowTitle(title);
    box.setText(text);
    auto *confirm = box.addButton(permanent ? tr("Delete permanently") : tr("Move to trash"),
                                  QMessageBox::DestructiveRole);
    box.addButton(QMessageBox::Cancel);
    box.exec();
    if (box.clickedButton() != confirm)
        return;

    m_status->setText(permanent ? tr("Deleting…") : tr("Moving to trash…"));
    m_client->deleteAssets({asset.id}, permanent);
}

void LibraryPage::handleAssetsDeleted(const QStringList &assetIds, bool permanent)
{
    removeAssetsFromTimeline(assetIds);
    m_status->setText(permanent ? tr("Deleted %n item(s).", nullptr, assetIds.size())
                                : tr("Moved %n item(s) to trash.", nullptr, assetIds.size()));
    updateEndpointHint();
}

void LibraryPage::removeAssetsFromTimeline(const QStringList &assetIds)
{
    if (assetIds.isEmpty())
        return;

    const QSet<QString> ids(assetIds.begin(), assetIds.end());
    for (auto it = m_assets.begin(); it != m_assets.end();) {
        if (ids.contains(it->id))
            it = m_assets.erase(it);
        else
            ++it;
    }

    for (const QString &id : ids) {
        m_requestedThumbnails.remove(id);
        if (MediaTile *tile = m_tilesById.take(id))
            tile->deleteLater();
    }

    for (auto sectionIt = m_sections.begin(); sectionIt != m_sections.end();) {
        DaySection &section = *sectionIt;
        QList<MediaTile *> remaining;
        remaining.reserve(section.tiles.size());
        for (MediaTile *tile : section.tiles) {
            if (tile && m_tilesById.contains(tile->asset().id))
                remaining.append(tile);
        }
        section.tiles = remaining;
        if (section.tiles.isEmpty()) {
            if (section.header)
                section.header->deleteLater();
            sectionIt = m_sections.erase(sectionIt);
        } else {
            ++sectionIt;
        }
    }

    if (!m_assets.isEmpty())
        m_newestAssetId = m_assets.first().id;
    else
        m_newestAssetId.clear();

    scheduleLayout();
    updateEmptyState();
}

void LibraryPage::chooseFilesToUpload()
{
    if (!m_client->isConfigured()) {
        m_status->setText(tr("Connect to Immich in Settings before uploading."));
        return;
    }

    const QStringList paths = QFileDialog::getOpenFileNames(
        this, tr("Upload media"),
        QStandardPaths::writableLocation(QStandardPaths::PicturesLocation),
        mediaFileFilter());
    enqueueUploads(paths);
}

void LibraryPage::enqueueUploads(const QStringList &paths)
{
    QStringList uploadable;
    uploadable.reserve(paths.size());
    for (const QString &path : paths) {
        if (isUploadableFile(path))
            uploadable.append(QFileInfo(path).absoluteFilePath());
    }
    if (uploadable.isEmpty()) {
        m_status->setText(tr("No supported media files to upload."));
        return;
    }

    m_uploadsTotal += uploadable.size();
    m_status->setText(tr("Uploading %1 file(s)…").arg(m_client->pendingUploadCount() +
                                                      uploadable.size()));
    m_client->uploadAssets(uploadable);
}

void LibraryPage::handleUploadProgress(const QString &filePath, qint64 bytesSent,
                                       qint64 bytesTotal)
{
    const QString name = QFileInfo(filePath).fileName();
    if (bytesTotal > 0) {
        const int percent = static_cast<int>((bytesSent * 100) / bytesTotal);
        m_status->setText(tr("Uploading %1… %2% (%3 left)")
                              .arg(name)
                              .arg(percent)
                              .arg(m_client->pendingUploadCount()));
    } else {
        m_status->setText(tr("Uploading %1…").arg(name));
    }
}

void LibraryPage::handleAssetUploaded(const QString &filePath, const QString &assetId,
                                      bool duplicate)
{
    Q_UNUSED(assetId);
    ++m_uploadsCompleted;
    const QString name = QFileInfo(filePath).fileName();
    const int remaining = m_client->pendingUploadCount();
    if (remaining == 0) {
        const int completed = m_uploadsCompleted;
        const int failed = m_uploadsFailed;
        if (failed > 0) {
            m_status->setText(
                tr("Upload finished: %1 ok, %2 failed").arg(completed).arg(failed));
        } else if (duplicate && completed == 1) {
            m_status->setText(tr("Already on server: %1").arg(name));
        } else {
            m_status->setText(tr("Uploaded %n file(s).", nullptr, completed));
        }
        m_uploadsCompleted = 0;
        m_uploadsFailed = 0;
        m_uploadsTotal = 0;
        if (completed > 0)
            QTimer::singleShot(600, this, &LibraryPage::refresh);
    } else {
        m_status->setText(
            duplicate ? tr("Duplicate skipped: %1 (%2 left)").arg(name).arg(remaining)
                      : tr("Uploaded %1 (%2 left)").arg(name).arg(remaining));
    }
}

void LibraryPage::handleDownloadProgress(const QString &assetId, qint64 bytesReceived,
                                         qint64 bytesTotal)
{
    Q_UNUSED(assetId);
    if (bytesTotal > 0) {
        const int percent = static_cast<int>((bytesReceived * 100) / bytesTotal);
        m_status->setText(tr("Downloading… %1%").arg(percent));
    }
}

void LibraryPage::handleAssetDownloaded(const QString &assetId, const QString &destinationPath)
{
    Q_UNUSED(assetId);
    m_status->setText(tr("Saved to %1").arg(QFileInfo(destinationPath).absoluteFilePath()));
}

bool LibraryPage::isUploadableFile(const QString &path) const
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
    return info.exists() && info.isFile() &&
           extensions.contains(info.suffix().toLower());
}

QStringList LibraryPage::uploadableLocalPaths(const QList<QUrl> &urls) const
{
    QStringList paths;
    for (const QUrl &url : urls) {
        if (!url.isLocalFile())
            continue;
        const QString path = url.toLocalFile();
        if (isUploadableFile(path))
            paths.append(path);
    }
    return paths;
}

void LibraryPage::setDropHighlight(bool active)
{
    if (m_dropActive == active)
        return;
    m_dropActive = active;
    if (!active) {
        m_dropOverlay->hide();
        return;
    }
    m_dropOverlay->setGeometry(rect());
    m_dropOverlay->setStyleSheet(
        QStringLiteral("QLabel#libraryDropOverlay {"
                       "background: rgba(20, 20, 24, 180);"
                       "color: white;"
                       "font-size: 18px;"
                       "font-weight: 600;"
                       "border: 2px dashed rgba(166, 133, 226, 220);"
                       "margin: 12px;"
                       "}"));
    m_dropOverlay->show();
    m_dropOverlay->raise();
}

bool LibraryPage::handleDragEvent(QEvent *event)
{
    if (!m_client->isConfigured())
        return false;

    switch (event->type()) {
    case QEvent::DragEnter:
    case QEvent::DragMove: {
        auto *dragEvent = static_cast<QDragMoveEvent *>(event);
        if (!dragEvent->mimeData() || !dragEvent->mimeData()->hasUrls())
            return false;
        if (uploadableLocalPaths(dragEvent->mimeData()->urls()).isEmpty())
            return false;
        dragEvent->acceptProposedAction();
        setDropHighlight(true);
        return true;
    }
    case QEvent::DragLeave:
        setDropHighlight(false);
        event->accept();
        return true;
    case QEvent::Drop: {
        auto *dropEvent = static_cast<QDropEvent *>(event);
        setDropHighlight(false);
        const QStringList paths = uploadableLocalPaths(dropEvent->mimeData()->urls());
        if (paths.isEmpty())
            return false;
        dropEvent->acceptProposedAction();
        enqueueUploads(paths);
        return true;
    }
    default:
        return false;
    }
}

bool LibraryPage::eventFilter(QObject *watched, QEvent *event)
{
    Q_UNUSED(watched);
    if (handleDragEvent(event))
        return true;
    return QWidget::eventFilter(watched, event);
}

void LibraryPage::dragEnterEvent(QDragEnterEvent *event)
{
    if (!handleDragEvent(event))
        QWidget::dragEnterEvent(event);
}

void LibraryPage::dragLeaveEvent(QDragLeaveEvent *event)
{
    if (!handleDragEvent(event))
        QWidget::dragLeaveEvent(event);
}

void LibraryPage::dropEvent(QDropEvent *event)
{
    if (!handleDragEvent(event))
        QWidget::dropEvent(event);
}

void LibraryPage::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    layoutTimeline();
    if (m_dropActive)
        m_dropOverlay->setGeometry(rect());
}

void LibraryPage::hideEvent(QHideEvent *event)
{
    QWidget::hideEvent(event);
    for (MediaTile *tile : std::as_const(m_tilesById))
        tile->clearThumbnail();
    setDropHighlight(false);
    updateAutoCheckTimer();
}

void LibraryPage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    scheduleVisibleMediaUpdate();
    updateAutoCheckTimer();
    QTimer::singleShot(0, this, &LibraryPage::checkForNewPhotos);
}

} // namespace Aurora
