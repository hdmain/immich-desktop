#include "core/AppSettings.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QUuid>

#ifdef Q_OS_WIN
#include <windows.h>
#include <dpapi.h>
#endif

namespace Aurora {
namespace {

QSettings globalSettings()
{
    return QSettings(QSettings::IniFormat, QSettings::UserScope,
                     QStringLiteral("Immich"), QStringLiteral("immich"));
}

bool isSafeProfileId(const QString &id)
{
    // UUIDs / "legacy" only - reject path separators so account files stay under profiles/.
    if (id.isEmpty() || id.contains(u'/') || id.contains(u'\\') || id.contains(QStringLiteral("..")))
        return false;
    for (const QChar ch : id) {
        if (!(ch.isLetterOrNumber() || ch == u'-' || ch == u'_'))
            return false;
    }
    return true;
}

QString accountFile(const QString &id)
{
    const QString safe = isSafeProfileId(id) ? id : QStringLiteral("legacy");
    return QFileInfo(globalSettings().fileName()).absolutePath() +
           QStringLiteral("/profiles/") + safe + QStringLiteral(".ini");
}

void ensureProfiles()
{
    auto settings = globalSettings();
    if (settings.contains(QStringLiteral("profiles/default")))
        return;
    // Keep the original cache locations for the migrated account only.
    const QString id = QStringLiteral("legacy");
    QDir().mkpath(QFileInfo(accountFile(id)).absolutePath());
    QSettings account(accountFile(id), QSettings::IniFormat);
    for (const auto &key : settings.allKeys()) {
        const QString group = key.section(u'/', 0, 0);
        if (group == u"immich" || group == u"folderSync" || group == u"download" ||
            group == u"timeline" || group == u"playback")
            account.setValue(key, settings.value(key));
    }
    account.sync();
    if (account.status() != QSettings::NoError)
        return;
    settings.setValue(QStringLiteral("profiles/names/legacy"), QCoreApplication::translate("Profiles", "Personal"));
    settings.setValue(QStringLiteral("profiles/default"), id);
    settings.setValue(QStringLiteral("profiles/active"), id);
    settings.sync();
    if (settings.status() == QSettings::NoError) {
        for (const auto &group : {"immich", "folderSync", "download", "timeline", "playback"})
            settings.remove(QString::fromLatin1(group));
        settings.sync();
    }
}

bool hasProfile(const QString &id)
{
    if (!isSafeProfileId(id))
        return false;
    for (const auto &profile : AppSettings::profiles())
        if (profile.id == id)
            return true;
    return false;
}

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

AppSettings::AppSettings(const QString &profileId)
    : m_settings(QSettings::IniFormat, QSettings::UserScope,
                 QStringLiteral("Immich"), QStringLiteral("immich"))
{
    ensureProfiles();
    m_accountSettings = std::make_unique<QSettings>(accountFile(hasProfile(profileId) ? profileId : activeProfileId()), QSettings::IniFormat);
}

QList<AppSettings::Profile> AppSettings::profiles()
{
    ensureProfiles();
    auto settings = globalSettings();
    settings.beginGroup(QStringLiteral("profiles/names"));
    QList<Profile> result;
    for (const auto &id : settings.childKeys()) {
        if (!isSafeProfileId(id))
            continue;
        result.append({id, settings.value(id).toString()});
    }
    return result;
}

QString AppSettings::activeProfileId()
{
    ensureProfiles();
    const QString id =
        globalSettings().value(QStringLiteral("profiles/active"), QStringLiteral("legacy")).toString();
    return isSafeProfileId(id) && hasProfile(id) ? id : QStringLiteral("legacy");
}

QString AppSettings::defaultProfileId()
{
    ensureProfiles();
    const QString id = globalSettings().value(QStringLiteral("profiles/default")).toString();
    return isSafeProfileId(id) && hasProfile(id) ? id : activeProfileId();
}

QString AppSettings::createProfile(const QString &name)
{
    if (name.trimmed().isEmpty())
        return {};
    ensureProfiles();
    const QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QDir().mkpath(QFileInfo(accountFile(id)).absolutePath());
    auto settings = globalSettings();
    settings.setValue(QStringLiteral("profiles/names/") + id, name.trimmed());
    settings.sync();
    return settings.status() == QSettings::NoError ? id : QString();
}

bool AppSettings::renameProfile(const QString &id, const QString &name)
{
    if (!hasProfile(id) || name.trimmed().isEmpty())
        return false;
    auto settings = globalSettings();
    settings.setValue(QStringLiteral("profiles/names/") + id, name.trimmed());
    settings.sync();
    return settings.status() == QSettings::NoError;
}

bool AppSettings::deleteProfile(const QString &id)
{
    if (!hasProfile(id) || id == activeProfileId() || profiles().size() <= 1)
        return false;
    // Delete credentials; caches can be pruned normally and never get reused by a new ID.
    if (QFile::exists(accountFile(id)) && !QFile::remove(accountFile(id)))
        return false;
    auto settings = globalSettings();
    if (id == defaultProfileId())
        settings.setValue(QStringLiteral("profiles/default"), activeProfileId());
    settings.remove(QStringLiteral("profiles/names/") + id);
    settings.sync();
    return settings.status() == QSettings::NoError;
}

bool AppSettings::setActiveProfile(const QString &id)
{
    if (!hasProfile(id))
        return false;
    auto settings = globalSettings();
    settings.setValue(QStringLiteral("profiles/active"), id);
    settings.sync();
    return settings.status() == QSettings::NoError;
}

bool AppSettings::setDefaultProfile(const QString &id)
{
    if (!hasProfile(id))
        return false;
    auto settings = globalSettings();
    settings.setValue(QStringLiteral("profiles/default"), id);
    settings.sync();
    return settings.status() == QSettings::NoError;
}

void AppSettings::activateDefaultProfile()
{
    setActiveProfile(defaultProfileId());
}

QString AppSettings::profileDataDirectory()
{
    const QString root = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    const QString id = activeProfileId();
    return id == u"legacy" ? root : root + QStringLiteral("/profiles/") + id;
}

QString AppSettings::profileCacheDirectory()
{
    const QString root = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    const QString id = activeProfileId();
    return id == u"legacy" ? root : root + QStringLiteral("/profiles/") + id;
}

void AppSettings::requestProfileSwitch(const QString &id)
{
    if (!hasProfile(id) || id == activeProfileId())
        return;
    QCoreApplication::instance()->setProperty("nextProfile", id);
    QCoreApplication::exit(ProfileSwitchExitCode);
}

QString AppSettings::loadLanguage() const
{
    return m_settings.value(QStringLiteral("localization/language")).toString();
}

void AppSettings::saveLanguage(const QString &language)
{
    m_settings.setValue(QStringLiteral("localization/language"), language);
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
    result.serverUrl = m_accountSettings->value(QStringLiteral("immich/serverUrl")).toString();
    result.localServerUrl =
        m_accountSettings->value(QStringLiteral("immich/localServerUrl")).toString();
    result.customHeaders = CustomHeaders::deserialize(
        m_accountSettings->value(QStringLiteral("immich/customHeaders")).toString());
    const QString storedKey =
        m_accountSettings->value(QStringLiteral("immich/apiKey")).toString();
    result.apiKey = unprotectSecret(storedKey);
#ifdef Q_OS_WIN
    // Transparently migrate legacy plaintext settings to per-user DPAPI storage.
    if (!storedKey.isEmpty() && !storedKey.startsWith(QStringLiteral("dpapi:")) &&
        !result.apiKey.isEmpty()) {
        const QString encrypted = protectSecret(result.apiKey);
        if (!encrypted.isEmpty()) {
            m_accountSettings->setValue(QStringLiteral("immich/apiKey"), encrypted);
            m_accountSettings->sync();
        }
    }
#endif

    result.certFormat = static_cast<TlsCertFormat>(
        m_accountSettings->value(QStringLiteral("immich/certFormat"), 0).toInt());
    result.clientCertPath = m_accountSettings->value(QStringLiteral("immich/clientCertPath")).toString();
    result.clientKeyPath = m_accountSettings->value(QStringLiteral("immich/clientKeyPath")).toString();
    result.caCertPath = m_accountSettings->value(QStringLiteral("immich/caCertPath")).toString();
    result.rememberKeyPassword = m_accountSettings->value(QStringLiteral("immich/rememberKeyPassword"), false).toBool();
    result.keyPassword = unprotectSecret(
        m_accountSettings->value(QStringLiteral("immich/keyPassword")).toString());

    return result;
}

void AppSettings::saveImmichConnection(const ImmichConnectionSettings &connection)
{
    m_accountSettings->setValue(QStringLiteral("immich/serverUrl"), connection.serverUrl.trimmed());
    m_accountSettings->setValue(QStringLiteral("immich/localServerUrl"),
                        connection.localServerUrl.trimmed());
    const QString apiKey = connection.apiKey.trimmed();
    const QString protectedKey = protectSecret(apiKey);
    m_accountSettings->setValue(QStringLiteral("immich/apiKey"), protectedKey);
    m_accountSettings->setValue(
        QStringLiteral("immich/customHeaders"),
        CustomHeaders::serialize(CustomHeaders::normalize(connection.customHeaders)));

    m_accountSettings->setValue(QStringLiteral("immich/certFormat"), static_cast<int>(connection.certFormat));
    m_accountSettings->setValue(QStringLiteral("immich/clientCertPath"), connection.clientCertPath.trimmed());
    m_accountSettings->setValue(QStringLiteral("immich/clientKeyPath"), connection.clientKeyPath.trimmed());
    m_accountSettings->setValue(QStringLiteral("immich/caCertPath"), connection.caCertPath.trimmed());
    m_accountSettings->setValue(QStringLiteral("immich/rememberKeyPassword"), connection.rememberKeyPassword);

    if (connection.rememberKeyPassword) {
        const QString protectedPassword = protectSecret(connection.keyPassword);
        m_accountSettings->setValue(QStringLiteral("immich/keyPassword"), protectedPassword);
    } else {
        m_accountSettings->remove(QStringLiteral("immich/keyPassword"));
    }

    m_accountSettings->sync();
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
    result.viewerInfoOpen =
        m_settings.value(QStringLiteral("window/viewerInfoOpen"), false).toBool();
    return result;
}

void AppSettings::saveWindow(const WindowSettings &window)
{
    m_settings.setValue(QStringLiteral("window/closeToTray"), window.closeToTray);
    m_settings.setValue(QStringLiteral("window/autoStart"), window.autoStart);
    m_settings.setValue(QStringLiteral("window/sidebarCollapsed"), window.sidebarCollapsed);
    m_settings.setValue(QStringLiteral("window/viewerInfoOpen"), window.viewerInfoOpen);
    m_settings.sync();
}

PlaybackSettings AppSettings::loadPlayback() const
{
    PlaybackSettings result;
    result.hoverPreviewEnabled =
        m_accountSettings->value(QStringLiteral("playback/hoverPreviewEnabled"), true).toBool();
    return result;
}

void AppSettings::savePlayback(const PlaybackSettings &playback)
{
    m_accountSettings->setValue(QStringLiteral("playback/hoverPreviewEnabled"),
                        playback.hoverPreviewEnabled);
    m_accountSettings->sync();
}

TimelineSettings AppSettings::loadTimeline() const
{
    TimelineSettings result;
    result.compactGrid =
        m_accountSettings->value(QStringLiteral("timeline/compactGrid"), false).toBool();
    result.tileSize =
        m_accountSettings->value(QStringLiteral("timeline/tileSize"), result.tileSize).toInt();
    return result;
}

void AppSettings::saveTimeline(const TimelineSettings &timeline)
{
    m_accountSettings->setValue(QStringLiteral("timeline/compactGrid"), timeline.compactGrid);
    m_accountSettings->setValue(QStringLiteral("timeline/tileSize"), timeline.tileSize);
    m_accountSettings->sync();
}

DownloadSettings AppSettings::loadDownload() const
{
    DownloadSettings result;
    result.lastDirectory =
        m_accountSettings->value(QStringLiteral("download/lastDirectory")).toString();
    return result;
}

void AppSettings::saveDownload(const DownloadSettings &download)
{
    m_accountSettings->setValue(QStringLiteral("download/lastDirectory"), download.lastDirectory);
    m_accountSettings->sync();
}

FolderSyncSettings AppSettings::loadFolderSync() const
{
    FolderSyncSettings result;
    result.enabled = m_accountSettings->value(QStringLiteral("folderSync/enabled"), false).toBool();
    result.localNetworkOnly =
        m_accountSettings->value(QStringLiteral("folderSync/localNetworkOnly"), false).toBool();
    if (m_accountSettings->contains(QStringLiteral("folderSync/folderPaths")))
        result.folderPaths = m_accountSettings->value(QStringLiteral("folderSync/folderPaths")).toStringList();
    else {
        const QString legacyFolder = m_accountSettings->value(QStringLiteral("folderSync/folderPath")).toString().trimmed();
        if (!legacyFolder.isEmpty())
            result.folderPaths.append(legacyFolder);
    }
    return result;
}

void AppSettings::saveFolderSync(const FolderSyncSettings &folderSync)
{
    m_accountSettings->setValue(QStringLiteral("folderSync/enabled"), folderSync.enabled);
    m_accountSettings->setValue(QStringLiteral("folderSync/localNetworkOnly"),
                        folderSync.localNetworkOnly);
    m_accountSettings->setValue(QStringLiteral("folderSync/folderPaths"), folderSync.folderPaths);
    m_accountSettings->remove(QStringLiteral("folderSync/folderPath"));
    m_accountSettings->sync();
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
