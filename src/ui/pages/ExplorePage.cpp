#include "ui/pages/ExplorePage.h"

#include "core/AppSettings.h"
#include "ui/widgets/MediaTile.h"
#include "ui/widgets/VideoHoverPreview.h"
#include "ui/widgets/VideoPlayerDialog.h"
#include "ui/widgets/ZoomPanWidget.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QHideEvent>
#include <QKeyEvent>
#include <QLabel>
#include <QLocale>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QShowEvent>
#include <QStackedWidget>
#include <QTimer>
#include <QVBoxLayout>

namespace Aurora {

namespace {
constexpr int kSidePad = 16;
constexpr int kGap = 4;
constexpr int kTargetRowHeight = 220;
constexpr int kMinRowHeight = 140;
constexpr int kMaxRowHeight = 320;

QPixmap circularPixmap(const QPixmap &source, const QSize &size)
{
    if (source.isNull() || !size.isValid() || size.width() <= 0 || size.height() <= 0)
        return {};
    const QPixmap scaled =
        source.scaled(size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    QPixmap circular(size);
    circular.fill(Qt::transparent);
    QPainter painter(&circular);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    QPainterPath path;
    path.addEllipse(QRect(QPoint(0, 0), size));
    painter.setClipPath(path);
    const int x = qMax(0, (scaled.width() - size.width()) / 2);
    const int y = qMax(0, (scaled.height() - size.height()) / 2);
    painter.drawPixmap(0, 0, scaled, x, y, size.width(), size.height());
    return circular;
}
} // namespace

ExploreCard::ExploreCard(Style style, QWidget *parent)
    : QFrame(parent)
    , m_style(style)
    , m_image(new QLabel(this))
    , m_caption(new QLabel(this))
{
    setObjectName(QStringLiteral("exploreCard"));
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::StrongFocus);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    m_image->setAlignment(Qt::AlignCenter);
    m_caption->setAlignment(Qt::AlignCenter);
    m_caption->setProperty("subheading", true);
    m_caption->setWordWrap(true);

    if (style == Style::Person) {
        setFixedSize(108, 136);
        m_image->setFixedSize(96, 96);
    } else if (style == Style::Place) {
        setFixedSize(168, 140);
        m_image->setFixedSize(168, 100);
    } else {
        setFixedSize(140, 140);
        m_image->setFixedSize(140, 112);
        m_caption->hide();
    }

    m_image->setStyleSheet(QStringLiteral(
        "QLabel { background: rgba(127,127,127,40); border-radius: 8px; }"));
    layout->addWidget(m_image, 0, Qt::AlignHCenter);
    layout->addWidget(m_caption);
}

void ExploreCard::setCaption(const QString &text)
{
    m_caption->setText(text);
}

void ExploreCard::setPixmap(const QPixmap &pixmap)
{
    if (pixmap.isNull()) {
        m_image->clear();
        m_image->setText(QStringLiteral("…"));
        return;
    }

    if (m_style == Style::Person) {
        m_image->setPixmap(circularPixmap(pixmap, m_image->size()));
        m_image->setStyleSheet(QStringLiteral(
            "QLabel { background: transparent; border-radius: 48px; }"));
    } else {
        m_image->setPixmap(pixmap.scaled(m_image->size(), Qt::KeepAspectRatioByExpanding,
                                         Qt::FastTransformation));
    }
}

void ExploreCard::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        emit activated();
    QFrame::mouseReleaseEvent(event);
}

void ExploreCard::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter ||
        event->key() == Qt::Key_Space) {
        emit activated();
        return;
    }
    QFrame::keyPressEvent(event);
}

