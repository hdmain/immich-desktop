#include "ui/pages/LibraryPage.h"

#include "ui/widgets/MediaTile.h"

#include <QDesktopServices>
#include <QDialog>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>

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
    , m_loadMoreButton(new QPushButton(tr("Load more"), this))
    , m_layoutTimer(new QTimer(this))
{
    m_layoutTimer->setSingleShot(true);
    m_layoutTimer->setInterval(40);
    connect(m_layoutTimer, &QTimer::timeout, this, &LibraryPage::layoutTimeline);
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

    m_emptyState->setAlignment(Qt::AlignCenter);
    m_emptyState->setWordWrap(true);
    m_emptyState->setProperty("subheading", true);
    m_emptyState->setMinimumHeight(180);
    m_loadMoreButton->setVisible(false);

    auto *footer = new QHBoxLayout;
    footer->setContentsMargins(0, 8, 0, 10);
    footer->addStretch();
    footer->addWidget(m_loadMoreButton);
    footer->addStretch();

    root->addWidget(toolbar);
    root->addWidget(m_emptyState);
    root->addWidget(m_scrollArea, 1);
    root->addLayout(footer);

    connect(m_refreshButton, &QPushButton::clicked, this, &LibraryPage::refresh);
    connect(m_loadMoreButton, &QPushButton::clicked, this, &LibraryPage::loadMore);
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

void LibraryPage::requestPage(int page, bool append)
{
    if (!m_client->isConfigured()) {
        updateEmptyState();
        return;
    }
    m_loading = true;
    m_appendRequest = append;
    m_refreshButton->setEnabled(false);
    m_loadMoreButton->setEnabled(false);
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
        m_client->loadThumbnail(asset.id);
    }

    m_nextPage = nextPage;
    m_loading = false;
    m_refreshButton->setEnabled(true);
    m_loadMoreButton->setEnabled(true);
    m_loadMoreButton->setVisible(!m_nextPage.isEmpty());
    m_status->setText(tr("%n item(s)", nullptr, m_assets.size()));
    scheduleLayout();
    updateEmptyState();
}

void LibraryPage::showThumbnail(const QString &assetId, const QPixmap &thumbnail)
{
    if (auto *tile = m_tilesById.value(assetId)) {
        tile->setThumbnail(thumbnail);
        scheduleLayout();
    }
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
    if (auto *tile = m_tilesById.value(assetId))
        tile->setThumbnailError(message);
}

void LibraryPage::showRequestError(const QString &operation, const QString &message)
{
    if (operation == tr("Load image"))
        return;
    m_loading = false;
    m_refreshButton->setEnabled(true);
    m_loadMoreButton->setEnabled(true);
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
    m_assets.clear();
    m_nextPage.clear();
}

void LibraryPage::layoutTimeline()
{
    const int viewportWidth = m_scrollArea->viewport()->width();
    const int availableWidth = qMax(240, viewportWidth - 2 * kSidePad);
    int y = 8;

    for (DaySection &section : m_sections) {
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
    }

    m_timelineHost->resize(viewportWidth, y + 16);
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

    auto *status = new QLabel(dialog);
    status->setProperty("subheading", true);
    status->setVisible(false);
    layout->addWidget(status);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, dialog);
    if (asset.isVideo()) {
        auto *playButton = buttons->addButton(tr("Play video"), QDialogButtonBox::ActionRole);
        playButton->setProperty("primary", true);
        auto playStream = [this, status, asset] {
            const QUrl streamUrl = m_client->videoStreamUrl(asset.id);
            if (!streamUrl.isValid()) {
                status->setVisible(true);
                status->setText(tr("Could not start video stream."));
                return;
            }
            status->setVisible(true);
            status->setText(tr("Streaming in your media player…"));
            QDesktopServices::openUrl(streamUrl);
        };
        connect(playButton, &QPushButton::clicked, dialog, playStream);
        // Videos open the stream immediately so playback can begin while buffering.
        QTimer::singleShot(0, dialog, playStream);
    }
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

} // namespace Aurora
