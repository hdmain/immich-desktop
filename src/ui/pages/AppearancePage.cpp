#include "ui/pages/AppearancePage.h"

#include "core/AppSettings.h"
#include "core/AutoStart.h"
#include "core/ImmichClient.h"
#include "core/Localization.h"
#include "core/ThemeManager.h"
#include "ui/pages/LibraryPage.h"
#include "ui/IconUtils.h"
#include "ui/widgets/ColorButton.h"
#include "ui/widgets/VideoHoverPreview.h"

#include <QCheckBox>
#include "ui/widgets/NoWheelComboBox.h"
#include <QDirIterator>
#include <QFileInfo>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QPainter>
#include <QScrollArea>
#include <QShowEvent>
#include <QSignalBlocker>
#include <QStandardPaths>
#include <QSystemTrayIcon>
#include <QVBoxLayout>

namespace Aurora {
namespace {

QFrame *sectionCard(const QString &title, const QString &description, QWidget *parent,
                    QVBoxLayout **contentLayout)
{
    auto *card = new QFrame(parent);
    card->setProperty("card", true);
    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(20, 18, 20, 20);
    layout->setSpacing(12);

    auto *titleLabel = new QLabel(title, card);
    titleLabel->setProperty("section", true);
    auto *descriptionLabel = new QLabel(description, card);
    descriptionLabel->setProperty("subheading", true);
    descriptionLabel->setWordWrap(true);
    layout->addWidget(titleLabel);
    layout->addWidget(descriptionLabel);
    layout->addSpacing(4);
    *contentLayout = layout;
    return card;
}

qint64 directorySizeBytes(const QString &path)
{
    if (path.isEmpty() || !QFileInfo::exists(path))
        return 0;

    qint64 total = 0;
    QDirIterator it(path, QDir::Files | QDir::Hidden | QDir::System | QDir::NoSymLinks,
                    QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        const qint64 size = it.fileInfo().size();
        if (size > 0)
            total += size;
    }
    return total;
}

QString formatBytes(qint64 bytes)
{
    constexpr double kb = 1024.0;
    constexpr double mb = kb * 1024.0;
    constexpr double gb = mb * 1024.0;
    if (bytes >= static_cast<qint64>(gb))
        return QStringLiteral("%1 GB").arg(bytes / gb, 0, 'f', 2);
    if (bytes >= static_cast<qint64>(mb))
        return QStringLiteral("%1 MB").arg(bytes / mb, 0, 'f', 1);
    if (bytes >= static_cast<qint64>(kb))
        return QStringLiteral("%1 KB").arg(bytes / kb, 0, 'f', 0);
    return QStringLiteral("%1 B").arg(bytes);
}

} // namespace

AppearancePage::AppearancePage(ThemeManager *themeManager, ImmichClient *client, QWidget *parent)
    : QWidget(parent)
    , m_themeManager(themeManager)
    , m_client(client)
    , m_themeCombo(new NoWheelComboBox(this))
    , m_closeToTray(new QCheckBox(tr("Close to system tray"), this))
    , m_autoStart(new QCheckBox(tr("Start immich when I sign in"), this))
    , m_cacheSizeLabel(new QLabel(this))
    , m_prefetchStatus(new QLabel(this))
    , m_prefetchButton(new QPushButton(tr("Download all thumbnails"), this))
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(4, 6, 4, 4);
    root->setSpacing(8);

    auto *heading = new QLabel(tr("Appearance"), this);
    heading->setProperty("heading", true);
    auto *subheading = new QLabel(
        tr("Choose a preset or build a palette that belongs to your product."),
        this);
    subheading->setProperty("subheading", true);
    root->addWidget(heading);
    root->addWidget(subheading);
    root->addSpacing(18);

    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto *content = new QWidget(scroll);
    auto *contentRoot = new QVBoxLayout(content);
    contentRoot->setContentsMargins(0, 0, 8, 0);
    contentRoot->setSpacing(14);