ExplorePage::ExplorePage(ImmichClient *client, QWidget *parent)
    : QWidget(parent)
    , m_client(client)
    , m_stack(new QStackedWidget(this))
    , m_collectionCompactGrid(AppSettings().loadTimeline().compactGrid)
{
    setObjectName(QStringLiteral("explorePage"));

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);
    root->addWidget(m_stack, 1);

    // ---- Browse ----
    m_browsePage = new QWidget;
    auto *browseRoot = new QVBoxLayout(m_browsePage);
    browseRoot->setContentsMargins(0, 0, 0, 0);
    browseRoot->setSpacing(0);

    auto *toolbar = new QWidget(m_browsePage);
    auto *toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(16, 12, 16, 10);
    toolbarLayout->setSpacing(12);

    auto *headingColumn = new QVBoxLayout;
    headingColumn->setSpacing(2);
    auto *heading = new QLabel(tr("Explore"), toolbar);
    heading->setProperty("heading", true);
    m_browseStatus = new QLabel(tr("People, places, and recent media"), toolbar);
    m_browseStatus->setProperty("subheading", true);
    headingColumn->addWidget(heading);
    headingColumn->addWidget(m_browseStatus);
    toolbarLayout->addLayout(headingColumn, 1);
    m_refreshButton = new QPushButton(tr("Refresh"), toolbar);
    toolbarLayout->addWidget(m_refreshButton);

    m_browseScroll = new QScrollArea(m_browsePage);
    m_browseScroll->setObjectName(QStringLiteral("exploreScroll"));
    m_browseScroll->setWidgetResizable(true);
    m_browseScroll->setFrameShape(QFrame::NoFrame);
    m_browseContent = new QWidget;
    m_browseLayout = new QVBoxLayout(m_browseContent);
    m_browseLayout->setContentsMargins(16, 8, 16, 24);
    m_browseLayout->setSpacing(18);

    auto createSection = [this](SectionRow *section, const QString &title, int rowHeight) {
        section->header = new QLabel(title, m_browseContent);
        section->header->setProperty("section", true);
        section->header->hide();

        section->scroll = new QScrollArea(m_browseContent);
        section->scroll->setWidgetResizable(false);
        section->scroll->setFrameShape(QFrame::NoFrame);
        section->scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        section->scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        section->scroll->setFixedHeight(rowHeight);
        section->scroll->hide();

        section->host = new QWidget;
        auto *layout = new QHBoxLayout(section->host);
        layout->setContentsMargins(0, 0, 0, 8);
        layout->setSpacing(12);
        layout->addStretch();
        section->scroll->setWidget(section->host);

        m_browseLayout->addWidget(section->header);
        m_browseLayout->addWidget(section->scroll);
    };

    createSection(&m_peopleSection, tr("People"), 168);
    createSection(&m_placesSection, tr("Places"), 168);
    createSection(&m_recentSection, tr("Recently added"), 168);
    m_browseLayout->addStretch();
    m_browseScroll->setWidget(m_browseContent);

    m_emptyState = new QLabel(m_browsePage);
    m_emptyState->setAlignment(Qt::AlignCenter);
    m_emptyState->setWordWrap(true);
    m_emptyState->setProperty("subheading", true);
    m_emptyState->setMinimumHeight(180);

    browseRoot->addWidget(toolbar);
    browseRoot->addWidget(m_emptyState);
    browseRoot->addWidget(m_browseScroll, 1);

    // ---- Collection (Library-style) ----
    m_collectionPage = new QWidget;
    auto *collectionRoot = new QVBoxLayout(m_collectionPage);
    collectionRoot->setContentsMargins(0, 0, 0, 0);
    collectionRoot->setSpacing(0);

    auto *collectionToolbar = new QWidget(m_collectionPage);
    collectionToolbar->setObjectName(QStringLiteral("libraryToolbar"));
    auto *collectionToolbarLayout = new QHBoxLayout(collectionToolbar);
    collectionToolbarLayout->setContentsMargins(16, 12, 16, 12);
    collectionToolbarLayout->setSpacing(14);

    m_collectionBack = new QPushButton(tr("← Explore"), collectionToolbar);
    m_collectionBack->setCursor(Qt::PointingHandCursor);

    m_collectionHero = new QLabel(collectionToolbar);
    m_collectionHero->setFixedSize(64, 64);
    m_collectionHero->setAlignment(Qt::AlignCenter);

    auto *collectionText = new QVBoxLayout;
    collectionText->setSpacing(2);
    m_collectionTitle = new QLabel(collectionToolbar);
    m_collectionTitle->setProperty("heading", true);
    m_collectionSubtitle = new QLabel(collectionToolbar);
    m_collectionSubtitle->setProperty("subheading", true);
    m_collectionSubtitle->setWordWrap(true);
    collectionText->addWidget(m_collectionTitle);
    collectionText->addWidget(m_collectionSubtitle);

    collectionToolbarLayout->addWidget(m_collectionBack);
    collectionToolbarLayout->addWidget(m_collectionHero);
    collectionToolbarLayout->addLayout(collectionText, 1);

    m_collectionScroll = new QScrollArea(m_collectionPage);
    m_collectionScroll->setObjectName(QStringLiteral("libraryScroll"));
    // Manual geometry like Library: resizable=true would clamp the host to the
    // viewport height and make the person/place gallery unscrollable.
    m_collectionScroll->setWidgetResizable(false);
    m_collectionScroll->setFrameShape(QFrame::NoFrame);
    m_collectionScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_collectionHost = new QWidget;
    m_collectionHost->setObjectName(QStringLiteral("timelineHost"));
    m_collectionScroll->setWidget(m_collectionHost);
    m_collectionScroll->viewport()->installEventFilter(this);
    connect(m_collectionScroll->verticalScrollBar(), &QScrollBar::valueChanged, this,
            [this](int) { scheduleCollectionVisibility(); });

    m_collectionHoverPreview =
        new VideoHoverPreview(m_client, m_collectionHost, this);
    m_collectionHoverPreview->setEnabled(
        AppSettings().loadPlayback().hoverPreviewEnabled);

    m_collectionLoadMore = new QPushButton(tr("Load more"), m_collectionPage);
    m_collectionLoadMore->setVisible(false);

    m_collectionLayoutTimer = new QTimer(this);
    m_collectionLayoutTimer->setSingleShot(true);
    m_collectionLayoutTimer->setInterval(40);
    connect(m_collectionLayoutTimer, &QTimer::timeout, this, &ExplorePage::layoutCollection);

    m_collectionVisibilityTimer = new QTimer(this);
    m_collectionVisibilityTimer->setSingleShot(true);
    m_collectionVisibilityTimer->setInterval(60);
    connect(m_collectionVisibilityTimer, &QTimer::timeout, this,
            &ExplorePage::updateVisibleCollectionThumbs);

    collectionRoot->addWidget(collectionToolbar);
    collectionRoot->addWidget(m_collectionScroll, 1);
    collectionRoot->addWidget(m_collectionLoadMore);

    // ---- Preview ----
    m_previewPage = new QWidget;
    auto *previewRoot = new QVBoxLayout(m_previewPage);
    previewRoot->setContentsMargins(0, 0, 0, 0);
    previewRoot->setSpacing(0);

    auto *previewToolbar = new QWidget(m_previewPage);
    auto *previewToolbarLayout = new QHBoxLayout(previewToolbar);
    previewToolbarLayout->setContentsMargins(16, 12, 16, 10);
    previewToolbarLayout->setSpacing(12);
    m_previewBack = new QPushButton(tr("← Back"), previewToolbar);
    m_previewBack->setCursor(Qt::PointingHandCursor);
    m_previewTitle = new QLabel(previewToolbar);
    m_previewTitle->setProperty("heading", true);
    previewToolbarLayout->addWidget(m_previewBack);
    previewToolbarLayout->addWidget(m_previewTitle, 1);

    m_previewView = new ZoomPanWidget(m_previewPage);
    m_previewView->setPlaceholderText(tr("Loading preview…"));
    previewRoot->addWidget(previewToolbar);
    previewRoot->addWidget(m_previewView, 1);

    m_stack->addWidget(m_browsePage);
    m_stack->addWidget(m_collectionPage);
    m_stack->addWidget(m_previewPage);

    connect(m_refreshButton, &QPushButton::clicked, this, &ExplorePage::refresh);
    connect(m_collectionBack, &QPushButton::clicked, this, &ExplorePage::backToBrowse);
    connect(m_previewBack, &QPushButton::clicked, this, &ExplorePage::backToCollection);
    connect(m_collectionLoadMore, &QPushButton::clicked, this,
            &ExplorePage::requestCollectionNextPage);

    connect(m_client, &ImmichClient::exploreLoaded, this, &ExplorePage::showExplore);
    connect(m_client, &ImmichClient::filteredAssetsLoaded, this, &ExplorePage::showFilteredAssets);
    connect(m_client, &ImmichClient::personThumbnailLoaded, this,
            &ExplorePage::showPersonThumbnail);
    connect(m_client, &ImmichClient::thumbnailLoaded, this, &ExplorePage::showAssetThumbnail);
    connect(m_client, &ImmichClient::previewLoaded, this, &ExplorePage::showPreviewImage);
    connect(m_client, &ImmichClient::requestFailed, this, &ExplorePage::showRequestError);
    connect(m_client, &ImmichClient::configurationChanged, this, [this](bool configured) {
        m_refreshButton->setEnabled(configured);
        if (configured) {
            m_loadedOnce = false;
            if (isVisible())
                refresh();
        } else {
            clearSections();
            clearCollectionTimeline();
            setPage(Page::Browse);
            updateEmptyState();
        }
    });

    m_refreshButton->setEnabled(m_client->isConfigured());
    updateEmptyState();
}

