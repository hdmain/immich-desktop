#pragma once

#include "core/AppSettings.h"
#include "core/ImmichTypes.h"
#include "core/OfflineStore.h"
#include "core/ThumbnailCache.h"
#include "core/UploadQueueStore.h"

#include <QDate>
#include <QDateTime>
#include <QHash>
#include <QImage>
#include <QJsonObject>
#include <QObject>
#include <QPixmap>
#include <QSet>
#include <QSslConfiguration>
#include <QStringList>
#include <QThreadPool>
#include <QVersionNumber>
#include <QUrl>
#include <QUuid>

#include <functional>

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
    // False until the server's version is known.
    bool serverAtLeast(int major, int minor) const;
    bool isOffline() const { return !isOnline(); }

    void testConnection();
    void loadAssets(int page = 1, int pageSize = 80, const QString &query = {},
                    const QJsonObject &searchOptions = {});
    void loadTimelineBuckets(const AssetSort &sort = {});
    void loadTimelineBucket(const QDate &month, const AssetSort &sort = {});
    void clearCachedLibrary();
    void pollNewestAssets(int pageSize = 20);
    bool loadExplore();
    void loadAssetsForFolder(const QString &path, const AssetSort &sort = {});
    // kind: person, city, album, tag, folder, favorites, archive or trash.
    // Pages are opaque: pass back the nextPage that filteredAssetsLoaded reported.
    void loadCollectionAssets(const QString &kind, const QString &value, int page = 1,
                              const AssetSort &sort = {});
    // The order fields for a /search/metadata body. Size and rating use Immich
    // 3.2's orderBy; dates use the older order field every server understands.
    static QJsonObject searchOrder(const AssetSort &sort);
    // Collections that can be ordered by date added (via the timeline).
    static bool canSortByDateAdded(const QString &kind);
    // A random batch from a collection ("library" for the whole timeline), for
    // Shuffle. Replies with randomAssetsLoaded; folders and the view history come
    // whole, so they have no more to give.
    void loadRandomAssets(const QString &kind, const QString &value);
    // Photos opened in the viewer, kept on this computer only (Immich doesn't
    // record views). Most recent first; also the "viewed" collection.
    void recordViewed(const ImmichAsset &asset);
    QList<ImmichAsset> recentlyViewed() const;
    void clearRecentlyViewed();
    // "Last viewed" order for the Library: viewed photos first, then the rest by
    // date taken. Replies with assetsLoaded, like a library page.
    void loadLibraryViewedFirst(int page, const AssetSort &sort);
    // Off pauses recording and hides the list; what's stored stays until cleared.
    void setRememberViewed(bool remember);
    // Kept in memory: grids ask for every photo they add, and reading settings is slow.
    bool remembersViewed() const { return m_rememberViewed; }
    // Saves an album's own newest/oldest-first order in Immich (album.update).
    void setAlbumOrder(const QString &albumId, bool ascending);
    void loadAlbums();
    void loadTags();
    void loadFolderPaths();
    void loadPeople();
    // CacheOnly looks in the memory and disk caches without going to the server
    // and stays silent on a miss; grids use it before deciding to fetch.
    enum class ImageSource { CacheOrServer, CacheOnly };
    // Server fetches run newest first; OnScreen ones go ahead of all others.
    enum class FetchPriority { Normal, OnScreen };
    void loadThumbnail(const QString &assetId, ImageSource source = ImageSource::CacheOrServer,
                       FetchPriority priority = FetchPriority::Normal);
    // Walk the library and download any thumbnails missing from the disk cache.
    void prefetchAllThumbnails();
    void cancelThumbnailPrefetch();
    bool isPrefetchingThumbnails() const;
    void loadPreview(const QString &assetId);
    // Emits thumbnailLoaded with an image of up to maximumDimension pixels,
    // decoded from the preview when the thumbnail is too small.
    void loadTileImage(const QString &assetId, int maximumDimension,
                       ImageSource source = ImageSource::CacheOrServer,
                       FetchPriority priority = FetchPriority::Normal);
    void loadPersonThumbnail(const QString &personId);
    // Drop in-RAM thumbnail/preview pixmaps (disk cache kept). Call when the
    // window is hidden to tray/minimized so idle RSS stays low.
    void releaseMemoryCaches();
    void uploadAssets(const QStringList &filePaths);
    void downloadAsset(const QString &assetId, const QString &destinationPath,
                       const QString &suggestedFileName = {});
    void fetchAssetOriginal(const QString &assetId);
    void fetchAssetInfo(const QString &assetId);
    void fetchAssetDetails(const QString &assetId);
    void deleteAssets(const QStringList &assetIds, bool permanent = false);
    void setFavorite(const QStringList &assetIds, bool favorite);
    void restoreFromTrash(const QStringList &assetIds);
    void restoreAllFromTrash();
    void emptyTrash();
    void loadTrashDays();
    void createSharedLink(const QString &assetId, const QUuid &requestId, const QString &password = {});
    QUrl sharedLinkUrl(const QString &key) const;
    bool isUploading() const;
    bool isDownloading() const;
    int pendingUploadCount() const;
    int activeDownloadCount() const;
    QUrl videoStreamUrl(const QString &assetId);

