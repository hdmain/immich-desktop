#pragma once

#include "core/CustomHeaders.h"
#include "core/Theme.h"
#include "core/TlsConfig.h"

#include <QDateTime>
#include <QSettings>
#include <QString>
#include <QStringList>

namespace Aurora {

struct AppearanceSettings {
    ThemePreset preset = ThemePreset::Dark;
    ThemePalette customPalette = ThemePalette::customDefault();
    QString iconPack = QStringLiteral("lucide");
    bool minimalistUi = false;
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
    CustomHeaders::HeaderList customHeaders;

    TlsCertFormat certFormat = TlsCertFormat::Pkcs12;
    QString clientCertPath;
    QString clientKeyPath;
    QString caCertPath;
    QString keyPassword;
    bool rememberKeyPassword = false;

    bool isConfigured() const
    {
        return !serverUrl.trimmed().isEmpty() && !apiKey.trimmed().isEmpty();
    }

    bool hasLocalServerUrl() const { return !localServerUrl.trimmed().isEmpty(); }

    bool hasClientCert() const { return !clientCertPath.trimmed().isEmpty(); }
};

struct WindowSettings {
    bool closeToTray = true;
    bool autoStart = false;
    bool sidebarCollapsed = false;
    bool viewerInfoOpen = false;
};

struct PlaybackSettings {
    bool hoverPreviewEnabled = true;
};

struct TimelineSettings {
    bool compactGrid = false;
    int tileSize = 180;
};

struct DownloadSettings {
    QString lastDirectory;
};

struct FolderSyncSettings {
    bool enabled = false;
    bool localNetworkOnly = false;
    QStringList folderPaths;
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
    void saveImmichConnection(const ImmichConnectionSettings &connection);

    WindowSettings loadWindow() const;
    void saveWindow(const WindowSettings &window);

    PlaybackSettings loadPlayback() const;
    void savePlayback(const PlaybackSettings &playback);

    TimelineSettings loadTimeline() const;
    void saveTimeline(const TimelineSettings &timeline);

    DownloadSettings loadDownload() const;
    void saveDownload(const DownloadSettings &download);

    FolderSyncSettings loadFolderSync() const;
    void saveFolderSync(const FolderSyncSettings &folderSync);

    SupportSettings loadSupport() const;
    void saveSupport(const SupportSettings &support);

private:
    mutable QSettings m_settings;
};

} // namespace Aurora
