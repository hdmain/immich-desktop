#include "core/UpdateManager.h"

#include "AppVersion.h"
#include "core/AppSettings.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QStandardPaths>
#include <QTimer>
#include <QVersionNumber>

namespace Aurora {
namespace {

QString stripVersionPrefix(QString value)
{
    value = value.trimmed();
    if (value.startsWith(QLatin1Char('v'), Qt::CaseInsensitive))
        value.remove(0, 1);
    return value;
}

bool isNewerVersion(const QString &candidate, const QString &current)
{
    const auto left = QVersionNumber::fromString(stripVersionPrefix(candidate));
    const auto right = QVersionNumber::fromString(stripVersionPrefix(current));
    return left > right;
}

} // namespace

UpdateManager::UpdateManager(QObject *parent)
    : QObject(parent)
    , m_network(new QNetworkAccessManager(this))
{
}

UpdateState UpdateManager::state() const { return m_state; }
UpdateInfo UpdateManager::availableUpdate() const { return m_update; }
QString UpdateManager::errorMessage() const { return m_error; }
QString UpdateManager::downloadedPackagePath() const { return m_downloadPath; }
UpdateSettings UpdateManager::settings() const { return m_settingsStore.loadUpdate(); }

void UpdateManager::setAutoCheckEnabled(bool enabled)
{
    auto update = m_settingsStore.loadUpdate();
    update.autoCheck = enabled;
    m_settingsStore.saveUpdate(update);
}

void UpdateManager::skipCurrentUpdate()
{
    if (m_update.version.isEmpty())
        return;
    auto update = m_settingsStore.loadUpdate();
    update.skippedVersion = stripVersionPrefix(m_update.version);
    m_settingsStore.saveUpdate(update);
    setState(UpdateState::UpToDate);
}

bool UpdateManager::shouldAutoCheck() const
{
    const auto update = settings();
    if (!update.autoCheck)
        return false;
    if (!update.lastCheckUtc.isValid())
        return true;
    return update.lastCheckUtc.secsTo(QDateTime::currentDateTimeUtc()) >= 24 * 60 * 60;
}

void UpdateManager::checkForUpdates(bool silent)
{
    if (m_state == UpdateState::Checking || m_state == UpdateState::Downloading ||
        m_state == UpdateState::Installing) {
        return;
    }

    m_silentCheck = silent;
    m_error.clear();
    setState(UpdateState::Checking);

    const QUrl url(QStringLiteral("https://api.github.com/repos/%1/releases/latest")
                       .arg(QString::fromLatin1(Config::GitHubRepository)));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      QStringLiteral("immich-desktop/%1")
                          .arg(QString::fromLatin1(Config::ApplicationVersion)));
    request.setRawHeader("Accept", "application/vnd.github+json");

    if (m_activeReply) {
        m_activeReply->abort();
        m_activeReply->deleteLater();
    }
    m_activeReply = m_network->get(request);
    connect(m_activeReply, &QNetworkReply::finished, this, &UpdateManager::handleReleaseReply);
}

void UpdateManager::downloadUpdate()
{
    if (m_state != UpdateState::Available && m_state != UpdateState::ReadyToInstall &&
        m_state != UpdateState::Failed) {
        return;
    }

    if (m_update.installKind == InstallKind::LinuxSnap) {
        m_error.clear();
        m_downloadPath.clear();
        setState(UpdateState::ReadyToInstall);
        return;
    }

    if (!m_update.downloadUrl.isValid()) {
        fail(tr("No downloadable update package was found for this platform."));
        return;
    }

    m_error.clear();
    m_downloadPath.clear();
    setState(UpdateState::Downloading);

    const QString tempRoot = QStandardPaths::writableLocation(QStandardPaths::TempLocation) +
                             QStringLiteral("/immich-desktop-updates");
    QDir().mkpath(tempRoot);
    m_downloadPath = tempRoot + QLatin1Char('/') + m_update.assetName;

    if (QFile::exists(m_downloadPath))
        QFile::remove(m_downloadPath);

    QNetworkRequest request(m_update.downloadUrl);
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      QStringLiteral("immich-desktop/%1")
                          .arg(QString::fromLatin1(Config::ApplicationVersion)));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);

    if (m_activeReply) {
        m_activeReply->abort();
        m_activeReply->deleteLater();
    }
    m_activeReply = m_network->get(request);
    connect(m_activeReply, &QNetworkReply::downloadProgress, this, &UpdateManager::downloadProgress);
    connect(m_activeReply, &QNetworkReply::finished, this, &UpdateManager::handleDownloadFinished);
}

