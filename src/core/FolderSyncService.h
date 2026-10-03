#pragma once

#include "core/AppSettings.h"
#include "core/ImmichTypes.h"

#include <QHash>
#include <QList>
#include <QObject>
#include <QSet>
#include <QString>

class QFileSystemWatcher;
class QTimer;

namespace Aurora {

class ImmichClient;

enum class FolderSyncUiStatus {
    Disabled,
    Unsaved,
    Saving,
    Saved,
    Error,
};

class FolderSyncService final : public QObject {
    Q_OBJECT

public:
    explicit FolderSyncService(ImmichClient *client, QObject *parent = nullptr);

    FolderSyncSettings settings() const;
    void setSettings(const FolderSyncSettings &settings);

    FolderSyncUiStatus uiStatus() const;
    QString statusMessage() const;
    QString statusIconPath() const;

    // Unsynced folder files for Library preview (with Unsaved/Saving/Error badge).
    QList<ImmichAsset> pendingLibraryAssets() const;

    void scanNow();

signals:
    void settingsChanged(const Aurora::FolderSyncSettings &settings);
    void statusChanged();
    void pendingLibraryAssetsChanged();

private slots:
    void handleDirectoryChanged(const QString &path);
    void handleUploadProgress(const QString &filePath, qint64 bytesSent, qint64 bytesTotal);
    void handleAssetUploaded(const QString &filePath, const QString &assetId, bool duplicate);
    void handleUploadFailed(const QString &filePath, const QString &message);
    void handleOnlineChanged(bool online);
    void handleConfigurationChanged(bool configured);
    void handleActiveEndpointChanged(bool usingLocal, const QString &activeUrl);
    void processStableFiles();
    void periodicScan();

private:
    struct PendingFile {
        qint64 size = -1;
        qint64 stableSinceMs = 0;
    };

    static bool isMediaFile(const QString &path);
    static bool isVideoFile(const QString &path);
    static QString fingerprint(const QString &path);
    static QString localAssetId(const QString &path);
    static ImmichAsset assetFromLocalFile(const QString &path);
    bool isUnderSyncFolder(const QString &path) const;
    bool alreadySynced(const QString &path) const;
    bool canUploadNow() const;
    void markSynced(const QString &path);
    void markError(const QString &path, const QString &message);
    void loadSyncedState();
    void saveSyncedState() const;
    void rebuildWatcher();
    void enqueueReadyFiles(const QStringList &paths);
    void refreshStatus();
    void setStatus(FolderSyncUiStatus status, const QString &message);
    void publishLibraryPending();
    QStringList collectMediaFiles() const;

    ImmichClient *m_client = nullptr;
    AppSettings m_store;
    FolderSyncSettings m_settings;
    QFileSystemWatcher *m_watcher = nullptr;
    QTimer *m_scanTimer = nullptr;
    QTimer *m_stabilityTimer = nullptr;
    QHash<QString, PendingFile> m_pending;
    QSet<QString> m_queued;
    QHash<QString, QString> m_syncedFingerprints;
    QHash<QString, QString> m_errors;
    QList<ImmichAsset> m_libraryPending;
    FolderSyncUiStatus m_uiStatus = FolderSyncUiStatus::Disabled;
    QString m_statusMessage;
    QString m_lastUploadName;
};

} // namespace Aurora
