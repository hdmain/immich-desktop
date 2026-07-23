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
    return result;
}

bool AppSettings::saveImmichConnection(const ImmichConnectionSettings &connection)
{
    m_settings.setValue(QStringLiteral("immich/serverUrl"), connection.serverUrl.trimmed());
    m_settings.setValue(QStringLiteral("immich/localServerUrl"),
                        connection.localServerUrl.trimmed());
    const QString apiKey = connection.apiKey.trimmed();
    if (apiKey.isEmpty()) {
        m_settings.setValue(QStringLiteral("immich/apiKey"), QString());
        m_settings.sync();
        return true;
    }

    const QString protectedKey = protectSecret(apiKey);
    if (protectedKey.isEmpty()) {
        // Encryption failed — never overwrite a previously stored key with "".
        m_settings.sync();
        return false;
    }
    m_settings.setValue(QStringLiteral("immich/apiKey"), protectedKey);
    m_settings.sync();
    return true;
}

WindowSettings AppSettings::loadWindow() const
{
    WindowSettings result;
    result.closeToTray =
        m_settings.value(QStringLiteral("window/closeToTray"), true).toBool();
    result.autoStart =
        m_settings.value(QStringLiteral("window/autoStart"), false).toBool();
    return result;
}

void AppSettings::saveWindow(const WindowSettings &window)
{
    m_settings.setValue(QStringLiteral("window/closeToTray"), window.closeToTray);
    m_settings.setValue(QStringLiteral("window/autoStart"), window.autoStart);
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
