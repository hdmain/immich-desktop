#pragma once

#include "core/CustomHeaders.h"
#include "core/Theme.h"
#include "core/TlsConfig.h"

#include <QDateTime>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <memory>

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
    explicit AppSettings(const QString &profileId = {});

    struct Profile { QString id; QString name; };
    static QList<Profile> profiles();
    static QString activeProfileId();
    static QString defaultProfileId();
    static QString createProfile(const QString &name);
    static bool renameProfile(const QString &id, const QString &name);
    static bool deleteProfile(const QString &id);
    static bool setActiveProfile(const QString &id);
    static bool setDefaultProfile(const QString &id);
    static void activateDefaultProfile();
    static QString profileDataDirectory();
    static QString profileCacheDirectory();
    // Switching is completed by main only after the old account's services stop.
    static void requestProfileSwitch(const QString &id);
    static constexpr int ProfileSwitchExitCode = 42;

    QString loadLanguage() const;
    void saveLanguage(const QString &language);

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
    std::unique_ptr<QSettings> m_accountSettings;
};

} // namespace Aurora
