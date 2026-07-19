#pragma once

#include "core/AppSettings.h"
#include "core/ThumbnailCache.h"

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
class QNetworkReply;
class QNetworkRequest;
class QTimer;

namespace Aurora {

class VideoStreamServer;

struct ImmichAsset {
    QString id;
    QString type;
    QString fileName;
    QString duration;
    QDateTime takenAt;
    qreal aspectRatio = 1.0;
    bool favorite = false;

    bool isVideo() const { return type.compare(QStringLiteral("VIDEO"), Qt::CaseInsensitive) == 0; }
};

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

    void testConnection();
    void loadAssets(int page = 1, int pageSize = 80, const QString &query = {});
    void pollNewestAssets(int pageSize = 20);
    void loadThumbnail(const QString &assetId);
    void loadPreview(const QString &assetId);
    void uploadAssets(const QStringList &filePaths);
    void downloadAsset(const QString &assetId, const QString &destinationPath,
                       const QString &suggestedFileName = {});
    void deleteAssets(const QStringList &assetIds, bool permanent = false);
    bool isUploading() const;
    int pendingUploadCount() const;
    QUrl videoStreamUrl(const QString &assetId);

signals:
    void configurationChanged(bool configured);
    void activeEndpointChanged(bool usingLocal, const QString &activeUrl);
    void connectionTested(bool success, const QString &message);
    void assetsLoaded(const QList<Aurora::ImmichAsset> &assets, const QString &nextPage,
                      const QString &query);
    void newestAssetsPolled(const QList<Aurora::ImmichAsset> &assets);
    void thumbnailLoaded(const QString &assetId, const QPixmap &thumbnail);
    void previewLoaded(const QString &assetId, const QPixmap &preview);
    void imageLoadFailed(const QString &assetId, const QString &resultSize,
                         const QString &message);
    void uploadProgress(const QString &filePath, qint64 bytesSent, qint64 bytesTotal);
    void assetUploaded(const QString &filePath, const QString &assetId, bool duplicate);
    void downloadProgress(const QString &assetId, qint64 bytesReceived, qint64 bytesTotal);
    void assetDownloaded(const QString &assetId, const QString &destinationPath);
    void assetsDeleted(const QStringList &assetIds, bool permanent);
    void requestFailed(const QString &operation, const QString &message);

private:
    static QString normalizeServerUrl(QString url);
    static QUrl apiUrlForBase(const QString &serverUrl, const QString &path);

    QUrl apiUrl(const QString &path) const;
    QNetworkRequest authenticatedRequest(const QUrl &url) const;
    QString errorMessage(QNetworkReply *reply, const QByteArray &body) const;
    bool ensureConfigured(const QString &operation);
    void searchAssets(int page, int pageSize, bool pollOnly, const QString &query = {});
    void loadImageAsync(const QString &assetId, const QString &resultSize);
    QImage decodeImage(const QByteArray &bytes, int maximumDimension) const;
    void ensureStreamServer();
    void processUploadQueue();
    void startUpload(const QString &filePath);
    void probeEndpoints();
    void setActiveServerUrl(const QString &url, bool usingLocal);
    void finishConnectionTest();

    AppSettings m_store;
    ImmichConnectionSettings m_connection;
    QString m_activeServerUrl;
    QNetworkAccessManager *m_network;
    ThumbnailCache m_thumbnailCache;
    QThreadPool m_imagePool;
    QTimer *m_endpointProbeTimer;
    QSet<QString> m_pendingImages;
    QStringList m_uploadQueue;
    VideoStreamServer *m_streamServer = nullptr;
    bool m_usingLocalEndpoint = false;
    bool m_pollInFlight = false;
    bool m_uploadInFlight = false;
    bool m_endpointProbeInFlight = false;
    bool m_connectionTestPending = false;
};

} // namespace Aurora
