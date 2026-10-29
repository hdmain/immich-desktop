#include "ui/pages/ExplorePage.h"

#include "core/AppSettings.h"
#include "ui/FolderPaths.h"
#include "ui/GridZoom.h"
#include "ui/IconUtils.h"
#include "ui/TileImageScheduler.h"
#include "ui/widgets/FlowLayout.h"
#include "ui/widgets/MediaTile.h"
#include "ui/widgets/MediaViewerDialog.h"
#include "ui/widgets/ScrollToTopButton.h"
#include "ui/widgets/ShareLinkDialog.h"
#include "ui/widgets/SortButton.h"
#include "ui/widgets/Toast.h"
#include "ui/widgets/VideoHoverPreview.h"
#include "ui/widgets/ZoomSlider.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QHideEvent>
#include <QKeyEvent>
#include <QLabel>
#include <QLocale>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QShortcut>
#include <QShowEvent>
#include <QStackedWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <QWheelEvent>

namespace Aurora {

namespace {
constexpr int kSidePad = 16;
constexpr int kGap = 4;

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

void ExploreCard::setTileSize(int size)
{
    if (m_style == Style::Person) {
        // Face thumbnails are ~256px crops; growing past that only blurs them.
        const int cap = qRound(256 / devicePixelRatioF());
        const int diameter = qBound(64, qRound(size * 0.6), cap);
        m_image->setFixedSize(diameter, diameter);
        setFixedSize(diameter + 12, diameter + 40);
    } else if (m_style == Style::Place) {
        const int width = qMax(120, qRound(size * 0.95));
        m_image->setFixedSize(width, qRound(width * 0.6));
        setFixedSize(width, m_image->height() + 44);
    }
    showPixmap();
}

QSize ExploreCard::imageSize() const
{
    return m_image->size();
}

void ExploreCard::setPixmap(const QPixmap &pixmap)
{
    // A late small thumbnail must not replace a sharper cover loaded for zoom.
    if (!pixmap.isNull() && !m_source.isNull() &&
        qMax(pixmap.width(), pixmap.height()) < qMax(m_source.width(), m_source.height()))
        return;
    m_source = pixmap;
    showPixmap();
}

void ExploreCard::showPixmap()
{
    const QPixmap &pixmap = m_source;
    if (pixmap.isNull()) {
        m_image->clear();
        m_image->setText(QStringLiteral("…"));
        return;
    }

    const qreal ratio = devicePixelRatioF();
    const QSize deviceSize = m_image->size() * ratio;
    QPixmap shown;
    if (m_style == Style::Person) {
        shown = circularPixmap(pixmap, deviceSize);
        m_image->setStyleSheet(QStringLiteral("QLabel { background: transparent; }"));
    } else {
        const QPixmap scaled =
            pixmap.scaled(deviceSize, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        shown = scaled.copy(QRect(QPoint((scaled.width() - deviceSize.width()) / 2,
                                         (scaled.height() - deviceSize.height()) / 2),
                                  deviceSize));
    }
    shown.setDevicePixelRatio(ratio);
    m_image->setPixmap(shown);
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

ExplorePage::ExplorePage(ThemeManager *themeManager, ImmichClient *client, QWidget *parent)
    : QWidget(parent)
    , m_themeManager(themeManager)
    , m_client(client)
    , m_stack(new QStackedWidget(this))
    , m_collectionCompactGrid(AppSettings().loadTimeline().compactGrid)
    , m_collectionTileSize(GridZoom::clamp(AppSettings().loadTimeline().tileSize))
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
    configureActionButton(m_refreshButton, QStringLiteral(":/icons/refresh-cw.svg"));
    setActionButtonLabel(m_refreshButton, QString());
    m_refreshButton->setToolTip(tr("Refresh"));
    m_refreshButton->setAccessibleName(tr("Refresh"));
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
        section->header = new QWidget(m_browseContent);
        auto *headerLayout = new QHBoxLayout(section->header);
        headerLayout->setContentsMargins(0, 0, 0, 0);
        auto *label = new QLabel(title, section->header);
        label->setProperty("section", true);
        headerLayout->addWidget(label);
        headerLayout->addStretch();
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
    auto *viewAllRecent = new QPushButton(tr("View all"), m_recentSection.header);
    viewAllRecent->setProperty("linkButton", true);
    viewAllRecent->setCursor(Qt::PointingHandCursor);
    viewAllRecent->setToolTip(tr("Show the whole library, newest uploads first"));
    m_recentSection.header->layout()->addWidget(viewAllRecent);
    connect(viewAllRecent, &QPushButton::clicked, this, &ExplorePage::recentlyAddedRequested);
    createSection(&m_viewedSection, tr("Recently viewed"), 168);
    auto *viewAllViewed = new QPushButton(tr("View all"), m_viewedSection.header);
    viewAllViewed->setProperty("linkButton", true);
    viewAllViewed->setCursor(Qt::PointingHandCursor);
    viewAllViewed->setToolTip(tr("Photos you opened lately, on this computer"));
    m_viewedSection.header->layout()->addWidget(viewAllViewed);
    connect(viewAllViewed, &QPushButton::clicked, this, &ExplorePage::showRecentlyViewed);
    connect(m_client, &ImmichClient::recentlyViewedChanged, this, [this] {
        if (m_loadedOnce)
            populateViewedRow();
        if (m_collectionFilterKind == QStringLiteral("viewed") && m_client->recentlyViewed().isEmpty())
            reloadCollection(); // Cleared from Settings.
        // Turning "Remember photos I open" off removes the Last viewed sort.
        if (m_collectionFilterKind.isEmpty() || m_collectionFilterKind == QStringLiteral("viewed"))
            return;
        const AssetSort before = m_collectionSortButton->sort();
        m_collectionSortButton->setFields(collectionSortFields());
        m_collectionSortButton->setSort(effectiveCollectionSort());
        if (effectiveCollectionSort() != before)
            reloadCollection();
    });
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

    m_collectionHero = new QLabel(collectionToolbar);
    m_collectionHero->setFixedSize(64, 64);
    m_collectionHero->setAlignment(Qt::AlignCenter);

    auto *collectionText = new QVBoxLayout;
    collectionText->setSpacing(2);
    m_collectionTrail = new QWidget(collectionToolbar);
    auto *trailLayout = new QHBoxLayout(m_collectionTrail);
    trailLayout->setContentsMargins(0, 0, 0, 0);
    trailLayout->setSpacing(8);
    m_collectionTitle = new QLabel(m_collectionTrail);
    m_collectionTitle->setProperty("heading", true);
    trailLayout->addWidget(m_collectionTitle);
    trailLayout->addStretch(1);
    m_collectionSubtitle = new QLabel(collectionToolbar);
    m_collectionSubtitle->setProperty("subheading", true);
    m_collectionSubtitle->setWordWrap(true);
    collectionText->addWidget(m_collectionTrail);
    collectionText->addWidget(m_collectionSubtitle);

    collectionToolbarLayout->addWidget(m_collectionHero);
    collectionToolbarLayout->addLayout(collectionText, 1);
    m_collectionZoomSlider = new ZoomSlider(collectionToolbar);
    m_collectionZoomSlider->setValue(m_collectionTileSize);
    m_collectionZoomSlider->setCompact(true);
    m_restoreAllButton = new QPushButton(tr("Restore all"), collectionToolbar);
    configureActionButton(m_restoreAllButton, QStringLiteral(":/icons/rotate-ccw.svg"));
    m_emptyTrashButton = new QPushButton(tr("Empty trash"), collectionToolbar);
    configureActionButton(m_emptyTrashButton, QStringLiteral(":/icons/circle-x.svg"));
    for (QPushButton *button : {m_restoreAllButton, m_emptyTrashButton}) {
        button->setCursor(Qt::PointingHandCursor);
        button->hide();
        collectionToolbarLayout->addWidget(button);
    }
    connect(m_restoreAllButton, &QPushButton::clicked, this, [this] {
        if (QMessageBox::question(this, tr("Restore all"),
                                  tr("Restore everything in the trash to your library?")) ==
            QMessageBox::Yes)
            m_client->restoreAllFromTrash();
    });
    connect(m_emptyTrashButton, &QPushButton::clicked, this, [this] {
        if (QMessageBox::warning(this, tr("Empty trash"),
                                 tr("Permanently delete everything in the trash? This can't "
                                    "be undone."),
                                 QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel) ==
            QMessageBox::Yes)
            m_client->emptyTrash();
    });
    m_collectionSortButton = new SortButton(collectionToolbar);
    connect(m_collectionSortButton, &SortButton::sortChanged, this,
            &ExplorePage::chooseCollectionSort);
    connect(m_client, &ImmichClient::serverVersionChanged, this, [this] {
        if (m_collectionFilterKind.isEmpty())
            return;
        const AssetSort before = m_collectionSortButton->sort();
        m_collectionSortButton->setFields(collectionSortFields());
        m_collectionSortButton->setSort(effectiveCollectionSort());
        if (effectiveCollectionSort() != before)
            reloadCollection();
    });
    connect(m_client, &ImmichClient::albumOrderFailed, this,
            [this](const QString &, const QString &message) { Toast::showMessage(this, message); });
    connect(m_client, &ImmichClient::albumOrderSaved, this, [](const QString &albumId, bool) {
        // Immich has it now; following Immich keeps web UI changes in sync.
        const QString key = QStringLiteral("album:") + albumId;
        if (AppSettings().loadSort(key).field == AssetSort::Field::TakenAt)
            AppSettings().clearSort(key);
    });
    collectionToolbarLayout->addWidget(m_collectionSortButton);
    m_collectionShuffleButton = new QPushButton(collectionToolbar);
    configureActionButton(m_collectionShuffleButton, QStringLiteral(":/icons/shuffle.svg"));
    m_collectionShuffleButton->setToolTip(tr("Shuffle (again for a new mix)"));
    m_collectionShuffleButton->setAccessibleName(tr("Shuffle"));
    m_collectionShuffleButton->setCursor(Qt::PointingHandCursor);
    collectionToolbarLayout->addWidget(m_collectionShuffleButton);
    connect(m_collectionShuffleButton, &QPushButton::clicked, this,
            &ExplorePage::shuffleCollection);
    connect(m_client, &ImmichClient::randomAssetsLoaded, this, &ExplorePage::showRandomAssets);
    collectionToolbarLayout->addWidget(m_collectionZoomSlider);
    connect(m_collectionZoomSlider, &ZoomSlider::valueChanged, this,
            &ExplorePage::setCollectionTileSize);
    connect(m_collectionZoomSlider, &ZoomSlider::stepRequested, this, [this](int steps) {
        setCollectionTileSize(GridZoom::step(m_collectionTileSize, steps));
    });

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
    new ScrollToTopButton(m_collectionScroll);
    m_collectionScroll->viewport()->installEventFilter(this);
    connect(m_collectionScroll->verticalScrollBar(), &QScrollBar::valueChanged, this,
            [this](int) { scheduleCollectionVisibility(); });

    m_collectionHoverPreview =
        new VideoHoverPreview(m_client, m_collectionHost, this);
    m_collectionHoverPreview->setEnabled(
        AppSettings().loadPlayback().hoverPreviewEnabled);

    m_collectionLoadMore = new QPushButton(tr("Load more"), m_collectionPage);
    configureActionButton(m_collectionLoadMore, QStringLiteral(":/icons/chevron-down.svg"));
    m_collectionLoadMore->setVisible(false);

    m_collectionLayoutTimer = new QTimer(this);
    m_collectionLayoutTimer->setSingleShot(true);
    m_collectionLayoutTimer->setInterval(40);
    connect(m_collectionLayoutTimer, &QTimer::timeout, this, &ExplorePage::layoutCollection);

    m_collectionVisibilityTimer = new QTimer(this);
    m_collectionTileImages =
        new TileImageScheduler(m_client, [this] { scheduleCollectionVisibility(); }, this);
    m_collectionVisibilityTimer->setSingleShot(true);
    m_collectionVisibilityTimer->setInterval(60);
    connect(m_collectionVisibilityTimer, &QTimer::timeout, this,
            &ExplorePage::updateVisibleCollectionThumbs);

    m_collectionChips = new QWidget(m_collectionPage);
    auto *chipsLayout = new FlowLayout(m_collectionChips, 8);
    chipsLayout->setContentsMargins(16, 0, 16, 10);
    m_collectionChips->hide();

    collectionRoot->addWidget(collectionToolbar);
    collectionRoot->addWidget(m_collectionChips);
    collectionRoot->addWidget(m_collectionScroll, 1);
    collectionRoot->addWidget(m_collectionLoadMore);

    // ---- Index (all albums, tags, folders or people) ----
    m_indexPage = new QWidget;
    auto *indexRoot = new QVBoxLayout(m_indexPage);
    indexRoot->setContentsMargins(0, 0, 0, 0);
    indexRoot->setSpacing(0);
    auto *indexToolbar = new QWidget(m_indexPage);
    indexToolbar->setObjectName(QStringLiteral("libraryToolbar"));
    auto *indexToolbarLayout = new QHBoxLayout(indexToolbar);
    indexToolbarLayout->setContentsMargins(16, 12, 16, 12);
    auto *indexText = new QVBoxLayout;
    indexText->setSpacing(2);
    indexToolbarLayout->addLayout(indexText, 1);
    auto *indexRefresh = new QPushButton(tr("Refresh"), indexToolbar);
    configureActionButton(indexRefresh, QStringLiteral(":/icons/refresh-cw.svg"));
    indexRefresh->setCursor(Qt::PointingHandCursor);
    indexToolbarLayout->addWidget(indexRefresh);
    connect(indexRefresh, &QPushButton::clicked, this, [this] {
        if (m_indexSection == BrowseSection::People)
            m_client->loadPeople();
        else
            emit listRefreshRequested(m_indexSection);
    });
    m_showHiddenPeople = new QPushButton(indexToolbar);
    m_showHiddenPeople->setCheckable(true);
    m_showHiddenPeople->setCursor(Qt::PointingHandCursor);
    m_showHiddenPeople->hide();
    indexToolbarLayout->addWidget(m_showHiddenPeople);
    connect(m_showHiddenPeople, &QPushButton::toggled, this,
            [this] { showIndex(BrowseSection::People); });
    m_indexZoomSlider = new ZoomSlider(indexToolbar);
    m_indexZoomSlider->setValue(m_collectionTileSize);
    indexToolbarLayout->addWidget(m_indexZoomSlider);
    connect(m_indexZoomSlider, &ZoomSlider::valueChanged, this,
            &ExplorePage::setCollectionTileSize);
    connect(m_indexZoomSlider, &ZoomSlider::stepRequested, this, [this](int steps) {
        setCollectionTileSize(GridZoom::step(m_collectionTileSize, steps));
    });
    m_indexTitle = new QLabel(indexToolbar);
    m_indexTitle->setProperty("heading", true);
    m_indexSubtitle = new QLabel(indexToolbar);
    m_indexSubtitle->setProperty("subheading", true);
    indexText->addWidget(m_indexTitle);
    indexText->addWidget(m_indexSubtitle);
    m_indexHost = new QWidget;
    auto *indexFlow = new FlowLayout(m_indexHost, 16);
    indexFlow->setContentsMargins(16, 8, 16, 16);
    m_indexScroll = new QScrollArea(m_indexPage);
    m_indexScroll->setObjectName(QStringLiteral("libraryScroll"));
    m_indexScroll->setWidget(m_indexHost);
    new ScrollToTopButton(m_indexScroll);
    // Home/End jump within whichever page is showing (overview, index or grid).
    auto scrollCurrent = [this](bool toTop) {
        QScrollArea *area = m_browseScroll;
        if (m_stack->currentIndex() == static_cast<int>(Page::Collection))
            area = m_collectionScroll;
        else if (m_stack->currentIndex() == static_cast<int>(Page::Index))
            area = m_indexScroll;
        area->verticalScrollBar()->setValue(toTop ? 0 : area->verticalScrollBar()->maximum());
    };
    for (const bool top : {true, false}) {
        const Qt::Key key = top ? Qt::Key_Home : Qt::Key_End;
        for (const QKeySequence &keys : {QKeySequence(key), QKeySequence(Qt::CTRL | key)}) {
            connect(new QShortcut(keys, this), &QShortcut::activated, this,
                    [scrollCurrent, top] { scrollCurrent(top); });
        }
    }
    m_indexScroll->setWidgetResizable(true);
    m_indexScroll->setFrameShape(QFrame::NoFrame);
    m_indexScroll->viewport()->installEventFilter(this);
    indexRoot->addWidget(indexToolbar);
    indexRoot->addWidget(m_indexScroll, 1);

    m_stack->addWidget(m_browsePage);
    m_stack->addWidget(m_collectionPage);
    m_stack->addWidget(m_indexPage);

    connect(m_refreshButton, &QPushButton::clicked, this, &ExplorePage::refresh);
    connect(m_client, &ImmichClient::albumsLoaded, this,
            [this](const QList<ImmichAlbum> &albums) {
                m_albums = albums;
                m_loadedLists.insert(BrowseSection::Albums);
                if (m_stack->currentIndex() == static_cast<int>(Page::Index) &&
                    m_indexSection == BrowseSection::Albums)
                    showIndex(BrowseSection::Albums);
            });
    connect(m_client, &ImmichClient::trashDaysLoaded, this, [this](int days) {
        m_trashDays = days;
    });
    connect(m_client, &ImmichClient::assetsRestored, this, [this](const QStringList &assetIds) {
        if (m_collectionFilterKind != QStringLiteral("trash"))
            return;
        if (assetIds.isEmpty())
            clearTrashView();
        else
            handleAssetsDeleted(assetIds);
    });
    connect(m_client, &ImmichClient::trashEmptied, this, [this] {
        if (m_collectionFilterKind == QStringLiteral("trash"))
            clearTrashView();
    });
    connect(m_client, &ImmichClient::peopleLoaded, this,
            [this](const QList<ImmichPerson> &people) {
                m_allPeople = people;
                m_loadedLists.insert(BrowseSection::People);
                if (m_stack->currentIndex() == static_cast<int>(Page::Index) &&
                    m_indexSection == BrowseSection::People)
                    showIndex(BrowseSection::People);
            });
    connect(m_client, &ImmichClient::tagsLoaded, this, [this](const QList<ImmichTag> &tags) {
        m_tags = tags;
        m_loadedLists.insert(BrowseSection::Tags);
        if (m_stack->currentIndex() == static_cast<int>(Page::Index) &&
            m_indexSection == BrowseSection::Tags)
            showIndex(BrowseSection::Tags);
        // A tag opened from the info panel before the list arrived has no parents yet.
        if (m_stack->currentIndex() == static_cast<int>(Page::Collection) &&
            m_collectionFilterKind == QStringLiteral("tag")) {
            for (const ImmichTag &tag : tags) {
                if (tag.id == m_collectionFilterValue)
                    showTagTrail(tag);
            }
        }
    });
    connect(m_client, &ImmichClient::folderPathsLoaded, this, [this](const QStringList &paths) {
        m_folderPaths = paths;
        m_loadedLists.insert(BrowseSection::Folders);
        m_folderRoot = folderTreeRoot(paths);
        if (m_stack->currentIndex() == static_cast<int>(Page::Index) &&
            m_indexSection == BrowseSection::Folders)
            showIndex(BrowseSection::Folders);
        if (m_stack->currentIndex() == static_cast<int>(Page::Collection) &&
            m_collectionFilterKind == QStringLiteral("folder"))
            showFolderTrail(m_collectionFilterValue, m_collectionTitle->text());
    });
    connect(m_collectionLoadMore, &QPushButton::clicked, this,
            &ExplorePage::requestCollectionNextPage);

    connect(m_client, &ImmichClient::exploreLoaded, this, &ExplorePage::showExplore);
    connect(m_client, &ImmichClient::filteredAssetsLoaded, this, &ExplorePage::showFilteredAssets);
    connect(m_client, &ImmichClient::personThumbnailLoaded, this,
            &ExplorePage::showPersonThumbnail);
    connect(m_client, &ImmichClient::thumbnailLoaded, this, &ExplorePage::showAssetThumbnail);
    connect(m_client, &ImmichClient::requestFailed, this, &ExplorePage::showRequestError);
    connect(m_client, &ImmichClient::browseListFailed, this, [this](const QString &kind, bool) {
        if (kind == QStringLiteral("folder") &&
            m_collectionFilterKind == QStringLiteral("folder") && m_collectionTiles.isEmpty())
            m_collectionSubtitle->setText(tr("This folder couldn't be loaded."));
    });
    connect(m_client, &ImmichClient::assetsDeleted, this,
            [this](const QStringList &assetIds, bool) { handleAssetsDeleted(assetIds); });
    connect(m_client, &ImmichClient::favoriteFailed, this,
            [this](const QStringList &assetIds, const QString &message) {
                // The viewer reports failures of its own requests.
                bool ours = false;
                for (const QString &id : assetIds)
                    ours = m_favoriteRequests.remove(id) || ours;
                if (ours)
                    Toast::showMessage(this, message);
            });
    connect(m_client, &ImmichClient::favoritesChanged, this,
            [this](const QStringList &assetIds, bool favorite) {
                for (const QString &id : assetIds)
                    m_favoriteRequests.remove(id);
                for (ImmichAsset &asset : m_recentAssets) {
                    if (assetIds.contains(asset.id))
                        asset.favorite = favorite;
                }
                if (!favorite && m_collectionFilterKind == QStringLiteral("favorites")) {
                    removeCollectionTiles(assetIds);
                    return;
                }
                for (const QString &id : assetIds) {
                    if (MediaTile *tile = m_collectionTiles.value(id)) {
                        ImmichAsset asset = tile->asset();
                        asset.favorite = favorite;
                        tile->updateAsset(asset);
                    }
                }
            });
    connect(m_client, &ImmichClient::configurationChanged, this, [this](bool configured) {
        m_loadedLists.clear();
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

    const QList<QPair<QKeySequence, int>> zoomKeys = {
        {QKeySequence::ZoomIn, 1},
        {QKeySequence(Qt::CTRL | Qt::Key_Equal), 1},
        {QKeySequence(QKeyCombination(Qt::ControlModifier | Qt::KeypadModifier, Qt::Key_Plus)), 1},
        {QKeySequence::ZoomOut, -1},
        {QKeySequence(QKeyCombination(Qt::ControlModifier | Qt::KeypadModifier, Qt::Key_Minus)), -1},
        {QKeySequence(Qt::CTRL | Qt::Key_0), 0}};
    for (const auto &[keys, steps] : zoomKeys) {
        auto *zoomShortcut = new QShortcut(keys, this);
        zoomShortcut->setContext(Qt::WindowShortcut);
        connect(zoomShortcut, &QShortcut::activated, this, [this, steps] {
            if (m_stack->currentIndex() == static_cast<int>(Page::Browse))
                return;
            setCollectionTileSize(steps == 0 ? GridZoom::kDefault
                                             : GridZoom::step(m_collectionTileSize, steps));
        });
    }

    m_refreshButton->setEnabled(m_client->isConfigured());
    updateEmptyState();
}

void ExplorePage::setCollectionTileSize(int size)
{
    size = GridZoom::clamp(size);
    if (m_collectionTileSize == size)
        return;

    QScrollBar *scrollBar = m_collectionScroll->verticalScrollBar();
    const qreal position =
        scrollBar->maximum() > 0 ? qreal(scrollBar->value()) / scrollBar->maximum() : 0.0;
    m_collectionTileSize = size;
    m_collectionZoomSlider->setValue(size);
    m_indexZoomSlider->setValue(size);
    layoutCollection();
    scrollBar->setValue(qRound(position * scrollBar->maximum()));
    applyIndexZoom();

    TimelineSettings timeline = AppSettings().loadTimeline();
    timeline.tileSize = size;
    AppSettings().saveTimeline(timeline);
}

void ExplorePage::applyIndexZoom()
{
    if (m_stack->currentIndex() != static_cast<int>(Page::Index))
        return;
    for (ExploreCard *card : m_indexHost->findChildren<ExploreCard *>())
        card->setTileSize(m_collectionTileSize);
    // Covers are ordinary photos, so large ones can use the sharper preview.
    for (auto it = m_indexAssetCards.cbegin(); it != m_indexAssetCards.cend(); ++it) {
        const int wanted =
            GridZoom::tileImageDimension(it.value()->imageSize(), devicePixelRatioF());
        if (wanted > m_indexRequestedCovers.value(it.key())) {
            m_indexRequestedCovers.insert(it.key(), wanted);
            m_client->loadTileImage(it.key(), wanted);
        }
    }
}

void ExplorePage::setPage(Page page)
{
    m_stack->setCurrentIndex(static_cast<int>(page));
}

void ExplorePage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    m_collectionCompactGrid = AppSettings().loadTimeline().compactGrid;
    m_collectionTileSize = GridZoom::clamp(AppSettings().loadTimeline().tileSize);
    m_collectionZoomSlider->setValue(m_collectionTileSize);
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
    m_collectionTileImages->clear();
    m_personThumbCache.clear();

    if (m_collectionHero)
        setCollectionHero({}, m_collectionPersonStyle);
}

void ExplorePage::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    m_collectionZoomSlider->setCompact(true);
    if (m_stack->currentIndex() == static_cast<int>(Page::Collection))
        scheduleCollectionLayout();
}

bool ExplorePage::eventFilter(QObject *watched, QEvent *event)
{
    if ((watched == m_collectionScroll->viewport() || watched == m_indexScroll->viewport()) &&
        event->type() == QEvent::Wheel) {
        auto *wheel = static_cast<QWheelEvent *>(event);
        if (wheel->modifiers() & Qt::ControlModifier) {
            const int delta = wheel->angleDelta().y();
            if (delta != 0)
                setCollectionTileSize(GridZoom::step(m_collectionTileSize, delta > 0 ? 1 : -1));
            return true;
        }
    }
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
    clearRow(&m_viewedSection);
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
    // Refresh the overview in place; an open album, tag or person stays open.
    clearSections();
    m_people = data.people;

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

    m_recentAssets = data.recentAssets;
    auto *recentLayout = qobject_cast<QHBoxLayout *>(m_recentSection.host->layout());
    for (const ImmichAsset &asset : data.recentAssets) {
        auto *card = new ExploreCard(ExploreCard::Style::Media, m_recentSection.host);
        card->setPixmap({});
        recentLayout->insertWidget(recentLayout->count() - 1, card);
        m_recentSection.cards.append(card);
        m_browseAssetCards.insert(asset.id, card);
        // Look the asset up on click so favorite changes since loading are included.
        connect(card, &ExploreCard::activated, this, [this, id = asset.id] {
            for (const ImmichAsset &recent : m_recentAssets) {
                if (recent.id == id) {
                    openAsset(recent, AssetSource::RecentlyAdded);
                    return;
                }
            }
        });
        m_client->loadThumbnail(asset.id);
    }
    populateSection(&m_recentSection, !data.recentAssets.isEmpty());
    populateViewedRow();

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
    if (ExploreCard *card = m_indexPersonCards.value(personId))
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
    for (ExploreCard *card : m_indexAssetCards.values(assetId))
        card->setPixmap(thumbnail);

    if (const QPointer<MediaTile> tile = m_collectionTiles.value(assetId)) {
        m_collectionTileImages->finished(assetId);
        tile->setThumbnail(thumbnail);
        if (GridZoom::tileImageDimension(tile->size(), tile->devicePixelRatioF()) >
            tile->thumbnailDimension())
            scheduleCollectionVisibility();
    }

    if (!m_collectionPersonStyle && !m_collectionHeroAssetId.isEmpty() &&
        m_stack->currentIndex() == static_cast<int>(Page::Collection) &&
        assetId == m_collectionHeroAssetId && !thumbnail.isNull())
        setCollectionHero(thumbnail, false);
}

void ExplorePage::showRequestError(const QString &operation, const QString &message)
{
    if (operation != tr("Load explore"))
        return;
    if (m_collectionLoadingMore) {
        // Stop loading by itself so a failing server isn't asked again on every scroll.
        m_collectionLoadingMore = false;
        m_collectionLoadFailed = true;
        m_collectionLoadMore->setEnabled(true);
        m_collectionLoadMore->setVisible(true);
        setActionButtonLabel(m_collectionLoadMore, tr("Try again"));
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
    setCrumbs({{tr("People"), [this] { showIndex(BrowseSection::People); }}}, title);
    setChips({}, {});
    // Force a fresh face fetch so the hero updates even if a prior request is in flight.
    m_client->loadPersonThumbnail(person.id);
    m_client->loadCollectionAssets(QStringLiteral("person"), person.id, 1,
                                   effectiveCollectionSort());
    emit sectionShown(BrowseSection::People, person.id, title);
}

void ExplorePage::showPerson(const ImmichPerson &person)
{
    openPerson(person);
}

void ExplorePage::showPlace(const QString &city)
{
    openPlace(ImmichPlace{city, {}});
}

void ExplorePage::openPlace(const ImmichPlace &place)
{
    m_pendingCollectionTitle = place.city;
    m_pendingPersonStyle = false;
    showCollectionPage(place.city, tr("Loading photos…"), QStringLiteral("city"), place.city,
                       {}, false);
    setCrumbs({{tr("Explore"), [this] { showOverview(); }}}, place.city);
    setChips({}, {});
    m_collectionHeroAssetId = place.sampleAsset.id;
    if (!place.sampleAsset.id.isEmpty())
        m_client->loadThumbnail(place.sampleAsset.id);
    m_client->loadCollectionAssets(QStringLiteral("city"), place.city, 1,
                                   effectiveCollectionSort());
    emit overviewShown();
}

void ExplorePage::showAlbum(const ImmichAlbum &album)
{
    // The sidebar's copy may predate an order change made here.
    m_collectionAlbum = album;
    for (const ImmichAlbum &known : std::as_const(m_albums)) {
        if (known.id == album.id)
            m_collectionAlbum = known;
    }
    openCollection(QStringLiteral("album"), album.id, album.name, BrowseSection::Albums);
    setCrumbs({{tr("Albums"), [this] { showIndex(BrowseSection::Albums); }}}, album.name);
    setChips({}, {});
    m_collectionHeroAssetId = album.thumbnailAssetId;
    if (!album.thumbnailAssetId.isEmpty())
        m_client->loadThumbnail(album.thumbnailAssetId);
}

void ExplorePage::showTag(const ImmichTag &tag)
{
    openCollection(QStringLiteral("tag"), tag.id, tag.name, BrowseSection::Tags);
    showTagTrail(tag);
}

void ExplorePage::showTagTrail(const ImmichTag &tag)
{
    QList<Crumb> crumbs;
    QHash<QString, ImmichTag> byId;
    for (const ImmichTag &entry : m_tags)
        byId.insert(entry.id, entry);
    for (QString parentId = tag.parentId; byId.contains(parentId);
         parentId = byId.value(parentId).parentId) {
        const ImmichTag parent = byId.value(parentId);
        crumbs.prepend({parent.name, [this, parent] { showTag(parent); }});
    }
    crumbs.prepend({tr("Tags"), [this] { showIndex(BrowseSection::Tags); }});
    setCrumbs(crumbs, tag.name);

    QList<Crumb> children;
    for (const ImmichTag &child : m_tags) {
        if (child.parentId == tag.id)
            children.append({child.name, [this, child] { showTag(child); }});
    }
    setChips(children, QStringLiteral(":/icons/tag.svg"));
}

void ExplorePage::showFolder(const QString &path, const QString &title)
{
    const QStringList segments =
        folderRelativePath(path, m_folderRoot).split(u'/', Qt::SkipEmptyParts);
    openCollection(QStringLiteral("folder"), path, segments.isEmpty() ? title : segments.last(),
                   BrowseSection::Folders);
    showFolderTrail(path, title);
}

void ExplorePage::showFolderTrail(const QString &path, const QString &title)
{
    const QStringList segments =
        folderRelativePath(path, m_folderRoot).split(u'/', Qt::SkipEmptyParts);
    const QString name = segments.isEmpty() ? title : segments.last();
    QList<Crumb> crumbs{{tr("Folders"), [this] { showIndex(BrowseSection::Folders); }}};
    QString ancestor = m_folderRoot;
    for (qsizetype i = 0; i + 1 < segments.size(); ++i) {
        ancestor = folderPathJoin(ancestor, segments.at(i));
        crumbs.append({segments.at(i), [this, ancestor] { showFolder(ancestor, {}); }});
    }
    setCrumbs(crumbs, name);
    setChips(folderChildren(path), QStringLiteral(":/icons/folder.svg"));
}

void ExplorePage::showOverview()
{
    backToBrowse();
}

void ExplorePage::showSection(BrowseSection section)
{
    switch (section) {
    case BrowseSection::People:
        m_client->loadPeople();
        showIndex(section);
        break;
    case BrowseSection::Albums:
    case BrowseSection::Tags:
    case BrowseSection::Folders:
        showIndex(section);
        break;
    case BrowseSection::Favorites:
        showLibraryView(QStringLiteral("favorites"), section, tr("Favorites"));
        break;
    case BrowseSection::Archive:
        showLibraryView(QStringLiteral("archive"), section, tr("Archive"));
        break;
    case BrowseSection::Trash:
        m_client->loadTrashDays();
        showLibraryView(QStringLiteral("trash"), section, tr("Trash"));
        break;
    }
}

void ExplorePage::showLibraryView(const QString &kind, BrowseSection section,
                                  const QString &title)
{
    openCollection(kind, {}, title, section);
    setCrumbs({}, title);
    setChips({}, {});
}

void ExplorePage::openCollection(const QString &kind, const QString &value,
                                 const QString &title, BrowseSection section)
{
    m_pendingCollectionTitle = title;
    m_pendingPersonStyle = false;
    showCollectionPage(title, tr("Loading photos…"), kind, value, {}, false);
    m_client->loadCollectionAssets(kind, value, 1, effectiveCollectionSort());
    emit sectionShown(section, value, title);
}

void ExplorePage::showIndex(BrowseSection section)
{
    m_indexSection = section;
    while (QLayoutItem *item = m_indexHost->layout()->takeAt(0)) {
        if (QWidget *widget = item->widget())
            widget->deleteLater();
        delete item;
    }
    m_indexAssetCards.clear();
    m_indexPersonCards.clear();
    m_indexRequestedCovers.clear();

    auto addChip = [this](const QString &label, const QString &icon,
                          std::function<void()> open) {
        auto *chip = new QPushButton(label, m_indexHost);
        configureActionButton(chip, icon);
        chip->setCursor(Qt::PointingHandCursor);
        connect(chip, &QPushButton::clicked, this, open);
        m_indexHost->layout()->addWidget(chip);
    };

    QString title;
    QString subtitle;
    if (section == BrowseSection::Albums) {
        title = tr("Albums");
        subtitle = m_albums.isEmpty() ? tr("No albums yet")
                                      : tr("%n album(s)", nullptr, m_albums.size());
        for (const ImmichAlbum &album : m_albums) {
            auto *card = new ExploreCard(ExploreCard::Style::Place, m_indexHost);
            card->setCaption(tr("%1\n%n item(s)", nullptr, album.assetCount).arg(album.name));
            card->setPixmap(QPixmap());
            m_indexHost->layout()->addWidget(card);
            connect(card, &ExploreCard::activated, this, [this, album] { showAlbum(album); });
            if (!album.thumbnailAssetId.isEmpty()) {
                m_indexAssetCards.insert(album.thumbnailAssetId, card);
                m_client->loadThumbnail(album.thumbnailAssetId);
            }
        }
    } else if (section == BrowseSection::People) {
        title = tr("People");
        const bool showHidden = m_showHiddenPeople->isChecked();
        int shown = 0;
        int hidden = 0;
        for (const ImmichPerson &person : m_allPeople.isEmpty() ? m_people : m_allPeople) {
            if (person.hidden)
                ++hidden;
            if (person.hidden && !showHidden)
                continue;
            ++shown;
            auto *card = new ExploreCard(ExploreCard::Style::Person, m_indexHost);
            const QString name = person.name.isEmpty() ? tr("Unnamed") : person.name;
            card->setCaption(person.hidden ? tr("%1 (hidden)").arg(name) : name);
            card->setPixmap(m_personThumbCache.value(person.id));
            m_indexHost->layout()->addWidget(card);
            m_indexPersonCards.insert(person.id, card);
            connect(card, &ExploreCard::activated, this, [this, person] { openPerson(person); });
            if (!m_personThumbCache.contains(person.id))
                m_client->loadPersonThumbnail(person.id);
        }
        subtitle = shown == 0 ? tr("No people recognized yet")
                              : tr("%n person(s)", nullptr, shown);
        // The label says what clicking does, so it always reflects the current state.
        m_showHiddenPeople->setText(showHidden ? tr("Hide hidden")
                                               : tr("Show hidden (%1)").arg(hidden));
        configureActionButton(m_showHiddenPeople, showHidden ? QStringLiteral(":/icons/eye-off.svg")
                                                             : QStringLiteral(":/icons/eye.svg"));
        m_showHiddenPeople->setToolTip(showHidden ? tr("Hide people marked hidden in Immich")
                                                  : tr("Show people marked hidden in Immich"));
    } else if (section == BrowseSection::Tags) {
        title = tr("Tags");
        QSet<QString> ids;
        for (const ImmichTag &tag : m_tags)
            ids.insert(tag.id);
        for (const ImmichTag &tag : m_tags) {
            if (ids.contains(tag.parentId))
                continue;
            addChip(tag.name, QStringLiteral(":/icons/tag.svg"), [this, tag] { showTag(tag); });
        }
        subtitle = m_tags.isEmpty() ? tr("No tags yet") : tr("%n tag(s)", nullptr, m_tags.size());
    } else if (section == BrowseSection::Folders) {
        title = tr("Folders");
        const QList<Crumb> folders = folderChildren(m_folderRoot);
        subtitle = folders.isEmpty() ? tr("No folders")
                                     : tr("%n folder(s)", nullptr, folders.size());
        for (const Crumb &folder : folders)
            addChip(folder.label, QStringLiteral(":/icons/folder.svg"), folder.open);
    }

    if (!m_loadedLists.contains(section) && m_indexHost->layout()->count() == 0)
        subtitle = tr("Loading…");
    m_showHiddenPeople->setVisible(section == BrowseSection::People &&
                                   (m_showHiddenPeople->isChecked() ||
                                    std::any_of(m_allPeople.cbegin(), m_allPeople.cend(),
                                                [](const ImmichPerson &p) { return p.hidden; })));
    m_indexTitle->setText(title);
    m_indexSubtitle->setText(subtitle);
    m_collectionTileSize = GridZoom::clamp(AppSettings().loadTimeline().tileSize);
    m_indexZoomSlider->setValue(m_collectionTileSize);
    setPage(Page::Index);
    applyIndexZoom();
    emit sectionShown(section, {}, title);
}

void ExplorePage::setCrumbs(const QList<Crumb> &crumbs, const QString &title)
{
    auto *layout = static_cast<QHBoxLayout *>(m_collectionTrail->layout());
    while (layout->count() > 0) {
        QLayoutItem *item = layout->takeAt(0);
        if (QWidget *widget = item->widget(); widget && widget != m_collectionTitle)
            widget->deleteLater();
        delete item;
    }
    for (const Crumb &crumb : crumbs) {
        auto *link = new QPushButton(crumb.label, m_collectionTrail);
        link->setProperty("linkButton", true);
        link->setCursor(Qt::PointingHandCursor);
        connect(link, &QPushButton::clicked, this, crumb.open);
        auto *separator = new QLabel(QStringLiteral("›"), m_collectionTrail);
        separator->setProperty("subheading", true);
        layout->addWidget(link);
        layout->addWidget(separator);
    }
    m_collectionTitle->setText(title);
    layout->addWidget(m_collectionTitle);
    layout->addStretch(1);
}

void ExplorePage::setChips(const QList<Crumb> &chips, const QString &iconResource)
{
    QLayout *layout = m_collectionChips->layout();
    while (QLayoutItem *item = layout->takeAt(0)) {
        if (QWidget *widget = item->widget())
            widget->deleteLater();
        delete item;
    }
    for (const Crumb &chip : chips) {
        auto *button = new QPushButton(chip.label, m_collectionChips);
        configureActionButton(button, iconResource);
        button->setCursor(Qt::PointingHandCursor);
        connect(button, &QPushButton::clicked, this, chip.open);
        layout->addWidget(button);
    }
    m_collectionChips->setVisible(!chips.isEmpty());
}

QList<ExplorePage::Crumb> ExplorePage::folderChildren(const QString &path)
{
    QStringList names;
    for (const QString &folder : m_folderPaths) {
        const QString relative = folderRelativePath(folder, path);
        const QString child = relative.section(u'/', 0, 0);
        if (!child.isEmpty() && !names.contains(child))
            names.append(child);
    }
    names.sort(Qt::CaseInsensitive);
    QList<Crumb> children;
    for (const QString &name : names) {
        const QString childPath = folderPathJoin(path, name);
        children.append({name, [this, childPath] { showFolder(childPath, {}); }});
    }
    return children;
}

void ExplorePage::showCollectionPage(const QString &title, const QString &subtitle,
                                     const QString &filterKind, const QString &filterValue,
                                     const QPixmap &heroThumb, bool personStyle)
{
    m_collectionFilterKind = filterKind;
    m_collectionFilterValue = filterValue;
    if (filterKind != QStringLiteral("album") || m_collectionAlbum.id != filterValue)
        m_collectionAlbum = {};
    m_collectionShuffled = false;
    loadCollectionSort();
    m_collectionHeroAssetId.clear();
    m_collectionPersonStyle = personStyle;
    m_collectionNextPage.clear();
    m_collectionLoadingMore = false;
    m_collectionLoadFailed = false;
    m_collectionCompactGrid = AppSettings().loadTimeline().compactGrid;
    m_collectionTileSize = GridZoom::clamp(AppSettings().loadTimeline().tileSize);
    m_collectionZoomSlider->setValue(m_collectionTileSize);

    clearCollectionTimeline();

    m_collectionTitle->setText(title);
    m_collectionSubtitle->setText(subtitle);
    setCollectionHero(heroThumb, personStyle);

    m_collectionLoadMore->setVisible(false);
    const bool trash = filterKind == QStringLiteral("trash");
    m_restoreAllButton->setVisible(trash);
    m_emptyTrashButton->setVisible(trash);
    m_collectionSortButton->setVisible(filterKind != QStringLiteral("viewed"));
    m_collectionShuffleButton->setVisible(!trash);
    setPage(Page::Collection);
}

void ExplorePage::updateCollectionSubtitle()
{
    if (m_collectionTiles.isEmpty()) {
        m_collectionSubtitle->setText(m_collectionFilterKind == QStringLiteral("trash")
                                          ? tr("Trash is empty")
                                          : tr("Nothing here yet"));
        return;
    }
    QString subtitle = tr("%n item(s)", nullptr, m_collectionTiles.size());
    if (m_collectionFilterKind == QStringLiteral("trash") && m_trashDays > 0)
        subtitle += tr(" · deleted permanently after %n day(s)", nullptr, m_trashDays);
    m_collectionSubtitle->setText(subtitle);
}

void ExplorePage::clearTrashView()
{
    clearCollectionTimeline();
    updateCollectionLoadMore({});
    m_collectionSubtitle->setText(tr("Trash is empty"));
}

void ExplorePage::handleAssetsDeleted(const QStringList &assetIds)
{
    for (const QString &assetId : assetIds) {
        const QList<ExploreCard *> cards = m_browseAssetCards.values(assetId);
        for (ExploreCard *card : cards) {
            // Place cards are keyed by their sample photo too; keep them until refresh.
            if (m_recentSection.cards.removeOne(card)) {
                m_browseAssetCards.remove(assetId, card);
                card->deleteLater();
            }
        }

        m_recentAssets.removeIf(
            [&assetId](const ImmichAsset &asset) { return asset.id == assetId; });
    }
    populateSection(&m_recentSection, !m_recentSection.cards.isEmpty());
    removeCollectionTiles(assetIds);
}

void ExplorePage::removeCollectionTiles(const QStringList &assetIds)
{
    if (m_collectionHoverPreview)
        m_collectionHoverPreview->stop();
    for (const QString &assetId : assetIds) {
        const QPointer<MediaTile> tile = m_collectionTiles.take(assetId);
        if (!tile)
            continue;
        for (DaySection &section : m_collectionSections)
            section.tiles.removeAll(tile.data());
        m_collectionTileImages->tileLeft(assetId, true);
        tile->deleteLater();
    }

    m_collectionSections.removeIf([](const DaySection &section) {
        if (!section.tiles.isEmpty())
            return false;
        if (section.header)
            section.header->deleteLater();
        return true;
    });
    if (!m_collectionFilterKind.isEmpty())
        updateCollectionSubtitle();
    scheduleCollectionLayout();
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
    m_collectionTileImages->clear();
    m_collectionHost->resize(m_collectionScroll->viewport()->width(), 1);
}

ExplorePage::DaySection *ExplorePage::sectionForDate(const QDate &date)
{
    // Shuffled, and orders without dates (size, rating, activity), use one plain section.
    if (m_collectionShuffled || !effectiveCollectionSort().groupsByDay()) {
        if (m_collectionSections.isEmpty())
            m_collectionSections.append(DaySection{});
        return &m_collectionSections.first();
    }
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
    return locale.toString(date, QLocale::LongFormat);
}

void ExplorePage::showFilteredAssets(const QString &filterKind, const QString &filterValue,
                                     const QList<ImmichAsset> &assets, const QString &nextPage,
                                     const AssetSort &sort)
{
    // A reply for this collection under a sort that has since changed, or from
    // before it was shuffled.
    if (filterKind == m_collectionFilterKind && filterValue == m_collectionFilterValue &&
        (sort != effectiveCollectionSort() || m_collectionShuffled))
        return;
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
    updateCollectionSubtitle();
    updateCollectionLoadMore(nextPage);
    scheduleCollectionLayout();
    scheduleCollectionVisibility();
}

void ExplorePage::appendCollectionAssets(const QList<ImmichAsset> &assets)
{
    for (const ImmichAsset &asset : assets) {
        if (m_collectionTiles.contains(asset.id))
            continue;

        // Results arrive in sort order, so days only need the matching date.
        const AssetSort::Field field = effectiveCollectionSort().field;
        QDate date = asset.takenAt.isValid() ? asset.takenAt.date() : QDate();
        if (field == AssetSort::Field::AddedAt && asset.addedAt.isValid())
            date = asset.addedAt.toLocalTime().date();
        else if (field == AssetSort::Field::Deleted && asset.updatedAt.isValid())
            date = asset.updatedAt.toLocalTime().date();
        else if (field == AssetSort::Field::Modified && asset.modifiedAt.isValid())
            date = asset.modifiedAt.toLocalTime().date();
        DaySection *section = sectionForDate(date);
        auto *tile = new MediaTile(asset, m_collectionHost);
        tile->setHoverPreview(m_collectionHoverPreview);
        section->tiles.append(tile);
        m_collectionTiles.insert(asset.id, tile);
        connect(tile, &MediaTile::activated, this,
                [this](const ImmichAsset &asset) { openAsset(asset, AssetSource::Collection); });
        connect(tile, &MediaTile::infoRequested, this, [this](const ImmichAsset &asset) {
            openAsset(asset, AssetSource::Collection)->setInfoPanelVisible(true);
        });
        connect(tile, &MediaTile::favoriteRequested, this,
                [this](const ImmichAsset &asset, bool favorite) {
                    m_favoriteRequests.insert(asset.id);
                    m_client->setFavorite({asset.id}, favorite);
                });
        connect(tile, &MediaTile::shareRequested, this, [this](const ImmichAsset &asset) {
            (new ShareLinkDialog(m_client, asset, this))->show();
        });
        connect(tile, &MediaTile::copyRequested, this, &ExplorePage::copyRequested);
        connect(tile, &MediaTile::downloadRequested, this, &ExplorePage::downloadRequested);
        connect(tile, &MediaTile::trashRequested, this, &ExplorePage::trashRequested);
        connect(tile, &MediaTile::deleteRequested, this, &ExplorePage::deleteRequested);
        connect(tile, &MediaTile::restoreRequested, this, [this](const ImmichAsset &asset) {
            m_client->restoreFromTrash({asset.id});
        });
        tile->setTrashed(m_collectionFilterKind == QStringLiteral("trash"));
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
        const int startY = 8;
        const int columns =
            qMax(1, (availableWidth + kGridGap) / (m_collectionTileSize + kGridGap));
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

    const qreal minRowHeight = GridZoom::minRowHeight(m_collectionTileSize);
    const qreal maxRowHeight = GridZoom::maxRowHeight(m_collectionTileSize);
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
            qreal height = m_collectionTileSize;
            const qreal naturalWidth = rowAspectSum * height + gaps;
            if (justifyToWidth || naturalWidth > availableWidth) {
                height = (availableWidth - gaps) / qMax(0.01, rowAspectSum);
                height = qBound(minRowHeight, height, maxRowHeight);
            } else {
                height = qBound(minRowHeight, height, maxRowHeight);
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
                (rowAspectSum + ratio) * m_collectionTileSize + kGap * row.size();
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
    maybeLoadMoreCollection();

    const int viewportHeight = m_collectionScroll->viewport()->height();
    const int scrollTop = m_collectionScroll->verticalScrollBar()->value();
    const QRect buffered(0, qMax(0, scrollTop - viewportHeight), m_collectionHost->width(),
                         viewportHeight * 3);

    const int middle = scrollTop + viewportHeight / 2;
    QList<MediaTile *> nearTiles;
    for (auto it = m_collectionTiles.cbegin(); it != m_collectionTiles.cend(); ++it) {
        MediaTile *tile = it.value().data();
        if (!tile)
            continue;
        if (!buffered.intersects(tile->geometry())) {
            const bool hadImage = tile->hasThumbnail();
            if (hadImage)
                tile->clearThumbnail();
            m_collectionTileImages->tileLeft(it.key(), hadImage);
            continue;
        }
        nearTiles.append(tile);
    }
    std::sort(nearTiles.begin(), nearTiles.end(), [middle](MediaTile *a, MediaTile *b) {
        return qAbs(a->geometry().center().y() - middle) <
               qAbs(b->geometry().center().y() - middle);
    });
    m_collectionTileImages->update(nearTiles,
                                   QRect(0, scrollTop, m_collectionHost->width(), viewportHeight));
}

// More photos load by themselves while scrolling; the button only shows while a
// batch is loading, or as "Try again" when one failed.
void ExplorePage::updateCollectionLoadMore(const QString &nextPage)
{
    m_collectionNextPage = nextPage;
    m_collectionLoadMore->setVisible(false);
}

void ExplorePage::maybeLoadMoreCollection()
{
    if (m_collectionLoadingMore || m_collectionLoadFailed)
        return;
    // Start about two screens before the end, so the next batch is there in time.
    // A batch too short to scroll counts as near the end, so it keeps filling.
    const QScrollBar *bar = m_collectionScroll->verticalScrollBar();
    if (bar->maximum() - bar->value() <= 2 * m_collectionScroll->viewport()->height())
        requestCollectionNextPage();
}

void ExplorePage::requestCollectionNextPage()
{
    bool ok = false;
    const int page = m_collectionNextPage.toInt(&ok);
    if (!ok || page <= 0 || m_collectionFilterKind.isEmpty())
        return;

    m_collectionLoadingMore = true;
    m_collectionLoadFailed = false;
    m_collectionLoadMore->setEnabled(false);
    m_collectionLoadMore->setVisible(true);
    setActionButtonLabel(m_collectionLoadMore, tr("Loading…"));

    // Shuffled, each batch is a fresh random pick rather than a page.
    if (m_collectionShuffled) {
        m_client->loadRandomAssets(m_collectionFilterKind, m_collectionFilterValue);
        return;
    }
    m_client->loadCollectionAssets(m_collectionFilterKind, m_collectionFilterValue, page,
                                   effectiveCollectionSort());
}

AssetSort ExplorePage::effectiveCollectionSort() const
{
    const QList<AssetSort::Field> fields = collectionSortFields();
    return fields.contains(m_collectionSort.field) ? m_collectionSort
                                                    : AssetSort{fields.first(), false};
}

QList<AssetSort::Field> ExplorePage::collectionSortFields() const
{
    using Field = AssetSort::Field;
    const QString &kind = m_collectionFilterKind;
    if (kind == QStringLiteral("viewed"))
        return {Field::Viewed};
    const bool viewed = m_client->remembersViewed();
    // Folders arrive whole with file details, so they sort like a file manager.
    if (kind == QStringLiteral("folder")) {
        QList<Field> fields{Field::Name, Field::TakenAt, Field::AddedAt, Field::Modified,
                            Field::Size, Field::Rating,  Field::Type};
        if (viewed)
            fields << Field::Viewed;
        return fields;
    }
    QList<Field> fields;
    if (kind == QStringLiteral("trash"))
        fields << Field::Deleted;
    fields << Field::TakenAt;
    if (ImmichClient::canSortByDateAdded(kind))
        fields << Field::AddedAt;
    // Size and rating come from search, which folders don't use.
    if (kind != QStringLiteral("folder") && m_client->serverAtLeast(3, 2))
        fields << Field::Size << Field::Rating;
    if (kind == QStringLiteral("album") && m_collectionAlbum.activityEnabled)
        fields << Field::Activity;
    // Finding the viewed photos in a collection needs 3.2's search filters.
    if (viewed && m_client->serverAtLeast(3, 2))
        fields << Field::Viewed;
    return fields;
}

QString ExplorePage::collectionSortKey() const
{
    return m_collectionFilterValue.isEmpty()
               ? m_collectionFilterKind
               : m_collectionFilterKind + u':' + m_collectionFilterValue;
}

// Each collection remembers its own sort. An album's date-taken order lives in
// Immich (shared with the web UI); it's only kept here when saving it failed.
void ExplorePage::loadCollectionSort()
{
    AssetSort fallback;
    if (m_collectionFilterKind == QStringLiteral("trash"))
        fallback = {AssetSort::Field::Deleted, false};
    else if (m_collectionFilterKind == QStringLiteral("folder"))
        fallback = {AssetSort::Field::Name, true};
    else if (!m_collectionAlbum.id.isEmpty())
        fallback = {AssetSort::Field::TakenAt, m_collectionAlbum.orderAscending};
    m_collectionSort = AppSettings().loadSort(collectionSortKey(), fallback);
    m_collectionSortButton->setFields(collectionSortFields());
    m_collectionSortButton->setSort(effectiveCollectionSort());
    m_collectionSortButton->setShuffled(false);
}

void ExplorePage::chooseCollectionSort(const AssetSort &sort)
{
    m_collectionShuffled = false;
    m_collectionSort = sort;
    AppSettings().saveSort(collectionSortKey(), sort);
    if (!m_collectionAlbum.id.isEmpty() && sort.field == AssetSort::Field::TakenAt &&
        sort.ascending != m_collectionAlbum.orderAscending) {
        m_collectionAlbum.orderAscending = sort.ascending;
        for (ImmichAlbum &album : m_albums) {
            if (album.id == m_collectionAlbum.id)
                album.orderAscending = sort.ascending;
        }
        m_client->setAlbumOrder(m_collectionAlbum.id, sort.ascending);
    }
    reloadCollection();
}

void ExplorePage::shuffleCollection()
{
    if (m_collectionFilterKind.isEmpty())
        return;
    m_collectionShuffled = true;
    m_collectionSortButton->setShuffled(true);
    m_collectionLoadingMore = false;
    m_collectionLoadFailed = false;
    m_collectionNextPage.clear();
    clearCollectionTimeline();
    updateCollectionLoadMore({});
    m_collectionSubtitle->setText(tr("Shuffling…"));
    m_client->loadRandomAssets(m_collectionFilterKind, m_collectionFilterValue);
}

void ExplorePage::showRandomAssets(const QString &kind, const QString &value,
                                   const QList<ImmichAsset> &assets, bool more)
{
    if (!m_collectionShuffled || kind != m_collectionFilterKind ||
        value != m_collectionFilterValue)
        return;
    // Random batches can repeat photos already shown.
    QList<ImmichAsset> fresh;
    for (const ImmichAsset &asset : assets) {
        if (!m_collectionTiles.contains(asset.id))
            fresh.append(asset);
    }
    m_collectionLoadingMore = false;
    appendCollectionAssets(fresh);
    if (m_collectionTiles.isEmpty())
        m_collectionSubtitle->setText(tr("Nothing here yet"));
    else
        updateCollectionSubtitle();
    // Any page number works: each shuffled batch is a fresh random pick.
    updateCollectionLoadMore(more && !fresh.isEmpty() ? QStringLiteral("1") : QString());
    scheduleCollectionLayout();
    scheduleCollectionVisibility();
}

void ExplorePage::reloadCollection()
{
    if (m_collectionFilterKind.isEmpty() ||
        m_stack->currentIndex() != static_cast<int>(Page::Collection))
        return;
    m_collectionLoadingMore = false;
    m_collectionLoadFailed = false;
    m_collectionNextPage.clear();
    clearCollectionTimeline();
    updateCollectionLoadMore({});
    m_collectionSubtitle->setText(tr("Loading photos…"));
    m_client->loadCollectionAssets(m_collectionFilterKind, m_collectionFilterValue, 1,
                                   effectiveCollectionSort());
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
    emit overviewShown();
}

void ExplorePage::populateViewedRow()
{
    constexpr int kRowLength = 30;
    auto *layout = qobject_cast<QHBoxLayout *>(m_viewedSection.host->layout());
    while (layout->count() > 1) {
        QLayoutItem *item = layout->takeAt(0);
        if (auto *card = qobject_cast<ExploreCard *>(item->widget())) {
            for (auto it = m_browseAssetCards.begin(); it != m_browseAssetCards.end();) {
                it = it.value() == card ? m_browseAssetCards.erase(it) : std::next(it);
            }
            card->deleteLater();
        }
        delete item;
    }
    m_viewedSection.cards.clear();
    m_viewedAssets = m_client->recentlyViewed().mid(0, kRowLength);
    for (const ImmichAsset &asset : std::as_const(m_viewedAssets)) {
        auto *card = new ExploreCard(ExploreCard::Style::Media, m_viewedSection.host);
        card->setPixmap({});
        layout->insertWidget(layout->count() - 1, card);
        m_viewedSection.cards.append(card);
        m_browseAssetCards.insert(asset.id, card);
        connect(card, &ExploreCard::activated, this,
                [this, asset] { openAsset(asset, AssetSource::RecentlyViewed); });
        m_client->loadThumbnail(asset.id);
    }
    populateSection(&m_viewedSection, !m_viewedAssets.isEmpty());
}

void ExplorePage::showRecentlyViewed()
{
    const QString title = tr("Recently viewed");
    m_pendingCollectionTitle = title;
    m_pendingPersonStyle = false;
    showCollectionPage(title, tr("Loading photos…"), QStringLiteral("viewed"), {}, {}, false);
    setCrumbs({{tr("Explore"), [this] { showOverview(); }}}, title);
    setChips({}, {});
    m_client->loadCollectionAssets(QStringLiteral("viewed"), {}, 1, effectiveCollectionSort());
    emit overviewShown();
}

MediaViewerDialog *ExplorePage::openAsset(const ImmichAsset &asset, AssetSource source)
{
    if (asset.isVideo() && m_collectionHoverPreview)
        m_collectionHoverPreview->stop();

    auto *viewer = new MediaViewerDialog(
        m_themeManager, m_client, asset,
        // Viewing reorders the history, so step through it as it was when opened.
        [this, source, viewed = m_viewedAssets](const QString &assetId, int step) {
            if (source == AssetSource::RecentlyViewed) {
                for (qsizetype i = 0; i < viewed.size(); ++i) {
                    if (viewed.at(i).id == assetId)
                        return viewed.value(i + step);
                }
                return ImmichAsset{};
            }
            return adjacentAsset(source, assetId, step);
        },
        this);
    connect(viewer, &MediaViewerDialog::copyRequested, this, &ExplorePage::copyRequested);
    connect(viewer, &MediaViewerDialog::downloadRequested, this,
            &ExplorePage::downloadRequested);
    connect(viewer, &MediaViewerDialog::trashRequested, this, &ExplorePage::trashRequested);
    connect(viewer, &MediaViewerDialog::deleteRequested, this, &ExplorePage::deleteRequested);
    connect(viewer, &MediaViewerDialog::personRequested, this, &ExplorePage::personRequested);
    connect(viewer, &MediaViewerDialog::placeRequested, this, &ExplorePage::placeRequested);
    connect(viewer, &MediaViewerDialog::dateRequested, this, &ExplorePage::dateRequested);
    connect(viewer, &MediaViewerDialog::albumRequested, this, &ExplorePage::albumRequested);
    connect(viewer, &MediaViewerDialog::tagRequested, this, &ExplorePage::tagRequested);
    connect(viewer, &MediaViewerDialog::folderRequested, this, &ExplorePage::folderRequested);
    if (source == AssetSource::Collection && m_collectionFilterKind == QStringLiteral("trash")) {
        viewer->setTrashMode(true);
        connect(viewer, &MediaViewerDialog::restoreRequested, this,
                [this](const ImmichAsset &asset) { m_client->restoreFromTrash({asset.id}); });
    }
    viewer->show();
    return viewer;
}

ImmichAsset ExplorePage::adjacentAsset(AssetSource source, const QString &assetId, int step)
{
    const bool fromCollection = source == AssetSource::Collection;
    QList<ImmichAsset> assets;
    if (fromCollection) {
        for (const DaySection &section : m_collectionSections) {
            for (const MediaTile *tile : section.tiles)
                assets.append(tile->asset());
        }
    } else {
        assets = m_recentAssets;
    }

    int index = -1;
    for (int i = 0; i < assets.size(); ++i) {
        if (assets.at(i).id == assetId) {
            index = i;
            break;
        }
    }
    if (index < 0)
        return {};

    const int target = index + step;
    if (fromCollection && target >= assets.size() - 1 && !m_collectionLoadingMore &&
        !m_collectionNextPage.isEmpty())
        requestCollectionNextPage();
    if (target < 0 || target >= assets.size())
        return {};
    return assets.at(target);
}

} // namespace Aurora
