#include "ui/pages/AppearancePage.h"

#include "core/AppSettings.h"
#include "core/AutoStart.h"
#include "core/ThemeManager.h"
#include "ui/widgets/ColorButton.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDirIterator>
#include <QFileInfo>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
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

AppearancePage::AppearancePage(ThemeManager *themeManager, QWidget *parent)
    : QWidget(parent)
    , m_themeManager(themeManager)
    , m_themeCombo(new QComboBox(this))
    , m_closeToTray(new QCheckBox(tr("Close to system tray"), this))
    , m_autoStart(new QCheckBox(tr("Start immich when I sign in"), this))
    , m_cacheSizeLabel(new QLabel(this))
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(4, 6, 4, 4);
    root->setSpacing(8);

    auto *heading = new QLabel(QStringLiteral("Appearance"), this);
    heading->setProperty("heading", true);
    auto *subheading = new QLabel(
        QStringLiteral("Choose a preset or build a palette that belongs to your product."),
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

    QVBoxLayout *presetLayout = nullptr;
    auto *presetCard = sectionCard(
        QStringLiteral("Theme preset"),
        QStringLiteral("Preset changes are applied immediately and saved automatically."),
        content, &presetLayout);
    m_themeCombo->addItem(QStringLiteral("Light"), static_cast<int>(ThemePreset::Light));
    m_themeCombo->addItem(QStringLiteral("Dark"), static_cast<int>(ThemePreset::Dark));
    m_themeCombo->addItem(QStringLiteral("Custom"), static_cast<int>(ThemePreset::Custom));
    presetLayout->addWidget(m_themeCombo, 0, Qt::AlignLeft);
    contentRoot->addWidget(presetCard);

    QVBoxLayout *paletteLayout = nullptr;
    auto *paletteCard = sectionCard(
        QStringLiteral("Custom palette"),
        QStringLiteral("Choosing a color automatically activates the Custom theme."),
        content, &paletteLayout);
    auto *colorGrid = new QGridLayout;
    colorGrid->setSpacing(10);
    colorGrid->addWidget(new ColorButton(QStringLiteral("Background"), ColorRole::Background,
                                         themeManager, paletteCard), 0, 0);
    colorGrid->addWidget(new ColorButton(QStringLiteral("Panels"), ColorRole::Panel,
                                         themeManager, paletteCard), 0, 1);
    colorGrid->addWidget(new ColorButton(QStringLiteral("Buttons"), ColorRole::Button,
                                         themeManager, paletteCard), 1, 0);
    colorGrid->addWidget(new ColorButton(QStringLiteral("Accent"), ColorRole::Accent,
                                         themeManager, paletteCard), 1, 1);
    colorGrid->setColumnStretch(0, 1);
    colorGrid->setColumnStretch(1, 1);
    paletteLayout->addLayout(colorGrid);
    auto *reset = new QPushButton(QStringLiteral("Reset custom colors"), paletteCard);
    reset->setCursor(Qt::PointingHandCursor);
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
    if (!trayAvailable)
        m_closeToTray->setToolTip(tr("System tray is not available on this desktop."));
    m_autoStart->setEnabled(AutoStart::isSupported());
    if (!AutoStart::isSupported())
        m_autoStart->setToolTip(tr("Autostart is not supported on this platform."));
    desktopLayout->addWidget(m_closeToTray);
    desktopLayout->addWidget(m_autoStart);
    contentRoot->addWidget(desktopCard);

    QVBoxLayout *cacheLayout = nullptr;
    auto *cacheCard = sectionCard(
        tr("Cache"),
        tr("Local storage used for thumbnails and offline library data."),
        content, &cacheLayout);
    m_cacheSizeLabel->setProperty("subheading", true);
    m_cacheSizeLabel->setWordWrap(true);
    auto *refreshCache = new QPushButton(tr("Refresh"), cacheCard);
    refreshCache->setCursor(Qt::PointingHandCursor);
    cacheLayout->addWidget(m_cacheSizeLabel);
    cacheLayout->addWidget(refreshCache, 0, Qt::AlignLeft);
    contentRoot->addWidget(cacheCard);

    contentRoot->addStretch();

    scroll->setWidget(content);
    root->addWidget(scroll, 1);

    connect(m_themeCombo, &QComboBox::currentIndexChanged, this, [this](int index) {
        m_themeManager->setPreset(
            static_cast<ThemePreset>(m_themeCombo->itemData(index).toInt()));
    });
    connect(reset, &QPushButton::clicked,
            m_themeManager, &ThemeManager::resetCustomPalette);
    connect(m_closeToTray, &QCheckBox::toggled, this, &AppearancePage::saveCloseToTray);
    connect(m_autoStart, &QCheckBox::toggled, this, &AppearancePage::saveAutoStart);
    connect(refreshCache, &QPushButton::clicked, this, &AppearancePage::refreshCacheSize);
    connect(m_themeManager, &ThemeManager::appearanceChanged, this,
            [this] { syncControls(); });
    syncControls();
    refreshCacheSize();
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

void AppearancePage::syncControls()
{
    const QSignalBlocker comboBlocker(m_themeCombo);
    const int index = m_themeCombo->findData(static_cast<int>(m_themeManager->preset()));
    m_themeCombo->setCurrentIndex(index);

    const WindowSettings window = AppSettings().loadWindow();
    const QSignalBlocker trayBlocker(m_closeToTray);
    m_closeToTray->setChecked(window.closeToTray);

    const QSignalBlocker autoStartBlocker(m_autoStart);
    // Prefer the live OS entry so the checkbox matches what will actually run.
    m_autoStart->setChecked(AutoStart::isSupported() ? AutoStart::isEnabled()
                                                     : window.autoStart);
}

} // namespace Aurora
