#include "ui/pages/AppearancePage.h"

#include "core/AppSettings.h"
#include "core/ThemeManager.h"
#include "ui/widgets/ColorButton.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
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

} // namespace

AppearancePage::AppearancePage(ThemeManager *themeManager, QWidget *parent)
    : QWidget(parent)
    , m_themeManager(themeManager)
    , m_themeCombo(new QComboBox(this))
    , m_closeToTray(new QCheckBox(tr("Close to system tray"), this))
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
        tr("Keep immich running in the background when you close the window."),
        content, &desktopLayout);
    m_closeToTray->setCursor(Qt::PointingHandCursor);
    const bool trayAvailable = QSystemTrayIcon::isSystemTrayAvailable();
    m_closeToTray->setEnabled(trayAvailable);
    if (!trayAvailable)
        m_closeToTray->setToolTip(tr("System tray is not available on this desktop."));
    desktopLayout->addWidget(m_closeToTray);
    contentRoot->addWidget(desktopCard);

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
    connect(m_themeManager, &ThemeManager::appearanceChanged, this,
            [this] { syncControls(); });
    syncControls();
}

void AppearancePage::saveCloseToTray(bool enabled)
{
    WindowSettings window = AppSettings().loadWindow();
    window.closeToTray = enabled;
    AppSettings().saveWindow(window);
}

void AppearancePage::syncControls()
{
    const QSignalBlocker comboBlocker(m_themeCombo);
    const int index = m_themeCombo->findData(static_cast<int>(m_themeManager->preset()));
    m_themeCombo->setCurrentIndex(index);

    const QSignalBlocker trayBlocker(m_closeToTray);
    m_closeToTray->setChecked(AppSettings().loadWindow().closeToTray);
}

} // namespace Aurora