    QVBoxLayout *languageLayout = nullptr;
    auto *languageCard = sectionCard(tr("Language"),
        tr("Language changes apply immediately. Untranslated text appears in English."),
        content, &languageLayout);
    auto *languageCombo = new NoWheelComboBox(languageCard);
    languageCombo->setObjectName(QStringLiteral("languageCombo"));
    languageCombo->setAccessibleName(tr("Language"));
    languageCombo->addItem(tr("System default"), QString());
    for (const auto &language : Localization::availableLanguages()) {
        const QLocale locale(language);
        languageCombo->addItem(locale.nativeLanguageName(), language);
    }
    languageCombo->setCurrentIndex(qMax(0, languageCombo->findData(AppSettings().loadLanguage())));
    languageLayout->addWidget(languageCombo, 0, Qt::AlignLeft);
    contentRoot->addWidget(languageCard);
    connect(languageCombo, &QComboBox::currentIndexChanged, this, [this, languageCombo] {
        const QString language = languageCombo->currentData().toString();
        AppSettings().saveLanguage(language);
        emit languageChangeRequested(language);
    });

    QVBoxLayout *presetLayout = nullptr;
    auto *presetCard = sectionCard(
        tr("Theme preset"),
        tr("Preset changes are applied immediately and saved automatically."),
        content, &presetLayout);
    m_themeCombo->addItem(tr("Light"), static_cast<int>(ThemePreset::Light));
    m_themeCombo->addItem(tr("Dark"), static_cast<int>(ThemePreset::Dark));
    m_themeCombo->addItem(tr("Custom"), static_cast<int>(ThemePreset::Custom));
    presetLayout->addWidget(m_themeCombo, 0, Qt::AlignLeft);
    contentRoot->addWidget(presetCard);

    QVBoxLayout *iconsLayout = nullptr;
    auto *iconsCard = sectionCard(tr("Icons and interface"),
        tr("Choose an icon pack and simplify action buttons. Changes are applied immediately and saved automatically."),
        content, &iconsLayout);
    auto *packLabel = new QLabel(tr("Icon pack"), iconsCard);
    m_iconPackCombo = new NoWheelComboBox(iconsCard);
    m_iconPackCombo->setObjectName(QStringLiteral("iconPackCombo"));
    m_iconPackCombo->setAccessibleName(tr("Icon pack"));
    packLabel->setBuddy(m_iconPackCombo);
    m_iconPackCombo->addItem(tr("Lucide (default)"), QStringLiteral("lucide"));
    m_iconPackCombo->addItem(tr("Tabler"), QStringLiteral("tabler"));
    m_iconPackCombo->addItem(tr("Bootstrap Icons"), QStringLiteral("bootstrap"));
    iconsLayout->addWidget(packLabel);
    iconsLayout->addWidget(m_iconPackCombo, 0, Qt::AlignLeft);
    m_iconPreview = new QLabel(iconsCard);
    m_iconPreview->setAccessibleName(tr("Preview: upload, library, settings"));
    iconsLayout->addWidget(m_iconPreview);
    m_minimalistUi = new QCheckBox(tr("Minimalist UI (icons instead of button labels)"), iconsCard);
    m_minimalistUi->setObjectName(QStringLiteral("minimalistUiToggle"));
    m_minimalistUi->setCursor(Qt::PointingHandCursor);
    iconsLayout->addWidget(m_minimalistUi);
    auto *minimalistHelp = new QLabel(tr("Navigation and action buttons use icons. Hover over an icon to see its action."), iconsCard);
    minimalistHelp->setProperty("subheading", true);
    minimalistHelp->setWordWrap(true);
    iconsLayout->addWidget(minimalistHelp);
    contentRoot->addWidget(iconsCard);

