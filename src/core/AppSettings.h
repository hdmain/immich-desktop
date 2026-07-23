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
    bool autoStart = false;
};

struct SupportSettings {
    bool githubStarDismissed = false;
    int launchCount = 0;
};

class AppSettings final {
public:
    AppSettings();

    AppearanceSettings loadAppearance() const;
    void saveAppearance(const AppearanceSettings &appearance);

    UpdateSettings loadUpdate() const;
    void saveUpdate(const UpdateSettings &update);

    ImmichConnectionSettings loadImmichConnection() const;
    // Returns false when the API key could not be protected for storage; URLs are
    // still saved and any previously stored key is left unchanged.
    bool saveImmichConnection(const ImmichConnectionSettings &connection);

    WindowSettings loadWindow() const;
    void saveWindow(const WindowSettings &window);

    SupportSettings loadSupport() const;
    void saveSupport(const SupportSettings &support);

private:
    mutable QSettings m_settings;
};

} // namespace Aurora
