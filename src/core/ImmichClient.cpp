#include "core/ImmichClient.h"

#include "core/VideoStreamServer.h"

#include <QBuffer>
#include <QEventLoop>
#include <QImage>
#include <QImageReader>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPointer>
#include <QUrl>
#include <QUrlQuery>

namespace Aurora {
namespace {

qreal aspectRatioFromObject(const QJsonObject &object)
{
    const QJsonObject exif = object.value(QStringLiteral("exifInfo")).toObject();
    const qreal width = exif.value(QStringLiteral("exifImageWidth")).toDouble(
        object.value(QStringLiteral("exifImageWidth")).toDouble());
    const qreal height = exif.value(QStringLiteral("exifImageHeight")).toDouble(
        object.value(QStringLiteral("exifImageHeight")).toDouble());
    if (width > 1.0 && height > 1.0)
        return qBound(0.2, width / height, 8.0);
    return 1.0;
}

} // namespace

ImmichClient::ImmichClient(QObject *parent)
    : QObject(parent)
    , m_connection(m_store.loadImmichConnection())
    , m_network(new QNetworkAccessManager(this))
{
    m_imagePool.setMaxThreadCount(4);
}

ImmichClient::~ImmichClient()
{
    m_imagePool.waitForDone();
}

ImmichConnectionSettings ImmichClient::connection() const
{
    return m_connection;
}

bool ImmichClient::isConfigured() const
{
    return m_connection.isConfigured();
}

void ImmichClient::setConnection(const ImmichConnectionSettings &connection, bool persist)
{
    ImmichConnectionSettings normalized = connection;
    normalized.serverUrl = normalized.serverUrl.trimmed();
    normalized.apiKey = normalized.apiKey.trimmed();
    while (normalized.serverUrl.endsWith(u'/'))
        normalized.serverUrl.chop(1);

    const bool changed = normalized.serverUrl != m_connection.serverUrl ||
                         normalized.apiKey != m_connection.apiKey;
    m_connection = normalized;
    if (persist)
        m_store.saveImmichConnection(m_connection);
    if (changed) {
        m_thumbnailCache.clearMemory();
        if (m_streamServer)
            ensureStreamServer();
        emit configurationChanged(isConfigured());
    }
}

void ImmichClient::ensureStreamServer()
{
    if (!m_streamServer)
        m_streamServer = new VideoStreamServer(this);
    if (!m_streamServer->start()) {
        emit requestFailed(tr("Video stream"),
                           tr("Could not start the local video stream proxy."));
        return;
    }

    QUrl apiBase(m_connection.serverUrl);
    QString basePath = apiBase.path();
    while (basePath.endsWith(u'/'))
        basePath.chop(1);
    if (!basePath.endsWith(QStringLiteral("/api")))
        basePath += QStringLiteral("/api");
    apiBase.setPath(basePath);
    apiBase.setQuery(QString());
    apiBase.setFragment({});
    m_streamServer->setCredentials(apiBase, m_connection.apiKey);
}

QUrl ImmichClient::videoStreamUrl(const QString &assetId)
{
    if (!ensureConfigured(tr("Play video")) || assetId.isEmpty())
        return {};
    ensureStreamServer();
    if (!m_streamServer || !m_streamServer->start())
        return {};
    return m_streamServer->streamUrl(assetId);
}

QUrl ImmichClient::apiUrl(const QString &path) const
{
    QUrl url(m_connection.serverUrl);
    QString basePath = url.path();
    while (basePath.endsWith(u'/'))
        basePath.chop(1);
    if (!basePath.endsWith(QStringLiteral("/api")))
        basePath += QStringLiteral("/api");

    QString endpoint = path;
    if (!endpoint.startsWith(u'/'))
        endpoint.prepend(u'/');
    url.setPath(basePath + endpoint);
    url.setQuery(QString());
    url.setFragment({});
    return url;
}

QNetworkRequest ImmichClient::authenticatedRequest(const QUrl &url) const
{
    QNetworkRequest result(url);
    result.setRawHeader("Accept", "*/*");
    result.setRawHeader("x-api-key", m_connection.apiKey.toUtf8());
    result.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                        QNetworkRequest::NoLessSafeRedirectPolicy);
    return result;
}

bool ImmichClient::ensureConfigured(const QString &operation)
{
    if (isConfigured())
        return true;
    emit requestFailed(operation, tr("Set the Immich server URL and API key in Settings first."));
    return false;
}