    QVBoxLayout *paletteLayout = nullptr;
    auto *paletteCard = sectionCard(
        tr("Custom palette"),
        tr("Choosing a color automatically activates the Custom theme."),
        content, &paletteLayout);
    auto *colorGrid = new QGridLayout;
    colorGrid->setSpacing(10);
    const QList<QPair<QString, ColorRole>> colors = {
        {tr("Background"), ColorRole::Background}, {tr("Panels"), ColorRole::Panel},
        {tr("Buttons"), ColorRole::Button},        {tr("Borders"), ColorRole::Border},
        {tr("Text"), ColorRole::Text},             {tr("Muted text"), ColorRole::MutedText},
        {tr("Accent"), ColorRole::Accent}};
    for (int i = 0; i < colors.size(); ++i) {
        colorGrid->addWidget(new ColorButton(colors.at(i).first, colors.at(i).second,
                                             themeManager, paletteCard),
                             i / 2, i % 2);
    }
    colorGrid->setColumnStretch(0, 1);
    colorGrid->setColumnStretch(1, 1);
    paletteLayout->addLayout(colorGrid);
    auto *reset = new QPushButton(tr("Reset custom colors"), paletteCard);
    reset->setCursor(Qt::PointingHandCursor);
    configureActionButton(reset, QStringLiteral(":/icons/refresh-cw.svg"));
    paletteLayout->addWidget(reset, 0, Qt::AlignLeft);
    contentRoot->addWidget(paletteCard);

    QVBoxLayout *desktopLayout = nullptr;
    auto *desktopCard = sectionCard(
        tr("Desktop"),
        tr("Control how immich behaves on your desktop and at sign-in."),
        content, &desktopLayout);
    m_closeToTray->setCursor(Qt::PointingHandCursor);
    m_autoStart->setCursor(Qt::PointingHandCursor);
    const bool trayAvailable = QSystemTrayIcon::isSystemTrayAvailable();
    m_closeToTray->setEnabled(trayAvailable);
    m_closeToTray->setToolTip(
        trayAvailable
            ? tr("Closing the window keeps immich running in the tray. Press Ctrl+Q to quit.")
            : tr("System tray is not available on this desktop."));
    m_autoStart->setEnabled(AutoStart::isSupported());
    if (!AutoStart::isSupported())
        m_autoStart->setToolTip(tr("Autostart is not supported on this platform."));
    desktopLayout->addWidget(m_closeToTray);
    desktopLayout->addWidget(m_autoStart);
    contentRoot->addWidget(desktopCard);

    QVBoxLayout *playbackLayout = nullptr;
    auto *playbackCard = sectionCard(
        tr("Playback"),
        tr("Control preview and playback behavior."),
        content, &playbackLayout);
    m_hoverPreview = new QCheckBox(tr("Enable video hover preview"), playbackCard);
    m_hoverPreview->setCursor(Qt::PointingHandCursor);
    auto *hoverHelp = new QLabel(
        tr("Plays a short muted clip when you hover a video tile. "
           "Disable this if hovering causes freezes or stutter."),
        playbackCard);
    hoverHelp->setProperty("subheading", true);
    hoverHelp->setWordWrap(true);
    auto *snapHelp = new QLabel(playbackCard);
    snapHelp->setProperty("subheading", true);
    snapHelp->setWordWrap(true);
    snapHelp->setTextFormat(Qt::RichText);
    snapHelp->setOpenExternalLinks(true);
    snapHelp->setText(
        tr("Note: the Snap build runs confined, so hover preview is disabled in the Snap. "
           "If video playback doesn't work from the Snap, install the GitHub release "
           "(<a href=\"https://github.com/hdmain/immich-desktop/releases\">.deb / AppImage</a>) , "
           "it uses your system's GStreamer/FFmpeg without confinement."));
    playbackLayout->addWidget(m_hoverPreview);
    playbackLayout->addWidget(hoverHelp);
    playbackLayout->addWidget(snapHelp);
    if (qEnvironmentVariableIsEmpty("SNAP") == false) {
        m_hoverPreview->setEnabled(false);
        m_hoverPreview->setToolTip(tr("Hover preview is unavailable in the Snap build."));
    }
    contentRoot->addWidget(playbackCard);

