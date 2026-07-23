#pragma once

#include "core/AppSettings.h"

#include <QObject>
#include <QUrl>

class QNetworkAccessManager;
class QNetworkReply;
class QSaveFile;

namespace Aurora {

enum class UpdateState {
    Idle,
    Checking,
    UpToDate,
    Available,
    Downloading,
    ReadyToInstall,
    Installing,
    Failed
};

enum class InstallKind {
    Unknown,
    WindowsExe,
    WindowsMsi,
    LinuxDeb,
    LinuxAppImage,
    LinuxSnap
};

struct UpdateInfo {
    QString version;
    QString tagName;
    QString releaseNotes;
    QString assetName;
    QUrl downloadUrl;
    qint64 sizeBytes = 0;
    InstallKind installKind = InstallKind::Unknown;
};

class UpdateManager final : public QObject {
    Q_OBJECT

public:
    explicit UpdateManager(QObject *parent = nullptr);
    ~UpdateManager() override;

    UpdateState state() const;
    UpdateInfo availableUpdate() const;
    QString errorMessage() const;
    QString downloadedPackagePath() const;
    UpdateSettings settings() const;

    void setAutoCheckEnabled(bool enabled);
    void skipCurrentUpdate();
    void checkForUpdates(bool silent = false);
    void downloadUpdate();
    void installUpdate();
    void applyUpdate();
    bool shouldAutoCheck() const;

signals:
    void stateChanged(Aurora::UpdateState state);
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void updateAvailable(const Aurora::UpdateInfo &info);
    void upToDate();
    void errorOccurred(const QString &message);
    void installStarted();

private:
    void setState(UpdateState state);
    void fail(const QString &message);
    void handleReleaseReply();
    void handleDownloadReadyRead();
    void handleDownloadFinished();
    bool parseReleasePayload(const QByteArray &payload, UpdateInfo *info, QString *error) const;
    InstallKind preferredInstallKind() const;
    bool assetMatches(const QString &name, InstallKind kind) const;
    bool launchInstaller(const QString &packagePath, InstallKind kind);

    AppSettings m_settingsStore;
    QNetworkAccessManager *m_network;
    QNetworkReply *m_activeReply = nullptr;
    QSaveFile *m_downloadFile = nullptr;
    UpdateState m_state = UpdateState::Idle;
    UpdateInfo m_update;
    QString m_error;
    QString m_downloadPath;
    bool m_silentCheck = false;
    bool m_installAfterDownload = false;
};

} // namespace Aurora
