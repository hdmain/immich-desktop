#pragma once

#include "core/AppSettings.h"
#include "core/ImmichTypes.h"
#include "core/OfflineStore.h"
#include "core/ThumbnailCache.h"
#include "core/UploadQueueStore.h"

#include <QDateTime>
#include <QHash>
#include <QImage>
#include <QObject>
#include <QPixmap>
#include <QSet>
#include <QStringList>
#include <QThreadPool>
#include <QUrl>

class QNetworkAccessManager;
class QNetworkInformation;
class QNetworkReply;
class QNetworkRequest;
class QTimer;

namespace Aurora {

class VideoStreamServer;

class ImmichClient final : public QObject {
    Q_OBJECT

public:
    explicit ImmichClient(QObject *parent = nullptr);
    ~ImmichClient() override;

    ImmichConnectionSettings connection() const;
    bool isConfigured() const;
    void setConnection(const ImmichConnectionSettings &connection, bool persist = true);

    QString activeServerUrl() const;
    bool usingLocalEndpoint() const;
    bool isOnline() const;
    bool isOffline() const { return !isOnline(); }

    void testConnection();
    void loadAssets(int page = 1, int pageSize = 80, const QString &query = {});
    void pollNewestAssets(int pageSize = 20);
    void loadExplore();
    void loadAssetsForPerson(const QString &personId, int page = 1, int pageSize = 80);
    void loadAssetsForCity(const QString &city, int page = 1, int pageSize = 80);
    void loadThumbnail(const QString &assetId);
    void loadPreview(const QString &assetId);
    void loadPersonThumbnail(const QString &personId);
    void uploadAssets(const QStringList &filePaths);
    void downloadAsset(const QString &assetId, const QString &destinationPath,
                       const QString &suggestedFileName = {});
    void fetchAssetOriginal(const QString &assetId);
    void deleteAssets(const QStringList &assetIds, bool permanent = false);
    bool isUploading() const;
    int pendingUploadCount() const;
    QUrl videoStreamUrl(const QString &assetId);

signals:
    void configurationChanged(bool configured);
    void activeEndpointChanged(bool usingLocal, const QString &activeUrl);
    void onlineChanged(bool online);
    void connectionTested(bool success, const QString &message);
    void assetsLoaded(const QList<Aurora::ImmichAsset> &assets, const QString &nextPage,
                      const QString &query, bool fromCache = false);
    void newestAssetsPolled(const QList<Aurora::ImmichAsset> &assets);
    void exploreLoaded(const Aurora::ImmichExploreData &data, bool fromCache = false);
    void filteredAssetsLoaded(const QString &filterKind, const QString &filterValue,
                              const QList<Aurora::ImmichAsset> &assets, const QString &nextPage);
    void thumbnailLoaded(const QString &assetId, const QPixmap &thumbnail);
    void previewLoaded(const QString &assetId, const QPixmap &preview);
    void personThumbnailLoaded(const QString &personId, const QPixmap &thumbnail);
    void imageLoadFailed(const QString &assetId, const QString &resultSize,
                         const QString &message);
    void uploadProgress(const QString &filePath, qint64 bytesSent, qint64 bytesTotal);
    void assetUploaded(const QString &filePath, const QString &assetId, bool duplicate);
    void uploadQueueChanged(int pendingCount);
    void downloadProgress(const QString &assetId, qint64 bytesReceived, qint64 bytesTotal);
    void assetDownloaded(const QString &assetId, const QString &destinationPath);
    void assetOriginalFetched(const QString &assetId, const QByteArray &bytes,
                              const QString &contentType);
    void assetsDeleted(const QStringList &assetIds, bool permanent);
    void requestFailed(const QString &operation, const QString &message);

private:
    static QString normalizeServerUrl(QString url);
    static QUrl apiUrlForBase(const QString &serverUrl, const QString &path);
    static bool isTransientNetworkError(QNetworkReply *reply);

    QUrl apiUrl(const QString &path) const;
    QNetworkRequest authenticatedRequest(const QUrl &url) const;
    QString errorMessage(QNetworkReply *reply, const QByteArray &body) const;
    bool ensureConfigured(const QString &operation);
    void searchAssets(int page, int pageSize, bool pollOnly, const QString &query = {});
    void searchFilteredAssets(const QString &filterKind, const QString &filterValue,
                              int page, int pageSize);
    void loadImageAsync(const QString &assetId, const QString &resultSize);
    void finishExploreLoad(bool peopleDone, bool exploreDone, const QString &error);
    bool emitCachedLibrary(const QString &query);
    bool emitCachedExplore();
    QImage decodeImage(const QByteArray &bytes, int maximumDimension) const;
    void ensureStreamServer();
    void processUploadQueue();
    bool startUpload(const QString &filePath);
    void persistUploadQueue();
    void restoreUploadQueue();
    void requeueUpload(const QString &filePath, bool toFront = true);
    void scheduleUploadRetry(int delayMs = 1500);
    void probeEndpoints();
    void probeReachability();
    void setOnline(bool online);
    void setActiveServerUrl(const QString &url, bool usingLocal);
    void finishConnectionTest();
    void setupNetworkMonitoring();

    AppSettings m_store;
    ImmichConnectionSettings m_connection;
    QString m_activeServerUrl;
    QNetworkAccessManager *m_network;
    ThumbnailCache m_thumbnailCache;
    OfflineStore m_offlineStore;
    UploadQueueStore m_uploadQueueStore;
    QThreadPool m_imagePool;
    QTimer *m_endpointProbeTimer;
    QTimer *m_reachabilityTimer;
    QSet<QString> m_pendingImages;
    QSet<QString> m_pendingPersonImages;
    QStringList m_uploadQueue;
    QString m_uploadInFlightPath;
    QHash<QString, int> m_uploadRetryCounts;
    VideoStreamServer *m_streamServer = nullptr;
    ImmichExploreData m_exploreBuffer;
    bool m_usingLocalEndpoint = false;
    bool m_online = true;
    bool m_pollInFlight = false;
    bool m_uploadInFlight = false;
    bool m_processingUploadQueue = false;
    bool m_uploadRetryScheduled = false;
    bool m_endpointProbeInFlight = false;
    bool m_reachabilityProbeInFlight = false;
    bool m_connectionTestPending = false;
    bool m_explorePeoplePending = false;
    bool m_exploreDataPending = false;
};

} // namespace Aurora