void UpdateManager::installUpdate()
{
    if (m_update.installKind == InstallKind::LinuxSnap) {
        setState(UpdateState::Installing);
        emit installStarted();
        if (!launchInstaller(QString(), InstallKind::LinuxSnap)) {
            fail(tr("Unable to refresh the Snap package. Try: snap refresh %1")
                     .arg(qEnvironmentVariable("SNAP_NAME", QStringLiteral("immich-desktop"))));
            return;
        }
        QTimer::singleShot(400, qApp, &QCoreApplication::quit);
        return;
    }

    if (m_downloadPath.isEmpty() || !QFileInfo::exists(m_downloadPath)) {
        fail(tr("The update package has not been downloaded yet."));
        return;
    }

    setState(UpdateState::Installing);
    emit installStarted();

    if (!launchInstaller(m_downloadPath, m_update.installKind)) {
        fail(tr("Unable to launch the update installer."));
        return;
    }

    QTimer::singleShot(400, qApp, &QCoreApplication::quit);
}

void UpdateManager::setState(UpdateState state)
{
    if (m_state == state)
        return;
    m_state = state;
    emit stateChanged(state);
}

void UpdateManager::fail(const QString &message)
{
    m_error = message;
    setState(UpdateState::Failed);
    emit errorOccurred(message);
}

void UpdateManager::handleReleaseReply()
{
    auto *reply = m_activeReply;
    m_activeReply = nullptr;
    if (!reply)
        return;
    reply->deleteLater();

    auto updateSettings = m_settingsStore.loadUpdate();
    updateSettings.lastCheckUtc = QDateTime::currentDateTimeUtc();
    m_settingsStore.saveUpdate(updateSettings);

    if (reply->error() != QNetworkReply::NoError) {
        if (reply->error() == QNetworkReply::OperationCanceledError)
            return;
        fail(tr("Update check failed: %1").arg(reply->errorString()));
        return;
    }

    UpdateInfo info;
    QString parseError;
    if (!parseReleasePayload(reply->readAll(), &info, &parseError)) {
        fail(parseError);
        return;
    }

    const QString current = QString::fromLatin1(Config::ApplicationVersion);
    if (!isNewerVersion(info.version, current)) {
        setState(UpdateState::UpToDate);
        emit upToDate();
        return;
    }

    if (m_silentCheck &&
        stripVersionPrefix(info.version) == stripVersionPrefix(updateSettings.skippedVersion)) {
        setState(UpdateState::UpToDate);
        emit upToDate();
        return;
    }

    m_update = info;
    setState(UpdateState::Available);
    emit updateAvailable(info);
}

void UpdateManager::handleDownloadFinished()
{
    auto *reply = m_activeReply;
    m_activeReply = nullptr;
    if (!reply)
        return;
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        if (reply->error() == QNetworkReply::OperationCanceledError)
            return;
        fail(tr("Download failed: %1").arg(reply->errorString()));
        return;
    }

    QFile file(m_downloadPath);
    if (!file.open(QIODevice::WriteOnly)) {
        fail(tr("Unable to write the downloaded package."));
        return;
    }
    file.write(reply->readAll());
    file.close();

#ifndef Q_OS_WIN
    if (m_update.installKind == InstallKind::LinuxAppImage)
        QFile::setPermissions(m_downloadPath,
                              QFile::permissions(m_downloadPath) | QFile::ExeUser | QFile::ExeGroup |
                                  QFile::ExeOther);
#endif

    setState(UpdateState::ReadyToInstall);
}