void ExplorePage::setPage(Page page)
{
    m_stack->setCurrentIndex(static_cast<int>(page));
}

void ExplorePage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    m_collectionCompactGrid = AppSettings().loadTimeline().compactGrid;
    if (m_client->isConfigured() && !m_loadedOnce)
        refresh();
    if (m_stack->currentIndex() == static_cast<int>(Page::Collection)) {
        if (m_collectionPersonStyle && m_collectionFilterKind == QStringLiteral("person") &&
            !m_collectionFilterValue.isEmpty())
            m_client->loadPersonThumbnail(m_collectionFilterValue);
        scheduleCollectionVisibility();
    }
}

void ExplorePage::hideEvent(QHideEvent *event)
{
    if (m_collectionHoverPreview)
        m_collectionHoverPreview->stop();
    releaseUiPixmaps();
    QWidget::hideEvent(event);
}

void ExplorePage::releaseUiPixmaps()
{
    for (auto it = m_collectionTiles.begin(); it != m_collectionTiles.end(); ++it) {
        if (MediaTile *tile = it.value().data())
            tile->clearThumbnail();
    }
    m_collectionRequestedThumbs.clear();
    m_personThumbCache.clear();

    if (m_previewView)
        m_previewView->setPixmap({});
    if (m_collectionHero)
        setCollectionHero({}, m_collectionPersonStyle);
}

