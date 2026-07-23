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
#include <QRegularExpression>
#include <QSaveFile>
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

UpdateManager::~UpdateManager()
{
    if (m_activeReply) {
        disconnect(m_activeReply, nullptr, this, nullptr);
        m_activeReply->abort();
    }
    if (m_downloadFile) {
        m_downloadFile->cancelWriting();
        delete m_downloadFile;
        m_downloadFile = nullptr;
    }
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
    request.setTransferTimeout(30000);

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
        if (m_installAfterDownload) {
            m_installAfterDownload = false;
            QTimer::singleShot(0, this, &UpdateManager::installUpdate);
        }
        return;
    }

    if (!m_update.downloadUrl.isValid()) {
        m_installAfterDownload = false;
        fail(tr("No downloadable update package was found for this platform."));
        return;
    }

    m_error.clear();
    m_downloadPath.clear();
    setState(UpdateState::Downloading);

    const QString safeAssetName = QFileInfo(m_update.assetName).fileName();
    if (safeAssetName.isEmpty() || safeAssetName != m_update.assetName) {
        m_installAfterDownload = false;
        fail(tr("The update package has an unsafe file name."));
        return;
    }

    const QString tempRoot = QStandardPaths::writableLocation(QStandardPaths::TempLocation) +
                             QStringLiteral("/immich-desktop-updates");
    if (!QDir().mkpath(tempRoot)) {
        m_installAfterDownload = false;
        fail(tr("Unable to create the update download directory."));
        return;
    }
    m_downloadPath = QDir(tempRoot).filePath(safeAssetName);

    if (QFile::exists(m_downloadPath))
        QFile::remove(m_downloadPath);

    delete m_downloadFile;
    m_downloadFile = new QSaveFile(m_downloadPath);
    if (!m_downloadFile->open(QIODevice::WriteOnly)) {
        delete m_downloadFile;
        m_downloadFile = nullptr;
        m_installAfterDownload = false;
        fail(tr("Unable to create the update package file."));
        return;
    }

    QNetworkRequest request(m_update.downloadUrl);
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      QStringLiteral("immich-desktop/%1")
                          .arg(QString::fromLatin1(Config::ApplicationVersion)));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setTransferTimeout(30000);

    if (m_activeReply) {
        m_activeReply->abort();
        m_activeReply->deleteLater();
    }
    m_activeReply = m_network->get(request);
    connect(m_activeReply, &QNetworkReply::downloadProgress, this, &UpdateManager::downloadProgress);
    connect(m_activeReply, &QNetworkReply::readyRead, this,
            &UpdateManager::handleDownloadReadyRead);
    connect(m_activeReply, &QNetworkReply::finished, this, &UpdateManager::handleDownloadFinished);
}

void UpdateManager::applyUpdate()
{
    if (m_state == UpdateState::ReadyToInstall) {
        installUpdate();
        return;
    }
    if (m_state != UpdateState::Available && m_state != UpdateState::Failed)
        return;

    m_installAfterDownload = true;
    downloadUpdate();
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
    m_installAfterDownload = false;
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

void UpdateManager::handleDownloadReadyRead()
{
    if (!m_activeReply || !m_downloadFile)
        return;

    constexpr qint64 maximumPackageBytes = 1024LL * 1024 * 1024;
    const QByteArray chunk = m_activeReply->readAll();
    const qint64 expectedSize = m_update.sizeBytes;
    const qint64 maximumAllowed =
        expectedSize > 0 ? qMin(maximumPackageBytes, expectedSize) : maximumPackageBytes;
    if (chunk.size() > maximumAllowed - m_downloadFile->size()) {
        m_activeReply->setProperty("downloadFailure",
                                   tr("The update package exceeded its advertised size."));
        m_activeReply->abort();
        return;
    }
    if (!chunk.isEmpty() && m_downloadFile->write(chunk) != chunk.size()) {
        m_activeReply->setProperty("downloadFailure",
                                   tr("Unable to write the downloaded package."));
        m_activeReply->abort();
    }
}

void UpdateManager::handleDownloadFinished()
{
    auto *reply = m_activeReply;
    if (!reply)
        return;
    handleDownloadReadyRead();
    m_activeReply = nullptr;
    reply->deleteLater();

    const QString downloadFailure = reply->property("downloadFailure").toString();
    if (!downloadFailure.isEmpty() || reply->error() != QNetworkReply::NoError) {
        if (m_downloadFile) {
            m_downloadFile->cancelWriting();
            delete m_downloadFile;
            m_downloadFile = nullptr;
        }
        if (reply->error() == QNetworkReply::OperationCanceledError &&
            downloadFailure.isEmpty())
            return;
        m_installAfterDownload = false;
        fail(downloadFailure.isEmpty()
                 ? tr("Download failed: %1").arg(reply->errorString())
                 : downloadFailure);
        return;
    }

    if (!m_downloadFile) {
        m_installAfterDownload = false;
        fail(tr("The update package file is unavailable."));
        return;
    }
    if (m_update.sizeBytes > 0 && m_downloadFile->size() != m_update.sizeBytes) {
        m_downloadFile->cancelWriting();
        delete m_downloadFile;
        m_downloadFile = nullptr;
        m_installAfterDownload = false;
        fail(tr("The update package size did not match the release metadata."));
        return;
    }
    if (!m_downloadFile->commit()) {
        delete m_downloadFile;
        m_downloadFile = nullptr;
        m_installAfterDownload = false;
        fail(tr("Unable to finalize the downloaded package."));
        return;
    }
    delete m_downloadFile;
    m_downloadFile = nullptr;

#ifndef Q_OS_WIN
    if (m_update.installKind == InstallKind::LinuxAppImage)
        QFile::setPermissions(m_downloadPath,
                              QFile::permissions(m_downloadPath) | QFile::ExeUser | QFile::ExeGroup |
                                  QFile::ExeOther);
#endif

    setState(UpdateState::ReadyToInstall);
    if (m_installAfterDownload) {
        m_installAfterDownload = false;
        QTimer::singleShot(0, this, &UpdateManager::installUpdate);
    }
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

    static const QRegularExpression safeNamePattern(
        QStringLiteral("^[A-Za-z0-9][A-Za-z0-9._+-]{0,199}$"));
    if (QFileInfo(info->assetName).fileName() != info->assetName ||
        !safeNamePattern.match(info->assetName).hasMatch()) {
        *error = tr("The selected update package has an unsafe file name.");
        return false;
    }
    if (!info->downloadUrl.isValid() ||
        info->downloadUrl.scheme() != QStringLiteral("https") ||
        info->downloadUrl.host().compare(QStringLiteral("github.com"),
                                         Qt::CaseInsensitive) != 0 ||
        !info->downloadUrl.userName().isEmpty() ||
        !info->downloadUrl.password().isEmpty()) {
        *error = tr("The selected update package has an untrusted download URL.");
        return false;
    }
    if (info->sizeBytes <= 0 || info->sizeBytes > 1024LL * 1024 * 1024) {
        *error = tr("The selected update package has an invalid size.");
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