bool UpdateManager::parseReleasePayload(const QByteArray &payload, UpdateInfo *info,
                                        QString *error) const
{
    const auto document = QJsonDocument::fromJson(payload);
    if (!document.isObject()) {
        *error = tr("The update server returned an unexpected response.");
        return false;
    }

    const auto root = document.object();
    info->tagName = root.value(QStringLiteral("tag_name")).toString();
    info->version = stripVersionPrefix(info->tagName);
    info->releaseNotes = root.value(QStringLiteral("body")).toString().trimmed();
    if (info->version.isEmpty()) {
        *error = tr("The latest release does not include a version tag.");
        return false;
    }

    const InstallKind preferred = preferredInstallKind();
    if (preferred == InstallKind::LinuxSnap) {
        info->installKind = InstallKind::LinuxSnap;
        info->assetName = QStringLiteral("Snap Store");
        info->downloadUrl = QUrl(QStringLiteral("https://snapcraft.io/immich-desktop"));
        info->sizeBytes = 0;
        return true;
    }

    const auto assets = root.value(QStringLiteral("assets")).toArray();

    QJsonObject chosen;
    for (const auto &value : assets) {
        const auto asset = value.toObject();
        const QString name = asset.value(QStringLiteral("name")).toString();
        if (assetMatches(name, preferred)) {
            chosen = asset;
            break;
        }
    }

    if (chosen.isEmpty() && preferred == InstallKind::WindowsExe) {
        for (const auto &value : assets) {
            const auto asset = value.toObject();
            if (assetMatches(asset.value(QStringLiteral("name")).toString(),
                             InstallKind::WindowsMsi)) {
                chosen = asset;
                break;
            }
        }
    }

    if (chosen.isEmpty() && preferred == InstallKind::LinuxDeb) {
        for (const auto &value : assets) {
            const auto asset = value.toObject();
            if (assetMatches(asset.value(QStringLiteral("name")).toString(),
                             InstallKind::LinuxAppImage)) {
                chosen = asset;
                break;
            }
        }
    }

    if (chosen.isEmpty()) {
        *error = tr("No compatible installer was published for this platform.");
        return false;
    }

    info->assetName = chosen.value(QStringLiteral("name")).toString();
    info->downloadUrl = QUrl(chosen.value(QStringLiteral("browser_download_url")).toString());
    info->sizeBytes = static_cast<qint64>(chosen.value(QStringLiteral("size")).toDouble());
    info->installKind = preferred;
    if (info->assetName.endsWith(QStringLiteral(".msi"), Qt::CaseInsensitive))
        info->installKind = InstallKind::WindowsMsi;
    else if (info->assetName.endsWith(QStringLiteral(".exe"), Qt::CaseInsensitive))
        info->installKind = InstallKind::WindowsExe;
    else if (info->assetName.endsWith(QStringLiteral(".deb"), Qt::CaseInsensitive))
        info->installKind = InstallKind::LinuxDeb;
    else if (info->assetName.endsWith(QStringLiteral(".AppImage"), Qt::CaseInsensitive))
        info->installKind = InstallKind::LinuxAppImage;

    if (!info->downloadUrl.isValid()) {
        *error = tr("The selected update package has an invalid download URL.");
        return false;
    }
    return true;
}

InstallKind UpdateManager::preferredInstallKind() const
{
#ifdef Q_OS_WIN
    return InstallKind::WindowsExe;
#else
    if (!qEnvironmentVariableIsEmpty("SNAP"))
        return InstallKind::LinuxSnap;
    if (!qEnvironmentVariableIsEmpty("APPIMAGE"))
        return InstallKind::LinuxAppImage;
    return InstallKind::LinuxDeb;
#endif
}

bool UpdateManager::assetMatches(const QString &name, InstallKind kind) const
{
    switch (kind) {
    case InstallKind::WindowsExe:
        return name.endsWith(QStringLiteral(".exe"), Qt::CaseInsensitive) &&
               name.contains(QStringLiteral("immich"), Qt::CaseInsensitive);
    case InstallKind::WindowsMsi:
        return name.endsWith(QStringLiteral(".msi"), Qt::CaseInsensitive) &&
               name.contains(QStringLiteral("immich"), Qt::CaseInsensitive);
    case InstallKind::LinuxDeb:
        return name.endsWith(QStringLiteral(".deb"), Qt::CaseInsensitive) &&
               name.contains(QStringLiteral("immich"), Qt::CaseInsensitive);
    case InstallKind::LinuxAppImage:
        return name.endsWith(QStringLiteral(".AppImage"), Qt::CaseInsensitive) &&
               name.contains(QStringLiteral("immich"), Qt::CaseInsensitive);
    case InstallKind::LinuxSnap:
        return name.endsWith(QStringLiteral(".snap"), Qt::CaseInsensitive) &&
               name.contains(QStringLiteral("immich"), Qt::CaseInsensitive);
    case InstallKind::Unknown:
        break;
    }
    return false;
}