void ExplorePage::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (m_stack->currentIndex() == static_cast<int>(Page::Collection))
        scheduleCollectionLayout();
}

bool ExplorePage::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_collectionScroll->viewport() &&
        event->type() == QEvent::Resize)
        scheduleCollectionLayout();
    return QWidget::eventFilter(watched, event);
}

void ExplorePage::refresh()
{
    if (!m_client->isConfigured() || m_loading)
        return;
    m_loading = true;
    m_refreshButton->setEnabled(false);
    m_browseStatus->setText(tr("Loading explore…"));
    setPage(Page::Browse);
    if (!m_client->loadExplore()) {
        m_loading = false;
        m_refreshButton->setEnabled(true);
        m_browseStatus->setText(tr("Explore could not be refreshed right now."));
    }
}

void ExplorePage::clearSections()
{
    auto clearRow = [](SectionRow *section) {
        if (!section->host)
            return;
        auto *layout = qobject_cast<QHBoxLayout *>(section->host->layout());
        if (!layout)
            return;
        while (layout->count() > 1) {
            QLayoutItem *item = layout->takeAt(0);
            if (item->widget())
                item->widget()->deleteLater();
            delete item;
        }
        section->cards.clear();
        section->header->hide();
        section->scroll->hide();
    };
    clearRow(&m_peopleSection);
    clearRow(&m_placesSection);
    clearRow(&m_recentSection);
    m_personCards.clear();
    m_browseAssetCards.clear();
}

void ExplorePage::populateSection(SectionRow *section, bool visible)
{
    section->header->setVisible(visible);
    section->scroll->setVisible(visible);
    if (visible)
        section->host->adjustSize();
}

