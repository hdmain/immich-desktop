#include "ui/pages/LibraryPage.h"

#include "core/AppSettings.h"
#include "core/FolderSyncService.h"
#include "ui/GridZoom.h"
#include "ui/IconUtils.h"
#include "ui/widgets/MediaTile.h"
#include "ui/widgets/MediaViewerDialog.h"
#include "ui/widgets/ShareLinkDialog.h"
#include "ui/widgets/SummaryTile.h"
#include "ui/widgets/Toast.h"
#include "ui/widgets/VideoHoverPreview.h"
#include "ui/widgets/ZoomSlider.h"
#include "ui/widgets/ZoomPanWidget.h"

#include <QDesktopServices>
#include <QImageReader>

#include <QApplication>
#include <QBuffer>
#include <QButtonGroup>
#include <QClipboard>
#include <QCheckBox>
#include "ui/widgets/NoWheelComboBox.h"
#include <QDateEdit>
#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFocusEvent>
#include <QFrame>
#include <QGraphicsOpacityEffect>
#include <QHideEvent>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QImage>
#include <QJsonArray>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QMessageBox>
#include <QMimeData>
#include <QPointer>
#include <QParallelAnimationGroup>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QSet>
#include <QShortcut>
#include <QSignalBlocker>
#include <QShowEvent>
#include <QStackedWidget>
#include <QStandardPaths>
#include <QTimer>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <QVariantAnimation>

#include <algorithm>
#include <utility>

namespace Aurora {
namespace {

constexpr int kGap = 3;
constexpr int kSidePad = 12;
constexpr int kAutoCheckIntervalMs = 30 * 1000;
constexpr int kSearchDebounceMs = 350;
constexpr int kParallelTimelineBuckets = 4;

QString mediaFileFilter()
{
    return QObject::tr(
        "Media files (*.jpg *.jpeg *.png *.gif *.webp *.heic *.heif *.tif *.tiff "
        "*.bmp *.raw *.dng *.cr2 *.nef *.arw *.mp4 *.mov *.m4v *.avi *.mkv *.webm "
        "*.3gp);;All files (*.*)");
}

} // namespace

LibraryPage::LibraryPage(ThemeManager *themeManager, ImmichClient *client,
                         FolderSyncService *folderSync, QWidget *parent)
    : QWidget(parent)
    , m_themeManager(themeManager)
    , m_client(client)
    , m_folderSync(folderSync)
    , m_contentStack(new QStackedWidget(this))
    , m_scrollArea(new QScrollArea(this))
    , m_timelineHost(new QWidget)
    , m_yearScrollArea(new QScrollArea(this))
    , m_yearGridHost(new QWidget)
    , m_yearDetailScrollArea(new QScrollArea(this))
    , m_yearDetailHost(new QWidget)
    , m_yearDetailStatus(new QLabel(this))
    , m_yearGridEmptyState(new QLabel(this))
    , m_status(new QLabel(this))
    , m_emptyState(new QLabel(this))
    , m_dropOverlay(new QLabel(this))
    , m_searchField(new QLineEdit(this))
    , m_uploadButton(new QPushButton(tr("Upload"), this))
    , m_refreshButton(new QPushButton(tr("Refresh"), this))
    , m_yearsButton(new QPushButton(tr("Years"), this))
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
    const TimelineSettings timeline = AppSettings().loadTimeline();
    m_compactGrid = timeline.compactGrid;
    m_tileSize = GridZoom::clamp(timeline.tileSize);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto *toolbar = new QWidget(this);
    toolbar->setObjectName(QStringLiteral("libraryToolbar"));
    auto *toolbarLayout = new QHBoxLayout(toolbar);
    m_searchToolbarLayout = toolbarLayout;
    toolbarLayout->setContentsMargins(16, 10, 16, 8);
    toolbarLayout->setSpacing(8);

    auto *headingColumn = new QVBoxLayout;
    headingColumn->setSpacing(2);
    m_heading = new QLabel(tr("Photos & videos"), toolbar);
    m_heading->setProperty("heading", true);
    m_status->setProperty("subheading", true);
    m_status->setText(tr("Your Immich library"));
    headingColumn->addWidget(m_heading);
    headingColumn->addWidget(m_status);
    toolbarLayout->addLayout(headingColumn, 1);

    m_searchField->setPlaceholderText(tr("Search photos and videos…"));
    m_searchField->setObjectName(QStringLiteral("librarySearchField"));
    m_searchField->setClearButtonEnabled(true);
    m_searchField->setMinimumWidth(220);
    m_searchField->setMaximumWidth(360);
    m_searchField->installEventFilter(this);
    toolbarLayout->addWidget(m_searchField, 1);
    m_searchSettingsButton = new QPushButton(toolbar);
    m_searchSettingsButton->setIcon(QIcon(renderSvgIcon(QStringLiteral(":/icons/settings.svg"),
        palette().color(QPalette::ButtonText), QSize(20, 20))));
    m_searchSettingsButton->installEventFilter(this);
    m_searchSettingsButton->setCheckable(true);
    m_searchSettingsButton->setObjectName(QStringLiteral("searchSettingsButton"));
    m_searchSettingsButton->setFixedWidth(40);
    m_searchSettingsButton->setToolTip(tr("Search settings"));
    m_searchSettingsButton->setAccessibleName(tr("Search settings"));
    m_searchSettingsButton->setCursor(Qt::PointingHandCursor);
    m_searchSettingsButton->hide();
    toolbarLayout->addWidget(m_searchSettingsButton);

    // Icon-only actions keep the main page quiet; labels live in tooltips.
    auto *actions = new QWidget(toolbar);
    auto *actionsLayout = new QHBoxLayout(actions);
    actionsLayout->setContentsMargins(0, 0, 0, 0);
    actionsLayout->setSpacing(4);
    configureActionButton(m_uploadButton, QStringLiteral(":/icons/upload.svg"));
    configureActionButton(m_refreshButton, QStringLiteral(":/icons/refresh-cw.svg"));
    configureActionButton(m_yearsButton, QStringLiteral(":/icons/calendar.svg"));
    configureActionButton(m_searchSettingsButton, QStringLiteral(":/icons/settings.svg"));
    for (QPushButton *button : {m_uploadButton, m_refreshButton, m_yearsButton}) {
        const QString tip = button->toolTip();
        setActionButtonLabel(button, QString());
        button->setToolTip(tip);
        button->setAccessibleName(tip);
    }
    m_zoomSlider = new ZoomSlider(actions);
    m_zoomSlider->setValue(m_tileSize);
    m_zoomSlider->setCompact(true);
    actionsLayout->addWidget(m_uploadButton);
    actionsLayout->addWidget(m_refreshButton);
    actionsLayout->addWidget(m_yearsButton);
    actionsLayout->addWidget(m_zoomSlider);
    toolbarLayout->addWidget(actions);
    connect(m_zoomSlider, &ZoomSlider::valueChanged, this, &LibraryPage::setTileSize);
    connect(m_zoomSlider, &ZoomSlider::stepRequested, this,
            [this](int steps) { setTileSize(GridZoom::step(m_tileSize, steps)); });

    m_timelineHost->setObjectName(QStringLiteral("timelineHost"));
    m_timelineHost->setAcceptDrops(true);
    m_timelineHost->installEventFilter(this);
    // Hover preview via GStreamer + QVideoSink software frames. Still disabled
    // under Snap (multimedia path has aborted there); full playback remains.
    if (qEnvironmentVariableIsEmpty("SNAP")) {
        m_videoHoverPreview = new VideoHoverPreview(m_client, m_timelineHost, this);
        m_videoHoverPreview->setEnabled(
            AppSettings().loadPlayback().hoverPreviewEnabled);
    }
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

    m_yearGridHost->setObjectName(QStringLiteral("yearGridHost"));
    m_yearScrollArea->setObjectName(QStringLiteral("yearGridScroll"));
    m_yearScrollArea->setWidgetResizable(false);
    m_yearScrollArea->setFrameShape(QFrame::NoFrame);
    m_yearScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_yearScrollArea->setWidget(m_yearGridHost);

    auto *yearPage = new QWidget(this);
    auto *yearPageLayout = new QVBoxLayout(yearPage);
    yearPageLayout->setContentsMargins(0, 0, 0, 0);
    yearPageLayout->setSpacing(0);
    auto *yearHeaderBar = new QWidget(yearPage);
    auto *yearHeaderLayout = new QHBoxLayout(yearHeaderBar);
    yearHeaderLayout->setContentsMargins(16, 12, 16, 10);
    auto *backToLibraryButton = new QPushButton(tr("Library"), yearHeaderBar);
    configureActionButton(backToLibraryButton, QStringLiteral(":/icons/arrow-left.svg"));
    backToLibraryButton->setCursor(Qt::PointingHandCursor);
    auto *yearHeading = new QLabel(tr("Years"), yearHeaderBar);
    yearHeading->setProperty("heading", true);
    yearHeaderLayout->addWidget(backToLibraryButton);
    yearHeaderLayout->addWidget(yearHeading, 1);
    yearPageLayout->addWidget(yearHeaderBar);
    m_yearGridEmptyState->setAlignment(Qt::AlignCenter);
    m_yearGridEmptyState->setWordWrap(true);
    m_yearGridEmptyState->setProperty("subheading", true);
    m_yearGridEmptyState->setMinimumHeight(120);
    m_yearGridEmptyState->hide();
    yearPageLayout->addWidget(m_yearGridEmptyState);
    yearPageLayout->addWidget(m_yearScrollArea, 1);
    m_contentStack->addWidget(m_scrollArea);
    m_contentStack->addWidget(yearPage);

    m_yearDetailHost->setObjectName(QStringLiteral("yearDetailHost"));
    m_yearDetailScrollArea->setObjectName(QStringLiteral("yearDetailScroll"));
    m_yearDetailScrollArea->setWidgetResizable(false);
    m_yearDetailScrollArea->setFrameShape(QFrame::NoFrame);
    m_yearDetailScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_yearDetailScrollArea->setWidget(m_yearDetailHost);

    auto *yearDetailPage = new QWidget(this);
    auto *yearDetailPageLayout = new QVBoxLayout(yearDetailPage);
    yearDetailPageLayout->setContentsMargins(0, 0, 0, 0);
    yearDetailPageLayout->setSpacing(0);
    auto *yearDetailHeaderBar = new QWidget(yearDetailPage);
    auto *yearDetailHeaderLayout = new QHBoxLayout(yearDetailHeaderBar);
    yearDetailHeaderLayout->setContentsMargins(16, 12, 16, 10);
    auto *backToYearGridButton = new QPushButton(tr("Years"), yearDetailHeaderBar);
    configureActionButton(backToYearGridButton, QStringLiteral(":/icons/arrow-left.svg"));
    backToYearGridButton->setCursor(Qt::PointingHandCursor);
    m_yearDetailStatus->setProperty("heading", true);
    yearDetailHeaderLayout->addWidget(backToYearGridButton);
    yearDetailHeaderLayout->addWidget(m_yearDetailStatus, 1);
    yearDetailPageLayout->addWidget(yearDetailHeaderBar);
    yearDetailPageLayout->addWidget(m_yearDetailScrollArea, 1);
    m_contentStack->addWidget(yearDetailPage);

