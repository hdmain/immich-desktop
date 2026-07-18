#pragma once

#include "core/Theme.h"

#include <QSettings>

namespace Aurora {

struct AppearanceSettings {
    ThemePreset preset = ThemePreset::Dark;
    ThemePalette customPalette = ThemePalette::customDefault();
};

class AppSettings final {
public:
    AppSettings();

    AppearanceSettings loadAppearance() const;
    void saveAppearance(const AppearanceSettings &appearance);

private:
    mutable QSettings m_settings;
};

} // namespace Aurora