    QVBoxLayout *timelineLayout = nullptr;
    auto *timelineCard = sectionCard(
        tr("Timeline"),
        tr("Choose how your library is laid out."),
        content, &timelineLayout);
    m_compactGrid = new QCheckBox(tr("Compact grid (no date headers)"), timelineCard);
    m_compactGrid->setCursor(Qt::PointingHandCursor);
    auto *compactGridHelp = new QLabel(
        tr("Uniform square thumbnails packed edge to edge, like a phone's photo grid, "
           "instead of date-grouped rows of varying sizes."),
        timelineCard);
    compactGridHelp->setProperty("subheading", true);
    compactGridHelp->setWordWrap(true);
    timelineLayout->addWidget(m_compactGrid);
    timelineLayout->addWidget(compactGridHelp);
    m_rememberViewed = new QCheckBox(tr("Remember photos I open"), timelineCard);
    m_rememberViewed->setCursor(Qt::PointingHandCursor);
    auto *rememberViewedHelp = new QLabel(
        tr("Photos open for a couple of seconds are listed under Recently viewed in Explore, "
           "can sort grids by last viewed, and count toward how often you view them. Kept "
           "on this computer only. Turning it off pauses this and keeps what's stored."),
        timelineCard);
    rememberViewedHelp->setProperty("subheading", true);
    rememberViewedHelp->setWordWrap(true);
    auto *clearViewed = new QPushButton(tr("Clear viewing history"), timelineCard);
    clearViewed->setCursor(Qt::PointingHandCursor);
    configureActionButton(clearViewed, QStringLiteral(":/icons/circle-x.svg"));
    connect(clearViewed, &QPushButton::clicked, this, [this] {
        if (QMessageBox::question(this, tr("Clear viewing history"),
                                  tr("Forget which photos you opened and how often? This can't "
                                     "be undone.")) == QMessageBox::Yes)
            emit clearViewedRequested();
    });
    timelineLayout->addWidget(m_rememberViewed);
    timelineLayout->addWidget(rememberViewedHelp);
    timelineLayout->addWidget(clearViewed, 0, Qt::AlignLeft);
    contentRoot->addWidget(timelineCard);

    QVBoxLayout *cacheLayout = nullptr;
    auto *cacheCard = sectionCard(
        tr("Cache"),
        tr("Local storage used for thumbnails and offline library data."),
        content, &cacheLayout);
    m_cacheSizeLabel->setProperty("subheading", true);
    m_cacheSizeLabel->setWordWrap(true);
    auto *refreshCache = new QPushButton(tr("Refresh"), cacheCard);
    refreshCache->setCursor(Qt::PointingHandCursor);
    configureActionButton(refreshCache, QStringLiteral(":/icons/refresh-cw.svg"));
    m_prefetchButton->setCursor(Qt::PointingHandCursor);
    configureActionButton(m_prefetchButton, QStringLiteral(":/icons/download.svg"));
    m_prefetchButton->setToolTip(
        tr("Fetch every library thumbnail into the local cache for faster browsing offline."));
    m_prefetchStatus->setProperty("subheading", true);
    m_prefetchStatus->setWordWrap(true);
    m_prefetchStatus->hide();
    auto *cacheButtons = new QHBoxLayout;
    cacheButtons->setSpacing(8);
    cacheButtons->addWidget(refreshCache, 0, Qt::AlignLeft);
    cacheButtons->addWidget(m_prefetchButton, 0, Qt::AlignLeft);
    cacheButtons->addStretch();
    cacheLayout->addWidget(m_cacheSizeLabel);
    cacheLayout->addLayout(cacheButtons);
    cacheLayout->addWidget(m_prefetchStatus);
    contentRoot->addWidget(cacheCard);

