#pragma once

#include "core/ImmichClient.h"

#include <QDate>
#include <QHash>
#include <QList>
#include <QMap>
#include <QSet>
#include <QWidget>

class QDragEnterEvent;
class QComboBox;
class QCheckBox;
class QDateEdit;
class QParallelAnimationGroup;
class QPropertyAnimation;
class QVariantAnimation;
class QGraphicsOpacityEffect;
class QButtonGroup;
class QDragLeaveEvent;
class QDropEvent;
class QEvent;
class QHideEvent;
class QHBoxLayout;
class QLabel;
class QLineEdit;
class QPushButton;
class QResizeEvent;
class QScrollArea;
class QShowEvent;
class QStackedWidget;
class QTimer;

namespace Aurora {

class ThemeManager;
class FolderSyncService;
class MediaTile;
class MediaViewerDialog;
class SummaryTile;
class VideoHoverPreview;
class ZoomSlider;

class LibraryPage final : public QWidget {
    Q_OBJECT

public:
    LibraryPage(ThemeManager *themeManager, ImmichClient *client,
                FolderSyncService *folderSync, QWidget *parent = nullptr);
    void setCompactGrid(bool enabled);
    void setTileSize(int size);

signals:
    void personRequested(const Aurora::ImmichPerson &person);
    void placeRequested(const QString &city);
    void dateRequested(const QDate &date);
    void albumRequested(const Aurora::ImmichAlbum &album);
    void tagRequested(const Aurora::ImmichTag &tag);
    void folderRequested(const QString &path, const QString &title);

public slots:
    void showDay(const QDate &day);
    void copyAsset(const Aurora::ImmichAsset &asset);
    void downloadAsset(const Aurora::ImmichAsset &asset);
    void trashAsset(const Aurora::ImmichAsset &asset);
    void deleteAssetPermanently(const Aurora::ImmichAsset &asset);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void refresh();
    void loadMore();
    void maybeLoadMore();
    void checkForNewPhotos();
    void applySearch();
    void chooseFilesToUpload();
    void handleNewestAssetsPolled(const QList<Aurora::ImmichAsset> &assets);
    void showAssets(const QList<Aurora::ImmichAsset> &assets, const QString &nextPage,
                    const QString &query, bool fromCache);
    void showThumbnail(const QString &assetId, const QPixmap &thumbnail);
    void showThumbnailError(const QString &assetId, const QString &resultSize,
                            const QString &message);
    void showRequestError(const QString &operation, const QString &message, bool willRetry = false);
    void handleAssetInfoLoaded(const Aurora::ImmichAsset &asset);
    void handleAssetInfoFailed(const QString &assetId, const QString &message);
    void pasteFromClipboard();
    void handleAssetsDeleted(const QStringList &assetIds, bool permanent);
    void handleUploadProgress(const QString &filePath, qint64 bytesSent, qint64 bytesTotal);
    void handleAssetUploaded(const QString &filePath, const QString &assetId, bool duplicate);
    void handleDownloadProgress(const QString &assetId, qint64 bytesReceived, qint64 bytesTotal);
    void handleAssetDownloaded(const QString &assetId, const QString &destinationPath);
    void handleAssetOriginalFetched(const QString &assetId, const QByteArray &bytes,
                                    const QString &contentType);
    void handleActiveEndpointChanged(bool usingLocal, const QString &activeUrl);
    void handleOnlineChanged(bool online);
    void handleUploadQueueChanged(int pendingCount);
    void syncPendingLocalAssets();
    void handleTimelineBucketsLoaded(const QList<Aurora::TimeBucketInfo> &buckets);
    void handleTimelineBucketLoaded(const QDate &month,
                                    const QList<Aurora::ImmichAsset> &assets);
    void handleTimelineBucketFailed(const QDate &month, const QString &message);
    void showYearGrid();
    void backToLibrary();
    void backToYearGrid();

private:
    struct DaySection {
        QDate date;
        QLabel *header = nullptr;
        QList<MediaTile *> tiles;
    };

