#include "core/ImmichClient.h"

#include "core/VideoStreamServer.h"

#include <QBuffer>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QImage>
#include <QImageReader>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaObject>
#include <QMimeDatabase>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPointer>
#include <QTimer>
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
    , m_activeServerUrl(normalizeServerUrl(m_connection.serverUrl))
    , m_network(new QNetworkAccessManager(this))
    , m_endpointProbeTimer(new QTimer(this))
{
    m_imagePool.setMaxThreadCount(8);
    m_endpointProbeTimer->setInterval(12 * 1000);
    connect(m_endpointProbeTimer, &QTimer::timeout, this, &ImmichClient::probeEndpoints);
    if (isConfigured()) {
        m_endpointProbeTimer->start();
        QTimer::singleShot(0, this, &ImmichClient::probeEndpoints);
    }
}

ImmichClient::~ImmichClient()
{
    m_imagePool.waitForDone();
}

QString ImmichClient::normalizeServerUrl(QString url)
{
    url = url.trimmed();
    while (url.endsWith(u'/'))
        url.chop(1);
    return url;
}

QUrl ImmichClient::apiUrlForBase(const QString &serverUrl, const QString &path)
{
    QUrl url(serverUrl);
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

ImmichConnectionSettings ImmichClient::connection() const
{
    return m_connection;
}

bool ImmichClient::isConfigured() const
{
    return m_connection.isConfigured();
}

QString ImmichClient::activeServerUrl() const
{
    return m_activeServerUrl.isEmpty() ? m_connection.serverUrl : m_activeServerUrl;
}

bool ImmichClient::usingLocalEndpoint() const
{
    return m_usingLocalEndpoint;
}

void ImmichClient::setConnection(const ImmichConnectionSettings &connection, bool persist)
{
    ImmichConnectionSettings normalized = connection;
    normalized.serverUrl = normalizeServerUrl(normalized.serverUrl);
    normalized.localServerUrl = normalizeServerUrl(normalized.localServerUrl);
    normalized.apiKey = normalized.apiKey.trimmed();

    const bool changed = normalized.serverUrl != m_connection.serverUrl ||
                         normalized.localServerUrl != m_connection.localServerUrl ||
                         normalized.apiKey != m_connection.apiKey;
    m_connection = normalized;
    if (persist)
        m_store.saveImmichConnection(m_connection);

    setActiveServerUrl(m_connection.serverUrl, false);

    if (isConfigured()) {
        if (!m_endpointProbeTimer->isActive())
            m_endpointProbeTimer->start();
        probeEndpoints();
    } else {
        m_endpointProbeTimer->stop();
    }

    if (changed) {
        m_thumbnailCache.clearMemory();
        if (m_streamServer)
            ensureStreamServer();
        emit configurationChanged(isConfigured());
    }
}

void ImmichClient::setActiveServerUrl(const QString &url, bool usingLocal)
{
    const QString normalized = normalizeServerUrl(url);
    if (normalized.isEmpty())
        return;

    const bool changed =
        normalized != m_activeServerUrl || usingLocal != m_usingLocalEndpoint;
    m_activeServerUrl = normalized;
    m_usingLocalEndpoint = usingLocal && m_connection.hasLocalServerUrl() &&
                           normalized == m_connection.localServerUrl;
    if (!changed)
        return;

    if (m_streamServer)
        ensureStreamServer();
    emit activeEndpointChanged(m_usingLocalEndpoint, m_activeServerUrl);
}

void ImmichClient::probeEndpoints()
{
    if (!isConfigured() || m_endpointProbeInFlight)
        return;

    if (!m_connection.hasLocalServerUrl()) {
        setActiveServerUrl(m_connection.serverUrl, false);
        if (m_connectionTestPending)
            finishConnectionTest();
        return;
    }

    m_endpointProbeInFlight = true;
    QNetworkRequest request(apiUrlForBase(m_connection.localServerUrl,
                                          QStringLiteral("/server/ping")));
    request.setRawHeader("Accept", "application/json");
    request.setTransferTimeout(1500);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);

    auto *reply = m_network->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        m_endpointProbeInFlight = false;
        const QByteArray body = reply->readAll();
        const bool reachable = reply->error() == QNetworkReply::NoError;
        bool pong = false;
        if (reachable) {
            const QJsonObject object = QJsonDocument::fromJson(body).object();
            const QString res = object.value(QStringLiteral("res")).toString();
            pong = res.compare(QStringLiteral("pong"), Qt::CaseInsensitive) == 0 ||
                   body.contains("pong");
        }

        if (reachable && pong)
            setActiveServerUrl(m_connection.localServerUrl, true);
        else
            setActiveServerUrl(m_connection.serverUrl, false);

        reply->deleteLater();

        if (m_connectionTestPending)
            finishConnectionTest();
    });
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

    QUrl apiBase(activeServerUrl());
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
    return apiUrlForBase(activeServerUrl(), path);
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

    m_connectionTestPending = true;
    if (m_endpointProbeInFlight)
        return;
    probeEndpoints();
}