void ExplorePage::showExplore(const ImmichExploreData &data, bool fromCache)
{
    m_loading = false;
    m_loadedOnce = true;
    m_refreshButton->setEnabled(true);
    clearSections();
    setPage(Page::Browse);

    auto *peopleLayout = qobject_cast<QHBoxLayout *>(m_peopleSection.host->layout());
    for (const ImmichPerson &person : data.people) {
        auto *card = new ExploreCard(ExploreCard::Style::Person, m_peopleSection.host);
        card->setCaption(person.name.isEmpty() ? tr("Unknown") : person.name);
        card->setPixmap({});
        peopleLayout->insertWidget(peopleLayout->count() - 1, card);
        m_peopleSection.cards.append(card);
        m_personCards.insert(person.id, card);
        connect(card, &ExploreCard::activated, this, [this, person] { openPerson(person); });
        m_client->loadPersonThumbnail(person.id);
    }
    populateSection(&m_peopleSection, !data.people.isEmpty());

    auto *placesLayout = qobject_cast<QHBoxLayout *>(m_placesSection.host->layout());
    for (const ImmichPlace &place : data.places) {
        auto *card = new ExploreCard(ExploreCard::Style::Place, m_placesSection.host);
        card->setCaption(place.city);
        card->setPixmap({});
        placesLayout->insertWidget(placesLayout->count() - 1, card);
        m_placesSection.cards.append(card);
        m_browseAssetCards.insert(place.sampleAsset.id, card);
        connect(card, &ExploreCard::activated, this, [this, place] { openPlace(place); });
        m_client->loadThumbnail(place.sampleAsset.id);
    }
    populateSection(&m_placesSection, !data.places.isEmpty());

    auto *recentLayout = qobject_cast<QHBoxLayout *>(m_recentSection.host->layout());
    for (const ImmichAsset &asset : data.recentAssets) {
        auto *card = new ExploreCard(ExploreCard::Style::Media, m_recentSection.host);
        card->setPixmap({});
        recentLayout->insertWidget(recentLayout->count() - 1, card);
        m_recentSection.cards.append(card);
        m_browseAssetCards.insert(asset.id, card);
        connect(card, &ExploreCard::activated, this, [this, asset] {
            m_previewFromCollection = false;
            openAsset(asset);
        });
        m_client->loadThumbnail(asset.id);
    }
    populateSection(&m_recentSection, !data.recentAssets.isEmpty());

    m_browseStatus->setText(
        fromCache
            ? tr("%1 people · %2 places · %3 recent · offline")
                  .arg(data.people.size())
                  .arg(data.places.size())
                  .arg(data.recentAssets.size())
            : tr("%1 people · %2 places · %3 recent")
                  .arg(data.people.size())
                  .arg(data.places.size())
                  .arg(data.recentAssets.size()));
    updateEmptyState();
}

void ExplorePage::setCollectionHero(const QPixmap &thumb, bool personStyle)
{
    if (thumb.isNull()) {
        m_collectionHero->clear();
        m_collectionHero->setStyleSheet(
            personStyle
                ? QStringLiteral(
                      "QLabel { background: rgba(127,127,127,40); border-radius: 32px; }")
                : QStringLiteral(
                      "QLabel { background: rgba(127,127,127,40); border-radius: 12px; }"));
        return;
    }

    if (personStyle) {
        m_collectionHero->setPixmap(circularPixmap(thumb, m_collectionHero->size()));
        m_collectionHero->setStyleSheet(QStringLiteral(
            "QLabel { background: transparent; border-radius: 32px; }"));
    } else {
        m_collectionHero->setPixmap(
            thumb.scaled(m_collectionHero->size(), Qt::KeepAspectRatioByExpanding,
                         Qt::FastTransformation));
        m_collectionHero->setStyleSheet(QStringLiteral(
            "QLabel { background: transparent; border-radius: 12px; }"));
    }
}

void ExplorePage::showPersonThumbnail(const QString &personId, const QPixmap &thumbnail)
{
    if (!thumbnail.isNull())
        m_personThumbCache.insert(personId, thumbnail);

    if (ExploreCard *card = m_personCards.value(personId))
        card->setPixmap(thumbnail);

    if (m_collectionPersonStyle && m_collectionFilterKind == QStringLiteral("person") &&
        m_collectionFilterValue == personId && !thumbnail.isNull() &&
        m_stack->currentIndex() == static_cast<int>(Page::Collection))
        setCollectionHero(thumbnail, true);
}

void ExplorePage::showAssetThumbnail(const QString &assetId, const QPixmap &thumbnail)
{
    for (ExploreCard *raw : m_browseAssetCards.values(assetId)) {
        if (raw)
            raw->setPixmap(thumbnail);
    }

    if (const QPointer<MediaTile> tile = m_collectionTiles.value(assetId))
        tile->setThumbnail(thumbnail);

    if (!m_collectionPersonStyle && m_collectionFilterKind == QStringLiteral("city") &&
        m_stack->currentIndex() == static_cast<int>(Page::Collection) &&
        assetId == m_collectionHeroAssetId && !thumbnail.isNull())
        setCollectionHero(thumbnail, false);
}

