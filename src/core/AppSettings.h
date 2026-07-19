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

struct ImmichConnectionSettings {
    QString serverUrl;
    QString localServerUrl;
    QString apiKey;

    bool isConfigured() const
    {
        return !serverUrl.trimmed().isEmpty() && !apiKey.trimmed().isEmpty();
    }

    bool hasLocalServerUrl() const { return !localServerUrl.trimmed().isEmpty(); }
};

struct WindowSettings {
    bool closeToTray = true;
};

class AppSettings final {
public:
    AppSettings();

    AppearanceSettings loadAppearance() const;
    void saveAppearance(const AppearanceSettings &appearance);

    UpdateSettings loadUpdate() const;
    void saveUpdate(const UpdateSettings &update);

    ImmichConnectionSettings loadImmichConnection() const;
    void saveImmichConnection(const ImmichConnectionSettings &connection);

    WindowSettings loadWindow() const;
    void saveWindow(const WindowSettings &window);

private:
    mutable QSettings m_settings;
};

} // namespace Aurora
