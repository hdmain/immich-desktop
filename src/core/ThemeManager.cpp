#include "core/ThemeManager.h"
#include "ui/IconUtils.h"

#include <QApplication>

namespace Aurora {

ThemeManager::ThemeManager(QObject *parent)
    : QObject(parent)
{
}

ThemePreset ThemeManager::preset() const { return m_settings.preset; }
const ThemePalette &ThemeManager::palette() const { return m_activePalette; }
const ThemePalette &ThemeManager::customPalette() const { return m_settings.customPalette; }

void ThemeManager::initialize()
{
    m_settings = m_store.loadAppearance();
    refresh(false);
}

void ThemeManager::setPreset(ThemePreset preset)
{
    if (m_settings.preset == preset)
        return;
    m_settings.preset = preset;
    refresh();
}

void ThemeManager::setIconPack(const QString &pack)
{
    if ((pack != QStringLiteral("lucide") && pack != QStringLiteral("tabler")
         && pack != QStringLiteral("bootstrap")) || pack == m_settings.iconPack)
        return;
    m_settings.iconPack = pack;
    refresh();
}

void ThemeManager::setMinimalistUi(bool enabled)
{
    if (m_settings.minimalistUi == enabled)
        return;
    m_settings.minimalistUi = enabled;
    refresh();
}

void ThemeManager::setCustomColor(ColorRole role, const QColor &color)
{
    if (!color.isValid())
        return;

    switch (role) {
    case ColorRole::Background: m_settings.customPalette.background = color; break;
    case ColorRole::Panel: m_settings.customPalette.panel = color; break;
    case ColorRole::Button: m_settings.customPalette.button = color; break;
    case ColorRole::Accent: m_settings.customPalette.accent = color; break;
    case ColorRole::Text: m_settings.customPalette.text = color; break;
    case ColorRole::MutedText: m_settings.customPalette.mutedText = color; break;
    case ColorRole::Border: m_settings.customPalette.border = color; break;
    }
    m_settings.preset = ThemePreset::Custom;
    refresh();
}

void ThemeManager::resetCustomPalette()
{
    m_settings.customPalette = ThemePalette::customDefault();
    m_settings.preset = ThemePreset::Custom;
    refresh();
}

void ThemeManager::refresh(bool persist)
{
    switch (m_settings.preset) {
    case ThemePreset::Light: m_activePalette = ThemePalette::light(); break;
    case ThemePreset::Dark: m_activePalette = ThemePalette::dark(); break;
    case ThemePreset::Custom: m_activePalette = m_settings.customPalette; break;
    }

    if (qApp)
        qApp->setStyleSheet(buildStyleSheet());
    setIconAppearance(m_settings.iconPack, m_settings.minimalistUi);
    if (persist)
        m_store.saveAppearance(m_settings);
    emit appearanceChanged();
}

QString ThemeManager::buildStyleSheet() const
{
    const auto &p = m_activePalette;
    const QString hover = p.accent.lighter(112).name();

    return QStringLiteral(R"(
        * {
            font-family: "Inter", "Segoe UI", sans-serif;
            font-size: 10.5pt;
            color: %1;
            outline: none;
        }
        QMainWindow, QWidget#windowSurface, QDialog, QMessageBox,
        QFileDialog, QColorDialog, QErrorMessage {
            background: %2;
        }
        QWidget#sidebar, QWidget#titleBar, QFrame[card="true"] {
            background: %3;
            border: 1px solid %4;
            border-radius: 16px;
        }
        QLabel[mediaPreview="true"] {
            background: %2;
            border: 1px solid %4;
            border-radius: 11px;
            color: %5;
        }
        QWidget#libraryPage, QWidget#timelineHost, QScrollArea#libraryScroll {
            background: %2;
        }
        QWidget#libraryToolbar { background: transparent; }
        QFrame#searchOptionsCard {
            background: %3;
            border: 1px solid %4;
            border-radius: 16px;
        }
        QLabel[searchCaption="true"] { color: %5; font-size: 9.5pt; font-weight: 600; }
        QLabel#searchFilterSummary {
            color: %5;
            background: %6;
            border-radius: 8px;
            padding: 4px 8px;
            font-size: 9pt;
        }
        QFrame#searchOptionsDivider { background: %4; border: none; }
        QFrame#searchOptionsCard QComboBox, QFrame#searchOptionsCard QLineEdit,
        QFrame#searchOptionsCard QDateEdit { min-width: 0; }
        QPushButton#resetSearchFilters { padding: 6px 10px; font-size: 9pt; }
        QPushButton#searchSettingsButton:checked { background: %7; border-color: %7; }
        QToolButton#searchModeOption {
            background: %6;
            color: %1;
            border: 1px solid %4;
            border-radius: 10px;
            padding: 9px 10px;
            font-weight: 500;
        }
        QToolButton#searchModeOption:hover { border-color: %7; }
        QToolButton#searchModeOption:checked {
            background: %7;
            color: white;
            border-color: %7;
        }
        QLabel#timelineDayHeader {
            color: %1;
            font-size: 11.25pt;
            font-weight: 600;
            background: transparent;
            border: none;
        }
        QLabel[heading="true"] { font-size: 19.5pt; font-weight: 700; }
        QLabel[subheading="true"] { color: %5; font-size: 9.75pt; }
        QLabel#sidebarGroupLabel {
            color: %5;
            font-size: 8.5pt;
            font-weight: 600;
            letter-spacing: 0.06em;
            text-transform: uppercase;
        }
        QLabel[section="true"] { font-size: 12pt; font-weight: 600; }
        QPushButton {
            background: %6;
            border: 1px solid %4;
            border-radius: 10px;
            padding: 9px 14px;
            font-weight: 600;
        }
        QPushButton:hover { border-color: %7; }
        QPushButton[iconOnly="true"] { padding: 9px; min-width: 20px; }
        QPushButton:pressed { background: %7; color: white; }
        QPushButton[primary="true"] { background: %7; color: white; border-color: %7; }
        QPushButton[windowControl="true"] {
            background: transparent;
            border: none;
            border-radius: 8px;
            padding: 0;
            font-size: 12.75pt;
        }
        QPushButton[windowControl="true"]:hover { background: %6; }
        QPushButton[zoomButton="true"] {
            background: transparent;
            border: none;
            border-radius: 8px;
            padding: 6px;
            min-width: 0;
        }
        QPushButton[zoomButton="true"]:hover { background: %6; }
        QPushButton#closeButton:hover { background: #E5484D; color: white; }
        QPushButton#videoOverlayButton {
            background: rgba(0, 0, 0, 140);
            border: none;
            border-radius: 36px;
        }
        QPushButton#videoOverlayButton:hover { background: rgba(0, 0, 0, 190); }
        QPushButton[linkButton="true"] {
            background: transparent;
            border: none;
            padding: 0;
            color: %7;
            text-align: left;
        }
        QPushButton[linkButton="true"]:hover { text-decoration: underline; }
        QLabel[linkLabel="true"] { color: %7; }
        QPushButton[personButton="true"] {
            background: transparent;
            border: none;
            border-radius: 10px;
            padding: 0;
        }
        QPushButton[personButton="true"]:hover { background: %6; }
        QLabel#personAvatar {
            background: %6;
            border-radius: 24px;
            font-weight: 600;
        }
        QLabel#mapPlaceholder {
            border: 1px dashed %4;
            border-radius: 10px;
        }
        QFrame#assetInfoPanel { border-left: 1px solid %4; }
        QFrame#toast { background: %6; border: 1px solid #E5484D; border-radius: 10px; }
        QFrame#toast QLabel { font-weight: 600; }
        QTreeWidget#browseTree {
            background: transparent;
            border: none;
            border-left: 1px solid %4;
            margin-left: 18px;
            padding-left: 8px;
            outline: 0;
        }
        QTreeWidget#browseTree::item {
            padding: 5px 4px;
            border-radius: 8px;
        }
        QTreeWidget#browseTree::item:hover { background: %6; }
        QTreeWidget#browseTree::item:selected { background: %7; color: white; }
        QTreeWidget#browseTree::branch { background: transparent; border-image: none; image: none; }
        QDialogButtonBox QPushButton { min-width: 88px; }
        QComboBox, QLineEdit, QSpinBox, QDoubleSpinBox, QDateEdit {
            background: %6;
            border: 1px solid %4;
            border-radius: 10px;
            padding: 9px 12px;
            min-width: 150px;
        }
        QComboBox:hover, QComboBox:focus, QLineEdit:hover, QLineEdit:focus,
        QSpinBox:hover, QSpinBox:focus, QDateEdit:hover, QDateEdit:focus {
            border-color: %7;
        }
        QComboBox, QDateEdit { padding-right: 30px; }
        QComboBox::drop-down, QDateEdit::drop-down {
            subcontrol-origin: padding;
            subcontrol-position: top right;
            background: transparent;
            border: none;
            width: 28px;
        }
        QComboBox::down-arrow, QDateEdit::down-arrow {
            image: url(%8);
            width: 14px;
            height: 14px;
        }
        QComboBox QAbstractItemView {
            background: %3;
            border: 1px solid %4;
            selection-background-color: %7;
            padding: 5px;
        }
        QCheckBox { spacing: 10px; }
        QCheckBox::indicator {
            width: 38px; height: 20px;
            border-radius: 10px;
            background: %6;
            border: 1px solid %4;
        }
        QCheckBox::indicator:checked { background: %7; border-color: %7; }
        QProgressBar {
            background: %6;
            border: 1px solid %4;
            border-radius: 8px;
            text-align: center;
            min-height: 18px;
        }
        QProgressBar::chunk {
            background: %7;
            border-radius: 7px;
        }
        QTextEdit, QPlainTextEdit {
            background: %6;
            border: 1px solid %4;
            border-radius: 12px;
            padding: 10px;
            selection-background-color: %7;
        }
        QScrollArea { border: none; background: transparent; }
        QScrollArea > QWidget > QWidget { background: transparent; }
        QScrollBar:vertical {
            background: transparent; width: 10px; margin: 2px;
        }
        QScrollBar::handle:vertical {
            background: %6; border-radius: 4px; min-height: 28px;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0;
        }
        QScrollBar:horizontal {
            background: transparent; height: 10px; margin: 2px;
        }
        QScrollBar::handle:horizontal {
            background: %6; border-radius: 4px; min-width: 28px;
        }
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
            width: 0;
        }
        QMenu {
            background: %3;
            border: 1px solid %4;
            border-radius: 10px;
            padding: 6px;
        }
        QMenu::item {
            padding: 8px 18px;
            border-radius: 6px;
        }
        QMenu::item:selected {
            background: %7;
            color: white;
        }
        QMenu::separator {
            height: 1px;
            background: %4;
            margin: 4px 8px;
        }
        QToolTip {
            background: %3; color: %1; border: 1px solid %4;
            border-radius: 6px; padding: 6px;
        }
        QSlider:horizontal { min-height: 20px; }
        QSlider::groove:horizontal {
            height: 6px;
            background: %6;
            border: 1px solid %4;
            border-radius: 3px;
        }
        QSlider::handle:horizontal {
            width: 16px;
            height: 16px;
            margin: -5px 0;
            background: %7;
            border-radius: 8px;
        }
        QSlider::sub-page:horizontal {
            background: %7;
            border-radius: 3px;
        }
        QAbstractItemView {
            background: %6;
            border: 1px solid %4;
            border-radius: 8px;
            selection-background-color: %7;
            selection-color: white;
            outline: none;
        }
        QHeaderView::section {
            background: %3;
            color: %1;
            border: none;
            border-right: 1px solid %4;
            border-bottom: 1px solid %4;
            padding: 8px 10px;
            font-weight: 600;
        }
        QMessageBox QLabel {
            color: %1;
            background: transparent;
        }
        QColorDialog QWidget {
            background: %2;
        }
    )")
        .arg(p.text.name(), p.background.name(), p.panel.name(), p.border.name(),
             p.mutedText.name(), p.button.name(), hover,
             isDarkPalette(p) ? QStringLiteral(":/ui/chevron-down-dark.svg")
                              : QStringLiteral(":/ui/chevron-down-light.svg"));
}

} // namespace Aurora