void ExplorePage::showPreviewImage(const QString &assetId, const QPixmap &preview)
{
    if (assetId != m_previewAssetId || m_stack->currentIndex() != static_cast<int>(Page::Preview))
        return;
    m_previewView->setPixmap(preview);
}

void ExplorePage::showRequestError(const QString &operation, const QString &message)
{
    if (operation != tr("Load explore"))
        return;
    if (m_collectionLoadingMore) {
        m_collectionLoadingMore = false;
        updateCollectionLoadMore(m_collectionNextPage);
        m_collectionSubtitle->setText(tr("Couldn't load more: %1").arg(message));
        return;
    }
    m_loading = false;
    m_refreshButton->setEnabled(true);
    m_browseStatus->setText(tr("%1 failed: %2").arg(operation, message));
    if (m_stack->currentIndex() == static_cast<int>(Page::Collection))
        m_collectionSubtitle->setText(tr("Couldn't load photos: %1").arg(message));
    updateEmptyState();
}

void ExplorePage::updateEmptyState()
{
    const bool onBrowse = m_stack->currentIndex() == static_cast<int>(Page::Browse);
    const bool empty = m_peopleSection.cards.isEmpty() && m_placesSection.cards.isEmpty() &&
                       m_recentSection.cards.isEmpty();
    m_emptyState->setVisible(onBrowse && empty);
    m_browseScroll->setVisible(onBrowse && !empty);
    if (!empty)
        return;
    m_emptyState->setText(
        m_client->isConfigured()
            ? tr("No people, places, or recent media yet.\n"
                 "Run face and location recognition on your Immich server, or add photos.")
            : tr("Connect to your Immich server in Settings → Immich Server\n"
                 "to explore people, places, and recent media."));
}

void ExplorePage::openPerson(const ImmichPerson &person)
{
    const QString title = person.name.isEmpty() ? tr("Unknown person") : person.name;
    m_pendingCollectionTitle = title;
    m_pendingPersonStyle = true;

    const QPixmap cachedFace = m_personThumbCache.value(person.id);
    showCollectionPage(title, tr("Loading photos…"), QStringLiteral("person"), person.id,
                       cachedFace, true);
    // Force a fresh face fetch so the hero updates even if a prior request is in flight.
    m_client->loadPersonThumbnail(person.id);
    m_client->loadAssetsForPerson(person.id);
}

void ExplorePage::openPlace(const ImmichPlace &place)
{
    m_pendingCollectionTitle = place.city;
    m_pendingPersonStyle = false;
    showCollectionPage(place.city, tr("Loading photos…"), QStringLiteral("city"), place.city,
                       {}, false);
    m_collectionHeroAssetId = place.sampleAsset.id;
    if (!place.sampleAsset.id.isEmpty())
        m_client->loadThumbnail(place.sampleAsset.id);
    m_client->loadAssetsForCity(place.city);
}

void ExplorePage::showCollectionPage(const QString &title, const QString &subtitle,
                                     const QString &filterKind, const QString &filterValue,
                                     const QPixmap &heroThumb, bool personStyle)
{
    m_collectionFilterKind = filterKind;
    m_collectionFilterValue = filterValue;
    m_collectionHeroAssetId.clear();
    m_collectionPersonStyle = personStyle;
    m_collectionNextPage.clear();
    m_collectionLoadingMore = false;
    m_collectionCompactGrid = AppSettings().loadTimeline().compactGrid;

    clearCollectionTimeline();

    m_collectionTitle->setText(title);
    m_collectionSubtitle->setText(subtitle);
    setCollectionHero(heroThumb, personStyle);

    m_collectionLoadMore->setVisible(false);
    setPage(Page::Collection);
}

void ExplorePage::clearCollectionTimeline()
{
    if (m_collectionHoverPreview)
        m_collectionHoverPreview->stop();

    for (DaySection &section : m_collectionSections) {
        if (section.header)
            section.header->deleteLater();
        for (MediaTile *tile : section.tiles)
            tile->deleteLater();
    }
    m_collectionSections.clear();
    m_collectionTiles.clear();
    m_collectionRequestedThumbs.clear();
    m_collectionHost->resize(m_collectionScroll->viewport()->width(), 1);
}