QString ImmichClient::errorMessage(QNetworkReply *reply, const QByteArray &body) const
{
    const QJsonDocument document = QJsonDocument::fromJson(body);
    if (document.isObject()) {
        const QJsonValue message = document.object().value(QStringLiteral("message"));
        if (message.isString() && !message.toString().isEmpty())
            return message.toString();
        if (message.isArray()) {
            QStringList messages;
            for (const auto &entry : message.toArray())
                messages.append(entry.toString());
            if (!messages.isEmpty())
                return messages.join(QStringLiteral(", "));
        }
    }
    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 401)
        return tr("Authentication failed. Check the API key and its permissions.");
    return reply->errorString();
}

void ImmichClient::testConnection()
{
    if (!ensureConfigured(tr("Connection test"))) {
        emit connectionTested(false, tr("Server URL and API key are required."));
        return;
    }

    auto *reply = m_network->get(authenticatedRequest(apiUrl(QStringLiteral("/users/me"))));
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        const QByteArray body = reply->readAll();
        if (reply->error() != QNetworkReply::NoError) {
            emit connectionTested(false, errorMessage(reply, body));
        } else {
            const QJsonObject user = QJsonDocument::fromJson(body).object();
            const QString identity = user.value(QStringLiteral("name")).toString(
                user.value(QStringLiteral("email")).toString());
            emit connectionTested(
                true, identity.isEmpty()
                          ? tr("Connected to Immich.")
                          : tr("Connected as %1.").arg(identity));
        }
        reply->deleteLater();
    });
}

void ImmichClient::loadAssets(int page, int pageSize)
{
    if (!ensureConfigured(tr("Load library")))
        return;

    QJsonObject body;
    body.insert(QStringLiteral("page"), qMax(1, page));
    body.insert(QStringLiteral("size"), qBound(1, pageSize, 250));
    body.insert(QStringLiteral("order"), QStringLiteral("desc"));
    body.insert(QStringLiteral("withExif"), true);
    body.insert(QStringLiteral("withStacked"), true);

    QNetworkRequest searchRequest = authenticatedRequest(apiUrl(QStringLiteral("/search/metadata")));
    searchRequest.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    searchRequest.setRawHeader("Accept", "application/json");
    auto *reply = m_network->post(searchRequest, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        const QByteArray body = reply->readAll();
        if (reply->error() != QNetworkReply::NoError) {
            emit requestFailed(tr("Load library"), errorMessage(reply, body));
            reply->deleteLater();
            return;
        }

        const QJsonObject assetsObject =
            QJsonDocument::fromJson(body).object().value(QStringLiteral("assets")).toObject();
        QList<ImmichAsset> assets;
        const QJsonArray items = assetsObject.value(QStringLiteral("items")).toArray();
        assets.reserve(items.size());
        for (const QJsonValue &value : items) {
            const QJsonObject object = value.toObject();
            ImmichAsset asset;
            asset.id = object.value(QStringLiteral("id")).toString();
            asset.type = object.value(QStringLiteral("type")).toString();
            asset.fileName = object.value(QStringLiteral("originalFileName")).toString();
            asset.duration = object.value(QStringLiteral("duration")).toString();
            asset.favorite = object.value(QStringLiteral("isFavorite")).toBool();
            asset.aspectRatio = aspectRatioFromObject(object);
            const QString takenAt = object.value(QStringLiteral("localDateTime")).toString(
                object.value(QStringLiteral("fileCreatedAt")).toString());
            asset.takenAt = QDateTime::fromString(takenAt, Qt::ISODate);
            if (!asset.id.isEmpty())
                assets.append(asset);
        }
        emit assetsLoaded(assets, assetsObject.value(QStringLiteral("nextPage")).toVariant().toString());
        reply->deleteLater();
    });
}

QImage ImmichClient::decodeImage(const QByteArray &bytes) const
{
    QBuffer buffer;
    buffer.setData(bytes);
    buffer.open(QIODevice::ReadOnly);
    QImageReader reader(&buffer);
    reader.setAutoTransform(true);
    return reader.read();
}

void ImmichClient::loadThumbnail(const QString &assetId)
{
    if (!ensureConfigured(tr("Load image")) || assetId.isEmpty())
        return;

    if (const QPixmap cached = m_thumbnailCache.memoryPixmap(assetId); !cached.isNull()) {
        emit thumbnailLoaded(assetId, cached);
        return;
    }

    loadImageAsync(assetId, QStringLiteral("thumbnail"));
}

