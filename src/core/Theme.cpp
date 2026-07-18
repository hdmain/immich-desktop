#include "core/Theme.h"

namespace Aurora {

ThemePalette ThemePalette::light()
{
    return {
        QColor("#F4F6FA"), QColor("#FFFFFF"), QColor("#EEF1F6"),
        QColor("#6366F1"), QColor("#172033"), QColor("#6B7280"),
        QColor("#DDE2EA")
    };
}

ThemePalette ThemePalette::dark()
{
    return {
        QColor("#0C1018"), QColor("#151B26"), QColor("#202838"),
        QColor("#818CF8"), QColor("#F5F7FB"), QColor("#98A2B3"),
        QColor("#293244")
    };
}

ThemePalette ThemePalette::customDefault()
{
    return {
        QColor("#10131D"), QColor("#191E2C"), QColor("#252C3E"),
        QColor("#2DD4BF"), QColor("#F4F7FA"), QColor("#9AA4B2"),
        QColor("#30394D")
    };
}

QString themePresetKey(ThemePreset preset)
{
    switch (preset) {
    case ThemePreset::Light: return QStringLiteral("light");
    case ThemePreset::Custom: return QStringLiteral("custom");
    case ThemePreset::Dark: return QStringLiteral("dark");
    }
    return QStringLiteral("dark");
}

ThemePreset themePresetFromKey(const QString &key)
{
    if (key == QStringLiteral("light"))
        return ThemePreset::Light;
    if (key == QStringLiteral("custom"))
        return ThemePreset::Custom;
    return ThemePreset::Dark;
}

} // namespace Aurora