ExplorePage::DaySection *ExplorePage::sectionForDate(const QDate &date)
{
    for (DaySection &section : m_collectionSections) {
        if (section.date == date)
            return &section;
    }

    DaySection section;
    section.date = date;
    section.header = new QLabel(formatDayHeader(date), m_collectionHost);
    section.header->setObjectName(QStringLiteral("timelineDayHeader"));
    section.header->setProperty("section", true);
    m_collectionSections.append(section);
    return &m_collectionSections.last();
}

QString ExplorePage::formatDayHeader(const QDate &date) const
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

void ExplorePage::showFilteredAssets(const QString &filterKind, const QString &filterValue,
                                     const QList<ImmichAsset> &assets, const QString &nextPage)
{
    const bool append =
        m_collectionLoadingMore && m_collectionFilterKind == filterKind &&
        m_collectionFilterValue == filterValue &&
        m_stack->currentIndex() == static_cast<int>(Page::Collection);
    m_collectionLoadingMore = false;

    if (!append) {
        if (m_stack->currentIndex() != static_cast<int>(Page::Collection) ||
            m_collectionFilterKind != filterKind || m_collectionFilterValue != filterValue) {
            const QString title = !m_pendingCollectionTitle.isEmpty()
                                      ? m_pendingCollectionTitle
                                      : (filterKind == QStringLiteral("city") ? filterValue
                                                                              : tr("Photos"));
            const QPixmap hero =
                (filterKind == QStringLiteral("person"))
                    ? m_personThumbCache.value(filterValue)
                    : QPixmap();
            showCollectionPage(title, {}, filterKind, filterValue, hero,
                               m_pendingPersonStyle || filterKind == QStringLiteral("person"));
        } else {
            clearCollectionTimeline();
        }
        m_pendingCollectionTitle.clear();
    }

    if (assets.isEmpty() && m_collectionTiles.isEmpty()) {
        m_collectionSubtitle->setText(tr("Nothing here yet"));
        updateCollectionLoadMore({});
        scheduleCollectionLayout();
        return;
    }

    appendCollectionAssets(assets);
    m_collectionSubtitle->setText(tr("%n item(s)", nullptr, m_collectionTiles.size()));
    updateCollectionLoadMore(nextPage);
    scheduleCollectionLayout();
    scheduleCollectionVisibility();
}

void ExplorePage::appendCollectionAssets(const QList<ImmichAsset> &assets)
{
    for (const ImmichAsset &asset : assets) {
        if (m_collectionTiles.contains(asset.id))
            continue;

        const QDate date = asset.takenAt.isValid() ? asset.takenAt.date() : QDate();
        DaySection *section = sectionForDate(date);
        auto *tile = new MediaTile(asset, m_collectionHost);
        tile->setHoverPreview(m_collectionHoverPreview);
        section->tiles.append(tile);
        m_collectionTiles.insert(asset.id, tile);
        connect(tile, &MediaTile::activated, this, [this, asset] {
            m_previewFromCollection = true;
            openAsset(asset);
        });
    }
}

void ExplorePage::scheduleCollectionLayout()
{
    if (!m_collectionLayoutTimer->isActive())
        m_collectionLayoutTimer->start();
}

void ExplorePage::scheduleCollectionVisibility()
{
    if (!m_collectionVisibilityTimer->isActive())
        m_collectionVisibilityTimer->start();
}

void ExplorePage::layoutCollection()
{
    if (m_stack->currentIndex() != static_cast<int>(Page::Collection))
        return;

    const int viewportWidth = m_collectionScroll->viewport()->width();
    const int availableWidth = qMax(240, viewportWidth - 2 * kSidePad);

    if (m_collectionCompactGrid) {
        constexpr int kGridGap = 3;
        constexpr int kTargetCellSize = 132;
        const int startY = 8;
        const int columns = qMax(1, (availableWidth + kGridGap) / (kTargetCellSize + kGridGap));
        const int cellSize = (availableWidth - (columns - 1) * kGridGap) / columns;

        int index = 0;
        for (DaySection &section : m_collectionSections) {
            if (section.header)
                section.header->hide();
            for (MediaTile *tile : section.tiles) {
                const int col = index % columns;
                const int row = index / columns;
                tile->setGeometry(kSidePad + col * (cellSize + kGridGap),
                                  startY + row * (cellSize + kGridGap), cellSize, cellSize);
                tile->show();
                ++index;
            }
        }
        const int totalRows = (index + columns - 1) / columns;
        m_collectionHost->resize(viewportWidth,
                                 startY + qMax(1, totalRows) * (cellSize + kGridGap) + 16);
        scheduleCollectionVisibility();
        return;
    }

    int y = 8;
    for (DaySection &section : m_collectionSections) {
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
        flushRow(false);
        y += 10;
    }

    m_collectionHost->resize(viewportWidth, qMax(y + 16, 80));
    scheduleCollectionVisibility();
}