    root->addWidget(toolbar);
    m_searchPanel = new QWidget(this);
    m_searchPanel->setObjectName(QStringLiteral("searchOptionsPanel"));
    m_searchPanel->installEventFilter(this);
    auto *panelLayout = new QVBoxLayout(m_searchPanel);
    panelLayout->setContentsMargins(16, 0, 16, 12);
    auto *searchCard = new QFrame(m_searchPanel);
    searchCard->setObjectName(QStringLiteral("searchOptionsCard"));
    auto *searchLayout = new QVBoxLayout(searchCard);
    searchLayout->setContentsMargins(18, 16, 18, 16);
    searchLayout->setSpacing(12);
    panelLayout->addWidget(searchCard);
    auto *filterHeader = new QHBoxLayout;
    auto *filterTitle = new QLabel(tr("Refine your search"), searchCard);
    filterTitle->setProperty("section", true);
    filterHeader->addWidget(filterTitle);
    m_searchFilterSummary = new QLabel(searchCard);
    m_searchFilterSummary->setObjectName(QStringLiteral("searchFilterSummary"));
    filterHeader->addWidget(m_searchFilterSummary);
    filterHeader->addStretch();
    auto *resetSearch = new QPushButton(tr("Reset filters"), m_searchPanel);
    configureActionButton(resetSearch, QStringLiteral(":/icons/refresh-cw.svg"));
    resetSearch->setObjectName(QStringLiteral("resetSearchFilters"));
    filterHeader->addWidget(resetSearch);
    searchLayout->addLayout(filterHeader);

    auto *filters = new QGridLayout;
    filters->setHorizontalSpacing(12);
    filters->setVerticalSpacing(6);
    auto addFilter = [this, filters](const QString &label, QWidget *field, int column) {
        auto *caption = new QLabel(label, m_searchPanel);
        caption->setBuddy(field);
        caption->setProperty("searchCaption", true);
        field->setAccessibleName(label);
        filters->addWidget(caption, 0, column);
        filters->addWidget(field, 1, column);
        filters->setColumnStretch(column, 1);
    };
    m_searchScope = new NoWheelComboBox(m_searchPanel);
    m_searchScope->setObjectName(QStringLiteral("searchScope"));
    m_searchScope->addItems({tr("All assets"), tr("Favorites"), tr("Archive")});
    addFilter(tr("Search in"), m_searchScope, 0);
    m_searchPerson = new NoWheelComboBox(m_searchPanel);
    m_searchPerson->setObjectName(QStringLiteral("searchPerson"));
    m_searchPerson->addItem(tr("All people"), QString());
    m_searchPerson->setMinimumContentsLength(12);
    m_searchPerson->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    addFilter(tr("People"), m_searchPerson, 1);
    m_searchPlace = new QLineEdit(m_searchPanel);
    m_searchPlace->setObjectName(QStringLiteral("searchPlace"));
    m_searchPlace->setPlaceholderText(tr("City"));
    m_searchPlace->setClearButtonEnabled(true);
    addFilter(tr("Places"), m_searchPlace, 2);
    m_searchMedia = new NoWheelComboBox(m_searchPanel);
    m_searchMedia->setObjectName(QStringLiteral("searchMedia"));
    m_searchMedia->addItem(tr("Photos and videos"), QString());
    m_searchMedia->addItem(tr("Photos"), QStringLiteral("IMAGE"));
    m_searchMedia->addItem(tr("Videos"), QStringLiteral("VIDEO"));
    addFilter(tr("Media"), m_searchMedia, 3);
    searchLayout->addLayout(filters);

    auto *dateRow = new QHBoxLayout;
    m_searchDateEnabled = new QCheckBox(tr("Date range"), m_searchPanel);
    m_searchDateEnabled->setObjectName(QStringLiteral("searchDateEnabled"));
    m_searchDateFrom = new QDateEdit(QDate::currentDate().addYears(-1), m_searchPanel);
    m_searchDateTo = new QDateEdit(QDate::currentDate(), m_searchPanel);
    m_searchDateFrom->setAccessibleName(tr("From date"));
    m_searchDateTo->setAccessibleName(tr("To date"));
    for (auto *field : {m_searchDateFrom, m_searchDateTo}) {
        field->setCalendarPopup(true);
        field->setDisplayFormat(QLocale().dateFormat(QLocale::ShortFormat));
        field->setEnabled(false);
    }
    dateRow->addWidget(m_searchDateEnabled);
    dateRow->addWidget(new QLabel(tr("From"), m_searchPanel));
    dateRow->addWidget(m_searchDateFrom);
    dateRow->addWidget(new QLabel(tr("To"), m_searchPanel));
    dateRow->addWidget(m_searchDateTo);
    dateRow->addStretch();
    searchLayout->addLayout(dateRow);

    auto *divider = new QFrame(searchCard);
    divider->setObjectName(QStringLiteral("searchOptionsDivider"));
    divider->setFixedHeight(1);
    searchLayout->addWidget(divider);
    auto *modeTitle = new QLabel(tr("Search by"), searchCard);
    modeTitle->setProperty("searchCaption", true);
    searchLayout->addWidget(modeTitle);

