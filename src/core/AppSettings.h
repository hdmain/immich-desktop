#pragma once

#include "core/Theme.h"

#include <QDateTime>
#include <QSettings>
#include <QString>

namespace Aurora {

struct AppearanceSettings {
    ThemePreset preset = ThemePreset::Dark;
    ThemePalette customPalette = ThemePalette::customDefault();
};

struct UpdateSettings {
    bool autoCheck = true;
    QString skippedVersion;
    QDateTime lastCheckUtc;
};

class AppSettings final {
public:
    AppSettings();

    AppearanceSettings loadAppearance() const;
    void saveAppearance(const AppearanceSettings &appearance);

    UpdateSettings loadUpdate() const;
    void saveUpdate(const UpdateSettings &update);

private:
    mutable QSettings m_settings;
};

} // namespace Aurora