void ImmichClient::finishConnectionTest()
{
    if (!m_connectionTestPending)
        return;
    m_connectionTestPending = false;

    auto *reply = m_network->get(authenticatedRequest(apiUrl(QStringLiteral("/users/me"))));
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        const QByteArray body = reply->readAll();
        if (reply->error() != QNetworkReply::NoError) {
            emit connectionTested(false, errorMessage(reply, body));
        } else {
            const QJsonObject user = QJsonDocument::fromJson(body).object();
            const QString identity = user.value(QStringLiteral("name")).toString(
                user.value(QStringLiteral("email")).toString());
            const QString via =
                m_usingLocalEndpoint ? tr("local endpoint") : tr("remote endpoint");
            emit connectionTested(
                true, identity.isEmpty()
                          ? tr("Connected via %1.").arg(via)
                          : tr("Connected as %1 via %2.").arg(identity, via));
        }
        reply->deleteLater();
    });
}

void ImmichClient::loadAssets(int page, int pageSize, const QString &query)
{
    searchAssets(page, pageSize, false, query);
}

void ImmichClient::pollNewestAssets(int pageSize)
{
    if (m_pollInFlight || !isConfigured())
        return;
    m_pollInFlight = true;
    searchAssets(1, pageSize, true);
}

void ImmichClient::searchAssets(int page, int pageSize, bool pollOnly, const QString &query)
{
    if (!ensureConfigured(pollOnly ? tr("Check for new photos")
                                   : (query.trimmed().isEmpty() ? tr("Load library")
                                                                : tr("Search")))) {
        if (pollOnly)
            m_pollInFlight = false;
        return;
    }

    const QString trimmedQuery = query.trimmed();
    const bool smartSearch = !pollOnly && !trimmedQuery.isEmpty();

    QJsonObject body;
    body.insert(QStringLiteral("page"), qMax(1, page));
    body.insert(QStringLiteral("size"), qBound(1, pageSize, 250));
    body.insert(QStringLiteral("withExif"), true);
    if (smartSearch) {
        body.insert(QStringLiteral("query"), trimmedQuery);
    } else {
        body.insert(QStringLiteral("order"), QStringLiteral("desc"));
        body.insert(QStringLiteral("withStacked"), true);
    }

    const QString endpoint = smartSearch ? QStringLiteral("/search/smart")
                                         : QStringLiteral("/search/metadata");
    QNetworkRequest searchRequest = authenticatedRequest(apiUrl(endpoint));
    searchRequest.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    searchRequest.setRawHeader("Accept", "application/json");
    auto *reply = m_network->post(searchRequest, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this,
            [this, reply, pollOnly, smartSearch, trimmedQuery] {
        const QByteArray body = reply->readAll();
        if (pollOnly)
            m_pollInFlight = false;

        if (reply->error() != QNetworkReply::NoError) {
            if (!pollOnly) {
                emit requestFailed(smartSearch ? tr("Search") : tr("Load library"),
                                   errorMessage(reply, body));
            }
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

        if (pollOnly)
            emit newestAssetsPolled(assets);
        else
            emit assetsLoaded(
                assets, assetsObject.value(QStringLiteral("nextPage")).toVariant().toString(),
                trimmedQuery);
        reply->deleteLater();
    });
}

QImage ImmichClient::decodeImage(const QByteArray &bytes, int maximumDimension) const
{
    QBuffer buffer;
    buffer.setData(bytes);
    buffer.open(QIODevice::ReadOnly);
    QImageReader reader(&buffer);
    reader.setAutoTransform(true);
    const QSize sourceSize = reader.size();
    if (sourceSize.isValid() &&
        (sourceSize.width() > maximumDimension || sourceSize.height() > maximumDimension)) {
        reader.setScaledSize(sourceSize.scaled(
            maximumDimension, maximumDimension, Qt::KeepAspectRatio));
    }

    QImage image = reader.read();
    if (!image.isNull() &&
        (image.width() > maximumDimension || image.height() > maximumDimension)) {
        image = image.scaled(maximumDimension, maximumDimension,
                             Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    return image;
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

bool ImmichClient::isUploading() const
{
    return m_uploadInFlight || !m_uploadQueue.isEmpty();
}

int ImmichClient::pendingUploadCount() const
{
    return m_uploadQueue.size() + (m_uploadInFlight ? 1 : 0);
}

void ImmichClient::uploadAssets(const QStringList &filePaths)
{
    if (!ensureConfigured(tr("Upload")))
        return;

    for (const QString &path : filePaths) {
        const QString absolute = QFileInfo(path).absoluteFilePath();
        if (absolute.isEmpty())
            continue;
        if (!m_uploadQueue.contains(absolute))
            m_uploadQueue.append(absolute);
    }
    processUploadQueue();
}

void ImmichClient::processUploadQueue()
{
    if (m_uploadInFlight || m_uploadQueue.isEmpty())
        return;
    startUpload(m_uploadQueue.takeFirst());
}

void ImmichClient::startUpload(const QString &filePath)
{
    const QFileInfo info(filePath);
    if (!info.exists() || !info.isFile()) {
        emit requestFailed(tr("Upload"), tr("File not found: %1").arg(filePath));
        processUploadQueue();
        return;
    }

    auto *file = new QFile(filePath);
    if (!file->open(QIODevice::ReadOnly)) {
        const QString message = file->errorString();
        delete file;
        emit requestFailed(tr("Upload"),
                           tr("Could not open %1: %2").arg(info.fileName(), message));
        processUploadQueue();
        return;
    }

    auto *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    auto addTextPart = [multiPart](const QString &name, const QString &value) {
        QHttpPart part;
        part.setHeader(QNetworkRequest::ContentDispositionHeader,
                       QVariant(QStringLiteral("form-data; name=\"%1\"").arg(name)));
        part.setBody(value.toUtf8());
        multiPart->append(part);
    };

    const QDateTime created =
        info.birthTime().isValid() ? info.birthTime() : info.lastModified();
    const QDateTime modified = info.lastModified().isValid()
                                   ? info.lastModified()
                                   : QDateTime::currentDateTime();
    const QString deviceAssetId =
        QStringLiteral("%1-%2-%3")
            .arg(info.absoluteFilePath(), QString::number(info.size()),
                 QString::number(modified.toSecsSinceEpoch()));

    addTextPart(QStringLiteral("deviceAssetId"), deviceAssetId);
    addTextPart(QStringLiteral("deviceId"), QStringLiteral("immich-desktop"));
    addTextPart(QStringLiteral("fileCreatedAt"),
                created.toUTC().toString(Qt::ISODateWithMs));
    addTextPart(QStringLiteral("fileModifiedAt"),
                modified.toUTC().toString(Qt::ISODateWithMs));
    addTextPart(QStringLiteral("filename"), info.fileName());
    addTextPart(QStringLiteral("isFavorite"), QStringLiteral("false"));

    QHttpPart filePart;
    const QString safeName = info.fileName().replace(QLatin1Char('"'), QLatin1Char('\''));
    filePart.setHeader(
        QNetworkRequest::ContentDispositionHeader,
        QVariant(QStringLiteral("form-data; name=\"assetData\"; filename=\"%1\"")
                     .arg(safeName)));
    const QMimeDatabase mimeDb;
    filePart.setHeader(QNetworkRequest::ContentTypeHeader,
                       QVariant(mimeDb.mimeTypeForFile(info).name()));
    filePart.setBodyDevice(file);
    file->setParent(multiPart);
    multiPart->append(filePart);

    QNetworkRequest request = authenticatedRequest(apiUrl(QStringLiteral("/assets")));
    request.setRawHeader("Accept", "application/json");

    m_uploadInFlight = true;
    auto *reply = m_network->post(request, multiPart);
    multiPart->setParent(reply);

    connect(reply, &QNetworkReply::uploadProgress, this,
            [this, filePath](qint64 bytesSent, qint64 bytesTotal) {
                emit uploadProgress(filePath, bytesSent, bytesTotal);
            });
    connect(reply, &QNetworkReply::finished, this, [this, reply, filePath] {
        const QByteArray body = reply->readAll();
        m_uploadInFlight = false;

        if (reply->error() != QNetworkReply::NoError) {
            emit requestFailed(tr("Upload"),
                               tr("%1: %2").arg(QFileInfo(filePath).fileName(),
                                                errorMessage(reply, body)));
        } else {
            const QJsonObject object = QJsonDocument::fromJson(body).object();
            const QString assetId = object.value(QStringLiteral("id")).toString();
            const bool duplicate =
                object.value(QStringLiteral("duplicate")).toBool() ||
                object.value(QStringLiteral("status")).toString().compare(
                    QStringLiteral("duplicate"), Qt::CaseInsensitive) == 0;
            emit assetUploaded(filePath, assetId, duplicate);
        }

        reply->deleteLater();
        processUploadQueue();
    });
}

void ImmichClient::downloadAsset(const QString &assetId, const QString &destinationPath,
                                 const QString &suggestedFileName)
{
    Q_UNUSED(suggestedFileName);
    if (!ensureConfigured(tr("Download")) || assetId.isEmpty() || destinationPath.isEmpty())
        return;

    auto *file = new QFile(destinationPath);
    if (!file->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        const QString message = file->errorString();
        delete file;
        emit requestFailed(tr("Download"),
                           tr("Could not write %1: %2").arg(destinationPath, message));
        return;
    }

    auto *reply =
        m_network->get(authenticatedRequest(apiUrl(QStringLiteral("/assets/%1/original").arg(assetId))));
    file->setParent(reply);

    connect(reply, &QNetworkReply::readyRead, this, [reply, file] {
        file->write(reply->readAll());
    });
    connect(reply, &QNetworkReply::downloadProgress, this,
            [this, assetId](qint64 bytesReceived, qint64 bytesTotal) {
                emit downloadProgress(assetId, bytesReceived, bytesTotal);
            });
    connect(reply, &QNetworkReply::finished, this,
            [this, reply, file, assetId, destinationPath] {
                if (reply->bytesAvailable() > 0)
                    file->write(reply->readAll());

                if (reply->error() != QNetworkReply::NoError) {
                    file->close();
                    file->remove();
                    emit requestFailed(tr("Download"),
                                       errorMessage(reply, reply->readAll()));
                    reply->deleteLater();
                    return;
                }

                if (!file->flush()) {
                    const QString message = file->errorString();
                    file->close();
                    file->remove();
                    emit requestFailed(tr("Download"),
                                       tr("Could not write %1: %2")
                                           .arg(destinationPath, message));
                    reply->deleteLater();
                    return;
                }

                file->close();
                emit assetDownloaded(assetId, destinationPath);
                reply->deleteLater();
            });
}

void ImmichClient::deleteAssets(const QStringList &assetIds, bool permanent)
{
    if (!ensureConfigured(tr("Delete")))
        return;

    QStringList ids;
    ids.reserve(assetIds.size());
    for (const QString &id : assetIds) {
        if (!id.trimmed().isEmpty())
            ids.append(id.trimmed());
    }
    if (ids.isEmpty())
        return;

    QJsonObject body;
    QJsonArray idArray;
    for (const QString &id : ids)
        idArray.append(id);
    body.insert(QStringLiteral("ids"), idArray);
    body.insert(QStringLiteral("force"), permanent);

    QNetworkRequest request = authenticatedRequest(apiUrl(QStringLiteral("/assets")));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setRawHeader("Accept", "application/json");

    auto *reply = m_network->sendCustomRequest(
        request, QByteArrayLiteral("DELETE"),
        QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply, ids, permanent] {
        const QByteArray body = reply->readAll();
        if (reply->error() != QNetworkReply::NoError) {
            emit requestFailed(tr("Delete"), errorMessage(reply, body));
        } else {
            emit assetsDeleted(ids, permanent);
        }
        reply->deleteLater();
    });
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
    const QString serverUrl = activeServerUrl();
    QPointer<ImmichClient> self(this);

    m_imagePool.start([this, self, assetId, resultSize, pendingKey, connection, serverUrl] {
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

        auto encodeThumbnail = [](const QImage &image) {
            QByteArray bytes;
            QBuffer buffer(&bytes);
            if (buffer.open(QIODevice::WriteOnly))
                image.save(&buffer, "JPG", 82);
            return bytes;
        };

        auto finishImage = [self, assetId, resultSize, pendingKey](const QImage &image,
                                                                   const QByteArray &cacheBytes) {
            if (!self)
                return;
            QMetaObject::invokeMethod(
                self,
                [self, assetId, resultSize, pendingKey, image, cacheBytes] {
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
                        self->m_thumbnailCache.store(assetId, cacheBytes, pixmap);
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
                const QImage image = self->decodeImage(cachedBytes, 512);
                if (!image.isNull()) {
                    // Rewrite old cache entries that may contain a full-size
                    // response into a bounded, compressed thumbnail.
                    const bool normalizedJpeg =
                        cachedBytes.size() <= 1024 * 1024 &&
                        cachedBytes.startsWith(QByteArray::fromHex("ffd8"));
                    finishImage(image, normalizedJpeg
                                           ? QByteArray()
                                           : encodeThumbnail(image));
                    return;
                }
            }
        }

        auto fetchSize = [&](const QString &size, bool reportError) -> QByteArray {
            QUrl url(serverUrl);
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

        const int maximumDimension =
            resultSize == QStringLiteral("thumbnail") ? 512 : 1920;
        QImage image = decodeImage(bytes, maximumDimension);
        if (image.isNull()) {
            bytes = fetchSize(QStringLiteral("fullsize"), true);
            if (bytes.isEmpty())
                return;
            image = decodeImage(bytes, maximumDimension);
        }

        if (image.isNull()) {
            finishFail(QObject::tr("Unsupported image format."));
            return;
        }

        QByteArray cacheBytes;
        if (resultSize == QStringLiteral("thumbnail"))
            cacheBytes = encodeThumbnail(image);
        finishImage(image, cacheBytes);
    });
}

} // namespace Aurora
