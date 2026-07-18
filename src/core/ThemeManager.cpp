#include "core/ThemeManager.h"

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

void ThemeManager::setCustomColor(ColorRole role, const QColor &color)
{
    if (!color.isValid())
        return;

    switch (role) {
    case ColorRole::Background: m_settings.customPalette.background = color; break;
    case ColorRole::Panel: m_settings.customPalette.panel = color; break;
    case ColorRole::Button: m_settings.customPalette.button = color; break;
    case ColorRole::Accent: m_settings.customPalette.accent = color; break;
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
            font-family: "Segoe UI Variable", "Segoe UI", sans-serif;
            font-size: 14px;
            color: %1;
            outline: none;
        }
        QMainWindow, QWidget#windowSurface { background: %2; }
        QWidget#sidebar, QWidget#titleBar, QFrame[card="true"] {
            background: %3;
            border: 1px solid %4;
            border-radius: 16px;
        }
        QLabel[heading="true"] { font-size: 26px; font-weight: 700; }
        QLabel[subheading="true"] { color: %5; font-size: 13px; }
        QLabel[section="true"] { font-size: 16px; font-weight: 600; }
        QPushButton {
            background: %6;
            border: 1px solid %4;
            border-radius: 10px;
            padding: 9px 14px;
            font-weight: 600;
        }
        QPushButton:hover { border-color: %7; }
        QPushButton:pressed { background: %7; color: white; }
        QPushButton[primary="true"] { background: %7; color: white; border-color: %7; }
        QPushButton[windowControl="true"] {
            background: transparent;
            border: none;
            border-radius: 8px;
            padding: 0;
            font-size: 17px;
        }
        QPushButton[windowControl="true"]:hover { background: %6; }
        QPushButton#closeButton:hover { background: #E5484D; color: white; }
        QComboBox {
            background: %6;
            border: 1px solid %4;
            border-radius: 10px;
            padding: 9px 12px;
            min-width: 150px;
        }
        QComboBox:hover, QComboBox:focus { border-color: %7; }
        QComboBox::drop-down { border: none; width: 28px; }
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
        QScrollArea { border: none; background: transparent; }
        QScrollArea > QWidget > QWidget { background: transparent; }
        QToolTip {
            background: %3; color: %1; border: 1px solid %4;
            border-radius: 6px; padding: 6px;
        }
    )")
        .arg(p.text.name(), p.background.name(), p.panel.name(), p.border.name(),
             p.mutedText.name(), p.button.name(), hover);
}

} // namespace Aurora