    contentRoot->addStretch();

    scroll->setWidget(content);
    root->addWidget(scroll, 1);

    connect(m_themeCombo, &QComboBox::currentIndexChanged, this, [this](int index) {
        m_themeManager->setPreset(
            static_cast<ThemePreset>(m_themeCombo->itemData(index).toInt()));
    });
    connect(m_iconPackCombo, &QComboBox::currentIndexChanged, this, [this](int index) {
        m_themeManager->setIconPack(m_iconPackCombo->itemData(index).toString());
    });
    connect(m_minimalistUi, &QCheckBox::toggled, m_themeManager, &ThemeManager::setMinimalistUi);
    connect(reset, &QPushButton::clicked,
            m_themeManager, &ThemeManager::resetCustomPalette);
    connect(m_closeToTray, &QCheckBox::toggled, this, &AppearancePage::saveCloseToTray);
    connect(m_autoStart, &QCheckBox::toggled, this, &AppearancePage::saveAutoStart);
    connect(m_hoverPreview, &QCheckBox::toggled, this, &AppearancePage::saveHoverPreview);
    connect(m_compactGrid, &QCheckBox::toggled, this, &AppearancePage::saveCompactGrid);
    connect(m_rememberViewed, &QCheckBox::toggled, this, &AppearancePage::rememberViewedChanged);
    connect(refreshCache, &QPushButton::clicked, this, &AppearancePage::refreshCacheSize);
    connect(m_prefetchButton, &QPushButton::clicked, this, &AppearancePage::startThumbnailPrefetch);
    if (m_client) {
        connect(m_client, &ImmichClient::thumbnailPrefetchProgress, this,
                [this](int done, int total, int downloading) {
                    m_prefetchStatus->setVisible(true);
                    m_prefetchStatus->setText(
                        tr("Downloading thumbnails… %1 / %2 (%3 in flight)")
                            .arg(done)
                            .arg(total)
                            .arg(downloading));
                });
        connect(m_client, &ImmichClient::thumbnailPrefetchFinished, this,
                [this](int downloaded, int alreadyCached) {
                    m_prefetchButton->setEnabled(true);
                    m_prefetchButton->setText(tr("Download all thumbnails"));
                    m_prefetchStatus->setVisible(true);
                    m_prefetchStatus->setText(
                        tr("Finished: %n new thumbnail(s) downloaded, %1 already cached.",
                           nullptr, downloaded)
                            .arg(alreadyCached));
                    refreshCacheSize();
                });
        connect(m_client, &ImmichClient::configurationChanged, this, [this](bool configured) {
            m_prefetchButton->setEnabled(configured && !m_client->isPrefetchingThumbnails());
        });
        m_prefetchButton->setEnabled(m_client->isConfigured());
    } else {
        m_prefetchButton->setEnabled(false);
    }
    connect(m_themeManager, &ThemeManager::appearanceChanged, this,
            [this] { syncControls(); });
    syncControls();
    refreshCacheSize();
}

void AppearancePage::startThumbnailPrefetch()
{
    if (!m_client || m_client->isPrefetchingThumbnails())
        return;
    m_prefetchButton->setEnabled(false);
    m_prefetchButton->setText(tr("Downloading…"));
    m_prefetchStatus->setVisible(true);
    m_prefetchStatus->setText(tr("Listing library…"));
    m_client->prefetchAllThumbnails();
}

void AppearancePage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    refreshCacheSize();
}

void AppearancePage::refreshCacheSize()
{
    const QString cacheRoot =
        QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    const qint64 bytes = directorySizeBytes(cacheRoot);
    m_cacheSizeLabel->setText(tr("Cache size: %1").arg(formatBytes(bytes)));
    m_cacheSizeLabel->setToolTip(cacheRoot);
}

void AppearancePage::saveCloseToTray(bool enabled)
{
    WindowSettings window = AppSettings().loadWindow();
    window.closeToTray = enabled;
    AppSettings().saveWindow(window);
}