    struct YearSummary {
        int year = 0;
        int count = 0;
        QDate coverMonth;
    };

    void requestPage(int page, bool append);
    void clearTimeline();
    void layoutTimeline();
    void scheduleLayout();
    void scheduleVisibleMediaUpdate();
    void updateVisibleMedia();
    bool isTileNearViewport(const MediaTile *tile) const;
    void updateEmptyState();
    void updateAutoCheckTimer();
    void updateEndpointHint();
    void setDropHighlight(bool active);
    void enqueueUploads(const QStringList &paths);
    void cleanupPasteTemp(const QString &path);
    bool pasteClipboardFiles();
    bool pasteClipboardImage();
    bool isSearchFieldFocused() const;
    QJsonObject searchOptions() const;
    void updateSearchExpansion();
    void updateToolbarDensity();
    void resetSearchFilters();
    void animateSearchPanel(bool open);
    void updateSearchOptionsUi();
    ImmichAsset currentAssetForClipboard() const;
    void promptDownload(const Aurora::ImmichAsset &asset);
    MediaViewerDialog *openAsset(const Aurora::ImmichAsset &asset);
    ImmichAsset adjacentAsset(const QString &assetId, int step);
    void confirmAndDelete(const Aurora::ImmichAsset &asset, bool permanent);
    void removeAssetsFromTimeline(const QStringList &assetIds);
    QStringList uploadableLocalPaths(const QList<QUrl> &urls) const;
    bool isUploadableFile(const QString &path) const;
    bool handleDragEvent(QEvent *event);
    QString formatDayHeader(const QDate &date) const;
    DaySection *sectionForDate(const QDate &date);
    void appendAssetsToTimeline(const QList<ImmichAsset> &assets);
    void prependLocalAsset(const ImmichAsset &asset);
    void removeLocalAsset(const QString &assetId);
    void loadLocalThumbnail(MediaTile *tile, const ImmichAsset &asset);
    void kickTimelineBucketFetches();
    void finishTimelineBucket(const QDate &month, const QList<ImmichAsset> &assets);
    void flushReadyTimelineBuckets();
    void updateTimelineLoadingStatus();
    int remainingIndexedAssetCount() const;
    int estimateReservedTimelineHeight(int remainingAssets) const;
    bool hasMoreToLoad() const;
    void jumpToDate(const QDate &day);
    void scrollToPendingDate();

    QList<YearSummary> computeYearSummaries() const;
    void buildYearGrid();
    void layoutYearGrid();
    void clearYearGrid();
    void handleYearTileClicked(int year);
    void handleYearCoverLoaded(int year, const QList<ImmichAsset> &assets);

    void buildYearDetailUi();
    void layoutYearDetail();
    void clearYearDetail();
    void handleYearDetailBucketLoaded(int year, const QDate &month,
                                      const QList<ImmichAsset> &assets);