    m_searchMode = new NoWheelComboBox(m_searchPanel);
    m_searchMode->setObjectName(QStringLiteral("searchMode"));
    m_searchMode->addItem(tr("Context"), QString());
    m_searchMode->addItem(tr("File name"), QStringLiteral("originalFileName"));
    m_searchMode->addItem(tr("Description"), QStringLiteral("description"));
    m_searchMode->addItem(tr("Full path or folder"), QStringLiteral("originalPath"));
    m_searchMode->addItem(tr("OCR"), QStringLiteral("ocr"));
    m_searchMode->setAccessibleName(tr("Search interpretation"));
    m_searchMode->hide(); // Keep one source of truth for query interpretation.
    m_searchModeButtons = new QButtonGroup(this);
    m_searchModeButtons->setExclusive(true);
    auto *modeRow = new QHBoxLayout;
    modeRow->setSpacing(8);
    const QStringList modeIcons = {QStringLiteral("images"), QStringLiteral("save"),
        QStringLiteral("layout-dashboard"), QStringLiteral("folder"), QStringLiteral("server")};
    for (int index = 0; index < m_searchMode->count(); ++index) {
        auto *button = new QToolButton(searchCard);
        button->setObjectName(QStringLiteral("searchModeOption"));
        button->setText(m_searchMode->itemText(index));
        button->setAccessibleName(button->text());
        button->setCursor(Qt::PointingHandCursor);
        button->setCheckable(true);
        button->setToolButtonStyle(Qt::ToolButtonTextOnly);
        button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        button->setProperty("modeIcon", QStringLiteral(":/icons/%1.svg").arg(modeIcons.at(index)));
        m_searchModeButtons->addButton(button, index);
        modeRow->addWidget(button, 1);
    }
    searchLayout->addLayout(modeRow);
    m_searchModeDescription = new QLabel(searchCard);
    m_searchModeDescription->setWordWrap(true);
    m_searchModeDescription->setProperty("subheading", true);
    searchLayout->addWidget(m_searchModeDescription);
    m_searchModeAnimation = new QVariantAnimation(this);
    m_searchModeAnimation->setDuration(140);
    m_searchModeAnimation->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_searchModeAnimation, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        m_searchModeDescription->setStyleSheet(QStringLiteral("color: %1;")
            .arg(value.value<QColor>().name(QColor::HexArgb)));
    });
    connect(m_searchModeButtons, &QButtonGroup::idClicked, m_searchMode, &QComboBox::setCurrentIndex);
    connect(m_searchMode, &QComboBox::currentIndexChanged, this, [this] {
        updateSearchOptionsUi();
        if (m_searchPanel->isVisible()) {
            m_searchModeAnimation->stop();
            const QColor color = m_searchModeDescription->palette().color(QPalette::WindowText);
            QColor faded = color;
            faded.setAlpha(110);
            m_searchModeAnimation->setStartValue(faded);
            m_searchModeAnimation->setEndValue(color);
            m_searchModeAnimation->start();
        }
    });
    m_searchPanelOpacity = new QGraphicsOpacityEffect(m_searchPanel);
    m_searchPanelOpacity->setOpacity(0.0);
    m_searchPanel->setGraphicsEffect(m_searchPanelOpacity);
    m_searchPanelAnimation = new QParallelAnimationGroup(this);
    m_searchPanelHeightAnimation = new QPropertyAnimation(m_searchPanel, "maximumHeight");
    m_searchPanelFadeAnimation = new QPropertyAnimation(m_searchPanelOpacity, "opacity");
    for (auto *animation : {m_searchPanelHeightAnimation, m_searchPanelFadeAnimation}) {
        animation->setDuration(180);
        animation->setEasingCurve(QEasingCurve::OutCubic);
        m_searchPanelAnimation->addAnimation(animation);
    }
    connect(m_searchPanelAnimation, &QParallelAnimationGroup::finished, this, [this] {
        if (m_searchSettingsButton->isChecked())
            m_searchPanel->setMaximumHeight(QWIDGETSIZE_MAX);
        else
            m_searchPanel->hide();
    });
    m_searchPanel->setMaximumHeight(0);
    m_searchPanel->hide();
    updateSearchOptionsUi();
    root->addWidget(m_searchPanel);
    root->addWidget(m_emptyState);
    root->addWidget(m_contentStack, 1);

    connect(backToLibraryButton, &QPushButton::clicked, this, &LibraryPage::backToLibrary);
    connect(backToYearGridButton, &QPushButton::clicked, this, &LibraryPage::backToYearGrid);
    connect(m_yearsButton, &QPushButton::clicked, this, &LibraryPage::showYearGrid);

    connect(m_uploadButton, &QPushButton::clicked, this, &LibraryPage::chooseFilesToUpload);
    connect(m_refreshButton, &QPushButton::clicked, this, &LibraryPage::refresh);
    connect(m_searchField, &QLineEdit::textChanged, this, [this] {
        updateSearchExpansion();
        m_searchDebounce->start();
    });
    connect(qApp, &QApplication::focusChanged, this, [this] {
        QTimer::singleShot(0, this, &LibraryPage::updateSearchExpansion);
    });
    connect(m_searchSettingsButton, &QPushButton::toggled, this, [this](bool open) {
        animateSearchPanel(open);
        updateSearchExpansion();
        if (open && m_searchPerson->count() == 1 && m_client->isOnline())
            m_client->loadExplore();
    });
    auto optionsChanged = [this] {
        updateSearchOptionsUi();
        if (!m_resettingSearchOptions)
            m_searchDebounce->start();
    };
    for (auto *field : {m_searchScope, m_searchPerson, m_searchMedia, m_searchMode})
        connect(field, &QComboBox::currentIndexChanged, this, optionsChanged);
    connect(m_searchPlace, &QLineEdit::textChanged, this, optionsChanged);
    connect(m_searchDateFrom, &QDateEdit::dateChanged, this, optionsChanged);
    connect(m_searchDateTo, &QDateEdit::dateChanged, this, optionsChanged);
    connect(m_searchDateEnabled, &QCheckBox::toggled, this, [this, optionsChanged](bool enabled) {
        m_searchDateFrom->setEnabled(enabled);
        m_searchDateTo->setEnabled(enabled);
        optionsChanged();
    });
    connect(resetSearch, &QPushButton::clicked, this, &LibraryPage::resetSearchFilters);
    connect(m_client, &ImmichClient::exploreLoaded, this, [this](const ImmichExploreData &data) {
        const QString selected = m_searchPerson->currentData().toString();
        const QSignalBlocker blocker(m_searchPerson);
        m_searchPerson->clear();
        m_searchPerson->addItem(tr("All people"), QString());
        for (const auto &person : data.people)
            m_searchPerson->addItem(person.name.isEmpty() ? tr("Unnamed person") : person.name, person.id);
        const int index = m_searchPerson->findData(selected);
        m_searchPerson->setCurrentIndex(qMax(0, index));
        updateSearchOptionsUi();
    });
    connect(m_searchField, &QLineEdit::returnPressed, this, [this] {
        m_searchDebounce->stop();
        applySearch();
    });
    connect(m_client, &ImmichClient::assetsLoaded, this, &LibraryPage::showAssets);
    connect(m_client, &ImmichClient::timelineBucketsLoaded,
            this, &LibraryPage::handleTimelineBucketsLoaded);
    connect(m_client, &ImmichClient::timelineBucketLoaded,
            this, &LibraryPage::handleTimelineBucketLoaded);
    connect(m_client, &ImmichClient::timelineBucketFailed,
            this, &LibraryPage::handleTimelineBucketFailed);
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
    connect(m_client, &ImmichClient::assetInfoLoaded,
            this, &LibraryPage::handleAssetInfoLoaded);
    connect(m_client, &ImmichClient::assetInfoFailed,
            this, &LibraryPage::handleAssetInfoFailed);
    connect(m_client, &ImmichClient::assetOriginalFetched,
            this, &LibraryPage::handleAssetOriginalFetched);
    connect(m_client, &ImmichClient::assetsDeleted, this, &LibraryPage::handleAssetsDeleted);
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
                const QSet<QString> ids(assetIds.begin(), assetIds.end());
                for (ImmichAsset &asset : m_assets) {
                    if (ids.contains(asset.id))
                        asset.favorite = favorite;
                }
                for (const QString &id : assetIds) {
                    if (MediaTile *tile = m_tilesById.value(id)) {
                        ImmichAsset asset = tile->asset();
                        asset.favorite = favorite;
                        tile->updateAsset(asset);
                    }
                }
            });
    // Restored photos rejoin the timeline; reload on the next visit rather than
    // immediately, since a full Library reload can take a while.
    connect(m_client, &ImmichClient::assetsRestored, this, [this] {
        if (isVisible())
            refresh();
        else
            m_reloadOnShow = true;
    });
    connect(m_client, &ImmichClient::activeEndpointChanged,
            this, &LibraryPage::handleActiveEndpointChanged);
    connect(m_client, &ImmichClient::onlineChanged, this, &LibraryPage::handleOnlineChanged);
    connect(m_client, &ImmichClient::uploadQueueChanged, this,
            &LibraryPage::handleUploadQueueChanged);
    if (m_folderSync) {
        connect(m_folderSync, &FolderSyncService::pendingLibraryAssetsChanged, this,
                &LibraryPage::syncPendingLocalAssets);
    }
    connect(m_client, &ImmichClient::configurationChanged, this, [this](bool configured) {
        m_uploadButton->setEnabled(configured);
        m_searchField->setEnabled(configured);
        m_searchSettingsButton->setEnabled(configured);
        m_searchPanel->setEnabled(configured);
        if (configured)
            refresh();
        else {
            clearTimeline();
            syncPendingLocalAssets();
            updateEmptyState();
        }
        updateAutoCheckTimer();
        updateEndpointHint();
    });

    m_uploadButton->setEnabled(m_client->isConfigured());
    m_searchField->setEnabled(m_client->isConfigured());
    m_searchSettingsButton->setEnabled(m_client->isConfigured());
    m_searchPanel->setEnabled(m_client->isConfigured());
    updateEmptyState();
    updateAutoCheckTimer();
    updateEndpointHint();

    auto *pasteShortcut = new QShortcut(QKeySequence::Paste, this);
    pasteShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(pasteShortcut, &QShortcut::activated, this, &LibraryPage::pasteFromClipboard);

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
            setTileSize(steps == 0 ? GridZoom::kDefault : GridZoom::step(m_tileSize, steps));
        });
    }

    auto *copyShortcut = new QShortcut(QKeySequence::Copy, this);
    copyShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(copyShortcut, &QShortcut::activated, this, [this] {
        if (isSearchFieldFocused())
            return;
        copyAsset(currentAssetForClipboard());
    });

    QTimer::singleShot(0, this, [this] {
        if (m_client->isConfigured())
            refresh();
    });
}

void LibraryPage::setCompactGrid(bool enabled)
{
    if (m_compactGrid == enabled)
        return;
    m_compactGrid = enabled;
    scheduleLayout();
}

void LibraryPage::setTileSize(int size)
{
    size = GridZoom::clamp(size);
    if (m_tileSize == size)
        return;

    QScrollBar *scrollBar = m_scrollArea->verticalScrollBar();
    const qreal position =
        scrollBar->maximum() > 0 ? qreal(scrollBar->value()) / scrollBar->maximum() : 0.0;
    m_tileSize = size;
    m_zoomSlider->setValue(size);
    layoutTimeline();
    scrollBar->setValue(qRound(position * scrollBar->maximum()));

    TimelineSettings timeline = AppSettings().loadTimeline();
    timeline.tileSize = size;
    AppSettings().saveTimeline(timeline);
}

void LibraryPage::refresh()
{
    if (m_loading)
        return;

    if (m_searchActive) {
        requestPage(1, false);
        return;
    }

    m_monthBuckets.clear();
    m_readyBucketAssets.clear();
    m_inflightBuckets.clear();
    ++m_timelineEpoch;
    m_nextBucketToRequest = 0;
    m_flushedBucketCount = 0;
    m_pendingScrollToDate = QDate();

    if (!m_client->isOnline()) {
        // Falls back to the flat cached snapshot via the existing page path.
        requestPage(1, false);
        return;
    }

    clearTimeline();
    syncPendingLocalAssets();
    m_loading = true;
    updateEmptyState();
    m_refreshButton->setEnabled(false);
    m_status->setText(tr("Loading library…"));
    // Keep the previous offline snapshot until a successful reload replaces it.
    m_client->loadTimelineBuckets();
}

bool LibraryPage::hasMoreToLoad() const
{
    // The library publishes a complete index; only search is paginated.
    return m_searchActive && !m_nextPage.isEmpty();
}

void LibraryPage::loadMore()
{
    if (m_loading)
        return;

    if (m_searchActive) {
        bool ok = false;
        const int page = m_nextPage.toInt(&ok);
        if (ok && page > 0)
            requestPage(page, true);
        return;
    }

}

void LibraryPage::maybeLoadMore()
{
    if (m_loading || !hasMoreToLoad() || !m_scrollArea->isVisible())
        return;

    const QScrollBar *scrollBar = m_scrollArea->verticalScrollBar();
    const int preloadDistance = qMax(500, m_scrollArea->viewport()->height());
    if (scrollBar->maximum() - scrollBar->value() <= preloadDistance)
        loadMore();
}

void LibraryPage::kickTimelineBucketFetches()
{
    if (m_searchActive || m_contentStack->currentIndex() != 0)
        return;
    while (m_inflightBuckets.size() < kParallelTimelineBuckets
           && m_nextBucketToRequest < m_monthBuckets.size()) {
        const QDate month = m_monthBuckets.at(m_nextBucketToRequest).month;
        ++m_nextBucketToRequest;
        m_inflightBuckets.insert(month, m_timelineEpoch);
        m_client->loadTimelineBucket(month);
    }
}

int LibraryPage::remainingIndexedAssetCount() const
{
    int remaining = 0;
    for (int i = m_flushedBucketCount; i < m_monthBuckets.size(); ++i)
        remaining += m_monthBuckets.at(i).count;
    return remaining;
}

int LibraryPage::estimateReservedTimelineHeight(int remainingAssets) const
{
    if (remainingAssets <= 0)
        return 0;
    const int viewportWidth = m_scrollArea->viewport()->width();
    const int availableWidth = qMax(240, viewportWidth - 2 * kSidePad);
    if (m_compactGrid) {
        constexpr int kGridGap = 3;
        const int columns = qMax(1, (availableWidth + kGridGap) / (m_tileSize + kGridGap));
        const int cellSize = (availableWidth - (columns - 1) * kGridGap) / columns;
        const int rows = (remainingAssets + columns - 1) / columns;
        return rows * (cellSize + kGridGap);
    }
    const int columns = qMax(1, (availableWidth + kGap) / (m_tileSize + kGap));
    const int rows = (remainingAssets + columns - 1) / columns;
    // Include a little room for day headers that appear once months flush in.
    return rows * (m_tileSize + kGap) + rows * 4;
}

void LibraryPage::updateTimelineLoadingStatus()
{
    if (m_searchActive || !m_loading)
        return;
    if (m_flushedBucketCount >= m_monthBuckets.size()) {
        m_status->setText(tr("%n item(s)", nullptr, m_assets.size()));
        return;
    }
    m_status->setText(tr("Loading library… %1/%2 months")
                          .arg(m_flushedBucketCount)
                          .arg(m_monthBuckets.size()));
}

void LibraryPage::flushReadyTimelineBuckets()
{
    bool appended = false;
    while (m_flushedBucketCount < m_monthBuckets.size()) {
        const QDate month = m_monthBuckets.at(m_flushedBucketCount).month;
        if (!m_readyBucketAssets.contains(month))
            break;
        appendAssetsToTimeline(m_readyBucketAssets.take(month));
        ++m_flushedBucketCount;
        appended = true;
    }
    if (!appended)
        return;
    updateEmptyState();
    layoutTimeline();
    scheduleLayout();
    if (m_pendingScrollToDate.isValid())
        QTimer::singleShot(60, this, &LibraryPage::scrollToPendingDate);
}

