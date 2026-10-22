#include "core/AppSettings.h"

#ifdef Q_OS_WIN
#include <windows.h>
#include <dpapi.h>
#endif

namespace Aurora {
namespace {

QColor readColor(const QSettings &settings, const QString &key, const QColor &fallback)
{
    const QColor color(settings.value(key, fallback.name(QColor::HexArgb)).toString());
    return color.isValid() ? color : fallback;
}

QString protectSecret(const QString &plainText)
{
#ifdef Q_OS_WIN
    const QByteArray input = plainText.toUtf8();
    DATA_BLOB inputBlob{
        static_cast<DWORD>(input.size()),
        reinterpret_cast<BYTE *>(const_cast<char *>(input.constData()))};
    DATA_BLOB outputBlob{};
    if (!CryptProtectData(&inputBlob, L"Immich API key", nullptr, nullptr, nullptr,
                          CRYPTPROTECT_UI_FORBIDDEN, &outputBlob))
        return {};
    const QByteArray encrypted(reinterpret_cast<const char *>(outputBlob.pbData),
                               static_cast<qsizetype>(outputBlob.cbData));
    LocalFree(outputBlob.pbData);
    return QStringLiteral("dpapi:") + QString::fromLatin1(encrypted.toBase64());
#else
    return plainText;
#endif
}

QString unprotectSecret(const QString &storedValue)
{
#ifdef Q_OS_WIN
    if (!storedValue.startsWith(QStringLiteral("dpapi:")))
        return storedValue;
    const QByteArray encrypted =
        QByteArray::fromBase64(storedValue.sliced(6).toLatin1());
    if (encrypted.isEmpty())
        return {};
    DATA_BLOB inputBlob{
        static_cast<DWORD>(encrypted.size()),
        reinterpret_cast<BYTE *>(const_cast<char *>(encrypted.constData()))};
    DATA_BLOB outputBlob{};
    if (!CryptUnprotectData(&inputBlob, nullptr, nullptr, nullptr, nullptr,
                            CRYPTPROTECT_UI_FORBIDDEN, &outputBlob))
        return {};
    const QString plainText =
        QString::fromUtf8(reinterpret_cast<const char *>(outputBlob.pbData),
                          static_cast<qsizetype>(outputBlob.cbData));
    SecureZeroMemory(outputBlob.pbData, outputBlob.cbData);
    LocalFree(outputBlob.pbData);
    return plainText;
#else
    return storedValue;
#endif
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
    result.iconPack = m_settings.value(QStringLiteral("appearance/iconPack"), QStringLiteral("lucide")).toString();
    if (result.iconPack != QStringLiteral("tabler") && result.iconPack != QStringLiteral("bootstrap"))
        result.iconPack = QStringLiteral("lucide");
    result.minimalistUi = m_settings.value(QStringLiteral("appearance/minimalistUi"), false).toBool();
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
    m_settings.setValue(QStringLiteral("appearance/iconPack"), appearance.iconPack);
    m_settings.setValue(QStringLiteral("appearance/minimalistUi"), appearance.minimalistUi);
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

UpdateSettings AppSettings::loadUpdate() const
{
    UpdateSettings result;
    result.autoCheck = m_settings.value(QStringLiteral("updates/autoCheck"), true).toBool();
    result.skippedVersion =
        m_settings.value(QStringLiteral("updates/skippedVersion")).toString();
    const auto lastCheck = m_settings.value(QStringLiteral("updates/lastCheckUtc")).toString();
    if (!lastCheck.isEmpty())
        result.lastCheckUtc = QDateTime::fromString(lastCheck, Qt::ISODate);
    return result;
}

void AppSettings::saveUpdate(const UpdateSettings &update)
{
    m_settings.setValue(QStringLiteral("updates/autoCheck"), update.autoCheck);
    m_settings.setValue(QStringLiteral("updates/skippedVersion"), update.skippedVersion);
    m_settings.setValue(QStringLiteral("updates/lastCheckUtc"),
                        update.lastCheckUtc.isValid()
                            ? update.lastCheckUtc.toString(Qt::ISODate)
                            : QString());
    m_settings.sync();
}

ImmichConnectionSettings AppSettings::loadImmichConnection() const
{
    ImmichConnectionSettings result;
    result.serverUrl = m_settings.value(QStringLiteral("immich/serverUrl")).toString();
    result.localServerUrl =
        m_settings.value(QStringLiteral("immich/localServerUrl")).toString();
    const QString storedKey =
        m_settings.value(QStringLiteral("immich/apiKey")).toString();
    result.apiKey = unprotectSecret(storedKey);
#ifdef Q_OS_WIN
    // Transparently migrate legacy plaintext settings to per-user DPAPI storage.
    if (!storedKey.isEmpty() && !storedKey.startsWith(QStringLiteral("dpapi:")) &&
        !result.apiKey.isEmpty()) {
        const QString encrypted = protectSecret(result.apiKey);
        if (!encrypted.isEmpty()) {
            m_settings.setValue(QStringLiteral("immich/apiKey"), encrypted);
            m_settings.sync();
        }
    }
#endif

    result.certFormat = static_cast<TlsCertFormat>(
        m_settings.value(QStringLiteral("immich/certFormat"), 0).toInt());
    result.clientCertPath = m_settings.value(QStringLiteral("immich/clientCertPath")).toString();
    result.clientKeyPath = m_settings.value(QStringLiteral("immich/clientKeyPath")).toString();
    result.caCertPath = m_settings.value(QStringLiteral("immich/caCertPath")).toString();
    result.rememberKeyPassword = m_settings.value(QStringLiteral("immich/rememberKeyPassword"), false).toBool();
    result.keyPassword = unprotectSecret(
        m_settings.value(QStringLiteral("immich/keyPassword")).toString());

    return result;
}

void AppSettings::saveImmichConnection(const ImmichConnectionSettings &connection)
{
    m_settings.setValue(QStringLiteral("immich/serverUrl"), connection.serverUrl.trimmed());
    m_settings.setValue(QStringLiteral("immich/localServerUrl"),
                        connection.localServerUrl.trimmed());
    const QString apiKey = connection.apiKey.trimmed();
    const QString protectedKey = protectSecret(apiKey);
    m_settings.setValue(QStringLiteral("immich/apiKey"), protectedKey);

    m_settings.setValue(QStringLiteral("immich/certFormat"), static_cast<int>(connection.certFormat));
    m_settings.setValue(QStringLiteral("immich/clientCertPath"), connection.clientCertPath.trimmed());
    m_settings.setValue(QStringLiteral("immich/clientKeyPath"), connection.clientKeyPath.trimmed());
    m_settings.setValue(QStringLiteral("immich/caCertPath"), connection.caCertPath.trimmed());
    m_settings.setValue(QStringLiteral("immich/rememberKeyPassword"), connection.rememberKeyPassword);

    if (connection.rememberKeyPassword) {
        const QString protectedPassword = protectSecret(connection.keyPassword);
        m_settings.setValue(QStringLiteral("immich/keyPassword"), protectedPassword);
    } else {
        m_settings.remove(QStringLiteral("immich/keyPassword"));
    }

    m_settings.sync();
}

WindowSettings AppSettings::loadWindow() const
{
    WindowSettings result;
    result.closeToTray =
        m_settings.value(QStringLiteral("window/closeToTray"), true).toBool();
    result.autoStart =
        m_settings.value(QStringLiteral("window/autoStart"), false).toBool();
    result.sidebarCollapsed =
        m_settings.value(QStringLiteral("window/sidebarCollapsed"), false).toBool();
    return result;
}

void AppSettings::saveWindow(const WindowSettings &window)
{
    m_settings.setValue(QStringLiteral("window/closeToTray"), window.closeToTray);
    m_settings.setValue(QStringLiteral("window/autoStart"), window.autoStart);
    m_settings.setValue(QStringLiteral("window/sidebarCollapsed"), window.sidebarCollapsed);
    m_settings.sync();
}

PlaybackSettings AppSettings::loadPlayback() const
{
    PlaybackSettings result;
    result.hoverPreviewEnabled =
        m_settings.value(QStringLiteral("playback/hoverPreviewEnabled"), true).toBool();
    return result;
}

void AppSettings::savePlayback(const PlaybackSettings &playback)
{
    m_settings.setValue(QStringLiteral("playback/hoverPreviewEnabled"),
                        playback.hoverPreviewEnabled);
    m_settings.sync();
}

TimelineSettings AppSettings::loadTimeline() const
{
    TimelineSettings result;
    result.compactGrid =
        m_settings.value(QStringLiteral("timeline/compactGrid"), false).toBool();
    result.tileSize =
        m_settings.value(QStringLiteral("timeline/tileSize"), result.tileSize).toInt();
    return result;
}

void AppSettings::saveTimeline(const TimelineSettings &timeline)
{
    m_settings.setValue(QStringLiteral("timeline/compactGrid"), timeline.compactGrid);
    m_settings.setValue(QStringLiteral("timeline/tileSize"), timeline.tileSize);
    m_settings.sync();
}

DownloadSettings AppSettings::loadDownload() const
{
    DownloadSettings result;
    result.lastDirectory =
        m_settings.value(QStringLiteral("download/lastDirectory")).toString();
    return result;
}

void AppSettings::saveDownload(const DownloadSettings &download)
{
    m_settings.setValue(QStringLiteral("download/lastDirectory"), download.lastDirectory);
    m_settings.sync();
}

FolderSyncSettings AppSettings::loadFolderSync() const
{
    FolderSyncSettings result;
    result.enabled = m_settings.value(QStringLiteral("folderSync/enabled"), false).toBool();
    result.localNetworkOnly =
        m_settings.value(QStringLiteral("folderSync/localNetworkOnly"), false).toBool();
    if (m_settings.contains(QStringLiteral("folderSync/folderPaths")))
        result.folderPaths = m_settings.value(QStringLiteral("folderSync/folderPaths")).toStringList();
    else {
        const QString legacyFolder = m_settings.value(QStringLiteral("folderSync/folderPath")).toString().trimmed();
        if (!legacyFolder.isEmpty())
            result.folderPaths.append(legacyFolder);
    }
    return result;
}

void AppSettings::saveFolderSync(const FolderSyncSettings &folderSync)
{
    m_settings.setValue(QStringLiteral("folderSync/enabled"), folderSync.enabled);
    m_settings.setValue(QStringLiteral("folderSync/localNetworkOnly"),
                        folderSync.localNetworkOnly);
    m_settings.setValue(QStringLiteral("folderSync/folderPaths"), folderSync.folderPaths);
    m_settings.remove(QStringLiteral("folderSync/folderPath"));
    m_settings.sync();
}

SupportSettings AppSettings::loadSupport() const
{
    SupportSettings result;
    result.githubStarDismissed =
        m_settings.value(QStringLiteral("support/githubStarDismissed"), false).toBool();
    result.launchCount =
        m_settings.value(QStringLiteral("support/launchCount"), 0).toInt();
    return result;
}

void AppSettings::saveSupport(const SupportSettings &support)
{
    m_settings.setValue(QStringLiteral("support/githubStarDismissed"),
                        support.githubStarDismissed);
    m_settings.setValue(QStringLiteral("support/launchCount"), support.launchCount);
    m_settings.sync();
}

} // namespace Aurora