    ThemeManager *m_themeManager;
    ImmichClient *m_client;
    FolderSyncService *m_folderSync = nullptr;
    QStackedWidget *m_contentStack;
    QScrollArea *m_scrollArea;
    QWidget *m_timelineHost;
    QScrollArea *m_yearScrollArea;
    QWidget *m_yearGridHost;
    QScrollArea *m_yearDetailScrollArea;
    QWidget *m_yearDetailHost;
    QLabel *m_yearDetailStatus;
    QLabel *m_yearGridEmptyState;
    VideoHoverPreview *m_videoHoverPreview = nullptr;
    QLabel *m_heading = nullptr;
    QLabel *m_status;
    QLabel *m_emptyState;
    QLabel *m_dropOverlay;
    QLineEdit *m_searchField;
    QHBoxLayout *m_searchToolbarLayout = nullptr;
    QPushButton *m_searchSettingsButton = nullptr;
    QWidget *m_searchPanel = nullptr;
    QParallelAnimationGroup *m_searchPanelAnimation = nullptr;
    QPropertyAnimation *m_searchPanelHeightAnimation = nullptr;
    QPropertyAnimation *m_searchPanelFadeAnimation = nullptr;
    QGraphicsOpacityEffect *m_searchPanelOpacity = nullptr;
    QVariantAnimation *m_searchModeAnimation = nullptr;
    QButtonGroup *m_searchModeButtons = nullptr;
    QLabel *m_searchModeDescription = nullptr;
    QLabel *m_searchFilterSummary = nullptr;
    QComboBox *m_searchMode = nullptr;
    QComboBox *m_searchScope = nullptr;
    QComboBox *m_searchPerson = nullptr;
    QComboBox *m_searchMedia = nullptr;
    QLineEdit *m_searchPlace = nullptr;
    QCheckBox *m_searchDateEnabled = nullptr;
    QDateEdit *m_searchDateFrom = nullptr;
    QDateEdit *m_searchDateTo = nullptr;
    QJsonObject m_searchOptions;
    bool m_searchActive = false;
    bool m_searchActivated = false;
    bool m_resettingSearchOptions = false;
    QPushButton *m_uploadButton;
    QPushButton *m_refreshButton;
    QPushButton *m_yearsButton;
    QTimer *m_layoutTimer;
    QTimer *m_visibilityTimer;
    QTimer *m_autoCheckTimer;
    QTimer *m_searchDebounce;
    QList<DaySection> m_sections;
    QHash<QString, MediaTile *> m_tilesById;
    QSet<QString> m_favoriteRequests; // Sent from tile menus, awaiting a reply.
    QSet<QString> m_localAssetIds;
    QSet<QString> m_requestedThumbnails;
    QHash<QString, int> m_thumbnailRetries;
    QHash<QString, int> m_requestedTileImages;
    QList<ImmichAsset> m_assets;
    QList<TimeBucketInfo> m_monthBuckets;
    // Progressive index: fetch several months in parallel, append in newest-first
    // order as a contiguous prefix becomes ready, and reserve scroll space for the rest.
    QHash<QDate, QList<ImmichAsset>> m_readyBucketAssets;
    QHash<QDate, int> m_inflightBuckets; // month -> timeline epoch when requested
    int m_timelineEpoch = 0;
    int m_nextBucketToRequest = 0;
    int m_flushedBucketCount = 0;
    QDate m_pendingScrollToDate;
    QDate m_pendingDay;
    bool m_reloadOnShow = false;

    // Year grid state.
    QList<YearSummary> m_yearSummaries;
    QList<SummaryTile *> m_yearTiles;
    QHash<int, SummaryTile *> m_yearTilesByYear;
    QHash<QString, SummaryTile *> m_summaryTilesByAssetId;

    // Year-detail (month -> day breakdown) state, for one open year at a time.
    int m_openYearDetail = -1;
    int m_yearDetailExpectedMonths = 0;
    int m_yearDetailReceivedMonths = 0;
    QMap<QDate, QList<ImmichAsset>> m_yearDetailAssetsByMonth; // key: 1st of month
    QHash<QDate, int> m_pendingYearCoverRequests;   // month -> year
    QHash<QDate, int> m_pendingYearDetailRequests;  // month -> year
    QList<QWidget *> m_yearDetailWidgets; // headers + day tiles, for teardown

    QString m_nextPage;
    QString m_newestAssetId;
    QString m_searchQuery;
    QString m_copyInFlightAssetId;
    QString m_downloadInfoPendingAssetId;
    QSet<QString> m_pasteTempFiles;
    ImmichAsset m_currentAsset;
    int m_uploadsCompleted = 0;
    int m_uploadsFailed = 0;
    int m_uploadsTotal = 0;
    bool m_loading = false;
    bool m_appendRequest = false;
    bool m_autoRefreshPending = false;
    bool m_dropActive = false;
    bool m_showingCached = false;
    bool m_compactGrid = false;
    int m_tileSize = 180;
    ZoomSlider *m_zoomSlider = nullptr;
};

} // namespace Aurora