public slots:
    void provideKeyPassword(const QString &password);

signals:
    void configurationChanged(bool configured);
    void serverVersionChanged();
    void recentlyViewedChanged();
    void randomAssetsLoaded(const QString &kind, const QString &value,
                            const QList<Aurora::ImmichAsset> &assets, bool more);
    void albumOrderSaved(const QString &albumId, bool ascending);
    void albumOrderFailed(const QString &albumId, const QString &message);
    void activeEndpointChanged(bool usingLocal, const QString &activeUrl);
    void onlineChanged(bool online);
    void connectionTested(bool success, const QString &message);
    void assetsLoaded(const QList<Aurora::ImmichAsset> &assets, const QString &nextPage,
                      const QString &query, bool fromCache = false);
    // The sort is the one requested, so callers can drop results of an older sort.
    void timelineBucketsLoaded(const QList<Aurora::TimeBucketInfo> &buckets,
                               const Aurora::AssetSort &sort);
    void timelineBucketLoaded(const QDate &month, const QList<Aurora::ImmichAsset> &assets,
                              const Aurora::AssetSort &sort);
    void timelineBucketFailed(const QDate &month, const QString &message,
                              const Aurora::AssetSort &sort);
    void newestAssetsPolled(const QList<Aurora::ImmichAsset> &assets);
    void exploreLoaded(const Aurora::ImmichExploreData &data, bool fromCache = false);
    void albumsLoaded(const QList<Aurora::ImmichAlbum> &albums);
    void tagsLoaded(const QList<Aurora::ImmichTag> &tags);
    void folderPathsLoaded(const QStringList &paths);
    // Every person, including those hidden in Immich (person.hidden is set).
    void peopleLoaded(const QList<Aurora::ImmichPerson> &people);
    // kind is "albums", "tags" or "folders" for the lists, "folder" for one folder's photos.
    void browseListFailed(const QString &kind, bool permissionDenied);
    void filteredAssetsLoaded(const QString &filterKind, const QString &filterValue,
                              const QList<Aurora::ImmichAsset> &assets, const QString &nextPage,
                              const Aurora::AssetSort &sort);
    void thumbnailLoaded(const QString &assetId, const QPixmap &thumbnail);
    // done = finished downloads + skips so far; total = assets seen while listing.
    void thumbnailPrefetchProgress(int done, int total, int downloading);
    void thumbnailPrefetchFinished(int downloaded, int alreadyCached);
    void previewLoaded(const QString &assetId, const QPixmap &preview);
    void personThumbnailLoaded(const QString &personId, const QPixmap &thumbnail);
    void imageLoadFailed(const QString &assetId, const QString &resultSize,
                         const QString &message);
    void uploadProgress(const QString &filePath, qint64 bytesSent, qint64 bytesTotal);
    void assetUploaded(const QString &filePath, const QString &assetId, bool duplicate);
    void assetUploadFailed(const QString &filePath, const QString &message);
    void uploadQueueChanged(int pendingCount);
    void downloadProgress(const QString &assetId, qint64 bytesReceived, qint64 bytesTotal);
    void assetDownloaded(const QString &assetId, const QString &destinationPath);
    void assetInfoLoaded(const Aurora::ImmichAsset &asset);
    void assetInfoFailed(const QString &assetId, const QString &message);
    void assetDetailsLoaded(const Aurora::ImmichAssetDetails &details);
    void assetDetailsFailed(const QString &assetId, const QString &message);
    void assetAlbumsLoaded(const QString &assetId, const QList<Aurora::ImmichAlbum> &albums);
    void assetAlbumsFailed(const QString &assetId, bool permissionDenied);
    void assetOriginalFetched(const QString &assetId, const QByteArray &bytes,
                              const QString &contentType);
    void assetsDeleted(const QStringList &assetIds, bool permanent);
    void favoritesChanged(const QStringList &assetIds, bool favorite);
    void favoriteFailed(const QStringList &assetIds, const QString &message);
    // assetIds is empty when everything in the trash was restored.
    void assetsRestored(const QStringList &assetIds);
    void trashEmptied();
    void trashDaysLoaded(int days);
    void sharedLinkCreated(const QUuid &requestId, const QUrl &url);
    void sharedLinkFailed(const QUuid &requestId, const QString &message);
    void requestFailed(const QString &operation, const QString &message, bool willRetry = false);
    void transferActivityChanged();
    void sslPasswordNeeded();

