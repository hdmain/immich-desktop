#pragma once

#include "core/AppSettings.h"
#include "core/ThumbnailCache.h"

#include <QDateTime>
#include <QHash>
#include <QImage>
#include <QObject>
#include <QPixmap>
#include <QSet>
#include <QThreadPool>
#include <QUrl>

class QNetworkAccessManager;
class QNetworkReply;
class QNetworkRequest;

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

    void testConnection();
    void loadAssets(int page = 1, int pageSize = 80);
    void loadThumbnail(const QString &assetId);
    void loadPreview(const QString &assetId);
    QUrl videoStreamUrl(const QString &assetId);

signals:
    void configurationChanged(bool configured);
    void connectionTested(bool success, const QString &message);
    void assetsLoaded(const QList<Aurora::ImmichAsset> &assets, const QString &nextPage);
    void thumbnailLoaded(const QString &assetId, const QPixmap &thumbnail);
    void previewLoaded(const QString &assetId, const QPixmap &preview);
    void imageLoadFailed(const QString &assetId, const QString &resultSize,
                         const QString &message);
    void requestFailed(const QString &operation, const QString &message);

private:
    QUrl apiUrl(const QString &path) const;
    QNetworkRequest authenticatedRequest(const QUrl &url) const;
    QString errorMessage(QNetworkReply *reply, const QByteArray &body) const;
    bool ensureConfigured(const QString &operation);
    void loadImageAsync(const QString &assetId, const QString &resultSize);
    QImage decodeImage(const QByteArray &bytes, int maximumDimension) const;
    void ensureStreamServer();

    AppSettings m_store;
    ImmichConnectionSettings m_connection;
    QNetworkAccessManager *m_network;
    ThumbnailCache m_thumbnailCache;
    QThreadPool m_imagePool;
    QSet<QString> m_pendingImages;
    VideoStreamServer *m_streamServer = nullptr;
};

} // namespace Aurora