bool UpdateManager::launchInstaller(const QString &packagePath, InstallKind kind)
{
#ifdef Q_OS_WIN
    const QString appPath = QCoreApplication::applicationFilePath();
    const QString helperPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) +
                               QStringLiteral("/immich-desktop-apply-update.cmd");
    QFile helper(helperPath);
    if (!helper.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        return false;

    QString script;
    if (kind == InstallKind::WindowsMsi) {
        script = QStringLiteral(
                     "@echo off\r\n"
                     "timeout /t 2 /nobreak >nul\r\n"
                     "msiexec /i \"%1\" /passive\r\n"
                     "if exist \"%2\" start \"\" \"%2\"\r\n")
                     .arg(QDir::toNativeSeparators(packagePath),
                          QDir::toNativeSeparators(appPath));
    } else {
        script = QStringLiteral(
                     "@echo off\r\n"
                     "timeout /t 2 /nobreak >nul\r\n"
                     "start /wait \"\" \"%1\" /S\r\n"
                     "if errorlevel 1 start /wait \"\" \"%1\"\r\n"
                     "if exist \"%2\" start \"\" \"%2\"\r\n")
                     .arg(QDir::toNativeSeparators(packagePath),
                          QDir::toNativeSeparators(appPath));
    }
    helper.write(script.toLocal8Bit());
    helper.close();

    return QProcess::startDetached(QStringLiteral("cmd.exe"),
                                   {QStringLiteral("/C"), helperPath});
#else
    if (kind == InstallKind::LinuxSnap) {
        const QString snapName =
            qEnvironmentVariable("SNAP_NAME", QStringLiteral("immich-desktop"));
        if (QProcess::startDetached(QStringLiteral("snap"),
                                    {QStringLiteral("refresh"), snapName})) {
            return true;
        }
        return QDesktopServices::openUrl(
            QUrl(QStringLiteral("snap://%1").arg(snapName)));
    }

    if (kind == InstallKind::LinuxAppImage) {
        const QString currentAppImage = qEnvironmentVariable("APPIMAGE");
        QString target = currentAppImage;
        if (target.isEmpty()) {
            target = QStandardPaths::writableLocation(QStandardPaths::HomeLocation) +
                     QStringLiteral("/Applications/") + QFileInfo(packagePath).fileName();
            QDir().mkpath(QFileInfo(target).absolutePath());
        }

        const QString helperPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) +
                                   QStringLiteral("/immich-desktop-apply-update.sh");
        QFile helper(helperPath);
        if (!helper.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
            return false;

        const QString script = QStringLiteral(
                                   "#!/bin/sh\n"
                                   "set -eu\n"
                                   "sleep 1\n"
                                   "cp \"%1\" \"%2.new\"\n"
                                   "chmod +x \"%2.new\"\n"
                                   "mv \"%2.new\" \"%2\"\n"
                                   "exec \"%2\"\n")
                                   .arg(packagePath, target);
        helper.write(script.toUtf8());
        helper.close();
        QFile::setPermissions(helperPath,
                              QFile::permissions(helperPath) | QFile::ExeUser | QFile::ExeGroup |
                                  QFile::ExeOther);
        return QProcess::startDetached(QStringLiteral("/bin/sh"), {helperPath});
    }

    if (kind == InstallKind::LinuxDeb) {
        if (QProcess::startDetached(QStringLiteral("pkexec"),
                                    {QStringLiteral("dpkg"), QStringLiteral("-i"), packagePath})) {
            return true;
        }
        return QDesktopServices::openUrl(QUrl::fromLocalFile(packagePath));
    }

    return QDesktopServices::openUrl(QUrl::fromLocalFile(packagePath));
#endif
}

} // namespace Aurora