void LibraryPage::finishTimelineBucket(const QDate &month, const QList<ImmichAsset> &assets)
{
    if (m_inflightBuckets.value(month, -1) != m_timelineEpoch)
        return;
    m_inflightBuckets.remove(month);
    m_readyBucketAssets.insert(month, assets);
    flushReadyTimelineBuckets();
    updateTimelineLoadingStatus();

    if (m_flushedBucketCount >= m_monthBuckets.size() && m_inflightBuckets.isEmpty()) {
        m_loading = false;
        // Otherwise the new-photo check stays paused after its first refresh.
        m_autoRefreshPending = false;
        m_refreshButton->setEnabled(true);
        m_status->setText(tr("%n item(s)", nullptr, m_assets.size()));
        updateEndpointHint();
        updateEmptyState();
        updateAutoCheckTimer();
    }
}

void LibraryPage::checkForNewPhotos()
{
    if (!isVisible() || m_loading || m_autoRefreshPending || !m_client->isConfigured() ||
        m_searchActive)
        return;
    m_client->pollNewestAssets(24);
}

void LibraryPage::applySearch()
{
    if (m_searchDateEnabled->isChecked() && m_searchDateFrom->date() > m_searchDateTo->date()) {
        m_status->setText(tr("The start date must be on or before the end date."));
        return;
    }
    const QString query = m_searchField->text().trimmed();
    const QJsonObject options = searchOptions();
    if (query == m_searchQuery && options == m_searchOptions)
        return;
    m_searchQuery = query;
    m_searchOptions = options;
    QJsonObject filters = options;
    filters.remove(QStringLiteral("mode"));
    m_searchActive = !query.isEmpty() || !filters.isEmpty();
    m_contentStack->setCurrentIndex(0);
    m_nextPage.clear();
    updateAutoCheckTimer();
    // Clearing search must return to timeline buckets, not flat /search/metadata.
    if (!m_searchActive && m_client->isOnline()) {
        m_loading = false; // allow refresh() even if a search request is still in flight
        refresh();
        return;
    }
    requestPage(1, false);
}

QJsonObject LibraryPage::searchOptions() const
{
    QJsonObject options;
    const QString mode = m_searchMode->currentData().toString();
    if (!mode.isEmpty())
        options.insert(QStringLiteral("mode"), mode);
    if (m_searchScope->currentIndex() == 1)
        options.insert(QStringLiteral("isFavorite"), true);
    else if (m_searchScope->currentIndex() == 2)
        options.insert(QStringLiteral("visibility"), QStringLiteral("archive"));
    const QString personId = m_searchPerson->currentData().toString();
    if (!personId.isEmpty())
        options.insert(QStringLiteral("personIds"), QJsonArray{personId});
    const QString city = m_searchPlace->text().trimmed();
    if (!city.isEmpty())
        options.insert(QStringLiteral("city"), city);
    const QString media = m_searchMedia->currentData().toString();
    if (!media.isEmpty())
        options.insert(QStringLiteral("type"), media);
    if (m_searchDateEnabled->isChecked()) {
        options.insert(QStringLiteral("takenAfter"),
            m_searchDateFrom->date().startOfDay().toUTC().toString(Qt::ISODate));
        options.insert(QStringLiteral("takenBefore"),
            m_searchDateTo->date().endOfDay().toUTC().toString(Qt::ISODateWithMs));
    }
    return options;
}

void LibraryPage::updateSearchExpansion()
{
    const bool expanded = m_searchField->hasFocus() || !m_searchField->text().isEmpty()
        || m_searchSettingsButton->hasFocus() || m_searchSettingsButton->isChecked()
        || m_searchPanel->isAncestorOf(QApplication::focusWidget());
    m_searchSettingsButton->setVisible(m_searchActivated && expanded);
    m_searchField->setMaximumWidth(expanded ? 520 : 360);
    m_searchToolbarLayout->setStretch(0, expanded ? 0 : 1);
    updateToolbarDensity();
}

void LibraryPage::resetSearchFilters()
{
    m_resettingSearchOptions = true;
    for (auto *field : {m_searchScope, m_searchPerson, m_searchMedia, m_searchMode})
        field->setCurrentIndex(0);
    m_searchPlace->clear();
    m_searchDateEnabled->setChecked(false);
    m_resettingSearchOptions = false;
    m_searchDebounce->stop();
    applySearch();
}

void LibraryPage::showDay(const QDate &day)
{
    if (m_searchActive || m_monthBuckets.isEmpty()) {
        // Jump once the timeline's month list has (re)loaded.
        m_pendingDay = day;
        if (m_searchActive) {
            m_searchField->clear();
            resetSearchFilters();
        }
        return;
    }
    jumpToDate(day);
}

void LibraryPage::updateToolbarDensity()
{
    const QWidget *toolbar = m_searchToolbarLayout->parentWidget();
    // Actions stay icon-only; hide zoom controls, then the title, when space is tight.
    auto fits = [this, toolbar](int stage) {
        m_zoomSlider->setVisible(stage < 1);
        m_zoomSlider->setCompact(true);
        m_heading->setVisible(stage < 2);
        m_status->setVisible(stage < 2);
        m_searchToolbarLayout->activate();
        return m_searchToolbarLayout->minimumSize().width() <= toolbar->width();
    };
    if (!fits(0) && !fits(1))
        fits(2);
}

void LibraryPage::animateSearchPanel(bool open)
{
    m_searchPanelAnimation->stop();
    const int currentHeight = m_searchPanel->isVisible() ? m_searchPanel->height() : 0;
    if (open) {
        m_searchPanel->setEnabled(m_client->isConfigured());
        m_searchPanel->show();
        m_searchPanel->layout()->activate();
    } else {
        // Exclude collapsing controls from keyboard navigation.
        m_searchSettingsButton->setFocus(Qt::OtherFocusReason);
        m_searchPanel->setEnabled(false);
    }
    const int targetHeight = open ? m_searchPanel->sizeHint().height() : 0;
    m_searchPanelHeightAnimation->setStartValue(currentHeight);
    m_searchPanelHeightAnimation->setEndValue(targetHeight);
    m_searchPanelFadeAnimation->setStartValue(m_searchPanelOpacity->opacity());
    m_searchPanelFadeAnimation->setEndValue(open ? 1.0 : 0.0);
    m_searchPanelAnimation->start();
}

void LibraryPage::updateSearchOptionsUi()
{
    if (!m_searchModeButtons)
        return;
    const QStringList descriptions = {
        tr("Find scenes, objects, and ideas in the visual content of your photos."),
        tr("Match the original name of a photo or video file."),
        tr("Search the descriptions saved with your photos and videos."),
        tr("Find media by its original full path or folder."),
        tr("Find words detected inside images, such as signs, receipts, and screenshots.")};
    const int mode = m_searchMode->currentIndex();
    const auto appearance = AppSettings().loadAppearance();
    const bool minimal = appearance.minimalistUi;
    const auto colors = appearance.preset == ThemePreset::Light ? ThemePalette::light()
        : (appearance.preset == ThemePreset::Custom ? appearance.customPalette : ThemePalette::dark());
    for (auto *option : m_searchModeButtons->buttons()) {
        auto *button = qobject_cast<QToolButton *>(option);
        const int index = m_searchModeButtons->id(button);
        button->setChecked(index == mode);
        button->setToolTip(m_searchMode->itemText(index) + QStringLiteral(" — ") + descriptions.at(index));
        button->setToolButtonStyle(minimal ? Qt::ToolButtonIconOnly : Qt::ToolButtonTextOnly);
        button->setIcon(QIcon(renderSvgIcon(button->property("modeIcon").toString(),
            index == mode ? QColor(Qt::white) : colors.text,
            QSize(18, 18), devicePixelRatioF())));
    }
    m_searchModeDescription->setText(descriptions.at(mode));
    m_searchModeDescription->setStyleSheet(QStringLiteral("color: %1;").arg(colors.mutedText.name()));
    QJsonObject filters = searchOptions();
    filters.remove(QStringLiteral("mode"));
    filters.remove(QStringLiteral("takenBefore")); // A date range is one filter.
    m_searchFilterSummary->setText(filters.isEmpty() ? tr("No filters")
        : tr("%n active filter(s)", nullptr, filters.size()));
}

void LibraryPage::handleNewestAssetsPolled(const QList<ImmichAsset> &assets)
{
    if (m_loading || m_autoRefreshPending || m_searchActive)
        return;

    int newCount = 0;
    for (const ImmichAsset &asset : assets) {
        if (!m_tilesById.contains(asset.id))
            ++newCount;
    }
    if (newCount == 0)
        return;

    m_autoRefreshPending = true;
    m_status->setText(tr("%n new photo(s) found, refreshing…", nullptr, newCount));
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
        if (m_searchActive) {
            m_status->setText(append ? tr("Loading more results…")
                : (m_searchQuery.isEmpty() ? tr("Searching with filters…")
                                          : tr("Searching for “%1”…").arg(m_searchQuery)));
        } else {
            m_status->setText(append ? tr("Loading more media…")
                                     : tr("Loading your library…"));
        }
    }
    m_client->loadAssets(page, 80, m_searchQuery, m_searchActive ? m_searchOptions : QJsonObject{});
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

    // Newest days first so local pending tiles stay above older Immich months.
    int insertAt = m_sections.size();
    for (int i = 0; i < m_sections.size(); ++i) {
        if (!date.isValid())
            break;
        if (!m_sections.at(i).date.isValid() || date > m_sections.at(i).date) {
            insertAt = i;
            break;
        }
    }
    m_sections.insert(insertAt, section);
    return &m_sections[insertAt];
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
    return locale.toString(date, QLocale::LongFormat);
}

void LibraryPage::showAssets(const QList<ImmichAsset> &assets, const QString &nextPage,
                             const QString &query, bool fromCache)
{
    if (query != m_searchQuery)
        return;

    m_showingCached = fromCache;
    if (!m_appendRequest)
        clearTimeline();

    appendAssetsToTimeline(assets);
    syncPendingLocalAssets();

    m_nextPage = fromCache ? QString() : nextPage;
    m_loading = false;
    m_autoRefreshPending = false;
    m_refreshButton->setEnabled(true);
    if (m_searchActive) {
        m_status->setText(m_searchQuery.isEmpty() ? tr("%n filtered result(s)", nullptr, m_assets.size())
            : tr("%n result(s) for “%1”", nullptr, m_assets.size()).arg(m_searchQuery));
    } else if (fromCache) {
        m_status->setText(tr("%n cached item(s) · offline", nullptr, m_assets.size()));
    } else {
        m_status->setText(tr("%n item(s)", nullptr, m_assets.size()));
        updateEndpointHint();
    }
    scheduleLayout();
    updateEmptyState();
    updateAutoCheckTimer();
}

