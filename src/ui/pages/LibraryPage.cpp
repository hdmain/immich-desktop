#include "ui/pages/LibraryPage.h"

#include "ui/widgets/MediaTile.h"
#include "ui/widgets/VideoPlayerDialog.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QHideEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QShowEvent>
#include <QTimer>
#include <QVBoxLayout>

#include <utility>

namespace Aurora {
namespace {

constexpr int kGap = 3;
constexpr int kSidePad = 12;
constexpr int kTargetRowHeight = 132;
constexpr int kMaxRowHeight = 148;
constexpr int kMinRowHeight = 96;

} // namespace

LibraryPage::LibraryPage(ImmichClient *client, QWidget *parent)
    : QWidget(parent)
    , m_client(client)
    , m_scrollArea(new QScrollArea(this))
    , m_timelineHost(new QWidget)
    , m_status(new QLabel(this))
    , m_emptyState(new QLabel(this))
    , m_refreshButton(new QPushButton(tr("Refresh"), this))
    , m_layoutTimer(new QTimer(this))
    , m_visibilityTimer(new QTimer(this))
{
    m_layoutTimer->setSingleShot(true);
    m_layoutTimer->setInterval(40);
    connect(m_layoutTimer, &QTimer::timeout, this, &LibraryPage::layoutTimeline);
    m_visibilityTimer->setSingleShot(true);
    m_visibilityTimer->setInterval(60);
    connect(m_visibilityTimer, &QTimer::timeout,
            this, &LibraryPage::updateVisibleMedia);
    setObjectName(QStringLiteral("libraryPage"));

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
    toolbarLayout->addWidget(m_refreshButton);

    m_timelineHost->setObjectName(QStringLiteral("timelineHost"));
    m_scrollArea->setObjectName(QStringLiteral("libraryScroll"));
    m_scrollArea->setWidgetResizable(false);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setWidget(m_timelineHost);
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
    root->addWidget(toolbar);
    root->addWidget(m_emptyState);
    root->addWidget(m_scrollArea, 1);

    connect(m_refreshButton, &QPushButton::clicked, this, &LibraryPage::refresh);
    connect(m_client, &ImmichClient::assetsLoaded, this, &LibraryPage::showAssets);
    connect(m_client, &ImmichClient::thumbnailLoaded, this, &LibraryPage::showThumbnail);
    connect(m_client, &ImmichClient::imageLoadFailed,
            this, &LibraryPage::showThumbnailError);
    connect(m_client, &ImmichClient::requestFailed, this, &LibraryPage::showRequestError);
    connect(m_client, &ImmichClient::configurationChanged, this, [this](bool configured) {
        if (configured)
            refresh();
        else {
            clearTimeline();
            updateEmptyState();
        }
    });

    updateEmptyState();
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

void LibraryPage::requestPage(int page, bool append)
{
    if (!m_client->isConfigured()) {
        updateEmptyState();
        return;
    }
    m_loading = true;
    m_appendRequest = append;
    m_refreshButton->setEnabled(false);
    m_status->setText(append ? tr("Loading more media…") : tr("Loading your library…"));
    m_client->loadAssets(page);
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

void LibraryPage::showAssets(const QList<ImmichAsset> &assets, const QString &nextPage)
{
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
    }

    m_nextPage = nextPage;
    m_loading = false;
    m_refreshButton->setEnabled(true);
    m_status->setText(tr("%n item(s)", nullptr, m_assets.size()));
    scheduleLayout();
    updateEmptyState();
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
    m_loading = false;
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
    m_emptyState->setText(
        m_client->isConfigured()
            ? tr("No photos or videos were returned by this Immich server.\n"
                 "Check that the API key has asset.read and asset.view permissions.")
            : tr("Connect to your Immich server in Settings → Immich Server\n"
                 "to browse photos and videos."));
}

void LibraryPage::openAsset(const ImmichAsset &asset)
{
    if (asset.isVideo()) {
        auto *player = new VideoPlayerDialog(m_client, asset, this);
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

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, dialog);
    connect(buttons, &QDialogButtonBox::rejected, dialog, &QDialog::reject);
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

void LibraryPage::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    layoutTimeline();
}

void LibraryPage::hideEvent(QHideEvent *event)
{
    QWidget::hideEvent(event);
    for (MediaTile *tile : std::as_const(m_tilesById))
        tile->clearThumbnail();
}

void LibraryPage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    scheduleVisibleMediaUpdate();
}

} // namespace Aurora