void AppearancePage::saveAutoStart(bool enabled)
{
    if (!AutoStart::setEnabled(enabled)) {
        QMessageBox box(this);
        box.setAttribute(Qt::WA_StyledBackground, true);
        box.setIcon(QMessageBox::Warning);
        box.setWindowTitle(tr("Autostart"));
        box.setText(AutoStart::lastError().isEmpty()
                        ? tr("Could not update autostart settings.")
                        : AutoStart::lastError());
        box.exec();
        const QSignalBlocker blocker(m_autoStart);
        m_autoStart->setChecked(AutoStart::isEnabled());
        return;
    }

    WindowSettings window = AppSettings().loadWindow();
    window.autoStart = enabled;
    AppSettings().saveWindow(window);
}

void AppearancePage::saveHoverPreview(bool enabled)
{
    PlaybackSettings s = AppSettings().loadPlayback();
    s.hoverPreviewEnabled = enabled;
    AppSettings().savePlayback(s);
    if (auto *w = window()) {
        for (auto *p : w->findChildren<VideoHoverPreview *>()) {
            p->setEnabled(enabled && qEnvironmentVariableIsEmpty("SNAP"));
        }
    }
}

void AppearancePage::syncControls()
{
    const QSignalBlocker comboBlocker(m_themeCombo);
    const int index = m_themeCombo->findData(static_cast<int>(m_themeManager->preset()));
    m_themeCombo->setCurrentIndex(index);
    const QSignalBlocker iconsBlocker(m_iconPackCombo);
    const QSignalBlocker minimalBlocker(m_minimalistUi);
    m_iconPackCombo->setCurrentIndex(m_iconPackCombo->findData(m_themeManager->iconPack()));
    m_minimalistUi->setChecked(m_themeManager->minimalistUi());
    const qreal scale = devicePixelRatioF();
    QPixmap preview(QSize(120, 28) * scale);
    preview.setDevicePixelRatio(scale);
    preview.fill(Qt::transparent);
    QPainter painter(&preview);
    int x = 0;
    for (const auto &name : {QStringLiteral("upload"), QStringLiteral("images"), QStringLiteral("settings")}) {
        painter.drawPixmap(x, 2, renderSvgIcon(QStringLiteral(":/icons/%1.svg").arg(name),
            m_themeManager->palette().text, QSize(24, 24), scale));
        x += 40;
    }
    painter.end();
    m_iconPreview->setPixmap(preview);

    const WindowSettings window = AppSettings().loadWindow();
    const QSignalBlocker trayBlocker(m_closeToTray);
    m_closeToTray->setChecked(window.closeToTray);

    const QSignalBlocker autoStartBlocker(m_autoStart);
    // Prefer the live OS entry so the checkbox matches what will actually run.
    m_autoStart->setChecked(AutoStart::isSupported() ? AutoStart::isEnabled()
                                                     : window.autoStart);

    if (m_hoverPreview) {
        const QSignalBlocker hb(m_hoverPreview);
        const bool on = AppSettings().loadPlayback().hoverPreviewEnabled;
        m_hoverPreview->setChecked(on && qEnvironmentVariableIsEmpty("SNAP"));
    }

    const QSignalBlocker compactGridBlocker(m_compactGrid);
    m_compactGrid->setChecked(AppSettings().loadTimeline().compactGrid);
    const QSignalBlocker rememberViewedBlocker(m_rememberViewed);
    m_rememberViewed->setChecked(AppSettings().loadTimeline().rememberViewed);
}

void AppearancePage::saveCompactGrid(bool enabled)
{
    TimelineSettings s = AppSettings().loadTimeline();
    s.compactGrid = enabled;
    AppSettings().saveTimeline(s);
    if (auto *w = window()) {
        for (auto *page : w->findChildren<LibraryPage *>())
            page->setCompactGrid(enabled);
    }
}

} // namespace Aurora