void LibraryPage::appendAssetsToTimeline(const QList<ImmichAsset> &assets)
{
    for (const ImmichAsset &asset : assets) {
        if (m_tilesById.contains(asset.id))
            continue;

        m_assets.append(asset);
        const QDate date = asset.takenAt.isValid() ? asset.takenAt.date() : QDate();
        DaySection *section = sectionForDate(date);
        auto *tile = new MediaTile(asset, m_timelineHost);
        if (!asset.isLocalPending())
            tile->setHoverPreview(m_videoHoverPreview);
        section->tiles.append(tile);
        m_tilesById.insert(asset.id, tile);
        connect(tile, &MediaTile::activated, this, &LibraryPage::openAsset);
        connect(tile, &MediaTile::infoRequested, this, [this](const ImmichAsset &asset) {
            if (MediaViewerDialog *viewer = openAsset(asset))
                viewer->setInfoPanelVisible(true);
        });
        connect(tile, &MediaTile::highlighted, this, [this](const ImmichAsset &asset) {
            m_currentAsset = asset;
        });
        connect(tile, &MediaTile::copyRequested, this, &LibraryPage::copyAsset);
        connect(tile, &MediaTile::downloadRequested, this, &LibraryPage::downloadAsset);
        connect(tile, &MediaTile::shareRequested, this, [this](const ImmichAsset &asset) {
            (new ShareLinkDialog(m_client, asset, this))->show();
        });
        connect(tile, &MediaTile::trashRequested, this, &LibraryPage::trashAsset);
        connect(tile, &MediaTile::deleteRequested, this, &LibraryPage::deleteAssetPermanently);
        connect(tile, &MediaTile::favoriteRequested, this,
                [this](const ImmichAsset &asset, bool favorite) {
                    m_favoriteRequests.insert(asset.id);
                    m_client->setFavorite({asset.id}, favorite);
                });
    }

    if (!m_assets.isEmpty())
        m_newestAssetId = m_assets.first().id;
}

void LibraryPage::prependLocalAsset(const ImmichAsset &asset)
{
    if (m_tilesById.contains(asset.id)) {
        if (MediaTile *tile = m_tilesById.value(asset.id))
            tile->updateAsset(asset);
        return;
    }

    m_assets.prepend(asset);
    const QDate date = asset.takenAt.isValid() ? asset.takenAt.date() : QDate();
    DaySection *section = sectionForDate(date);
    auto *tile = new MediaTile(asset, m_timelineHost);
    section->tiles.prepend(tile);
    m_tilesById.insert(asset.id, tile);
    m_localAssetIds.insert(asset.id);
    connect(tile, &MediaTile::activated, this, &LibraryPage::openAsset);
    connect(tile, &MediaTile::highlighted, this, [this](const ImmichAsset &highlighted) {
        m_currentAsset = highlighted;
    });
    loadLocalThumbnail(tile, asset);
}

void LibraryPage::removeLocalAsset(const QString &assetId)
{
    MediaTile *tile = m_tilesById.take(assetId);
    m_localAssetIds.remove(assetId);
    m_requestedThumbnails.remove(assetId);
    for (int i = 0; i < m_assets.size(); ++i) {
        if (m_assets.at(i).id == assetId) {
            m_assets.removeAt(i);
            break;
        }
    }
    if (!tile)
        return;
    for (DaySection &section : m_sections)
        section.tiles.removeAll(tile);
    tile->deleteLater();
}

void LibraryPage::loadLocalThumbnail(MediaTile *tile, const ImmichAsset &asset)
{
    if (!tile || asset.localPath.isEmpty())
        return;
    QImageReader reader(asset.localPath);
    reader.setAutoTransform(true);
    QSize size = reader.size();
    if (size.isValid()) {
        size.scale(256, 256, Qt::KeepAspectRatioByExpanding);
        reader.setScaledSize(size);
    }
    const QImage image = reader.read();
    if (!image.isNull())
        tile->setThumbnail(QPixmap::fromImage(image));
}

void LibraryPage::syncPendingLocalAssets()
{
    if (!m_folderSync)
        return;

    // Keep search results clean - local pending only in the main timeline.
    if (m_searchActive) {
        const QStringList localIds = m_localAssetIds.values();
        for (const QString &id : localIds)
            removeLocalAsset(id);
        scheduleLayout();
        updateEmptyState();
        return;
    }

    const QList<ImmichAsset> pending = m_folderSync->pendingLibraryAssets();
    QSet<QString> keep;
    keep.reserve(pending.size());
    for (const ImmichAsset &asset : pending) {
        keep.insert(asset.id);
        if (m_tilesById.contains(asset.id)) {
            if (MediaTile *tile = m_tilesById.value(asset.id)) {
                tile->updateAsset(asset);
                if (!tile->hasThumbnail())
                    loadLocalThumbnail(tile, asset);
            }
        } else {
            prependLocalAsset(asset);
        }
    }

    const QStringList existing = m_localAssetIds.values();
    for (const QString &id : existing) {
        if (!keep.contains(id))
            removeLocalAsset(id);
    }

    scheduleLayout();
    scheduleVisibleMediaUpdate();
    updateEmptyState();
}

void LibraryPage::handleTimelineBucketsLoaded(const QList<TimeBucketInfo> &buckets)
{
    if (m_searchActive)
        return;

    m_monthBuckets = buckets;
    m_readyBucketAssets.clear();
    m_inflightBuckets.clear();
    m_nextBucketToRequest = 0;
    m_flushedBucketCount = 0;

    if (m_pendingDay.isValid())
        m_pendingScrollToDate = std::exchange(m_pendingDay, QDate());

    if (m_monthBuckets.isEmpty()) {
        m_loading = false;
        m_refreshButton->setEnabled(true);
        m_status->setText(tr("%n item(s)", nullptr, m_assets.size()));
        updateEmptyState();
        if (m_contentStack->currentIndex() == 1)
            showYearGrid();
        return;
    }

    // Years view only needs the bucket list for the year grid; don't start
    // filling the main library timeline in the background.
    if (m_contentStack->currentIndex() != 0) {
        m_loading = false;
        m_refreshButton->setEnabled(true);
        if (m_contentStack->currentIndex() == 1)
            buildYearGrid();
        return;
    }

    m_loading = true;
    m_refreshButton->setEnabled(false);
    updateTimelineLoadingStatus();
    updateEmptyState();
    // Reserve scroll space from bucket counts before any month arrives.
    layoutTimeline();
    kickTimelineBucketFetches();
}

void LibraryPage::handleTimelineBucketLoaded(const QDate &month,
                                             const QList<ImmichAsset> &assets)
{
    if (m_pendingYearCoverRequests.contains(month)) {
        const int year = m_pendingYearCoverRequests.take(month);
        handleYearCoverLoaded(year, assets);
    }
    if (m_pendingYearDetailRequests.contains(month)) {
        const int year = m_pendingYearDetailRequests.take(month);
        handleYearDetailBucketLoaded(year, month, assets);
    }

    if (m_searchActive)
        return;
    if (m_inflightBuckets.value(month, -1) != m_timelineEpoch)
        return;

    finishTimelineBucket(month, assets);
    kickTimelineBucketFetches();
}

void LibraryPage::handleTimelineBucketFailed(const QDate &month, const QString &message)
{
    m_pendingYearCoverRequests.remove(month);
    if (m_pendingYearDetailRequests.contains(month)) {
        const int year = m_pendingYearDetailRequests.take(month);
        handleYearDetailBucketLoaded(year, month, {});
    }

    if (m_searchActive)
        return;
    if (m_inflightBuckets.value(month, -1) != m_timelineEpoch)
        return;

    // Skip the failed month so later months can still stream in.
    finishTimelineBucket(month, {});
    kickTimelineBucketFetches();
    if (m_assets.isEmpty() && m_flushedBucketCount >= m_monthBuckets.size()) {
        m_status->setText(tr("Couldn't load library: %1. Refresh to retry.").arg(message));
        updateEmptyState();
        m_emptyState->setText(m_status->text());
    } else if (m_loading) {
        m_status->setText(tr("Loading library… %1/%2 months (some months failed)")
                              .arg(m_flushedBucketCount)
                              .arg(m_monthBuckets.size()));
    }
}

QList<LibraryPage::YearSummary> LibraryPage::computeYearSummaries() const
{
    QHash<int, YearSummary> byYear;
    for (const TimeBucketInfo &bucket : m_monthBuckets) {
        const int year = bucket.month.year();
        auto it = byYear.find(year);
        if (it == byYear.end()) {
            // m_monthBuckets is ordered newest-first, so the first bucket seen
            // for a year is already its most recent month.
            byYear.insert(year, {year, bucket.count, bucket.month});
        } else {
            it->count += bucket.count;
        }
    }
    QList<YearSummary> years = byYear.values();
    std::sort(years.begin(), years.end(),
             [](const YearSummary &a, const YearSummary &b) { return a.year > b.year; });
    return years;
}

void LibraryPage::showYearGrid()
{
    m_contentStack->setCurrentIndex(1);

    if (!m_client->isOnline()) {
        clearYearGrid();
        m_yearScrollArea->hide();
        m_yearGridEmptyState->setText(
            tr("You're offline - Years needs a connection to browse by month."));
        m_yearGridEmptyState->show();
        return;
    }
    m_yearGridEmptyState->hide();
    m_yearScrollArea->show();

    if (m_monthBuckets.isEmpty()) {
        m_status->setText(tr("Loading…"));
        m_client->loadTimelineBuckets();
        return;
    }
    buildYearGrid();
}

void LibraryPage::backToLibrary()
{
    m_contentStack->setCurrentIndex(0);
    if (!m_searchActive && m_flushedBucketCount < m_monthBuckets.size()) {
        m_loading = true;
        m_refreshButton->setEnabled(false);
        updateTimelineLoadingStatus();
        kickTimelineBucketFetches();
    }
    updateEmptyState();
    scheduleVisibleMediaUpdate();
}

void LibraryPage::backToYearGrid()
{
    m_contentStack->setCurrentIndex(1);
}

void LibraryPage::clearYearGrid()
{
    for (auto it = m_summaryTilesByAssetId.begin(); it != m_summaryTilesByAssetId.end(); ) {
        if (m_yearTiles.contains(it.value()))
            it = m_summaryTilesByAssetId.erase(it);
        else
            ++it;
    }
    for (SummaryTile *tile : std::as_const(m_yearTiles))
        tile->deleteLater();
    m_yearTiles.clear();
    m_yearTilesByYear.clear();
    m_pendingYearCoverRequests.clear();
}

