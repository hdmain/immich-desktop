#pragma once

#include "core/ImmichClient.h"

#include <QDate>
#include <QHash>
#include <QList>
#include <QSet>
#include <QWidget>

class QDragEnterEvent;
class QDragLeaveEvent;
class QDropEvent;
class QEvent;
class QHideEvent;
class QLabel;
class QLineEdit;
class QPushButton;
class QResizeEvent;
class QScrollArea;
class QShowEvent;
class QTimer;

namespace Aurora {

class MediaTile;
class VideoHoverPreview;

class LibraryPage final : public QWidget {
    Q_OBJECT

public:
    explicit LibraryPage(ImmichClient *client, QWidget *parent = nullptr);

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
    void showRequestError(const QString &operation, const QString &message);
    void openAsset(const Aurora::ImmichAsset &asset);
    void downloadAsset(const Aurora::ImmichAsset &asset);
    void copyAsset(const Aurora::ImmichAsset &asset);
    void pasteFromClipboard();
    void trashAsset(const Aurora::ImmichAsset &asset);
    void deleteAssetPermanently(const Aurora::ImmichAsset &asset);
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

private:
    struct DaySection {
        QDate date;
        QLabel *header = nullptr;
        QList<MediaTile *> tiles;
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
    ImmichAsset currentAssetForClipboard() const;
    void confirmAndDelete(const Aurora::ImmichAsset &asset, bool permanent);
    void removeAssetsFromTimeline(const QStringList &assetIds);
    QStringList uploadableLocalPaths(const QList<QUrl> &urls) const;
    bool isUploadableFile(const QString &path) const;
    bool handleDragEvent(QEvent *event);
    QString formatDayHeader(const QDate &date) const;
    DaySection *sectionForDate(const QDate &date);

    ImmichClient *m_client;
    QScrollArea *m_scrollArea;
    QWidget *m_timelineHost;
    VideoHoverPreview *m_videoHoverPreview = nullptr;
    QLabel *m_status;
    QLabel *m_emptyState;
    QLabel *m_dropOverlay;
    QLineEdit *m_searchField;
    QPushButton *m_uploadButton;
    QPushButton *m_refreshButton;
    QTimer *m_layoutTimer;
    QTimer *m_visibilityTimer;
    QTimer *m_autoCheckTimer;
    QTimer *m_searchDebounce;
    QList<DaySection> m_sections;
    QHash<QString, MediaTile *> m_tilesById;
    QSet<QString> m_requestedThumbnails;
    QList<ImmichAsset> m_assets;
    QString m_nextPage;
    QString m_newestAssetId;
    QString m_searchQuery;
    QString m_copyInFlightAssetId;
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
};

} // namespace Aurora