void ExplorePage::updateVisibleCollectionThumbs()
{
    if (m_stack->currentIndex() != static_cast<int>(Page::Collection))
        return;

    const int viewportHeight = m_collectionScroll->viewport()->height();
    const int scrollTop = m_collectionScroll->verticalScrollBar()->value();
    const QRect buffered(0, qMax(0, scrollTop - viewportHeight), m_collectionHost->width(),
                         viewportHeight * 3);

    constexpr int kMaxRequests = 20;
    int requested = 0;
    for (auto it = m_collectionTiles.cbegin(); it != m_collectionTiles.cend(); ++it) {
        MediaTile *tile = it.value().data();
        if (!tile)
            continue;

        if (!buffered.intersects(tile->geometry())) {
            if (tile->hasThumbnail()) {
                tile->clearThumbnail();
                m_collectionRequestedThumbs.remove(it.key());
            }
            continue;
        }

        if (tile->hasThumbnail() || tile->hasThumbnailError() ||
            m_collectionRequestedThumbs.contains(it.key()))
            continue;
        m_collectionRequestedThumbs.insert(it.key());
        m_client->loadThumbnail(it.key());
        if (++requested >= kMaxRequests) {
            scheduleCollectionVisibility();
            break;
        }
    }
}

void ExplorePage::updateCollectionLoadMore(const QString &nextPage)
{
    m_collectionNextPage = nextPage;
    bool ok = false;
    const int page = nextPage.toInt(&ok);
    const bool hasMore = ok && page > 0;
    m_collectionLoadMore->setEnabled(hasMore);
    m_collectionLoadMore->setVisible(hasMore);
    m_collectionLoadMore->setText(tr("Load more"));
}

void ExplorePage::requestCollectionNextPage()
{
    bool ok = false;
    const int page = m_collectionNextPage.toInt(&ok);
    if (!ok || page <= 0 || m_collectionFilterKind.isEmpty() ||
        m_collectionFilterValue.isEmpty())
        return;

    m_collectionLoadingMore = true;
    m_collectionLoadMore->setEnabled(false);
    m_collectionLoadMore->setText(tr("Loading…"));

    if (m_collectionFilterKind == QStringLiteral("person"))
        m_client->loadAssetsForPerson(m_collectionFilterValue, page);
    else if (m_collectionFilterKind == QStringLiteral("city"))
        m_client->loadAssetsForCity(m_collectionFilterValue, page);
    else
        m_collectionLoadingMore = false;
}

void ExplorePage::backToBrowse()
{
    if (m_collectionHoverPreview)
        m_collectionHoverPreview->stop();
    m_collectionLoadingMore = false;
    m_collectionFilterKind.clear();
    m_collectionFilterValue.clear();
    clearCollectionTimeline();
    setPage(Page::Browse);
    updateEmptyState();
}

void ExplorePage::backToCollection()
{
    if (m_previewFromCollection && !m_collectionFilterKind.isEmpty())
        setPage(Page::Collection);
    else
        backToBrowse();
}

void ExplorePage::showPreviewPage(const ImmichAsset &asset)
{
    m_previewAssetId = asset.id;
    m_previewTitle->setText(asset.fileName.isEmpty() ? tr("Photo") : asset.fileName);
    m_previewView->setPixmap({});
    m_previewView->setPlaceholderText(tr("Loading preview…"));
    setPage(Page::Preview);
    m_client->loadPreview(asset.id);
}

void ExplorePage::openAsset(const ImmichAsset &asset)
{
    if (asset.isVideo()) {
        if (m_collectionHoverPreview)
            m_collectionHoverPreview->stop();
        auto *player = new VideoPlayerDialog(m_client, asset, this);
        player->show();
        return;
    }

    if (m_stack->currentIndex() != static_cast<int>(Page::Collection))
        m_previewFromCollection = false;
    showPreviewPage(asset);
}

} // namespace Aurora
