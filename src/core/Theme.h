#pragma once

#include <QColor>
#include <QString>

namespace Aurora {

enum class ThemePreset {
    Light,
    Dark,
    Custom
};

struct ThemePalette {
    QColor background;
    QColor panel;
    QColor button;
    QColor accent;
    QColor text;
    QColor mutedText;
    QColor border;

    static ThemePalette light();
    static ThemePalette dark();
    static ThemePalette customDefault();

    bool operator==(const ThemePalette &) const = default;
};

QString themePresetKey(ThemePreset preset);
ThemePreset themePresetFromKey(const QString &key);

} // namespace Aurora