private:
    void loadServerVersion();
    static QString normalizeServerUrl(QString url);
    static QUrl apiUrlForBase(const QString &serverUrl, const QString &path);
    static bool isTransientNetworkError(QNetworkReply *reply);

    QUrl apiUrl(const QString &path) const;
    QNetworkRequest authenticatedRequest(const QUrl &url) const;
    QString errorMessage(QNetworkReply *reply, const QByteArray &body) const;
    bool ensureConfigured(const QString &operation);
    void searchAssets(int page, int pageSize, bool pollOnly, const QString &query = {},
                      const QJsonObject &searchOptions = {});
    void searchFilteredAssets(const QString &filterKind, const QString &filterValue,
                              int page, int pageSize, const AssetSort &sort);
    void searchAllPages(const QJsonObject &filter,
                        std::function<void(const QList<ImmichAsset> &)> done);
    void loadAlbumByActivity(const QString &albumId, const AssetSort &sort);
    void loadTrashByDeletion(const AssetSort &sort);
    void searchViewedFirst(const QJsonObject &flatFilter, int page, const AssetSort &sort,
                           std::function<void(const QList<ImmichAsset> &, const QString &)> done);
    void loadCollectionTimeline(const QString &kind, const QString &value, int page,
                                const AssetSort &sort);
    void loadCollectionTimelinePage(int firstBucket);
    void loadImageAsync(const QString &assetId, const QString &resultSize,
                        ImageSource source = ImageSource::CacheOrServer,
                        FetchPriority fetchPriority = FetchPriority::Normal);
    void prefetchLibraryPage(int page);
    void queuePrefetchThumbnail(const QString &assetId);
    void notePrefetchThumbnailDone(const QString &assetId, bool downloaded);
    void emitPrefetchProgress();
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
    void beginActiveDownload();
    void endActiveDownload();
    void probeEndpoints();
    void probeReachability();
    void setOnline(bool online);
    void setActiveServerUrl(const QString &url, bool usingLocal);
    void finishConnectionTest();
    void requestAssetDetails(const QString &assetId);
    void postTrashAction(const QString &path, const QJsonObject &body,
                         std::function<void()> done);
    void fetchBrowseList(const QString &kind, const QUrl &url,
                         std::function<void(const QByteArray &)> handle);
    void setupNetworkMonitoring();
    void queueLibraryCache(const QList<ImmichAsset> &assets);
    void flushLibraryCache();
    void rebuildSslConfig();
    QString effectivePassword() const;
    QSslConfiguration sslConfigForActiveEndpoint() const;

    AppSettings m_store;
    ImmichConnectionSettings m_connection;
    QString m_currentUserId;
    QString m_activeServerUrl;
    QSslConfiguration m_sslConfig;
    QString m_sessionKeyPassword;
    QNetworkAccessManager *m_network;
    ThumbnailCache m_thumbnailCache;
    ThumbnailCache m_previewCache{QStringLiteral("previews"), 8 * 1024,
                                  512LL * 1024 * 1024};
    OfflineStore m_offlineStore;
    UploadQueueStore m_uploadQueueStore;
    QThreadPool m_imagePool;
    QTimer *m_endpointProbeTimer;
    QTimer *m_reachabilityTimer;
    // Offline-cache merges are batched: rewriting the whole cache for each of
    // hundreds of timeline months made Library loading quadratic.
    QTimer *m_cacheFlushTimer;
    QList<ImmichAsset> m_pendingCacheAssets;
    QHash<QString, int> m_pendingImages; // Key -> priority of the queued job.
    int m_imageRequestSerial = 0; // Later server fetches run first.
    QSet<QString> m_pendingPersonImages;
    QStringList m_uploadQueue;
    QString m_uploadInFlightPath;
    QHash<QString, int> m_uploadRetryCounts;
    VideoStreamServer *m_streamServer = nullptr;
    ImmichExploreData m_exploreBuffer;
    bool m_usingLocalEndpoint = false;
    bool m_online = false;
    bool m_pollInFlight = false;
    quint64 m_searchGeneration = 0;
    QVersionNumber m_serverVersion;
    bool m_rememberViewed = true;
    quint64 m_collectionGeneration = 0; // Bumped when a collection starts loading.
    // Next-page cursors for size/rating searches ("library", "collection"); both
    // views request pages one after another, so the latest cursor is the next.
    QHash<QString, QString> m_searchCursors;
    // The collection being loaded month by month for a date-added sort.
    struct CollectionTimeline {
        QString kind;
        QString value;
        AssetSort sort;
        QList<TimeBucketInfo> buckets;
        quint64 generation = 0;
    } m_collectionTimeline;
    bool m_uploadInFlight = false;
    bool m_processingUploadQueue = false;
    bool m_uploadRetryScheduled = false;
    int m_activeDownloads = 0;
    bool m_endpointProbeInFlight = false;
    bool m_reachabilityProbeInFlight = false;
    bool m_connectionTestPending = false;
    bool m_explorePeoplePending = false;
    bool m_exploreDataPending = false;
    QString m_exploreError;
    bool m_prefetchActive = false;
    bool m_prefetchListingDone = false;
    int m_prefetchTotal = 0;
    int m_prefetchDownloaded = 0;
    int m_prefetchSkipped = 0;
    QSet<QString> m_prefetchPending;
};

} // namespace Aurora