void ImmichClient::loadPreview(const QString &assetId)
{
    loadImageAsync(assetId, QStringLiteral("preview"));
}

void ImmichClient::loadImageAsync(const QString &assetId, const QString &resultSize)
{
    if (!ensureConfigured(tr("Load image")) || assetId.isEmpty())
        return;

    const QString pendingKey = resultSize + u':' + assetId;
    if (m_pendingImages.contains(pendingKey))
        return;
    m_pendingImages.insert(pendingKey);

    const ImmichConnectionSettings connection = m_connection;
    QPointer<ImmichClient> self(this);

    m_imagePool.start([this, self, assetId, resultSize, pendingKey, connection] {
        auto finishFail = [self, assetId, resultSize, pendingKey](const QString &message) {
            if (!self)
                return;
            QMetaObject::invokeMethod(self, [self, assetId, resultSize, pendingKey, message] {
                if (!self)
                    return;
                self->m_pendingImages.remove(pendingKey);
                emit self->imageLoadFailed(assetId, resultSize, message);
                emit self->requestFailed(self->tr("Load image"), message);
            }, Qt::QueuedConnection);
        };

        if (!self)
            return;

        auto finishImage = [self, assetId, resultSize, pendingKey](const QImage &image,
                                                                   const QByteArray &bytes) {
            if (!self)
                return;
            QMetaObject::invokeMethod(
                self,
                [self, assetId, resultSize, pendingKey, image, bytes] {
                    if (!self)
                        return;
                    self->m_pendingImages.remove(pendingKey);
                    const QPixmap pixmap = QPixmap::fromImage(image);
                    if (pixmap.isNull()) {
                        emit self->imageLoadFailed(assetId, resultSize,
                                                   self->tr("Unsupported image format."));
                        return;
                    }
                    if (resultSize == QStringLiteral("thumbnail")) {
                        self->m_thumbnailCache.store(assetId, bytes, pixmap);
                        emit self->thumbnailLoaded(assetId, pixmap);
                    } else {
                        emit self->previewLoaded(assetId, pixmap);
                    }
                },
                Qt::QueuedConnection);
        };

        if (resultSize == QStringLiteral("thumbnail")) {
            const QByteArray cachedBytes = self->m_thumbnailCache.readDisk(assetId);
            if (!cachedBytes.isEmpty()) {
                const QImage image = self->decodeImage(cachedBytes);
                if (!image.isNull()) {
                    finishImage(image, {});
                    return;
                }
            }
        }

        auto fetchSize = [&](const QString &size, bool reportError) -> QByteArray {
            QUrl url(connection.serverUrl);
            QString basePath = url.path();
            while (basePath.endsWith(u'/'))
                basePath.chop(1);
            if (!basePath.endsWith(QStringLiteral("/api")))
                basePath += QStringLiteral("/api");
            url.setPath(basePath + QStringLiteral("/assets/%1/thumbnail").arg(assetId));
            QUrlQuery query;
            query.addQueryItem(QStringLiteral("size"), size);
            url.setQuery(query);

            QNetworkAccessManager network;
            QNetworkRequest request(url);
            request.setRawHeader("Accept", "*/*");
            request.setRawHeader("x-api-key", connection.apiKey.toUtf8());
            request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                                 QNetworkRequest::NoLessSafeRedirectPolicy);
            QNetworkReply *reply = network.get(request);
            QEventLoop loop;
            QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
            loop.exec();
            const QByteArray body = reply->readAll();
            if (reply->error() != QNetworkReply::NoError) {
                QString error = reply->errorString();
                const int status =
                    reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
                if (status == 401) {
                    error = QObject::tr(
                        "Authentication failed. Check the API key and its permissions.");
                }
                reply->deleteLater();
                if (reportError)
                    finishFail(error);
                return {};
            }
            reply->deleteLater();
            return body;
        };

        QByteArray bytes = fetchSize(resultSize == QStringLiteral("preview")
                                         ? QStringLiteral("preview")
                                         : QStringLiteral("thumbnail"),
                                     true);
        if (bytes.isEmpty())
            return;

        QImage image = decodeImage(bytes);
        if (image.isNull()) {
            bytes = fetchSize(QStringLiteral("fullsize"), true);
            if (bytes.isEmpty())
                return;
            image = decodeImage(bytes);
        }

        if (image.isNull()) {
            finishFail(QObject::tr("Unsupported image format."));
            return;
        }

        finishImage(image, bytes);
    });
}

} // namespace Aurora
