#include "core/AppSettings.h"

namespace Aurora {
namespace {

QColor readColor(const QSettings &settings, const QString &key, const QColor &fallback)
{
    const QColor color(settings.value(key, fallback.name(QColor::HexArgb)).toString());
    return color.isValid() ? color : fallback;
}

} // namespace

AppSettings::AppSettings()
    : m_settings(QSettings::IniFormat, QSettings::UserScope,
                 QStringLiteral("Immich"), QStringLiteral("immich"))
{
}

AppearanceSettings AppSettings::loadAppearance() const
{
    AppearanceSettings result;
    result.preset = themePresetFromKey(
        m_settings.value(QStringLiteral("appearance/theme"), QStringLiteral("dark")).toString());

    const auto defaults = ThemePalette::customDefault();
    m_settings.beginGroup(QStringLiteral("appearance/customPalette"));
    result.customPalette.background = readColor(m_settings, QStringLiteral("background"), defaults.background);
    result.customPalette.panel = readColor(m_settings, QStringLiteral("panel"), defaults.panel);
    result.customPalette.button = readColor(m_settings, QStringLiteral("button"), defaults.button);
    result.customPalette.accent = readColor(m_settings, QStringLiteral("accent"), defaults.accent);
    result.customPalette.text = readColor(m_settings, QStringLiteral("text"), defaults.text);
    result.customPalette.mutedText = readColor(m_settings, QStringLiteral("mutedText"), defaults.mutedText);
    result.customPalette.border = readColor(m_settings, QStringLiteral("border"), defaults.border);
    m_settings.endGroup();
    return result;
}

void AppSettings::saveAppearance(const AppearanceSettings &appearance)
{
    m_settings.setValue(QStringLiteral("appearance/theme"), themePresetKey(appearance.preset));

    m_settings.beginGroup(QStringLiteral("appearance/customPalette"));
    m_settings.setValue(QStringLiteral("background"), appearance.customPalette.background.name(QColor::HexArgb));
    m_settings.setValue(QStringLiteral("panel"), appearance.customPalette.panel.name(QColor::HexArgb));
    m_settings.setValue(QStringLiteral("button"), appearance.customPalette.button.name(QColor::HexArgb));
    m_settings.setValue(QStringLiteral("accent"), appearance.customPalette.accent.name(QColor::HexArgb));
    m_settings.setValue(QStringLiteral("text"), appearance.customPalette.text.name(QColor::HexArgb));
    m_settings.setValue(QStringLiteral("mutedText"), appearance.customPalette.mutedText.name(QColor::HexArgb));
    m_settings.setValue(QStringLiteral("border"), appearance.customPalette.border.name(QColor::HexArgb));
    m_settings.endGroup();
    m_settings.sync();
}

} // namespace Aurora