void LibraryPage::buildYearGrid()
{
    clearYearGrid();
    m_yearSummaries = computeYearSummaries();

    for (const YearSummary &summary : std::as_const(m_yearSummaries)) {
        auto *tile = new SummaryTile(QString::number(summary.year), summary.count,
                                     m_yearGridHost);
        connect(tile, &SummaryTile::clicked, this, [this, year = summary.year] {
            handleYearTileClicked(year);
        });
        m_yearTiles.append(tile);
        m_yearTilesByYear.insert(summary.year, tile);

        if (summary.coverMonth.isValid()) {
            m_pendingYearCoverRequests.insert(summary.coverMonth, summary.year);
            m_client->loadTimelineBucket(summary.coverMonth);
        }
    }
    layoutYearGrid();
}

void LibraryPage::layoutYearGrid()
{
    const int viewportWidth = m_yearScrollArea->viewport()->width();
    const int availableWidth = qMax(240, viewportWidth - 2 * kSidePad);
    constexpr int kTileGap = 10;
    constexpr int kTileWidth = 220;
    constexpr int kTileHeight = 150;
    const int columns = qMax(1, (availableWidth + kTileGap) / (kTileWidth + kTileGap));
    const int cellWidth = (availableWidth - (columns - 1) * kTileGap) / columns;

    int x = kSidePad;
    int y = kSidePad;
    int col = 0;
    for (SummaryTile *tile : std::as_const(m_yearTiles)) {
        tile->setGeometry(x, y, cellWidth, kTileHeight);
        tile->show();
        ++col;
        if (col >= columns) {
            col = 0;
            x = kSidePad;
            y += kTileHeight + kTileGap;
        } else {
            x += cellWidth + kTileGap;
        }
    }
    if (col != 0)
        y += kTileHeight + kTileGap;
    m_yearGridHost->resize(viewportWidth, qMax(kTileHeight + 2 * kSidePad, y + kSidePad));
}

void LibraryPage::handleYearTileClicked(int year)
{
    m_openYearDetail = year;
    m_yearDetailAssetsByMonth.clear();
    m_pendingYearDetailRequests.clear();
    m_yearDetailExpectedMonths = 0;
    m_yearDetailReceivedMonths = 0;
    clearYearDetail();
    m_contentStack->setCurrentIndex(2);

    if (!m_client->isOnline()) {
        m_yearDetailStatus->setText(
            tr("%1 - you're offline, can't load months.").arg(year));
        return;
    }

    for (const TimeBucketInfo &bucket : std::as_const(m_monthBuckets)) {
        if (bucket.month.year() != year)
            continue;
        ++m_yearDetailExpectedMonths;
        m_pendingYearDetailRequests.insert(bucket.month, year);
        m_client->loadTimelineBucket(bucket.month);
    }
    m_yearDetailStatus->setText(tr("%1 - loading…").arg(year));
}

void LibraryPage::handleYearCoverLoaded(int year, const QList<ImmichAsset> &assets)
{
    if (assets.isEmpty())
        return;
    SummaryTile *tile = m_yearTilesByYear.value(year);
    if (!tile)
        return;
    const QString assetId = assets.first().id;
    m_summaryTilesByAssetId.insert(assetId, tile);
    m_client->loadThumbnail(assetId);
}

void LibraryPage::handleYearDetailBucketLoaded(int year, const QDate &month,
                                               const QList<ImmichAsset> &assets)
{
    if (year != m_openYearDetail)
        return;
    m_yearDetailAssetsByMonth.insert(month, assets);
    ++m_yearDetailReceivedMonths;
    if (m_yearDetailReceivedMonths >= m_yearDetailExpectedMonths)
        buildYearDetailUi();
}

void LibraryPage::clearYearDetail()
{
    for (QWidget *widget : std::as_const(m_yearDetailWidgets)) {
        if (auto *tile = qobject_cast<SummaryTile *>(widget)) {
            for (auto it = m_summaryTilesByAssetId.begin();
                 it != m_summaryTilesByAssetId.end(); ) {
                if (it.value() == tile)
                    it = m_summaryTilesByAssetId.erase(it);
                else
                    ++it;
            }
        }
        widget->deleteLater();
    }
    m_yearDetailWidgets.clear();
}

void LibraryPage::buildYearDetailUi()
{
    clearYearDetail();
    m_yearDetailStatus->setText(QString::number(m_openYearDetail));

    const QLocale locale;
    // Most recent month first, matching the rest of the app's newest-first
    // convention.
    QList<QDate> months = m_yearDetailAssetsByMonth.keys();
    std::sort(months.begin(), months.end(), std::greater<QDate>());

    for (const QDate &month : months) {
        const QList<ImmichAsset> &assets = m_yearDetailAssetsByMonth.value(month);
        if (assets.isEmpty())
            continue;

        QMap<QDate, QList<ImmichAsset>> byDay;
        for (const ImmichAsset &asset : assets) {
            const QDate day = asset.takenAt.isValid() ? asset.takenAt.date() : QDate();
            if (day.isValid())
                byDay[day].append(asset);
        }
        if (byDay.isEmpty())
            continue;

        auto *header = new QLabel(locale.monthName(month.month(), QLocale::LongFormat),
                                  m_yearDetailHost);
        header->setObjectName(QStringLiteral("timelineDayHeader"));
        header->setProperty("section", true);
        m_yearDetailWidgets.append(header);

        QList<QDate> days = byDay.keys();
        std::sort(days.begin(), days.end(), std::greater<QDate>());
        for (const QDate &day : days) {
            const QList<ImmichAsset> &dayAssets = byDay.value(day);
            auto *tile = new SummaryTile(QString::number(day.day()), dayAssets.size(),
                                         m_yearDetailHost);
            tile->setShowCount(false);
            connect(tile, &SummaryTile::clicked, this, [this, day] { jumpToDate(day); });
            m_yearDetailWidgets.append(tile);

            const QString assetId = dayAssets.first().id;
            m_summaryTilesByAssetId.insert(assetId, tile);
            m_client->loadThumbnail(assetId);
        }
    }
    layoutYearDetail();
}

void LibraryPage::layoutYearDetail()
{
    const int viewportWidth = m_yearDetailScrollArea->viewport()->width();
    const int availableWidth = qMax(240, viewportWidth - 2 * kSidePad);
    constexpr int kTileGap = 8;
    constexpr int kTileSize = 110;
    const int columns = qMax(1, (availableWidth + kTileGap) / (kTileSize + kTileGap));

    int y = kSidePad;
    int col = 0;
    bool rowStarted = false;
    for (QWidget *widget : std::as_const(m_yearDetailWidgets)) {
        if (auto *header = qobject_cast<QLabel *>(widget)) {
            if (rowStarted) {
                y += kTileSize + kTileGap;
                rowStarted = false;
            }
            header->setGeometry(kSidePad, y, availableWidth, 26);
            header->show();
            y += 32;
            col = 0;
            continue;
        }
        if (col == columns) {
            col = 0;
            y += kTileSize + kTileGap;
        }
        widget->setGeometry(kSidePad + col * (kTileSize + kTileGap), y, kTileSize, kTileSize);
        widget->show();
        ++col;
        rowStarted = true;
    }
    if (rowStarted)
        y += kTileSize + kTileGap;
    m_yearDetailHost->resize(viewportWidth, y + kSidePad);
}

void LibraryPage::jumpToDate(const QDate &day)
{
    m_contentStack->setCurrentIndex(0);
    if (!day.isValid() || m_monthBuckets.isEmpty())
        return;

    const QDate targetMonth(day.year(), day.month(), 1);
    int index = -1;
    for (int i = 0; i < m_monthBuckets.size(); ++i) {
        if (m_monthBuckets.at(i).month == targetMonth) {
            index = i;
            break;
        }
    }
    if (index < 0)
        return;

    if (index < m_flushedBucketCount) {
        m_pendingScrollToDate = day;
        scrollToPendingDate();
        return;
    }

    // Keep loading in the background; scroll once this month joins the timeline.
    m_pendingScrollToDate = day;
    if (!m_loading) {
        m_loading = true;
        m_refreshButton->setEnabled(false);
    }
    updateTimelineLoadingStatus();
    kickTimelineBucketFetches();
}

void LibraryPage::scrollToPendingDate()
{
    if (!m_pendingScrollToDate.isValid())
        return;
    for (const DaySection &section : std::as_const(m_sections)) {
        if (section.date != m_pendingScrollToDate)
            continue;
        // Compact grid mode hides/ignores per-day headers entirely, so their
        // geometry is meaningless here - use the day's first tile instead,
        // which is always positioned correctly regardless of layout mode.
        if (m_compactGrid) {
            if (!section.tiles.isEmpty())
                m_scrollArea->verticalScrollBar()->setValue(qMax(0, section.tiles.first()->y()));
        } else if (section.header) {
            m_scrollArea->verticalScrollBar()->setValue(qMax(0, section.header->y() - 12));
        }
        m_pendingScrollToDate = QDate();
        return;
    }
    // Day not in the timeline yet - keep the pending target for a later bucket.
}

void LibraryPage::showThumbnail(const QString &assetId, const QPixmap &thumbnail)
{
    if (SummaryTile *raw = m_summaryTilesByAssetId.take(assetId)) {
        const QPointer<SummaryTile> summaryTile(raw);
        if (summaryTile)
            summaryTile->setThumbnail(thumbnail);
        return;
    }

    m_requestedThumbnails.remove(assetId);
    m_thumbnailRetries.remove(assetId);
    const QPointer<MediaTile> tile = m_tilesById.value(assetId);
    if (!tile || m_tilesById.value(assetId) != tile.data())
        return;

    const qreal oldRatio = tile->aspectRatio();
    tile->setThumbnail(thumbnail);
    if (GridZoom::tileImageDimension(tile->size(), tile->devicePixelRatioF()) >
        tile->thumbnailDimension())
        scheduleVisibleMediaUpdate();
    // Compact grid ignores aspect ratio. Masonry only needs a relayout when the
    // ratio actually changes - relayouting on every thumbnail freezes scrolling.
    if (!m_compactGrid && qAbs(tile->aspectRatio() - oldRatio) > 0.03)
        scheduleLayout();
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
    const QPointer<MediaTile> tile = m_tilesById.value(assetId);
    if (!tile || m_tilesById.value(assetId) != tile.data() || !isTileNearViewport(tile))
        return;
    tile->setThumbnailError(message);

    // Immich generates thumbnails after an upload finishes, so a just-uploaded
    // photo briefly has none (404). Retry a few times before giving up.
    static constexpr int kRetryDelaysMs[] = {5000, 20000, 60000};
    const int attempt = m_thumbnailRetries.value(assetId);
    if (attempt >= int(std::size(kRetryDelaysMs)))
        return;
    m_thumbnailRetries.insert(assetId, attempt + 1);
    QTimer::singleShot(kRetryDelaysMs[attempt], this, [this, assetId] {
        if (MediaTile *retryTile = m_tilesById.value(assetId)) {
            retryTile->clearThumbnailError();
            scheduleVisibleMediaUpdate();
        }
    });
}

void LibraryPage::showRequestError(const QString &operation, const QString &message, bool willRetry)
{
    if (operation == tr("Load image"))
        return;
    if (operation == tr("Copy")) {
        m_copyInFlightAssetId.clear();
        m_status->setText(tr("Copy failed: %1").arg(message));
        return;
    }
    if (operation == tr("Upload")) {
        for (const QString &path : QSet<QString>(m_pasteTempFiles)) {
            if (message.contains(QFileInfo(path).fileName()))
                cleanupPasteTemp(path);
        }
        if (!willRetry)
            ++m_uploadsFailed;
        const int remaining = m_client->pendingUploadCount();
        if (remaining == 0 && !willRetry) {
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
            m_status->setText(willRetry
                                  ? tr("%1 (%2 left)").arg(message, QString::number(remaining))
                                  : tr("Upload failed: %1 (%2 left)")
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
    // QVideoWidget is reparented onto the active tile; stop first so tile
    // deletion cannot destroy the shared preview widget out from under us.
    if (m_videoHoverPreview)
        m_videoHoverPreview->stop();

    for (DaySection &section : m_sections) {
        if (section.header)
            section.header->deleteLater();
        for (MediaTile *tile : section.tiles)
            tile->deleteLater();
    }
    m_sections.clear();
    m_tilesById.clear();
    m_localAssetIds.clear();
    m_requestedThumbnails.clear();
    m_assets.clear();
    m_readyBucketAssets.clear();
    m_inflightBuckets.clear();
    m_nextBucketToRequest = 0;
    m_flushedBucketCount = 0;
    m_nextPage.clear();
    m_newestAssetId.clear();
}

void LibraryPage::layoutTimeline()
{
    const int viewportWidth = m_scrollArea->viewport()->width();
    const int availableWidth = qMax(240, viewportWidth - 2 * kSidePad);

    if (m_compactGrid) {
        constexpr int kGridGap = 3;
        const int startY = 8;
        const int columns = qMax(1, (availableWidth + kGridGap) / (m_tileSize + kGridGap));
        const int cellSize = (availableWidth - (columns - 1) * kGridGap) / columns;

        int index = 0;
        for (DaySection &section : m_sections) {
            if (section.header)
                section.header->hide();
            for (MediaTile *tile : section.tiles) {
                const int col = index % columns;
                const int row = index / columns;
                tile->setGeometry(kSidePad + col * (cellSize + kGridGap),
                                  startY + row * (cellSize + kGridGap), cellSize, cellSize);
                ++index;
            }
        }

        const int totalRows = (index + columns - 1) / columns;
        const int reserved = estimateReservedTimelineHeight(remainingIndexedAssetCount());
        m_timelineHost->resize(viewportWidth,
                               startY + totalRows * (cellSize + kGridGap) + reserved + 16);
        scheduleVisibleMediaUpdate();
        QTimer::singleShot(0, this, &LibraryPage::maybeLoadMore);
        return;
    }

    const qreal minRowHeight = GridZoom::minRowHeight(m_tileSize);
    const qreal maxRowHeight = GridZoom::maxRowHeight(m_tileSize);
    int y = 8;

    for (int sectionIndex = 0; sectionIndex < m_sections.size(); ++sectionIndex) {
        DaySection &section = m_sections[sectionIndex];

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
            qreal height = m_tileSize;
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
                (rowAspectSum + ratio) * m_tileSize + kGap * row.size();
            if (!row.isEmpty() && projected > availableWidth)
                flushRow(true);
            row.append(tile);
            rowAspectSum += ratio;
        }
        // Short trailing rows (e.g. a single photo) keep a capped natural size.
        flushRow(false);
        y += 10;
    }

    const int reserved = estimateReservedTimelineHeight(remainingIndexedAssetCount());
    m_timelineHost->resize(viewportWidth, y + reserved + 16);
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
    // Don't evict tile thumbnails just because the Library page isn't the
    // active stack page (Years/Year-detail showing instead) - that's not the
    // same as being scrolled far off-screen, and clearing them here meant
    // returning from Years showed a grid of blanks until something else
    // happened to trigger a reload.
    if (m_contentStack->currentWidget() != m_scrollArea)
        return;

    const int viewportHeight = m_scrollArea->viewport()->height();
    const int scrollTop = m_scrollArea->verticalScrollBar()->value();
    // Keep ~1 viewport of padding loaded; drop everything farther away so
    // long scroll sessions don't retain every decoded thumb in tile widgets.
    const QRect keepRect(0, qMax(0, scrollTop - viewportHeight), m_timelineHost->width(),
                         viewportHeight * 3);

    // Showing a tile for the first time is costly (style polish), so only tiles
    // near the viewport are shown; a 700-photo month would otherwise stall the UI.
    for (MediaTile *tile : std::as_const(m_tilesById)) {
        if (tile)
            tile->setVisible(keepRect.intersects(tile->geometry()));
    }

    constexpr int kMaxThumbnailRequestsPerTick = 16;
    int requested = 0;
    for (auto it = m_tilesById.cbegin(); it != m_tilesById.cend(); ++it) {
        MediaTile *tile = it.value();
        const QString assetId = it.key();
        if (!tile)
            continue;

        if (!keepRect.intersects(tile->geometry())) {
            // Coming back to a failed tile later should try again.
            tile->clearThumbnailError();
            if (tile->hasThumbnail()) {
                tile->clearThumbnail();
                m_requestedThumbnails.remove(assetId);
                m_requestedTileImages.remove(assetId);
            }
            continue;
        }

        if (tile->hasThumbnail()) {
            const int wanted =
                GridZoom::tileImageDimension(tile->size(), tile->devicePixelRatioF());
            if (wanted > tile->thumbnailDimension() &&
                wanted > m_requestedTileImages.value(assetId) &&
                !m_localAssetIds.contains(assetId)) {
                m_requestedTileImages.insert(assetId, wanted);
                m_client->loadTileImage(assetId, wanted);
                if (++requested >= kMaxThumbnailRequestsPerTick) {
                    scheduleVisibleMediaUpdate();
                    break;
                }
            }
            continue;
        }
        if (tile->hasThumbnailError() || m_requestedThumbnails.contains(assetId))
            continue;
        m_requestedThumbnails.insert(assetId);
        if (m_localAssetIds.contains(assetId)) {
            loadLocalThumbnail(tile, tile->asset());
        } else {
            m_client->loadThumbnail(assetId);
        }
        if (++requested >= kMaxThumbnailRequestsPerTick) {
            scheduleVisibleMediaUpdate();
            break;
        }
    }
}

void LibraryPage::updateEmptyState()
{
    // While months are still streaming in, keep the scroll view visible so the
    // reserved height from /timeline/buckets is usable immediately.
    const bool streamingPlaceholder =
        m_assets.isEmpty() && m_loading && !m_searchActive && !m_monthBuckets.isEmpty();
    const bool empty = m_assets.isEmpty() && !streamingPlaceholder;
    // Only touch visibility of the Library page's own widgets when it's
    // actually the active stack page - otherwise this fights with
    // QStackedWidget's page management (e.g. a background new-photos poll
    // can call this while Years/Year-detail is showing) and forces the
    // Library grid visible again, overlapping whichever page is current.
    if (m_contentStack->currentWidget() == m_scrollArea) {
        m_emptyState->setVisible(empty);
        m_scrollArea->setVisible(!empty);
    }
    if (!empty)
        return;
    if (!m_client->isConfigured()) {
        m_emptyState->setText(
            tr("Connect to your Immich server in Settings → Immich Server\n"
               "to browse photos and videos."));
        return;
    }
    if (m_loading && !m_searchActive) {
        m_emptyState->setText(tr("Loading library…"));
        return;
    }
    if (m_searchActive) {
        m_emptyState->setText(
            m_searchQuery.isEmpty() ? tr("No results for these filters.\nTry different filters.")
                : tr("No results for “%1”.\nTry a different search or filters.").arg(m_searchQuery));
        return;
    }
    m_emptyState->setText(
        tr("No photos or videos yet.\n"
           "Drop files here, paste with Ctrl+V, or use Upload to add media."));
}

void LibraryPage::updateAutoCheckTimer()
{
    if (m_client->isConfigured() && isVisible() && !m_searchActive) {
        if (!m_autoCheckTimer->isActive())
            m_autoCheckTimer->start();
    } else {
        m_autoCheckTimer->stop();
    }
}

void LibraryPage::updateEndpointHint()
{
    if (!m_client->isConfigured() || m_loading || m_searchActive)
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

void LibraryPage::handleOnlineChanged(bool online)
{
    if (online) {
        if (m_showingCached || m_client->pendingUploadCount() > 0) {
            m_status->setText(m_client->pendingUploadCount() > 0
                                  ? tr("Back online , resuming uploads…")
                                  : tr("Back online , refreshing…"));
            if (m_showingCached)
                refresh();
        }
    } else if (!m_showingCached && !m_assets.isEmpty()) {
        m_status->setText(tr("%n item(s) · offline", nullptr, m_assets.size()));
    }
}

void LibraryPage::handleUploadQueueChanged(int pendingCount)
{
    if (pendingCount <= 0 || m_client->isOnline())
        return;
    m_status->setText(tr("%n file(s) waiting to upload", nullptr, pendingCount));
}

MediaViewerDialog *LibraryPage::openAsset(const ImmichAsset &asset)
{
    m_currentAsset = asset;
    if (asset.isLocalPending()) {
        if (!asset.localPath.isEmpty())
            QDesktopServices::openUrl(QUrl::fromLocalFile(asset.localPath));
        return nullptr;
    }
    if (asset.isVideo() && m_videoHoverPreview)
        m_videoHoverPreview->stop();

    auto *viewer = new MediaViewerDialog(
        m_themeManager, m_client, asset,
        [this](const QString &assetId, int step) { return adjacentAsset(assetId, step); },
        this);
    connect(viewer, &MediaViewerDialog::copyRequested, this, &LibraryPage::copyAsset);
    connect(viewer, &MediaViewerDialog::downloadRequested, this, &LibraryPage::downloadAsset);
    connect(viewer, &MediaViewerDialog::trashRequested, this, &LibraryPage::trashAsset);
    connect(viewer, &MediaViewerDialog::deleteRequested, this,
            &LibraryPage::deleteAssetPermanently);
    connect(viewer, &MediaViewerDialog::personRequested, this, &LibraryPage::personRequested);
    connect(viewer, &MediaViewerDialog::placeRequested, this, &LibraryPage::placeRequested);
    connect(viewer, &MediaViewerDialog::dateRequested, this, &LibraryPage::dateRequested);
    connect(viewer, &MediaViewerDialog::albumRequested, this, &LibraryPage::albumRequested);
    connect(viewer, &MediaViewerDialog::tagRequested, this, &LibraryPage::tagRequested);
    connect(viewer, &MediaViewerDialog::folderRequested, this, &LibraryPage::folderRequested);
    viewer->show();
    return viewer;
}

ImmichAsset LibraryPage::adjacentAsset(const QString &assetId, int step)
{
    QList<const MediaTile *> tiles;
    for (const DaySection &section : m_sections) {
        for (const MediaTile *tile : section.tiles) {
            if (!tile->asset().isLocalPending())
                tiles.append(tile);
        }
    }

    int index = -1;
    for (int i = 0; i < tiles.size(); ++i) {
        if (tiles.at(i)->asset().id == assetId) {
            index = i;
            break;
        }
    }
    if (index < 0)
        return {};

    const int target = index + step;
    if (target >= tiles.size() - 1 && hasMoreToLoad())
        loadMore();
    if (target < 0 || target >= tiles.size())
        return {};
    return tiles.at(target)->asset();
}

void LibraryPage::downloadAsset(const ImmichAsset &asset)
{
    if (!m_client->isConfigured() || asset.id.isEmpty())
        return;

    if (!asset.fileName.isEmpty()) {
        promptDownload(asset);
        return;
    }

    // Timeline buckets omit originalFileName, so look it up before suggesting a name.
    m_downloadInfoPendingAssetId = asset.id;
    m_status->setText(tr("Preparing download…"));
    m_client->fetchAssetInfo(asset.id);
}

void LibraryPage::handleAssetInfoLoaded(const ImmichAsset &info)
{
    if (info.id.isEmpty())
        return;
    if (info.fileName.isEmpty()) {
        handleAssetInfoFailed(info.id, QString());
        return;
    }

    for (ImmichAsset &asset : m_assets) {
        if (asset.id == info.id) {
            asset.fileName = info.fileName;
            if (MediaTile *tile = m_tilesById.value(asset.id))
                tile->updateAsset(asset);
            break;
        }
    }
    if (m_currentAsset.id == info.id)
        m_currentAsset.fileName = info.fileName;

    if (info.id != m_downloadInfoPendingAssetId)
        return;
    m_downloadInfoPendingAssetId.clear();
    m_status->clear();
    promptDownload(info);
}

void LibraryPage::handleAssetInfoFailed(const QString &assetId, const QString &message)
{
    Q_UNUSED(message);
    if (assetId != m_downloadInfoPendingAssetId)
        return;
    m_downloadInfoPendingAssetId.clear();
    m_status->clear();
    ImmichAsset asset;
    asset.id = assetId;
    promptDownload(asset);
}

void LibraryPage::promptDownload(const ImmichAsset &asset)
{
    const QString suggestedName =
        asset.fileName.isEmpty() ? QStringLiteral("immich-%1").arg(asset.id) : asset.fileName;

    AppSettings settings;
    DownloadSettings download = settings.loadDownload();
    QString directory = download.lastDirectory;
    if (directory.isEmpty() || !QFileInfo(directory).isDir())
        directory = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);

    const QString path = QFileDialog::getSaveFileName(
        this, tr("Download media"), QDir(directory).filePath(suggestedName),
        mediaFileFilter());
    if (path.isEmpty())
        return;

    download.lastDirectory = QFileInfo(path).absolutePath();
    settings.saveDownload(download);

    m_status->setText(tr("Downloading %1…").arg(QFileInfo(path).fileName()));
    m_client->downloadAsset(asset.id, path, suggestedName);
}

void LibraryPage::copyAsset(const ImmichAsset &asset)
{
    if (!m_client->isConfigured() || asset.id.isEmpty()) {
        m_status->setText(tr("Connect to Immich in Settings before copying."));
        return;
    }
    if (asset.isVideo()) {
        m_status->setText(tr("Copy works for photos , download videos instead."));
        return;
    }

    m_currentAsset = asset;
    m_copyInFlightAssetId = asset.id;
    m_status->setText(tr("Copying %1…")
                          .arg(asset.fileName.isEmpty() ? tr("photo") : asset.fileName));
    m_client->fetchAssetOriginal(asset.id);
}

void LibraryPage::pasteFromClipboard()
{
    if (isSearchFieldFocused())
        return;
    if (!m_client->isConfigured()) {
        m_status->setText(tr("Connect to Immich in Settings before uploading."));
        return;
    }

    if (pasteClipboardFiles())
        return;
    if (pasteClipboardImage())
        return;

    m_status->setText(tr("Clipboard has no image or media files to upload."));
}

bool LibraryPage::isSearchFieldFocused() const
{
    QWidget *focus = QApplication::focusWidget();
    while (focus) {
        if (focus == m_searchField || focus == m_searchPanel)
            return true;
        focus = focus->parentWidget();
    }
    return false;
}

ImmichAsset LibraryPage::currentAssetForClipboard() const
{
    if (auto *tile = qobject_cast<MediaTile *>(QApplication::focusWidget()))
        return tile->asset();
    return m_currentAsset;
}

bool LibraryPage::pasteClipboardFiles()
{
    const QMimeData *mime = QApplication::clipboard()->mimeData();
    if (!mime || !mime->hasUrls())
        return false;

    const QStringList paths = uploadableLocalPaths(mime->urls());
    if (paths.isEmpty())
        return false;

    enqueueUploads(paths);
    return true;
}

bool LibraryPage::pasteClipboardImage()
{
    const QMimeData *mime = QApplication::clipboard()->mimeData();
    if (!mime || !mime->hasImage())
        return false;

    const QImage image = qvariant_cast<QImage>(mime->imageData());
    if (image.isNull())
        return false;

    const QString tempDir =
        QStandardPaths::writableLocation(QStandardPaths::TempLocation) +
        QStringLiteral("/immich-desktop-paste");
    QDir().mkpath(tempDir);
    const QString path =
        QStringLiteral("%1/paste-%2.png")
            .arg(tempDir, QString::number(QDateTime::currentMSecsSinceEpoch()));

    if (!image.save(path, "PNG")) {
        m_status->setText(tr("Could not save clipboard image for upload."));
        return true;
    }

    m_pasteTempFiles.insert(path);
    enqueueUploads({path});
    return true;
}

void LibraryPage::cleanupPasteTemp(const QString &path)
{
    if (!m_pasteTempFiles.contains(path))
        return;
    m_pasteTempFiles.remove(path);
    QFile::remove(path);
}

void LibraryPage::handleAssetOriginalFetched(const QString &assetId, const QByteArray &bytes,
                                             const QString &contentType)
{
    if (assetId != m_copyInFlightAssetId)
        return;
    m_copyInFlightAssetId.clear();

    QImage image;
    if (!image.loadFromData(bytes)) {
        m_status->setText(tr("Could not decode image for clipboard."));
        return;
    }

    auto *mime = new QMimeData;
    mime->setImageData(image);
    if (!contentType.isEmpty())
        mime->setData(contentType, bytes);
    // Also expose PNG so more apps accept the paste.
    QByteArray pngBytes;
    QBuffer pngBuffer(&pngBytes);
    pngBuffer.open(QIODevice::WriteOnly);
    image.save(&pngBuffer, "PNG");
    mime->setData(QStringLiteral("image/png"), pngBytes);

    QApplication::clipboard()->setMimeData(mime);
    const QString name = m_currentAsset.id == assetId && !m_currentAsset.fileName.isEmpty()
                             ? m_currentAsset.fileName
                             : tr("photo");
    m_status->setText(tr("Copied %1 to clipboard").arg(name));
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
    box.setAttribute(Qt::WA_StyledBackground, true);
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
    if (!m_client->isOnline()) {
        m_status->setText(
            tr("Queued %n file(s) for upload when online.", nullptr, uploadable.size()));
    } else {
        m_status->setText(tr("Uploading %n file(s)…", nullptr, m_client->pendingUploadCount() +
                                                          uploadable.size()));
    }
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
    cleanupPasteTemp(filePath);
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
                       "font-size: 13.5pt;"
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
    if (watched == m_scrollArea->viewport() && event->type() == QEvent::Wheel) {
        auto *wheel = static_cast<QWheelEvent *>(event);
        if (wheel->modifiers() & Qt::ControlModifier) {
            const int delta = wheel->angleDelta().y();
            if (delta != 0)
                setTileSize(GridZoom::step(m_tileSize, delta > 0 ? 1 : -1));
            return true;
        }
    }
    if (watched == m_searchPanel &&
        (event->type() == QEvent::PaletteChange || event->type() == QEvent::StyleChange))
        QTimer::singleShot(0, this, &LibraryPage::updateSearchOptionsUi);
    if (watched == m_searchField && event->type() == QEvent::MouseButtonPress) {
        m_searchActivated = true;
        updateSearchExpansion();
    }
    if (watched == m_searchField && event->type() == QEvent::FocusIn) {
        const auto reason = static_cast<QFocusEvent *>(event)->reason();
        if (reason == Qt::MouseFocusReason || reason == Qt::TabFocusReason
            || reason == Qt::BacktabFocusReason || reason == Qt::ShortcutFocusReason)
            m_searchActivated = true;
    }
    if (watched == m_searchField &&
        (event->type() == QEvent::FocusIn || event->type() == QEvent::FocusOut))
        QTimer::singleShot(0, this, &LibraryPage::updateSearchExpansion);
    if (watched == m_searchSettingsButton && event->type() == QEvent::PaletteChange)
        m_searchSettingsButton->setIcon(QIcon(renderSvgIcon(QStringLiteral(":/icons/settings.svg"),
            m_searchSettingsButton->palette().color(QPalette::ButtonText), QSize(20, 20))));
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
    updateToolbarDensity();
    layoutTimeline();
    layoutYearGrid();
    layoutYearDetail();
    if (m_dropActive)
        m_dropOverlay->setGeometry(rect());
}

void LibraryPage::hideEvent(QHideEvent *event)
{
    if (m_videoHoverPreview)
        m_videoHoverPreview->stop();
    QWidget::hideEvent(event);
    for (MediaTile *tile : std::as_const(m_tilesById))
        tile->clearThumbnail();
    // Allow thumbs to be requested again on show (clearThumbnail alone left
    // ids stuck in this set and the grid stayed blank).
    m_requestedThumbnails.clear();
    setDropHighlight(false);
    updateAutoCheckTimer();
}

void LibraryPage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    if (std::exchange(m_reloadOnShow, false))
        refresh();
    syncPendingLocalAssets();
    scheduleVisibleMediaUpdate();
    updateAutoCheckTimer();
    QTimer::singleShot(0, this, &LibraryPage::checkForNewPhotos);
}

} // namespace Aurora
