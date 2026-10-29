#include "core/ImmichClient.h"

#include "core/CustomHeaders.h"
#include "core/TlsConfig.h"
#include "core/VideoStreamServer.h"

#include <QBuffer>
#include <QCollator>
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
#include <QNetworkInformation>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRandomGenerator>
#include <QPointer>
#include <QRegularExpression>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>

#include <algorithm>
#include <climits>
#include <memory>

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

ImmichAsset assetFromObject(const QJsonObject &object)
{
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
    asset.addedAt =
        QDateTime::fromString(object.value(QStringLiteral("createdAt")).toString(), Qt::ISODate);
    asset.updatedAt =
        QDateTime::fromString(object.value(QStringLiteral("updatedAt")).toString(), Qt::ISODate);
    asset.modifiedAt = QDateTime::fromString(
        object.value(QStringLiteral("fileModifiedAt")).toString(), Qt::ISODate);
    const QJsonObject exif = object.value(QStringLiteral("exifInfo")).toObject();
    asset.fileSize = qint64(exif.value(QStringLiteral("fileSizeInByte")).toDouble());
    asset.rating = exif.value(QStringLiteral("rating")).toInt();
    return asset;
}

// Immich stores uploads as <media>/library/<user>/<storage template>/file when a
// storage template is active, and as <media>/upload/<user>/xx/yy/file otherwise.
// Show only the template part; hashed upload folders mean nothing to people.
// External library paths are the user's own folders and stay as they are.
QString displayFolder(const QString &originalPath)
{
    const QString folder = originalPath.left(qMax(0, originalPath.lastIndexOf(u'/')));
    const QStringList parts = folder.split(u'/', Qt::SkipEmptyParts);
    const qsizetype library = parts.indexOf(QStringLiteral("library"));
    if (library >= 0 && library + 2 <= parts.size())
        return parts.mid(library + 2).join(u'/');
    if (parts.contains(QStringLiteral("upload")))
        return {};
    return folder;
}

ImmichAssetDetails assetDetailsFromObject(const QJsonObject &object)
{
    ImmichAssetDetails details;
    details.asset = assetFromObject(object);
    details.localDateTime =
        QDateTime::fromString(object.value(QStringLiteral("localDateTime")).toString(),
                              Qt::ISODateWithMs)
            .toUTC();

    const QJsonObject exif = object.value(QStringLiteral("exifInfo")).toObject();
    details.timeZone = exif.value(QStringLiteral("timeZone")).toString();
    details.width = exif.value(QStringLiteral("exifImageWidth")).toInt(
        object.value(QStringLiteral("width")).toInt());
    details.height = exif.value(QStringLiteral("exifImageHeight")).toInt(
        object.value(QStringLiteral("height")).toInt());
    details.fileSize = exif.value(QStringLiteral("fileSizeInByte")).toInteger();
    details.make = exif.value(QStringLiteral("make")).toString();
    details.model = exif.value(QStringLiteral("model")).toString();
    details.lens = exif.value(QStringLiteral("lensModel")).toString();
    details.fNumber = exif.value(QStringLiteral("fNumber")).toDouble();
    details.focalLength = exif.value(QStringLiteral("focalLength")).toDouble();
    details.iso = exif.value(QStringLiteral("iso")).toInt();
    details.exposureTime = exif.value(QStringLiteral("exposureTime")).toString();
    const QJsonValue latitude = exif.value(QStringLiteral("latitude"));
    const QJsonValue longitude = exif.value(QStringLiteral("longitude"));
    details.hasLocation = latitude.isDouble() && longitude.isDouble();
    details.latitude = latitude.toDouble();
    details.longitude = longitude.toDouble();
    details.city = exif.value(QStringLiteral("city")).toString();
    details.state = exif.value(QStringLiteral("state")).toString();
    details.country = exif.value(QStringLiteral("country")).toString();
    details.description = exif.value(QStringLiteral("description")).toString();

    const QString originalPath = object.value(QStringLiteral("originalPath")).toString();
    details.folder = displayFolder(originalPath);
    details.folderPath = originalPath.left(qMax(0, originalPath.lastIndexOf(u'/')));

    for (const QJsonValue &value : object.value(QStringLiteral("people")).toArray()) {
        const QJsonObject person = value.toObject();
        ImmichPerson entry;
        entry.id = person.value(QStringLiteral("id")).toString();
        entry.name = person.value(QStringLiteral("name")).toString();
        entry.hidden = person.value(QStringLiteral("isHidden")).toBool();
        if (!entry.id.isEmpty() && !entry.hidden)
            details.people.append(entry);
    }
    details.ownerId = object.value(QStringLiteral("ownerId")).toString();
    const QJsonObject owner = object.value(QStringLiteral("owner")).toObject();
    details.ownerName = owner.value(QStringLiteral("name")).toString(
        owner.value(QStringLiteral("email")).toString());

    for (const QJsonValue &value : object.value(QStringLiteral("tags")).toArray()) {
        const QJsonObject tagObject = value.toObject();
        ImmichTag tag;
        tag.id = tagObject.value(QStringLiteral("id")).toString();
        tag.name = tagObject.value(QStringLiteral("name")).toString();
        tag.path = tagObject.value(QStringLiteral("value")).toString(tag.name);
        tag.parentId = tagObject.value(QStringLiteral("parentId")).toString();
        if (!tag.id.isEmpty())
            details.tags.append(tag);
    }
    return details;
}

QList<ImmichAsset> assetsFromSearchBody(const QByteArray &body, QString *nextPage,
                                        QString *nextCursor = nullptr)
{
    const QJsonObject assetsObject =
        QJsonDocument::fromJson(body).object().value(QStringLiteral("assets")).toObject();
    if (nextPage)
        *nextPage = assetsObject.value(QStringLiteral("nextPage")).toVariant().toString();
    if (nextCursor)
        *nextCursor = assetsObject.value(QStringLiteral("nextCursor")).toString();

    QList<ImmichAsset> assets;
    const QJsonArray items = assetsObject.value(QStringLiteral("items")).toArray();
    assets.reserve(items.size());
    for (const QJsonValue &value : items) {
        ImmichAsset asset = assetFromObject(value.toObject());
        if (!asset.id.isEmpty())
            assets.append(asset);
    }
    return assets;
}

// /timeline/bucket returns a "columnar" response (one array per field,
// parallel-indexed) rather than an array of asset objects, and omits
// originalFileName entirely. The Library keeps only active timeline assets;
// collections (archive, trash) keep everything the filter returned.
QList<ImmichAsset> assetsFromBucketBody(const QByteArray &body, bool timelineOnly = true)
{
    const QJsonObject root = QJsonDocument::fromJson(body).object();
    const QJsonArray ids = root.value(QStringLiteral("id")).toArray();
    const QJsonArray isImage = root.value(QStringLiteral("isImage")).toArray();
    const QJsonArray isFavorite = root.value(QStringLiteral("isFavorite")).toArray();
    const QJsonArray isTrashed = root.value(QStringLiteral("isTrashed")).toArray();
    const QJsonArray ratio = root.value(QStringLiteral("ratio")).toArray();
    const QJsonArray duration = root.value(QStringLiteral("duration")).toArray();
    const QJsonArray fileCreatedAt = root.value(QStringLiteral("fileCreatedAt")).toArray();
    const QJsonArray createdAt = root.value(QStringLiteral("createdAt")).toArray();
    const QJsonArray visibility = root.value(QStringLiteral("visibility")).toArray();
    const QJsonArray status = root.value(QStringLiteral("status")).toArray();

    auto at = [](const QJsonArray &array, int i) -> QJsonValue {
        return (i >= 0 && i < array.size()) ? array.at(i) : QJsonValue();
    };

    QList<ImmichAsset> assets;
    assets.reserve(ids.size());
    for (int i = 0; i < ids.size(); ++i) {
        if (timelineOnly) {
            if (at(isTrashed, i).toBool())
                continue;
            if (at(status, i).toString(QStringLiteral("active")) != QStringLiteral("active"))
                continue;
            if (at(visibility, i).toString(QStringLiteral("timeline")) !=
                QStringLiteral("timeline"))
                continue;
        }

        ImmichAsset asset;
        asset.id = ids.at(i).toString();
        if (asset.id.isEmpty())
            continue;
        asset.type = at(isImage, i).toBool(true) ? QStringLiteral("IMAGE")
                                                  : QStringLiteral("VIDEO");
        asset.favorite = at(isFavorite, i).toBool();
        asset.aspectRatio = qBound(0.2, at(ratio, i).toDouble(1.0), 8.0);
        asset.duration = at(duration, i).toString();
        asset.takenAt = QDateTime::fromString(at(fileCreatedAt, i).toString(), Qt::ISODate);
        asset.addedAt = QDateTime::fromString(at(createdAt, i).toString(), Qt::ISODate);
        assets.append(asset);
    }
    return assets;
}

bool isValidImmichId(const QString &id)
{
    static const QRegularExpression pattern(
        QStringLiteral("^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-"
                       "[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$"));
    return pattern.match(id).hasMatch();
}

// Months in display order: newest first unless sorting oldest first.
QList<TimeBucketInfo> bucketsFromBody(const QByteArray &body, const AssetSort &sort)
{
    QList<TimeBucketInfo> buckets;
    const QJsonArray items = QJsonDocument::fromJson(body).array();
    buckets.reserve(items.size());
    for (const QJsonValue &value : items) {
        const QJsonObject object = value.toObject();
        const QString bucketStr = object.value(QStringLiteral("timeBucket")).toString();
        const QDate month = QDate::fromString(bucketStr.left(10), Qt::ISODate);
        const int count = object.value(QStringLiteral("count")).toInt();
        if (month.isValid() && count > 0)
            buckets.append({month, count});
    }
    std::sort(buckets.begin(), buckets.end(),
              [ascending = sort.ascending](const TimeBucketInfo &a, const TimeBucketInfo &b) {
                  return ascending ? a.month < b.month : a.month > b.month;
              });
    return buckets;
}

ImmichAlbum albumFromObject(const QJsonObject &object)
{
    ImmichAlbum album;
    album.id = object.value(QStringLiteral("id")).toString();
    album.name = object.value(QStringLiteral("albumName")).toString();
    album.assetCount = object.value(QStringLiteral("assetCount")).toInt();
    album.thumbnailAssetId = object.value(QStringLiteral("albumThumbnailAssetId")).toString();
    album.orderAscending = object.value(QStringLiteral("order")).toString() == QStringLiteral("asc");
    album.activityEnabled = object.value(QStringLiteral("shared")).toBool() &&
                            object.value(QStringLiteral("isActivityEnabled")).toBool();
    return album;
}

// File-manager orders for a folder. Names compare naturally (IMG_2 before
// IMG_10); ties fall back to the name so the order is stable.
void sortFolderAssets(QList<ImmichAsset> *assets, const AssetSort &sort)
{
    QCollator collator;
    collator.setNumericMode(true);
    collator.setCaseSensitivity(Qt::CaseInsensitive);
    using Field = AssetSort::Field;
    auto key = [&sort](const ImmichAsset &asset) -> QVariant {
        switch (sort.field) {
        case Field::AddedAt: return asset.addedAt;
        case Field::Modified: return asset.modifiedAt;
        case Field::Size: return asset.fileSize;
        case Field::Rating: return asset.rating;
        case Field::Type: return asset.isVideo() ? 1 : 0;
        case Field::Name: return {};
        default: return asset.takenAt;
        }
    };
    std::stable_sort(assets->begin(), assets->end(),
                     [&](const ImmichAsset &a, const ImmichAsset &b) {
                         int order = 0;
                         const QPartialOrdering byKey = QVariant::compare(key(a), key(b));
                         if (byKey == QPartialOrdering::Less)
                             order = -1;
                         else if (byKey == QPartialOrdering::Greater)
                             order = 1;
                         if (order == 0)
                             order = collator.compare(a.fileName, b.fileName);
                         return sort.ascending ? order < 0 : order > 0;
                     });
}

// The timeline's filter for a collection (see searchFilteredAssets for search).
QUrlQuery collectionTimelineQuery(const QString &kind, const QString &value)
{
    QUrlQuery query;
    if (kind == QStringLiteral("person"))
        query.addQueryItem(QStringLiteral("personId"), value);
    else if (kind == QStringLiteral("album"))
        query.addQueryItem(QStringLiteral("albumId"), value);
    else if (kind == QStringLiteral("tag"))
        query.addQueryItem(QStringLiteral("tagId"), value);
    else if (kind == QStringLiteral("favorites"))
        query.addQueryItem(QStringLiteral("isFavorite"), QStringLiteral("true"));
    else if (kind == QStringLiteral("archive"))
        query.addQueryItem(QStringLiteral("visibility"), QStringLiteral("archive"));
    else if (kind == QStringLiteral("trash"))
        query.addQueryItem(QStringLiteral("isTrashed"), QStringLiteral("true"));
    return query;
}

// Only non-default sorts add parameters, so servers older than 3.0 (no orderBy)
// keep working with the default.
void addSortQuery(QUrlQuery *query, const AssetSort &sort)
{
    if (sort.ascending)
        query->addQueryItem(QStringLiteral("order"), QStringLiteral("asc"));
    if (sort.field == AssetSort::Field::AddedAt)
        query->addQueryItem(QStringLiteral("orderBy"), QStringLiteral("createdAt"));
}

// Immich 3.2's search shape: orderBy (size, rating) only works with a structured
// filter, and the server rejects mixing it with the older flat fields, so the
// flat fields this app sends are converted. Pages become a cursor (see
// m_searchCursors). Unlike the old shape it includes trashed photos unless told.
QJsonObject toFilterShape(const QJsonObject &flat)
{
    QJsonObject body;
    QJsonObject filter;
    QJsonObject takenAt;
    for (auto it = flat.begin(); it != flat.end(); ++it) {
        const QString &key = it.key();
        const QJsonValue value = it.value();
        if (key == QStringLiteral("size") || key == QStringLiteral("withExif") ||
            key == QStringLiteral("withStacked") || key == QStringLiteral("orderBy"))
            body.insert(key, value);
        else if (key == QStringLiteral("visibility") || key == QStringLiteral("city") ||
                 key == QStringLiteral("type") || key == QStringLiteral("isFavorite"))
            filter.insert(key, QJsonObject{{QStringLiteral("eq"), value}});
        else if (key == QStringLiteral("personIds")) // Photos with all of these people.
            filter.insert(key, QJsonObject{{QStringLiteral("all"), value}});
        else if (key == QStringLiteral("albumIds") || key == QStringLiteral("tagIds"))
            filter.insert(key, QJsonObject{{QStringLiteral("any"), value}});
        else if (key == QStringLiteral("originalFileName") || key == QStringLiteral("description") ||
                 key == QStringLiteral("originalPath"))
            filter.insert(key, QJsonObject{{QStringLiteral("like"), value}});
        else if (key == QStringLiteral("ocr"))
            filter.insert(key, QJsonObject{{QStringLiteral("matches"), value}});
        else if (key == QStringLiteral("takenAfter"))
            takenAt.insert(QStringLiteral("gte"), value);
        else if (key == QStringLiteral("takenBefore"))
            takenAt.insert(QStringLiteral("lte"), value);
    }
    if (!takenAt.isEmpty())
        filter.insert(QStringLiteral("takenAt"), takenAt);
    const bool trash = flat.value(QStringLiteral("withDeleted")).toBool();
    filter.insert(QStringLiteral("trashedAt"),
                  QJsonObject{{trash ? QStringLiteral("ne") : QStringLiteral("eq"), QJsonValue::Null}});
    body.insert(QStringLiteral("filter"), filter);
    return body;
}

// The search filter for a collection, or an empty object for unknown kinds.
QJsonObject collectionSearchFilter(const QString &kind, const QString &value)
{
    if (kind == QStringLiteral("person"))
        return {{QStringLiteral("personIds"), QJsonArray{value}}};
    if (kind == QStringLiteral("city"))
        return {{QStringLiteral("city"), value}};
    if (kind == QStringLiteral("album"))
        return {{QStringLiteral("albumIds"), QJsonArray{value}}};
    if (kind == QStringLiteral("tag"))
        return {{QStringLiteral("tagIds"), QJsonArray{value}}};
    if (kind == QStringLiteral("favorites"))
        return {{QStringLiteral("isFavorite"), true}};
    if (kind == QStringLiteral("archive"))
        return {{QStringLiteral("visibility"), QStringLiteral("archive")}};
    if (kind == QStringLiteral("trash"))
        return {{QStringLiteral("withDeleted"), true},
                {QStringLiteral("trashedAfter"), QStringLiteral("1970-01-01T00:00:00.000Z")}};
    return {};
}

} // namespace

ImmichClient::ImmichClient(QObject *parent)
    : QObject(parent)
    , m_connection(m_store.loadImmichConnection())
    , m_activeServerUrl(normalizeServerUrl(m_connection.serverUrl))
    , m_network(new QNetworkAccessManager(this))
    , m_endpointProbeTimer(new QTimer(this))
    , m_reachabilityTimer(new QTimer(this))
    , m_cacheFlushTimer(new QTimer(this))
{
    m_imagePool.setMaxThreadCount(4);
    m_rememberViewed = AppSettings().loadTimeline().rememberViewed;
    m_cacheFlushTimer->setSingleShot(true);
    m_cacheFlushTimer->setInterval(2000);
    connect(m_cacheFlushTimer, &QTimer::timeout, this, &ImmichClient::flushLibraryCache);
    m_endpointProbeTimer->setInterval(12 * 1000);
    connect(m_endpointProbeTimer, &QTimer::timeout, this, &ImmichClient::probeEndpoints);
    m_reachabilityTimer->setInterval(15 * 1000);
    connect(m_reachabilityTimer, &QTimer::timeout, this, &ImmichClient::probeReachability);
    setupNetworkMonitoring();
    restoreUploadQueue();
    // Build the client-cert SSL config from stored settings BEFORE any request
    // fires. Without this, startup auto-load requests go out cert-less and poison
    // the QNetworkAccessManager connection pool for the host, so later
    // cert-bearing requests reuse the cert-less socket and the mTLS handshake
    // never happens (observed as SSL error 11).
    rebuildSslConfig();
    if (isConfigured()) {
        m_endpointProbeTimer->start();
        m_reachabilityTimer->start();
        QTimer::singleShot(0, this, &ImmichClient::probeEndpoints);
        QTimer::singleShot(0, this, &ImmichClient::probeReachability);
        QTimer::singleShot(2000, this, &ImmichClient::processUploadQueue);
        QTimer::singleShot(0, this, &ImmichClient::loadServerVersion);
    }
}

bool ImmichClient::serverAtLeast(int major, int minor) const
{
    return !m_serverVersion.isNull() &&
           QVersionNumber::compare(m_serverVersion, QVersionNumber(major, minor)) >= 0;
}

void ImmichClient::loadServerVersion()
{
    if (!isConfigured())
        return;
    QNetworkRequest request = authenticatedRequest(apiUrl(QStringLiteral("/server/version")));
    request.setRawHeader("Accept", "application/json");
    auto *reply = m_network->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError)
            return;
        const QJsonObject object = QJsonDocument::fromJson(reply->readAll()).object();
        const QVersionNumber version(object.value(QStringLiteral("major")).toInt(),
                                     object.value(QStringLiteral("minor")).toInt(),
                                     object.value(QStringLiteral("patch")).toInt());
        if (version != m_serverVersion) {
            m_serverVersion = version;
            emit serverVersionChanged();
        }
    });
}

ImmichClient::~ImmichClient()
{
    flushLibraryCache();
    m_endpointProbeTimer->stop();
    m_reachabilityTimer->stop();
    if (m_network) {
        const auto replies = m_network->findChildren<QNetworkReply *>();
        for (QNetworkReply *reply : replies) {
            QObject::disconnect(reply, nullptr, this, nullptr);
            reply->abort();
        }
    }
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

bool ImmichClient::isOnline() const
{
    return m_online;
}

void ImmichClient::setupNetworkMonitoring()
{
    // loadDefaultBackend() arrived in Qt 6.3; CI/Linux packages are often 6.2.
#if QT_VERSION >= QT_VERSION_CHECK(6, 3, 0)
    const bool loaded = QNetworkInformation::loadDefaultBackend();
#else
    const bool loaded =
        QNetworkInformation::load(QNetworkInformation::Feature::Reachability);
#endif
    if (!loaded)
        return;
    if (auto *info = QNetworkInformation::instance()) {
        connect(info, &QNetworkInformation::reachabilityChanged, this,
                [this](QNetworkInformation::Reachability reachability) {
                    if (reachability == QNetworkInformation::Reachability::Disconnected)
                        setOnline(false);
                    else
                        probeReachability();
                });
        if (info->reachability() == QNetworkInformation::Reachability::Disconnected)
            m_online = false;
    }
}

void ImmichClient::setOnline(bool online)
{
    if (m_online == online)
        return;
    m_online = online;
    emit onlineChanged(m_online);
    if (m_online) {
        processUploadQueue();
        if (m_serverVersion.isNull())
            loadServerVersion();
    }
}

bool ImmichClient::isTransientNetworkError(QNetworkReply *reply)
{
    if (!reply)
        return false;
    switch (reply->error()) {
    case QNetworkReply::ConnectionRefusedError:
    case QNetworkReply::RemoteHostClosedError:
    case QNetworkReply::HostNotFoundError:
    case QNetworkReply::TimeoutError:
    case QNetworkReply::TemporaryNetworkFailureError:
    case QNetworkReply::NetworkSessionFailedError:
    case QNetworkReply::UnknownNetworkError:
    case QNetworkReply::ProxyConnectionRefusedError:
    case QNetworkReply::ProxyConnectionClosedError:
    case QNetworkReply::ProxyNotFoundError:
    case QNetworkReply::ProxyTimeoutError:
    case QNetworkReply::OperationCanceledError:
        return true;
    default:
        return false;
    }
}

void ImmichClient::queueLibraryCache(const QList<ImmichAsset> &assets)
{
    m_pendingCacheAssets.append(assets);
    m_cacheFlushTimer->start();
}

void ImmichClient::flushLibraryCache()
{
    m_cacheFlushTimer->stop();
    if (m_pendingCacheAssets.isEmpty())
        return;
    m_offlineStore.mergeLibrary(m_connection.serverUrl, std::exchange(m_pendingCacheAssets, {}));
}

void ImmichClient::setConnection(const ImmichConnectionSettings &connection, bool persist)
{
    flushLibraryCache();
    ImmichConnectionSettings normalized = connection;
    normalized.serverUrl = normalizeServerUrl(normalized.serverUrl);
    normalized.localServerUrl = normalizeServerUrl(normalized.localServerUrl);
    normalized.apiKey = normalized.apiKey.trimmed();
    normalized.customHeaders = CustomHeaders::normalize(normalized.customHeaders);

    const bool changed = normalized.serverUrl != m_connection.serverUrl ||
                         normalized.localServerUrl != m_connection.localServerUrl ||
                         normalized.apiKey != m_connection.apiKey ||
                         normalized.customHeaders != m_connection.customHeaders;
    m_connection = normalized;
    if (persist)
        m_store.saveImmichConnection(m_connection);

    rebuildSslConfig();

    setActiveServerUrl(m_connection.serverUrl, false);

    if (isConfigured()) {
        if (!m_endpointProbeTimer->isActive())
            m_endpointProbeTimer->start();
        if (!m_reachabilityTimer->isActive())
            m_reachabilityTimer->start();
        probeEndpoints();
        probeReachability();
    } else {
        m_endpointProbeTimer->stop();
        m_reachabilityTimer->stop();
    }

    if (changed) {
        cancelThumbnailPrefetch();
        m_currentUserId.clear();
        m_serverVersion = {};
        m_rememberViewed = AppSettings().loadTimeline().rememberViewed;
        releaseMemoryCaches();
        if (m_streamServer)
            ensureStreamServer();
        emit configurationChanged(isConfigured());
        loadServerVersion();
    }
}

void ImmichClient::rebuildSslConfig()
{
    const TlsConfigResult result = TlsConfig::build(m_connection, effectivePassword());
    // Empty-but-needed or a wrong non-empty password both re-open the prompt
    // (bounded UI-side by the 3-attempt cap). Other build errors toast.
    if (result.needsPassword || result.badPassword) {
        emit sslPasswordNeeded();
        return;
    }
    if (!result.ok) {
        emit requestFailed(tr("Connection"), result.error);
        return;
    }
    m_sslConfig = result.config;
    // Drop any pooled connections so a later request can't reuse a socket whose
    // TLS handshake happened under the old (or cert-less) config.
    m_network->clearConnectionCache();
}

void ImmichClient::provideKeyPassword(const QString &password)
{
    m_sessionKeyPassword = password;
    rebuildSslConfig();
    if (isConfigured()) {
        probeEndpoints();
        probeReachability();
    }
}

QString ImmichClient::effectivePassword() const
{
    return m_sessionKeyPassword.isEmpty() ? m_connection.keyPassword : m_sessionKeyPassword;
}

QSslConfiguration ImmichClient::sslConfigForActiveEndpoint() const
{
    if (!m_usingLocalEndpoint && m_connection.hasClientCert())
        return m_sslConfig;
    return QSslConfiguration::defaultConfiguration();
}

void ImmichClient::releaseMemoryCaches()
{
    m_thumbnailCache.clearMemory();
    m_previewCache.clearMemory();
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
    const QString probedLocalUrl = m_connection.localServerUrl;
    const QString probedRemoteUrl = m_connection.serverUrl;
    QNetworkRequest request(apiUrlForBase(probedLocalUrl, QStringLiteral("/server/ping")));
    request.setRawHeader("Accept", "application/json");
    request.setTransferTimeout(1500);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::SameOriginRedirectPolicy);
    // Local probe targets the LAN endpoint, which never uses the client cert.
    request.setSslConfiguration(QSslConfiguration::defaultConfiguration());
    CustomHeaders::applyTo(request, m_connection.customHeaders);

    auto *reply = m_network->get(request);
    connect(reply, &QNetworkReply::finished, this,
            [this, reply, probedLocalUrl, probedRemoteUrl] {
        m_endpointProbeInFlight = false;
        // Ignore stale probes after the user changes connection settings.
        if (m_connection.localServerUrl != probedLocalUrl ||
            m_connection.serverUrl != probedRemoteUrl) {
            reply->deleteLater();
            if (isConfigured())
                probeEndpoints();
            return;
        }

        const QByteArray body = reply->readAll();
        const bool reachable = reply->error() == QNetworkReply::NoError;
        bool pong = false;
        if (reachable) {
            const QJsonObject object = QJsonDocument::fromJson(body).object();
            const QString res = object.value(QStringLiteral("res")).toString();
            pong = res.compare(QStringLiteral("pong"), Qt::CaseInsensitive) == 0 ||
                   body.contains("pong");
        }

        if (reachable && pong) {
            setActiveServerUrl(probedLocalUrl, true);
            setOnline(true);
        } else {
            setActiveServerUrl(probedRemoteUrl, false);
        }

        reply->deleteLater();

        if (m_connectionTestPending)
            finishConnectionTest();
    });
}

void ImmichClient::probeReachability()
{
    if (!isConfigured() || m_reachabilityProbeInFlight)
        return;

    m_reachabilityProbeInFlight = true;
    QNetworkRequest request(apiUrl(QStringLiteral("/server/ping")));
    request.setRawHeader("Accept", "application/json");
    request.setTransferTimeout(2500);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::SameOriginRedirectPolicy);
    request.setSslConfiguration(sslConfigForActiveEndpoint());
    CustomHeaders::applyTo(request, m_connection.customHeaders);

    auto *reply = m_network->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        m_reachabilityProbeInFlight = false;
        const QByteArray body = reply->readAll();
        const bool reachable = reply->error() == QNetworkReply::NoError &&
                               (body.contains("pong") ||
                                QJsonDocument::fromJson(body)
                                        .object()
                                        .value(QStringLiteral("res"))
                                        .toString()
                                        .compare(QStringLiteral("pong"),
                                                 Qt::CaseInsensitive) == 0);
        setOnline(reachable);
        reply->deleteLater();
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
    m_streamServer->setCredentials(apiBase, m_connection.apiKey,
                                   sslConfigForActiveEndpoint(),
                                   m_connection.customHeaders);
}

QUrl ImmichClient::videoStreamUrl(const QString &assetId)
{
    if (!ensureConfigured(tr("Play video")) || !isValidImmichId(assetId)) {
        if (!assetId.isEmpty() && !isValidImmichId(assetId))
            emit requestFailed(tr("Play video"), tr("The asset identifier is invalid."));
        return {};
    }
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
                        QNetworkRequest::SameOriginRedirectPolicy);
    result.setTransferTimeout(30000);
    result.setSslConfiguration(sslConfigForActiveEndpoint());
    CustomHeaders::applyTo(result, m_connection.customHeaders);
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
    if (reply->error() == QNetworkReply::SslHandshakeFailedError) {
        // A wrong key password surfaces here, not as needsPassword — re-prompt
        // on the remote cert endpoint (debounced UI-side).
        if (!m_usingLocalEndpoint && m_connection.hasClientCert())
            emit const_cast<ImmichClient *>(this)->sslPasswordNeeded();
        return tr("TLS handshake failed — check your client certificate and CA certificate.");
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

void ImmichClient::loadAssets(int page, int pageSize, const QString &query,
                              const QJsonObject &searchOptions)
{
    searchAssets(page, pageSize, false, query, searchOptions);
}

void ImmichClient::loadTimelineBuckets(const AssetSort &sort)
{
    ++m_searchGeneration;
    if (!ensureConfigured(tr("Load library")))
        return;
    if (!m_online) {
        emit requestFailed(tr("Load library"),
                           tr("You're offline - Years/Month browsing needs a connection."));
        return;
    }

    QUrl url = apiUrl(QStringLiteral("/timeline/buckets"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("size"), QStringLiteral("MONTH"));
    addSortQuery(&query, sort);
    url.setQuery(query);

    QNetworkRequest request = authenticatedRequest(url);
    request.setRawHeader("Accept", "application/json");
    auto *reply = m_network->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, sort] {
        const QByteArray body = reply->readAll();
        if (reply->error() != QNetworkReply::NoError) {
            emit requestFailed(tr("Load library"), errorMessage(reply, body));
            reply->deleteLater();
            return;
        }
        emit timelineBucketsLoaded(bucketsFromBody(body, sort), sort);
        reply->deleteLater();
    });
}

void ImmichClient::loadTimelineBucket(const QDate &month, const AssetSort &sort)
{
    if (!ensureConfigured(tr("Load library")))
        return;
    if (!m_online) {
        emit timelineBucketFailed(month, tr("You're offline."), sort);
        return;
    }

    QUrl url = apiUrl(QStringLiteral("/timeline/bucket"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("timeBucket"),
                       QDate(month.year(), month.month(), 1).toString(Qt::ISODate));
    query.addQueryItem(QStringLiteral("size"), QStringLiteral("MONTH"));
    addSortQuery(&query, sort);
    url.setQuery(query);

    QNetworkRequest request = authenticatedRequest(url);
    request.setRawHeader("Accept", "application/json");
    auto *reply = m_network->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, month, sort] {
        const QByteArray body = reply->readAll();
        if (reply->error() != QNetworkReply::NoError) {
            emit timelineBucketFailed(month, errorMessage(reply, body), sort);
            reply->deleteLater();
            return;
        }
        const QList<ImmichAsset> assets = assetsFromBucketBody(body);
        queueLibraryCache(assets);
        emit timelineBucketLoaded(month, assets, sort);
        reply->deleteLater();
    });
}

void ImmichClient::clearCachedLibrary()
{
    m_offlineStore.saveLibrary(m_connection.serverUrl, {}, {});
}

void ImmichClient::pollNewestAssets(int pageSize)
{
    if (m_pollInFlight || !isConfigured())
        return;
    m_pollInFlight = true;
    searchAssets(1, pageSize, true);
}

void ImmichClient::searchAssets(int page, int pageSize, bool pollOnly, const QString &query,
                                const QJsonObject &searchOptions)
{
    if (!ensureConfigured(pollOnly ? tr("Check for new photos")
                                   : (query.trimmed().isEmpty() ? tr("Load library")
                                                                : tr("Search")))) {
        if (pollOnly)
            m_pollInFlight = false;
        return;
    }

    const QString trimmedQuery = query.trimmed();
    const QString mode = searchOptions.value(QStringLiteral("mode")).toString();
    QJsonObject filters = searchOptions;
    filters.remove(QStringLiteral("mode"));
    const bool filteredSearch = !pollOnly && (!trimmedQuery.isEmpty() || !filters.isEmpty());
    const bool smartSearch = !pollOnly && !trimmedQuery.isEmpty() && mode.isEmpty();
    const quint64 generation = pollOnly ? m_searchGeneration : ++m_searchGeneration;

    if (!pollOnly && !m_online) {
        if (!filteredSearch && page <= 1 && emitCachedLibrary(trimmedQuery))
            return;
        emit requestFailed(filteredSearch ? tr("Search") : tr("Load library"),
                           tr("You are offline and no cached library is available."));
        return;
    }

    QJsonObject body = pollOnly ? QJsonObject{} : filters;
    // The timeline never shows hidden assets (e.g. the video half of a Live Photo),
    // so they must not count as new photos.
    if (pollOnly)
        body.insert(QStringLiteral("visibility"), QStringLiteral("timeline"));
    body.insert(QStringLiteral("page"), qMax(1, page));
    body.insert(QStringLiteral("size"), qBound(1, pageSize, 250));
    body.insert(QStringLiteral("withExif"), true);
    if (smartSearch) {
        body.insert(QStringLiteral("query"), trimmedQuery);
    } else {
        if (!pollOnly && !trimmedQuery.isEmpty() && !mode.isEmpty())
            body.insert(mode, trimmedQuery);
        // Callers may pick an order (see searchOrder); newest first otherwise.
        if (!body.contains(QStringLiteral("order")) && !body.contains(QStringLiteral("orderBy")))
            body.insert(QStringLiteral("order"), QStringLiteral("desc"));
        body.insert(QStringLiteral("withStacked"), true);
    }
    const bool filterShape = body.contains(QStringLiteral("orderBy"));
    if (filterShape) {
        body = toFilterShape(body);
        if (page > 1)
            body.insert(QStringLiteral("cursor"), m_searchCursors.value(QStringLiteral("library")));
    }

    const QString endpoint = smartSearch ? QStringLiteral("/search/smart")
                                         : QStringLiteral("/search/metadata");
    QNetworkRequest searchRequest = authenticatedRequest(apiUrl(endpoint));
    searchRequest.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    searchRequest.setRawHeader("Accept", "application/json");
    searchRequest.setTransferTimeout(20000);
    auto *reply = m_network->post(searchRequest, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this,
            [this, reply, pollOnly, filteredSearch, trimmedQuery, page, generation, filterShape] {
        const QByteArray body = reply->readAll();
        if (!pollOnly && generation != m_searchGeneration) {
            reply->deleteLater();
            return;
        }
        if (pollOnly)
            m_pollInFlight = false;

        if (reply->error() != QNetworkReply::NoError) {
            if (isTransientNetworkError(reply))
                setOnline(false);
            if (!pollOnly) {
                if (!filteredSearch && page <= 1 && emitCachedLibrary(trimmedQuery)) {
                    reply->deleteLater();
                    return;
                }
                emit requestFailed(filteredSearch ? tr("Search") : tr("Load library"),
                                   errorMessage(reply, body));
            }
            reply->deleteLater();
            return;
        }

        setOnline(true);
        QString nextPage;
        QString nextCursor;
        QList<ImmichAsset> assets = assetsFromSearchBody(body, &nextPage, &nextCursor);
        if (filterShape) {
            m_searchCursors.insert(QStringLiteral("library"), nextCursor);
            nextPage = nextCursor.isEmpty() ? QString() : QString::number(page + 1);
        }

        if (pollOnly) {
            emit newestAssetsPolled(assets);
        } else {
            if (!filteredSearch) {
                if (page <= 1) {
                    // A fresh first page replaces the cache; older queued months are stale.
                    m_pendingCacheAssets.clear();
                    m_cacheFlushTimer->stop();
                    m_offlineStore.saveLibrary(m_connection.serverUrl, assets, trimmedQuery);
                } else {
                    queueLibraryCache(assets);
                }
            }
            emit assetsLoaded(assets, nextPage, trimmedQuery, false);
        }
        reply->deleteLater();
    });
}

bool ImmichClient::emitCachedLibrary(const QString &query)
{
    if (!query.isEmpty())
        return false;
    QList<ImmichAsset> assets;
    if (!m_offlineStore.loadLibrary(m_connection.serverUrl, &assets))
        return false;
    emit assetsLoaded(assets, {}, query, true);
    return true;
}

bool ImmichClient::loadExplore()
{
    if (!ensureConfigured(tr("Load explore")))
        return false;
    if (m_explorePeoplePending || m_exploreDataPending)
        return false;

    if (!m_online) {
        if (emitCachedExplore())
            return true;
        emit requestFailed(tr("Load explore"),
                           tr("You are offline and no cached explore data is available."));
        return false;
    }

    m_exploreBuffer = ImmichExploreData{};
    m_exploreError.clear();
    m_explorePeoplePending = true;
    m_exploreDataPending = true;

    QUrl peopleUrl = apiUrl(QStringLiteral("/people"));
    QUrlQuery peopleQuery;
    peopleQuery.addQueryItem(QStringLiteral("withHidden"), QStringLiteral("false"));
    peopleQuery.addQueryItem(QStringLiteral("page"), QStringLiteral("1"));
    peopleQuery.addQueryItem(QStringLiteral("size"), QStringLiteral("100"));
    peopleUrl.setQuery(peopleQuery);

    QNetworkRequest peopleRequest = authenticatedRequest(peopleUrl);
    peopleRequest.setRawHeader("Accept", "application/json");
    auto *peopleReply = m_network->get(peopleRequest);
    connect(peopleReply, &QNetworkReply::finished, this, [this, peopleReply] {
        const QByteArray body = peopleReply->readAll();
        QString error;
        if (peopleReply->error() != QNetworkReply::NoError) {
            if (isTransientNetworkError(peopleReply))
                setOnline(false);
            error = errorMessage(peopleReply, body);
        } else {
            const QJsonObject object = QJsonDocument::fromJson(body).object();
            const QJsonArray people = object.value(QStringLiteral("people")).toArray();
            m_exploreBuffer.people.reserve(people.size());
            for (const QJsonValue &value : people) {
                const QJsonObject personObject = value.toObject();
                ImmichPerson person;
                person.id = personObject.value(QStringLiteral("id")).toString();
                person.name = personObject.value(QStringLiteral("name")).toString();
                person.favorite = personObject.value(QStringLiteral("isFavorite")).toBool();
                person.hidden = personObject.value(QStringLiteral("isHidden")).toBool();
                if (!person.id.isEmpty() && !person.hidden)
                    m_exploreBuffer.people.append(person);
            }
        }
        peopleReply->deleteLater();
        if (!error.isEmpty()) {
            if (!m_exploreError.isEmpty())
                m_exploreError += QLatin1Char('\n');
            m_exploreError += error;
        }
        finishExploreLoad(true, false, m_exploreError);
    });

    QNetworkRequest exploreRequest =
        authenticatedRequest(apiUrl(QStringLiteral("/search/explore")));
    exploreRequest.setRawHeader("Accept", "application/json");
    auto *exploreReply = m_network->get(exploreRequest);
    connect(exploreReply, &QNetworkReply::finished, this, [this, exploreReply] {
        const QByteArray body = exploreReply->readAll();
        QString error;
        if (exploreReply->error() != QNetworkReply::NoError) {
            if (isTransientNetworkError(exploreReply))
                setOnline(false);
            error = errorMessage(exploreReply, body);
        } else {
            const QJsonArray sections = QJsonDocument::fromJson(body).array();
            for (const QJsonValue &sectionValue : sections) {
                const QJsonObject section = sectionValue.toObject();
                const QString field = section.value(QStringLiteral("fieldName")).toString();
                const QJsonArray items = section.value(QStringLiteral("items")).toArray();
                if (field == QStringLiteral("exifInfo.city")) {
                    for (const QJsonValue &itemValue : items) {
                        const QJsonObject item = itemValue.toObject();
                        ImmichPlace place;
                        place.city = item.value(QStringLiteral("value")).toString();
                        place.sampleAsset =
                            assetFromObject(item.value(QStringLiteral("data")).toObject());
                        if (!place.city.isEmpty() && !place.sampleAsset.id.isEmpty())
                            m_exploreBuffer.places.append(place);
                    }
                } else if (field == QStringLiteral("createdAt")) {
                    for (const QJsonValue &itemValue : items) {
                        const QJsonObject item = itemValue.toObject();
                        ImmichAsset asset =
                            assetFromObject(item.value(QStringLiteral("data")).toObject());
                        if (!asset.id.isEmpty())
                            m_exploreBuffer.recentAssets.append(asset);
                    }
                }
            }
        }
        exploreReply->deleteLater();
        if (!error.isEmpty()) {
            if (!m_exploreError.isEmpty())
                m_exploreError += QLatin1Char('\n');
            m_exploreError += error;
        }
        finishExploreLoad(false, true, m_exploreError);
    });
    return true;
}

void ImmichClient::finishExploreLoad(bool peopleDone, bool exploreDone, const QString &error)
{
    if (peopleDone)
        m_explorePeoplePending = false;
    if (exploreDone)
        m_exploreDataPending = false;

    if (m_explorePeoplePending || m_exploreDataPending)
        return;

    const bool empty = m_exploreBuffer.people.isEmpty() && m_exploreBuffer.places.isEmpty() &&
                       m_exploreBuffer.recentAssets.isEmpty();
    if (!error.isEmpty() && empty) {
        if (emitCachedExplore())
            return;
        emit requestFailed(tr("Load explore"), error);
        return;
    }

    if (!empty) {
        setOnline(true);
        m_offlineStore.saveExplore(m_connection.serverUrl, m_exploreBuffer);
        emit exploreLoaded(m_exploreBuffer, false);
        // Surface a partial failure so the UI can note missing people/places.
        if (!error.isEmpty())
            emit requestFailed(tr("Load explore"), error);
        return;
    }

    emit exploreLoaded(m_exploreBuffer, false);
}

bool ImmichClient::emitCachedExplore()
{
    ImmichExploreData data;
    if (!m_offlineStore.loadExplore(m_connection.serverUrl, &data))
        return false;
    emit exploreLoaded(data, true);
    return true;
}

void ImmichClient::loadAssetsForFolder(const QString &path, const AssetSort &sort)
{
    QUrl url = apiUrl(QStringLiteral("/view/folder"));
    url.setQuery(QUrlQuery{{QStringLiteral("path"), path}});
    fetchBrowseList(QStringLiteral("folder"), url, [this, path, sort](const QByteArray &body) {
        QList<ImmichAsset> assets;
        for (const QJsonValue &value : QJsonDocument::fromJson(body).array()) {
            const ImmichAsset asset = assetFromObject(value.toObject());
            if (!asset.id.isEmpty())
                assets.append(asset);
        }
        // The folder arrives whole, with file details, so it's sorted here. After
        // the viewed photos, "Last viewed" lists the rest newest first, like elsewhere.
        sortFolderAssets(&assets, sort.field == AssetSort::Field::Viewed
                                      ? AssetSort{AssetSort::Field::TakenAt, false}
                                      : sort);
        if (sort.field == AssetSort::Field::Viewed) {
            // Viewed photos first in viewing order; the rest keep their date order.
            QHash<QString, int> order;
            const QList<ImmichAsset> history = recentlyViewed();
            for (int i = 0; i < history.size(); ++i)
                order.insert(history.at(i).id, i);
            std::stable_sort(assets.begin(), assets.end(),
                             [&order, &sort](const ImmichAsset &a, const ImmichAsset &b) {
                                 const int first = order.value(a.id, -1);
                                 const int second = order.value(b.id, -1);
                                 if ((first < 0) != (second < 0))
                                     return first >= 0;
                                 if (first < 0)
                                     return false;
                                 return sort.ascending ? first > second : first < second;
                             });
        }
        emit filteredAssetsLoaded(QStringLiteral("folder"), path, assets, {}, sort);
    });
}

QJsonObject ImmichClient::searchOrder(const AssetSort &sort)
{
    const QString direction = sort.ascending ? QStringLiteral("asc") : QStringLiteral("desc");
    if (sort.field != AssetSort::Field::Size && sort.field != AssetSort::Field::Rating)
        return {{QStringLiteral("order"), direction}};
    const QString field = sort.field == AssetSort::Field::Size ? QStringLiteral("fileSizeInBytes")
                                                               : QStringLiteral("rating");
    return {{QStringLiteral("orderBy"),
             QJsonObject{{QStringLiteral("field"), field}, {QStringLiteral("direction"), direction}}}};
}

bool ImmichClient::canSortByDateAdded(const QString &kind)
{
    static const QStringList kinds = {
        QStringLiteral("person"),    QStringLiteral("album"),   QStringLiteral("tag"),
        QStringLiteral("favorites"), QStringLiteral("archive"), QStringLiteral("trash"),
        QStringLiteral("folder")};
    return kinds.contains(kind);
}

void ImmichClient::loadCollectionAssets(const QString &kind, const QString &value, int page,
                                        const AssetSort &sort)
{
    if (page <= 1)
        ++m_collectionGeneration;
    if (kind == QStringLiteral("viewed")) {
        // Local history; queued so callers see the same async flow as a fetch.
        QTimer::singleShot(0, this, [this, sort] {
            emit filteredAssetsLoaded(QStringLiteral("viewed"), {}, recentlyViewed(), {}, sort);
        });
    } else if (kind == QStringLiteral("folder"))
        loadAssetsForFolder(value, sort);
    else if (sort.field == AssetSort::Field::AddedAt && canSortByDateAdded(kind))
        loadCollectionTimeline(kind, value, page, sort);
    else if (sort.field == AssetSort::Field::Activity && kind == QStringLiteral("album"))
        loadAlbumByActivity(value, sort);
    else if (sort.field == AssetSort::Field::Deleted && kind == QStringLiteral("trash"))
        loadTrashByDeletion(sort);
    else if (sort.field == AssetSort::Field::Viewed) {
        const quint64 generation = m_collectionGeneration;
        searchViewedFirst(collectionSearchFilter(kind, value), page, sort,
                          [this, kind, value, sort, generation](const QList<ImmichAsset> &assets,
                                                                const QString &nextPage) {
                              if (generation == m_collectionGeneration)
                                  emit filteredAssetsLoaded(kind, value, assets, nextPage, sort);
                          });
    }
    else
        searchFilteredAssets(kind, value, page, 80, sort);
}

// Fetches every page of a metadata search, for orders the server can't do.
void ImmichClient::searchAllPages(const QJsonObject &filter,
                                  std::function<void(const QList<ImmichAsset> &)> done)
{
    auto assets = std::make_shared<QList<ImmichAsset>>();
    auto fetchPage = std::make_shared<std::function<void(int)>>();
    const quint64 generation = m_collectionGeneration;
    *fetchPage = [this, filter, done, assets, fetchPage, generation](int page) {
        QJsonObject body = filter;
        body.insert(QStringLiteral("page"), page);
        body.insert(QStringLiteral("size"), 250);
        body.insert(QStringLiteral("withExif"), true);
        body.insert(QStringLiteral("withStacked"), true);
        QNetworkRequest request = authenticatedRequest(apiUrl(QStringLiteral("/search/metadata")));
        request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
        request.setRawHeader("Accept", "application/json");
        auto *reply =
            m_network->post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
        connect(reply, &QNetworkReply::finished, this,
                [this, reply, done, assets, fetchPage, generation, page] {
                    const QByteArray body = reply->readAll();
                    reply->deleteLater();
                    if (generation != m_collectionGeneration)
                        return;
                    if (reply->error() != QNetworkReply::NoError) {
                        emit requestFailed(tr("Load explore"), errorMessage(reply, body));
                        *fetchPage = nullptr; // Break the self-reference.
                        return;
                    }
                    QString nextPage;
                    assets->append(assetsFromSearchBody(body, &nextPage));
                    if (!nextPage.isEmpty()) {
                        (*fetchPage)(page + 1);
                        return;
                    }
                    done(*assets);
                    *fetchPage = nullptr;
                });
    };
    (*fetchPage)(1);
}

// Shared albums with likes and comments: the photos with the most recent
// activity first, then the rest by date taken.
void ImmichClient::loadAlbumByActivity(const QString &albumId, const AssetSort &sort)
{
    if (!ensureConfigured(tr("Load explore")))
        return;
    QUrl url = apiUrl(QStringLiteral("/activities"));
    url.setQuery(QUrlQuery{{QStringLiteral("albumId"), albumId}});
    QNetworkRequest request = authenticatedRequest(url);
    request.setRawHeader("Accept", "application/json");
    auto *reply = m_network->get(request);
    const quint64 generation = m_collectionGeneration;
    connect(reply, &QNetworkReply::finished, this, [this, reply, albumId, sort, generation] {
        const QByteArray body = reply->readAll();
        reply->deleteLater();
        if (generation != m_collectionGeneration)
            return;
        if (reply->error() != QNetworkReply::NoError) {
            const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            emit requestFailed(tr("Load explore"),
                               status == 403
                                   ? tr("Likes and comments need the activity.read permission.")
                                   : errorMessage(reply, body));
            return;
        }
        QHash<QString, QDateTime> latest;
        for (const QJsonValue &value : QJsonDocument::fromJson(body).array()) {
            const QJsonObject activity = value.toObject();
            const QString assetId = activity.value(QStringLiteral("assetId")).toString();
            const QDateTime when = QDateTime::fromString(
                activity.value(QStringLiteral("createdAt")).toString(), Qt::ISODate);
            if (!assetId.isEmpty() && when > latest.value(assetId))
                latest.insert(assetId, when);
        }
        searchAllPages(collectionSearchFilter(QStringLiteral("album"), albumId),
                       [this, albumId, sort, latest](const QList<ImmichAsset> &found) {
                           QList<ImmichAsset> assets = found;
                           std::stable_sort(assets.begin(), assets.end(),
                                            [&latest, &sort](const ImmichAsset &a,
                                                             const ImmichAsset &b) {
                               const QDateTime first = latest.value(a.id);
                               const QDateTime second = latest.value(b.id);
                               if (first.isValid() != second.isValid())
                                   return first.isValid();
                               if (first.isValid() && first != second)
                                   return sort.ascending ? first < second : first > second;
                               return a.takenAt > b.takenAt;
                           });
                           emit filteredAssetsLoaded(QStringLiteral("album"), albumId, assets,
                                                     {}, sort);
                       });
    });
}

// Assets don't expose when they were trashed, but trashing updates their
// "updated" time, which is as good in practice.
void ImmichClient::loadTrashByDeletion(const AssetSort &sort)
{
    if (!ensureConfigured(tr("Load explore")))
        return;
    searchAllPages(collectionSearchFilter(QStringLiteral("trash"), {}),
                   [this, sort](const QList<ImmichAsset> &found) {
                       QList<ImmichAsset> assets = found;
                       std::stable_sort(assets.begin(), assets.end(),
                                        [&sort](const ImmichAsset &a, const ImmichAsset &b) {
                                            return sort.ascending ? a.updatedAt < b.updatedAt
                                                                  : a.updatedAt > b.updatedAt;
                                        });
                       emit filteredAssetsLoaded(QStringLiteral("trash"), {}, assets, {}, sort);
                   });
}

void ImmichClient::loadRandomAssets(const QString &kind, const QString &value)
{
    if (!ensureConfigured(tr("Load explore")))
        return;
    auto shuffled = [](QList<ImmichAsset> assets) {
        std::shuffle(assets.begin(), assets.end(), *QRandomGenerator::global());
        return assets;
    };
    if (kind == QStringLiteral("viewed")) {
        QTimer::singleShot(0, this, [this, shuffled] {
            emit randomAssetsLoaded(QStringLiteral("viewed"), {}, shuffled(recentlyViewed()), false);
        });
        return;
    }
    if (kind == QStringLiteral("folder")) {
        QUrl url = apiUrl(QStringLiteral("/view/folder"));
        url.setQuery(QUrlQuery{{QStringLiteral("path"), value}});
        fetchBrowseList(QStringLiteral("folder"), url, [this, value, shuffled](const QByteArray &body) {
            QList<ImmichAsset> assets;
            for (const QJsonValue &item : QJsonDocument::fromJson(body).array()) {
                const ImmichAsset asset = assetFromObject(item.toObject());
                if (!asset.id.isEmpty())
                    assets.append(asset);
            }
            emit randomAssetsLoaded(QStringLiteral("folder"), value, shuffled(assets), false);
        });
        return;
    }

    QJsonObject body = kind == QStringLiteral("library")
                           ? QJsonObject{{QStringLiteral("visibility"), QStringLiteral("timeline")}}
                           : collectionSearchFilter(kind, value);
    if (body.isEmpty())
        return;
    body.insert(QStringLiteral("size"), 80);
    body.insert(QStringLiteral("withExif"), true);
    QNetworkRequest request = authenticatedRequest(apiUrl(QStringLiteral("/search/random")));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setRawHeader("Accept", "application/json");
    auto *reply = m_network->post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply, kind, value] {
        const QByteArray body = reply->readAll();
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit requestFailed(tr("Load explore"), errorMessage(reply, body));
            return;
        }
        QList<ImmichAsset> assets;
        for (const QJsonValue &item : QJsonDocument::fromJson(body).array()) {
            const ImmichAsset asset = assetFromObject(item.toObject());
            if (!asset.id.isEmpty())
                assets.append(asset);
        }
        emit randomAssetsLoaded(kind, value, assets, !assets.isEmpty());
    });
}

void ImmichClient::setRememberViewed(bool remember)
{
    TimelineSettings timeline = AppSettings().loadTimeline();
    timeline.rememberViewed = remember;
    AppSettings().saveTimeline(timeline);
    m_rememberViewed = remember;
    emit recentlyViewedChanged();
}

// Page 1 holds the viewed photos that are in this view, found with one search
// (an OR of their ids, Immich 3.2+) and put in viewing order. Later pages are the
// usual newest-first pages; callers skip photos they already show.
void ImmichClient::searchViewedFirst(
    const QJsonObject &flatFilter, int page, const AssetSort &sort,
    std::function<void(const QList<ImmichAsset> &, const QString &)> done)
{
    if (!ensureConfigured(tr("Load explore")))
        return;
    const QList<ImmichAsset> history = recentlyViewed();
    QJsonObject body;
    if (page <= 1 && !history.isEmpty()) {
        body = toFilterShape(flatFilter);
        QJsonObject filter = body.value(QStringLiteral("filter")).toObject();
        QJsonArray branches;
        for (const ImmichAsset &asset : history)
            branches.append(QJsonObject{{QStringLiteral("id"), QJsonObject{{QStringLiteral("eq"), asset.id}}}});
        filter.insert(QStringLiteral("or"), branches);
        body.insert(QStringLiteral("filter"), filter);
        body.insert(QStringLiteral("size"), int(history.size()));
        body.insert(QStringLiteral("withExif"), true);
    } else {
        // The rest, newest first; with no history page 1 starts here.
        body = flatFilter;
        body.insert(QStringLiteral("page"), history.isEmpty() ? page : page - 1);
        body.insert(QStringLiteral("size"), 80);
        body.insert(QStringLiteral("order"), QStringLiteral("desc"));
        body.insert(QStringLiteral("withExif"), true);
        body.insert(QStringLiteral("withStacked"), true);
    }
    const bool viewedPage = body.contains(QStringLiteral("filter"));
    QNetworkRequest request = authenticatedRequest(apiUrl(QStringLiteral("/search/metadata")));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setRawHeader("Accept", "application/json");
    auto *reply = m_network->post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this,
            [this, reply, history, sort, viewedPage, done] {
                const QByteArray body = reply->readAll();
                reply->deleteLater();
                if (reply->error() != QNetworkReply::NoError) {
                    emit requestFailed(tr("Load explore"), errorMessage(reply, body));
                    return;
                }
                QString nextPage;
                QList<ImmichAsset> assets = assetsFromSearchBody(body, &nextPage);
                if (viewedPage) {
                    QHash<QString, int> order;
                    for (int i = 0; i < history.size(); ++i)
                        order.insert(history.at(i).id, i);
                    std::sort(assets.begin(), assets.end(),
                              [&order, &sort](const ImmichAsset &a, const ImmichAsset &b) {
                                  const int first = order.value(a.id);
                                  const int second = order.value(b.id);
                                  return sort.ascending ? first > second : first < second;
                              });
                    done(assets, QStringLiteral("2"));
                    return;
                }
                // Our page numbers run one ahead of the server's while page 1 is the history.
                const int serverNext = nextPage.toInt();
                done(assets, serverNext > 0 ? QString::number(history.isEmpty() ? serverNext
                                                                                 : serverNext + 1)
                                            : QString());
            });
}

void ImmichClient::loadLibraryViewedFirst(int page, const AssetSort &sort)
{
    const quint64 generation = ++m_searchGeneration;
    searchViewedFirst({{QStringLiteral("visibility"), QStringLiteral("timeline")}}, page, sort,
                      [this, generation](const QList<ImmichAsset> &assets, const QString &nextPage) {
                          if (generation == m_searchGeneration)
                              emit assetsLoaded(assets, nextPage, {}, false);
                      });
}

void ImmichClient::recordViewed(const ImmichAsset &asset)
{
    if (!isConfigured() || asset.id.isEmpty() || asset.isLocalPending() || !remembersViewed())
        return;
    constexpr int kMaxHistory = 300;
    QList<ImmichAsset> history = m_offlineStore.loadViewHistory(m_connection.serverUrl);
    history.removeIf([&asset](const ImmichAsset &seen) { return seen.id == asset.id; });
    history.prepend(asset);
    if (history.size() > kMaxHistory)
        history.resize(kMaxHistory);
    m_offlineStore.saveViewHistory(m_connection.serverUrl, history);

    QHash<QString, AssetViewStats> stats = m_offlineStore.loadViewStats(m_connection.serverUrl);
    AssetViewStats &entry = stats[asset.id];
    entry.asset = asset;
    entry.viewedAt.append(QDateTime::currentSecsSinceEpoch());
    m_offlineStore.saveViewStats(m_connection.serverUrl, stats);
    emit recentlyViewedChanged();
}

QList<ImmichAsset> ImmichClient::recentlyViewed() const
{
    return isConfigured() && remembersViewed()
               ? m_offlineStore.loadViewHistory(m_connection.serverUrl)
               : QList<ImmichAsset>{};
}

void ImmichClient::clearRecentlyViewed()
{
    m_offlineStore.saveViewHistory(m_connection.serverUrl, {});
    m_offlineStore.saveViewStats(m_connection.serverUrl, {});
    emit recentlyViewedChanged();
}

void ImmichClient::setAlbumOrder(const QString &albumId, bool ascending)
{
    if (!ensureConfigured(tr("Album order")) || !isValidImmichId(albumId))
        return;
    QNetworkRequest request =
        authenticatedRequest(apiUrl(QStringLiteral("/albums/%1").arg(albumId)));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setRawHeader("Accept", "application/json");
    const QJsonObject body{
        {QStringLiteral("order"), ascending ? QStringLiteral("asc") : QStringLiteral("desc")}};
    auto *reply = m_network->sendCustomRequest(request, QByteArrayLiteral("PATCH"),
                                               QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply, albumId, ascending] {
        const QByteArray body = reply->readAll();
        reply->deleteLater();
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status == 403)
            emit albumOrderFailed(albumId,
                                  tr("Saving the album's order in Immich needs the album.update "
                                     "permission; it's kept in this app only."));
        else if (reply->error() != QNetworkReply::NoError)
            emit albumOrderFailed(albumId, errorMessage(reply, body));
        else
            emit albumOrderSaved(albumId, ascending);
    });
}

// Search can't order by upload date, but the timeline can (Immich 3.0+), so a
// date-added collection is fetched as timeline months. A "page" is a run of
// months holding about 80 photos; page N starts at month N - 1.
void ImmichClient::loadCollectionTimeline(const QString &kind, const QString &value, int page,
                                          const AssetSort &sort)
{
    if (!ensureConfigured(tr("Load explore")))
        return;
    CollectionTimeline &timeline = m_collectionTimeline;
    const bool sameCollection =
        timeline.kind == kind && timeline.value == value && timeline.sort == sort;
    if (page > 1 && sameCollection) {
        loadCollectionTimelinePage(page - 1);
        return;
    }

    timeline = {kind, value, sort, {}, timeline.generation + 1};
    QUrl url = apiUrl(QStringLiteral("/timeline/buckets"));
    QUrlQuery query = collectionTimelineQuery(kind, value);
    query.addQueryItem(QStringLiteral("size"), QStringLiteral("MONTH"));
    addSortQuery(&query, sort);
    url.setQuery(query);
    QNetworkRequest request = authenticatedRequest(url);
    request.setRawHeader("Accept", "application/json");
    auto *reply = m_network->get(request);
    connect(reply, &QNetworkReply::finished, this,
            [this, reply, generation = timeline.generation] {
                const QByteArray body = reply->readAll();
                reply->deleteLater();
                if (generation != m_collectionTimeline.generation)
                    return;
                if (reply->error() != QNetworkReply::NoError) {
                    emit requestFailed(tr("Load explore"), errorMessage(reply, body));
                    return;
                }
                m_collectionTimeline.buckets = bucketsFromBody(body, m_collectionTimeline.sort);
                loadCollectionTimelinePage(0);
            });
}

void ImmichClient::loadCollectionTimelinePage(int firstBucket)
{
    const CollectionTimeline &timeline = m_collectionTimeline;
    int lastBucket = firstBucket;
    int count = 0;
    while (lastBucket < timeline.buckets.size() && (count == 0 || count < 80))
        count += timeline.buckets.at(lastBucket++).count;
    if (firstBucket >= lastBucket) {
        emit filteredAssetsLoaded(timeline.kind, timeline.value, {}, {}, timeline.sort);
        return;
    }

    struct PageLoad {
        QList<QList<ImmichAsset>> months;
        int remaining = 0;
        QString error;
    };
    auto load = std::make_shared<PageLoad>();
    load->months.resize(lastBucket - firstBucket);
    load->remaining = lastBucket - firstBucket;
    const QString nextPage =
        lastBucket < timeline.buckets.size() ? QString::number(lastBucket + 1) : QString();
    const bool timelineOnly = timeline.kind != QStringLiteral("archive") &&
                              timeline.kind != QStringLiteral("trash");

    for (int i = firstBucket; i < lastBucket; ++i) {
        const QDate month = timeline.buckets.at(i).month;
        QUrl url = apiUrl(QStringLiteral("/timeline/bucket"));
        QUrlQuery query = collectionTimelineQuery(timeline.kind, timeline.value);
        query.addQueryItem(QStringLiteral("timeBucket"), month.toString(Qt::ISODate));
        query.addQueryItem(QStringLiteral("size"), QStringLiteral("MONTH"));
        addSortQuery(&query, timeline.sort);
        url.setQuery(query);
        QNetworkRequest request = authenticatedRequest(url);
        request.setRawHeader("Accept", "application/json");
        auto *reply = m_network->get(request);
        connect(reply, &QNetworkReply::finished, this,
                [this, reply, load, index = i - firstBucket, nextPage, timelineOnly,
                 generation = timeline.generation] {
                    const QByteArray body = reply->readAll();
                    reply->deleteLater();
                    if (generation != m_collectionTimeline.generation)
                        return;
                    if (reply->error() != QNetworkReply::NoError)
                        load->error = errorMessage(reply, body);
                    else
                        load->months[index] = assetsFromBucketBody(body, timelineOnly);
                    if (--load->remaining > 0)
                        return;
                    if (!load->error.isEmpty()) {
                        emit requestFailed(tr("Load explore"), load->error);
                        return;
                    }
                    QList<ImmichAsset> assets;
                    for (const QList<ImmichAsset> &month : load->months)
                        assets.append(month);
                    emit filteredAssetsLoaded(m_collectionTimeline.kind,
                                              m_collectionTimeline.value, assets, nextPage,
                                              m_collectionTimeline.sort);
                });
    }
}

void ImmichClient::loadAlbums()
{
    fetchBrowseList(QStringLiteral("albums"), apiUrl(QStringLiteral("/albums")),
                    [this](const QByteArray &body) {
                        QList<ImmichAlbum> albums;
                        for (const QJsonValue &value : QJsonDocument::fromJson(body).array()) {
                            const ImmichAlbum album = albumFromObject(value.toObject());
                            if (!album.id.isEmpty())
                                albums.append(album);
                        }
                        std::sort(albums.begin(), albums.end(),
                                  [](const ImmichAlbum &a, const ImmichAlbum &b) {
                                      return QString::localeAwareCompare(a.name, b.name) < 0;
                                  });
                        emit albumsLoaded(albums);
                    });
}

void ImmichClient::loadTags()
{
    fetchBrowseList(QStringLiteral("tags"), apiUrl(QStringLiteral("/tags")),
                    [this](const QByteArray &body) {
                        QList<ImmichTag> tags;
                        for (const QJsonValue &value : QJsonDocument::fromJson(body).array()) {
                            const QJsonObject object = value.toObject();
                            ImmichTag tag;
                            tag.id = object.value(QStringLiteral("id")).toString();
                            tag.name = object.value(QStringLiteral("name")).toString();
                            tag.path = object.value(QStringLiteral("value")).toString(tag.name);
                            tag.parentId = object.value(QStringLiteral("parentId")).toString();
                            if (!tag.id.isEmpty())
                                tags.append(tag);
                        }
                        std::sort(tags.begin(), tags.end(),
                                  [](const ImmichTag &a, const ImmichTag &b) {
                                      return QString::localeAwareCompare(a.path, b.path) < 0;
                                  });
                        emit tagsLoaded(tags);
                    });
}

void ImmichClient::loadFolderPaths()
{
    fetchBrowseList(QStringLiteral("folders"), apiUrl(QStringLiteral("/view/folder/unique-paths")),
                    [this](const QByteArray &body) {
                        QStringList paths;
                        for (const QJsonValue &value : QJsonDocument::fromJson(body).array())
                            paths.append(value.toString());
                        paths.removeAll(QString());
                        paths.sort();
                        emit folderPathsLoaded(paths);
                    });
}

void ImmichClient::loadPeople()
{
    QUrl url = apiUrl(QStringLiteral("/people"));
    url.setQuery(QUrlQuery{{QStringLiteral("withHidden"), QStringLiteral("true")},
                           {QStringLiteral("size"), QStringLiteral("1000")}});
    fetchBrowseList(QStringLiteral("people"), url, [this](const QByteArray &body) {
        QList<ImmichPerson> people;
        const QJsonArray items =
            QJsonDocument::fromJson(body).object().value(QStringLiteral("people")).toArray();
        for (const QJsonValue &value : items) {
            const QJsonObject object = value.toObject();
            ImmichPerson person;
            person.id = object.value(QStringLiteral("id")).toString();
            person.name = object.value(QStringLiteral("name")).toString();
            person.favorite = object.value(QStringLiteral("isFavorite")).toBool();
            person.hidden = object.value(QStringLiteral("isHidden")).toBool();
            if (!person.id.isEmpty())
                people.append(person);
        }
        emit peopleLoaded(people);
    });
}

void ImmichClient::fetchBrowseList(const QString &kind, const QUrl &url,
                                   std::function<void(const QByteArray &)> handle)
{
    if (!isConfigured())
        return;
    QNetworkRequest request = authenticatedRequest(url);
    request.setRawHeader("Accept", "application/json");
    auto *reply = m_network->get(request);
    connect(reply, &QNetworkReply::finished, this,
            [this, reply, kind, handle = std::move(handle)] {
                const QByteArray body = reply->readAll();
                if (reply->error() == QNetworkReply::NoError) {
                    handle(body);
                } else {
                    const int status =
                        reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
                    emit browseListFailed(kind, status == 403);
                }
                reply->deleteLater();
            });
}

void ImmichClient::searchFilteredAssets(const QString &filterKind, const QString &filterValue,
                                        int page, int pageSize, const AssetSort &sort)
{
    // Favorites, archive and trash are whole-library views without a value.
    const bool needsValue = filterKind != QStringLiteral("favorites") &&
                            filterKind != QStringLiteral("archive") &&
                            filterKind != QStringLiteral("trash");
    if (!ensureConfigured(tr("Load explore")) || (needsValue && filterValue.trimmed().isEmpty()))
        return;

    QJsonObject body = collectionSearchFilter(filterKind, filterValue);
    if (body.isEmpty())
        return;
    body.insert(QStringLiteral("page"), qMax(1, page));
    body.insert(QStringLiteral("size"), qBound(1, pageSize, 250));
    body.insert(QStringLiteral("withExif"), true);
    body.insert(QStringLiteral("withStacked"), true);
    const QJsonObject order = searchOrder(sort);
    for (auto it = order.begin(); it != order.end(); ++it)
        body.insert(it.key(), it.value());
    const bool filterShape = body.contains(QStringLiteral("orderBy"));
    if (filterShape) {
        body = toFilterShape(body);
        if (page > 1)
            body.insert(QStringLiteral("cursor"), m_searchCursors.value(QStringLiteral("collection")));
    }

    QNetworkRequest request = authenticatedRequest(apiUrl(QStringLiteral("/search/metadata")));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setRawHeader("Accept", "application/json");
    auto *reply = m_network->post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this,
            [this, reply, filterKind, filterValue, sort, filterShape, page] {
                const QByteArray body = reply->readAll();
                if (reply->error() != QNetworkReply::NoError) {
                    emit requestFailed(tr("Load explore"), errorMessage(reply, body));
                    reply->deleteLater();
                    return;
                }
                QString nextPage;
                QString nextCursor;
                const QList<ImmichAsset> assets = assetsFromSearchBody(body, &nextPage, &nextCursor);
                if (filterShape) {
                    m_searchCursors.insert(QStringLiteral("collection"), nextCursor);
                    nextPage = nextCursor.isEmpty() ? QString() : QString::number(page + 1);
                }
                emit filteredAssetsLoaded(filterKind, filterValue, assets, nextPage, sort);
                reply->deleteLater();
            });
}

void ImmichClient::loadPersonThumbnail(const QString &personId)
{
    if (!ensureConfigured(tr("Load image")))
        return;
    if (!isValidImmichId(personId)) {
        emit imageLoadFailed(personId, QStringLiteral("person"),
                             tr("The person identifier is invalid."));
        return;
    }

    const QString cacheKey = QStringLiteral("person:") + personId;
    if (const QPixmap cached = m_thumbnailCache.memoryPixmap(cacheKey); !cached.isNull()) {
        emit personThumbnailLoaded(personId, cached);
        return;
    }
    const QByteArray diskBytes = m_thumbnailCache.readDisk(cacheKey);
    if (!diskBytes.isEmpty()) {
        const QImage image = decodeImage(diskBytes, 256);
        if (!image.isNull()) {
            const QPixmap pixmap = QPixmap::fromImage(image);
            m_thumbnailCache.store(cacheKey, {}, pixmap);
            emit personThumbnailLoaded(personId, pixmap);
            return;
        }
    }

    if (!m_online) {
        emit imageLoadFailed(personId, QStringLiteral("person"),
                             tr("Offline - person photo not cached."));
        return;
    }

    if (m_pendingPersonImages.contains(personId))
        return;
    m_pendingPersonImages.insert(personId);

    auto *reply =
        m_network->get(authenticatedRequest(apiUrl(QStringLiteral("/people/%1/thumbnail").arg(personId))));
    connect(reply, &QNetworkReply::finished, this, [this, reply, personId, cacheKey] {
        m_pendingPersonImages.remove(personId);
        const QByteArray body = reply->readAll();
        if (reply->error() != QNetworkReply::NoError) {
            if (isTransientNetworkError(reply))
                setOnline(false);
            emit imageLoadFailed(personId, QStringLiteral("person"),
                                 errorMessage(reply, body));
            reply->deleteLater();
            return;
        }
        const QImage image = decodeImage(body, 256);
        if (image.isNull()) {
            emit imageLoadFailed(personId, QStringLiteral("person"),
                                 tr("Unsupported image format."));
        } else {
            const QPixmap pixmap = QPixmap::fromImage(image);
            QByteArray encoded;
            QBuffer buffer(&encoded);
            if (buffer.open(QIODevice::WriteOnly))
                image.save(&buffer, "JPG", 82);
            m_thumbnailCache.store(cacheKey, encoded, pixmap);
            emit personThumbnailLoaded(personId, pixmap);
        }
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

void ImmichClient::loadThumbnail(const QString &assetId, ImageSource source,
                                 FetchPriority priority)
{
    if (!ensureConfigured(tr("Load image")) || assetId.isEmpty())
        return;

    if (const QPixmap cached = m_thumbnailCache.memoryPixmap(assetId); !cached.isNull()) {
        emit thumbnailLoaded(assetId, cached);
        notePrefetchThumbnailDone(assetId, false);
        return;
    }

    loadImageAsync(assetId, QStringLiteral("thumbnail"), source, priority);
}

void ImmichClient::prefetchAllThumbnails()
{
    if (m_prefetchActive || !ensureConfigured(tr("Download thumbnails")))
        return;

    m_prefetchActive = true;
    m_prefetchListingDone = false;
    m_prefetchTotal = 0;
    m_prefetchDownloaded = 0;
    m_prefetchSkipped = 0;
    m_prefetchPending.clear();
    emitPrefetchProgress();
    prefetchLibraryPage(1);
}

void ImmichClient::cancelThumbnailPrefetch()
{
    if (!m_prefetchActive)
        return;
    m_prefetchActive = false;
    m_prefetchListingDone = true;
    m_prefetchPending.clear();
    emit thumbnailPrefetchFinished(m_prefetchDownloaded, m_prefetchSkipped);
}

bool ImmichClient::isPrefetchingThumbnails() const
{
    return m_prefetchActive;
}

void ImmichClient::prefetchLibraryPage(int page)
{
    if (!m_prefetchActive)
        return;

    if (!m_online) {
        QList<ImmichAsset> cached;
        if (m_offlineStore.loadLibrary(m_connection.serverUrl, &cached)) {
            for (const ImmichAsset &asset : cached)
                queuePrefetchThumbnail(asset.id);
        }
        m_prefetchListingDone = true;
        emitPrefetchProgress();
        if (m_prefetchPending.isEmpty()) {
            m_prefetchActive = false;
            emit thumbnailPrefetchFinished(m_prefetchDownloaded, m_prefetchSkipped);
        }
        return;
    }

    QJsonObject body;
    body.insert(QStringLiteral("visibility"), QStringLiteral("timeline"));
    body.insert(QStringLiteral("page"), qMax(1, page));
    body.insert(QStringLiteral("size"), 250);
    body.insert(QStringLiteral("order"), QStringLiteral("desc"));
    body.insert(QStringLiteral("withExif"), false);
    body.insert(QStringLiteral("withStacked"), true);

    QNetworkRequest request = authenticatedRequest(apiUrl(QStringLiteral("/search/metadata")));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setRawHeader("Accept", "application/json");
    request.setTransferTimeout(30000);
    auto *reply = m_network->post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply, page] {
        const QByteArray body = reply->readAll();
        if (!m_prefetchActive) {
            reply->deleteLater();
            return;
        }
        if (reply->error() != QNetworkReply::NoError) {
            reply->deleteLater();
            m_prefetchListingDone = true;
            emitPrefetchProgress();
            if (m_prefetchPending.isEmpty()) {
                m_prefetchActive = false;
                emit thumbnailPrefetchFinished(m_prefetchDownloaded, m_prefetchSkipped);
                emit requestFailed(tr("Download thumbnails"), errorMessage(reply, body));
            }
            return;
        }

        setOnline(true);
        QString nextPage;
        const QList<ImmichAsset> assets = assetsFromSearchBody(body, &nextPage);
        for (const ImmichAsset &asset : assets)
            queuePrefetchThumbnail(asset.id);
        emitPrefetchProgress();
        reply->deleteLater();

        bool ok = false;
        const int next = nextPage.toInt(&ok);
        if (ok && next > page) {
            prefetchLibraryPage(next);
            return;
        }

        m_prefetchListingDone = true;
        if (m_prefetchPending.isEmpty()) {
            m_prefetchActive = false;
            emit thumbnailPrefetchFinished(m_prefetchDownloaded, m_prefetchSkipped);
        }
    });
}

void ImmichClient::queuePrefetchThumbnail(const QString &assetId)
{
    if (!m_prefetchActive || assetId.isEmpty() || m_prefetchPending.contains(assetId))
        return;
    ++m_prefetchTotal;
    if (m_thumbnailCache.hasDisk(assetId)) {
        ++m_prefetchSkipped;
        return;
    }
    m_prefetchPending.insert(assetId);
    loadThumbnail(assetId, ImageSource::CacheOrServer, FetchPriority::Normal);
}

void ImmichClient::notePrefetchThumbnailDone(const QString &assetId, bool downloaded)
{
    if (!m_prefetchPending.remove(assetId))
        return;
    if (downloaded)
        ++m_prefetchDownloaded;
    else
        ++m_prefetchSkipped;
    emitPrefetchProgress();
    if (m_prefetchActive && m_prefetchListingDone && m_prefetchPending.isEmpty()) {
        m_prefetchActive = false;
        emit thumbnailPrefetchFinished(m_prefetchDownloaded, m_prefetchSkipped);
    }
}

void ImmichClient::emitPrefetchProgress()
{
    emit thumbnailPrefetchProgress(m_prefetchDownloaded + m_prefetchSkipped, m_prefetchTotal,
                                   m_prefetchPending.size());
}

void ImmichClient::loadPreview(const QString &assetId)
{
    loadImageAsync(assetId, QStringLiteral("preview"));
}

void ImmichClient::loadTileImage(const QString &assetId, int maximumDimension,
                                 ImageSource source, FetchPriority priority)
{
    if (maximumDimension <= 256) {
        loadThumbnail(assetId, source, priority);
        return;
    }
    loadImageAsync(assetId, QStringLiteral("tile:%1").arg(maximumDimension), source, priority);
}

bool ImmichClient::isUploading() const
{
    return m_uploadInFlight || !m_uploadQueue.isEmpty();
}

bool ImmichClient::isDownloading() const
{
    return m_activeDownloads > 0;
}

int ImmichClient::pendingUploadCount() const
{
    return m_uploadQueue.size() + (m_uploadInFlight ? 1 : 0);
}

int ImmichClient::activeDownloadCount() const
{
    return m_activeDownloads;
}

void ImmichClient::beginActiveDownload()
{
    ++m_activeDownloads;
    emit transferActivityChanged();
}

void ImmichClient::endActiveDownload()
{
    if (m_activeDownloads > 0)
        --m_activeDownloads;
    emit transferActivityChanged();
}

void ImmichClient::uploadAssets(const QStringList &filePaths)
{
    if (!ensureConfigured(tr("Upload")))
        return;

    QStringList added;
    for (const QString &path : filePaths) {
        const QString absolute = QFileInfo(path).absoluteFilePath();
        if (absolute.isEmpty() || !QFileInfo::exists(absolute))
            continue;
        if (!m_uploadQueue.contains(absolute) && absolute != m_uploadInFlightPath) {
            m_uploadQueue.append(absolute);
            added.append(absolute);
        }
    }
    if (added.isEmpty() && filePaths.isEmpty())
        return;

    persistUploadQueue();
    emit uploadQueueChanged(pendingUploadCount());
    emit transferActivityChanged();

    if (!m_online)
        return;
    processUploadQueue();
}

void ImmichClient::persistUploadQueue()
{
    QStringList all = m_uploadQueue;
    if (!m_uploadInFlightPath.isEmpty() && !all.contains(m_uploadInFlightPath))
        all.prepend(m_uploadInFlightPath);
    m_uploadQueueStore.save(all);
}

void ImmichClient::restoreUploadQueue()
{
    const QStringList stored = m_uploadQueueStore.load();
    for (const QString &path : stored) {
        if (!m_uploadQueue.contains(path))
            m_uploadQueue.append(path);
    }
    if (!m_uploadQueue.isEmpty())
        emit uploadQueueChanged(pendingUploadCount());
}

void ImmichClient::requeueUpload(const QString &filePath, bool toFront)
{
    const QString absolute = QFileInfo(filePath).absoluteFilePath();
    if (absolute.isEmpty())
        return;
    m_uploadQueue.removeAll(absolute);
    if (toFront)
        m_uploadQueue.prepend(absolute);
    else
        m_uploadQueue.append(absolute);
    persistUploadQueue();
    emit uploadQueueChanged(pendingUploadCount());
}

void ImmichClient::scheduleUploadRetry(int delayMs)
{
    if (m_uploadRetryScheduled)
        return;
    m_uploadRetryScheduled = true;
    QTimer::singleShot(delayMs, this, [this] {
        m_uploadRetryScheduled = false;
        processUploadQueue();
    });
}

void ImmichClient::processUploadQueue()
{
    if (m_processingUploadQueue || m_uploadInFlight || !m_online || m_uploadRetryScheduled)
        return;

    m_processingUploadQueue = true;
    // Bound passes so a failed open+requeue cannot spin forever on the UI thread.
    int remainingPasses = m_uploadQueue.size();
    while (!m_uploadInFlight && !m_uploadQueue.isEmpty() && m_online && remainingPasses > 0) {
        --remainingPasses;
        const QString next = m_uploadQueue.takeFirst();
        if (startUpload(next))
            break;
        if (m_uploadRetryScheduled)
            break;
    }
    m_processingUploadQueue = false;
    persistUploadQueue();
    emit uploadQueueChanged(pendingUploadCount());
    emit transferActivityChanged();
}

bool ImmichClient::startUpload(const QString &filePath)
{
    const QFileInfo info(filePath);
    if (!info.exists() || !info.isFile()) {
        m_uploadRetryCounts.remove(filePath);
        m_uploadQueueStore.remove(filePath);
        const QString message = tr("File not found: %1").arg(filePath);
        emit assetUploadFailed(filePath, message);
        emit requestFailed(tr("Upload"), message);
        return false;
    }

    if (info.size() <= 0) {
        m_uploadRetryCounts.remove(filePath);
        m_uploadQueueStore.remove(filePath);
        const QString message = tr("File is empty: %1").arg(info.fileName());
        emit assetUploadFailed(filePath, message);
        emit requestFailed(tr("Upload"), message);
        return false;
    }

    auto *file = new QFile(filePath);
    if (!file->open(QIODevice::ReadOnly)) {
        const QString message = file->errorString();
        delete file;
        const int attempts = ++m_uploadRetryCounts[filePath];
        if (attempts >= 5) {
            m_uploadRetryCounts.remove(filePath);
            m_uploadQueueStore.remove(filePath);
            const QString detail =
                tr("Could not open %1 after several tries: %2")
                    .arg(info.fileName(), message);
            emit assetUploadFailed(filePath, detail);
            emit requestFailed(tr("Upload"), detail);
        } else {
            // Defer retry - never recurse into processUploadQueue here.
            requeueUpload(filePath, false);
            scheduleUploadRetry(2000 * attempts);
            emit requestFailed(
                tr("Upload"),
                tr("Could not open %1: %2 (will retry)")
                    .arg(info.fileName(), message), true);
        }
        return false;
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
    m_uploadInFlightPath = filePath;
    persistUploadQueue();
    auto *reply = m_network->post(request, multiPart);
    multiPart->setParent(reply);

    connect(reply, &QNetworkReply::uploadProgress, this,
            [this, filePath](qint64 bytesSent, qint64 bytesTotal) {
                emit uploadProgress(filePath, bytesSent, bytesTotal);
            });
    connect(reply, &QNetworkReply::finished, this, [this, reply, filePath] {
        // Capture error before any further work; reply stays valid until deleteLater.
        const auto replyError = reply->error();
        const QByteArray body = reply->readAll();
        const bool transient = replyError != QNetworkReply::NoError &&
                               isTransientNetworkError(reply);

        m_uploadInFlight = false;
        m_uploadInFlightPath.clear();

        if (replyError != QNetworkReply::NoError) {
            if (transient) {
                setOnline(false);
                requeueUpload(filePath, true);
                emit requestFailed(
                    tr("Upload"),
                    tr("%1 interrupted - queued for retry when online.")
                        .arg(QFileInfo(filePath).fileName()), true);
            } else {
                m_uploadRetryCounts.remove(filePath);
                m_uploadQueueStore.remove(filePath);
                persistUploadQueue();
                const QString detail =
                    tr("%1: %2").arg(QFileInfo(filePath).fileName(),
                                     errorMessage(reply, body));
                emit assetUploadFailed(filePath, detail);
                emit requestFailed(tr("Upload"), detail);
            }
            emit uploadQueueChanged(pendingUploadCount());
            reply->deleteLater();
            if (!transient)
                QTimer::singleShot(0, this, &ImmichClient::processUploadQueue);
            return;
        }

        m_uploadRetryCounts.remove(filePath);
        m_uploadQueueStore.remove(filePath);
        persistUploadQueue();
        const QJsonObject object = QJsonDocument::fromJson(body).object();
        const QString assetId = object.value(QStringLiteral("id")).toString();
        const bool duplicate =
            object.value(QStringLiteral("duplicate")).toBool() ||
            object.value(QStringLiteral("status")).toString().compare(
                QStringLiteral("duplicate"), Qt::CaseInsensitive) == 0;
        emit assetUploaded(filePath, assetId, duplicate);
        emit uploadQueueChanged(pendingUploadCount());
        reply->deleteLater();
        if (!m_online)
            setOnline(true);
        else
            QTimer::singleShot(0, this, &ImmichClient::processUploadQueue);
    });
    return true;
}

void ImmichClient::downloadAsset(const QString &assetId, const QString &destinationPath,
                                 const QString &suggestedFileName)
{
    Q_UNUSED(suggestedFileName);
    if (!ensureConfigured(tr("Download")) || destinationPath.isEmpty())
        return;
    if (!isValidImmichId(assetId)) {
        emit requestFailed(tr("Download"), tr("The asset identifier is invalid."));
        return;
    }

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
    beginActiveDownload();

    connect(reply, &QNetworkReply::readyRead, this, [reply, file] {
        const QByteArray chunk = reply->readAll();
        if (!chunk.isEmpty() && file->write(chunk) != chunk.size()) {
            reply->setProperty("destinationWriteFailed", true);
            reply->setProperty("destinationWriteError", file->errorString());
            reply->abort();
        }
    });
    connect(reply, &QNetworkReply::downloadProgress, this,
            [this, assetId](qint64 bytesReceived, qint64 bytesTotal) {
                emit downloadProgress(assetId, bytesReceived, bytesTotal);
            });
    connect(reply, &QNetworkReply::finished, this,
            [this, reply, file, assetId, destinationPath] {
                if (reply->bytesAvailable() > 0) {
                    const QByteArray chunk = reply->readAll();
                    if (file->write(chunk) != chunk.size()) {
                        reply->setProperty("destinationWriteFailed", true);
                        reply->setProperty("destinationWriteError", file->errorString());
                    }
                }

                if (reply->property("destinationWriteFailed").toBool()) {
                    const QString message =
                        reply->property("destinationWriteError").toString();
                    file->close();
                    file->remove();
                    endActiveDownload();
                    emit requestFailed(tr("Download"),
                                       tr("Could not write %1: %2")
                                           .arg(destinationPath, message));
                    reply->deleteLater();
                    return;
                }

                if (reply->error() != QNetworkReply::NoError) {
                    file->close();
                    file->remove();
                    endActiveDownload();
                    emit requestFailed(tr("Download"),
                                       errorMessage(reply, reply->readAll()));
                    reply->deleteLater();
                    return;
                }

                if (!file->flush()) {
                    const QString message = file->errorString();
                    file->close();
                    file->remove();
                    endActiveDownload();
                    emit requestFailed(tr("Download"),
                                       tr("Could not write %1: %2")
                                           .arg(destinationPath, message));
                    reply->deleteLater();
                    return;
                }

                file->close();
                endActiveDownload();
                emit assetDownloaded(assetId, destinationPath);
                reply->deleteLater();
            });
}

void ImmichClient::fetchAssetInfo(const QString &assetId)
{
    if (!isConfigured() || !isValidImmichId(assetId)) {
        emit assetInfoFailed(assetId, tr("The asset identifier is invalid."));
        return;
    }

    auto *reply =
        m_network->get(authenticatedRequest(apiUrl(QStringLiteral("/assets/%1").arg(assetId))));
    connect(reply, &QNetworkReply::finished, this, [this, reply, assetId] {
        const QByteArray body = reply->readAll();
        if (reply->error() != QNetworkReply::NoError) {
            emit assetInfoFailed(assetId, errorMessage(reply, body));
            reply->deleteLater();
            return;
        }
        ImmichAsset asset = assetFromObject(QJsonDocument::fromJson(body).object());
        if (asset.id.isEmpty())
            asset.id = assetId;
        emit assetInfoLoaded(asset);
        reply->deleteLater();
    });
}

void ImmichClient::fetchAssetDetails(const QString &assetId)
{
    if (!isConfigured() || !isValidImmichId(assetId)) {
        emit assetDetailsFailed(assetId, tr("The asset identifier is invalid."));
        return;
    }
    if (!m_currentUserId.isEmpty()) {
        requestAssetDetails(assetId);
        return;
    }

    // Know who we are first, so "shared by" can tell other users' photos apart.
    auto *reply = m_network->get(authenticatedRequest(apiUrl(QStringLiteral("/users/me"))));
    connect(reply, &QNetworkReply::finished, this, [this, reply, assetId] {
        if (reply->error() == QNetworkReply::NoError) {
            m_currentUserId = QJsonDocument::fromJson(reply->readAll())
                                  .object()
                                  .value(QStringLiteral("id"))
                                  .toString();
        }
        requestAssetDetails(assetId);
        reply->deleteLater();
    });
}

void ImmichClient::requestAssetDetails(const QString &assetId)
{
    auto *reply =
        m_network->get(authenticatedRequest(apiUrl(QStringLiteral("/assets/%1").arg(assetId))));
    connect(reply, &QNetworkReply::finished, this, [this, reply, assetId] {
        const QByteArray body = reply->readAll();
        if (reply->error() != QNetworkReply::NoError) {
            emit assetDetailsFailed(assetId, errorMessage(reply, body));
        } else {
            ImmichAssetDetails details =
                assetDetailsFromObject(QJsonDocument::fromJson(body).object());
            details.asset.id = assetId;
            details.sharedByOtherUser = !m_currentUserId.isEmpty() &&
                                        !details.ownerId.isEmpty() &&
                                        details.ownerId != m_currentUserId;
            emit assetDetailsLoaded(details);
        }
        reply->deleteLater();
    });

    // Albums need album.read, which API keys made for older versions may lack.
    QUrl albumsUrl = apiUrl(QStringLiteral("/albums"));
    albumsUrl.setQuery(QUrlQuery{{QStringLiteral("assetId"), assetId}});
    auto *albumsReply = m_network->get(authenticatedRequest(albumsUrl));
    connect(albumsReply, &QNetworkReply::finished, this, [this, albumsReply, assetId] {
        const QByteArray body = albumsReply->readAll();
        if (albumsReply->error() == QNetworkReply::NoError) {
            QList<ImmichAlbum> albums;
            for (const QJsonValue &value : QJsonDocument::fromJson(body).array()) {
                const ImmichAlbum album = albumFromObject(value.toObject());
                if (!album.id.isEmpty())
                    albums.append(album);
            }
            emit assetAlbumsLoaded(assetId, albums);
        } else {
            const int status =
                albumsReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            emit assetAlbumsFailed(assetId, status == 403);
        }
        albumsReply->deleteLater();
    });
}

void ImmichClient::fetchAssetOriginal(const QString &assetId)
{
    if (!ensureConfigured(tr("Copy")))
        return;
    if (!isValidImmichId(assetId)) {
        emit requestFailed(tr("Copy"), tr("The asset identifier is invalid."));
        return;
    }

    auto *reply =
        m_network->get(authenticatedRequest(apiUrl(QStringLiteral("/assets/%1/original").arg(assetId))));
    reply->setReadBufferSize(1024 * 1024);
    auto body = std::make_shared<QByteArray>();
    auto tooLarge = std::make_shared<bool>(false);
    connect(reply, &QNetworkReply::metaDataChanged, this,
            [reply, tooLarge] {
                constexpr qint64 limit = 64LL * 1024 * 1024;
                const qint64 contentLength =
                    reply->header(QNetworkRequest::ContentLengthHeader).toLongLong();
                if (contentLength > limit) {
                    *tooLarge = true;
                    reply->abort();
                }
            });
    connect(reply, &QNetworkReply::readyRead, this,
            [reply, body, tooLarge] {
                if (*tooLarge)
                    return;
                constexpr qint64 limit = 64LL * 1024 * 1024;
                const QByteArray chunk = reply->readAll();
                if (body->size() + chunk.size() > limit) {
                    *tooLarge = true;
                    body->clear();
                    reply->abort();
                    return;
                }
                body->append(chunk);
            });
    beginActiveDownload();
    connect(reply, &QNetworkReply::finished, this,
            [this, reply, assetId, body, tooLarge] {
        endActiveDownload();
        if (*tooLarge) {
            emit requestFailed(
                tr("Copy"),
                tr("This original is larger than the 64 MB clipboard safety limit. "
                   "Download it to disk instead."));
            reply->deleteLater();
            return;
        }
        const QByteArray remaining = reply->readAll();
        constexpr qint64 limit = 64LL * 1024 * 1024;
        if (body->size() + remaining.size() > limit) {
            emit requestFailed(
                tr("Copy"),
                tr("This original is larger than the 64 MB clipboard safety limit. "
                   "Download it to disk instead."));
            reply->deleteLater();
            return;
        }
        body->append(remaining);
        if (reply->error() != QNetworkReply::NoError) {
            emit requestFailed(tr("Copy"), errorMessage(reply, *body));
            reply->deleteLater();
            return;
        }
        const QString contentType =
            QString::fromUtf8(reply->header(QNetworkRequest::ContentTypeHeader)
                                  .toByteArray())
                .section(u';', 0, 0)
                .trimmed();
        emit assetOriginalFetched(assetId, *body, contentType);
        reply->deleteLater();
    });
}

void ImmichClient::restoreFromTrash(const QStringList &assetIds)
{
    QJsonArray ids;
    for (const QString &id : assetIds) {
        if (isValidImmichId(id))
            ids.append(id);
    }
    if (ids.isEmpty())
        return;
    postTrashAction(QStringLiteral("/trash/restore/assets"), {{QStringLiteral("ids"), ids}},
                    [this, assetIds] { emit assetsRestored(assetIds); });
}

void ImmichClient::restoreAllFromTrash()
{
    postTrashAction(QStringLiteral("/trash/restore"), {}, [this] { emit assetsRestored({}); });
}

void ImmichClient::emptyTrash()
{
    postTrashAction(QStringLiteral("/trash/empty"), {}, [this] { emit trashEmptied(); });
}

void ImmichClient::postTrashAction(const QString &path, const QJsonObject &body,
                                   std::function<void()> done)
{
    if (!ensureConfigured(tr("Trash")))
        return;
    QNetworkRequest request = authenticatedRequest(apiUrl(path));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setRawHeader("Accept", "application/json");
    auto *reply = m_network->post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply, done = std::move(done)] {
        const QByteArray response = reply->readAll();
        if (reply->error() != QNetworkReply::NoError)
            emit requestFailed(tr("Trash"), errorMessage(reply, response));
        else
            done();
        reply->deleteLater();
    });
}

void ImmichClient::loadTrashDays()
{
    if (!isConfigured())
        return;
    auto *reply = m_network->get(authenticatedRequest(apiUrl(QStringLiteral("/server/config"))));
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        if (reply->error() == QNetworkReply::NoError) {
            const int days = QJsonDocument::fromJson(reply->readAll())
                                 .object()
                                 .value(QStringLiteral("trashDays"))
                                 .toInt();
            if (days > 0)
                emit trashDaysLoaded(days);
        }
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
        const QString trimmed = id.trimmed();
        if (isValidImmichId(trimmed))
            ids.append(trimmed);
    }
    if (ids.isEmpty()) {
        if (!assetIds.isEmpty())
            emit requestFailed(tr("Delete"), tr("No valid asset identifiers were provided."));
        return;
    }

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
            QList<ImmichAsset> history = m_offlineStore.loadViewHistory(m_connection.serverUrl);
            if (history.removeIf([&ids](const ImmichAsset &seen) { return ids.contains(seen.id); })) {
                m_offlineStore.saveViewHistory(m_connection.serverUrl, history);
                emit recentlyViewedChanged();
            }
            QHash<QString, AssetViewStats> stats =
                m_offlineStore.loadViewStats(m_connection.serverUrl);
            if (stats.removeIf([&ids](const auto &entry) { return ids.contains(entry.key()); }))
                m_offlineStore.saveViewStats(m_connection.serverUrl, stats);
            emit assetsDeleted(ids, permanent);
        }
        reply->deleteLater();
    });
}

void ImmichClient::setFavorite(const QStringList &assetIds, bool favorite)
{
    if (!ensureConfigured(tr("Favorite")))
        return;

    QStringList ids;
    QJsonArray idArray;
    for (const QString &id : assetIds) {
        const QString trimmed = id.trimmed();
        if (isValidImmichId(trimmed)) {
            ids.append(trimmed);
            idArray.append(trimmed);
        }
    }
    if (ids.isEmpty())
        return;

    QJsonObject body;
    body.insert(QStringLiteral("ids"), idArray);
    body.insert(QStringLiteral("isFavorite"), favorite);

    QNetworkRequest request = authenticatedRequest(apiUrl(QStringLiteral("/assets")));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setRawHeader("Accept", "application/json");
    auto *reply = m_network->put(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply, ids, favorite] {
        const QByteArray body = reply->readAll();
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status == 403)
            emit favoriteFailed(ids, tr("Favorites need the asset.update permission on your API key."));
        else if (reply->error() != QNetworkReply::NoError)
            emit favoriteFailed(ids, errorMessage(reply, body));
        else
            emit favoritesChanged(ids, favorite);
        reply->deleteLater();
    });
}

QUrl ImmichClient::sharedLinkUrl(const QString &key) const
{
    static const QRegularExpression validKey(QStringLiteral("^[A-Za-z0-9_-]+$"));
    if (!validKey.match(key).hasMatch())
        return {};
    // Public links always use the configured Immich URL, independently of
    // the local endpoint selected for API traffic.
    QUrl url(m_connection.serverUrl);
    if (!url.isValid() || url.host().isEmpty() ||
        (url.scheme() != QStringLiteral("https") && url.scheme() != QStringLiteral("http")))
        return {};
    QString path = url.path();
    while (path.endsWith(u'/'))
        path.chop(1);
    if (path.endsWith(QStringLiteral("/api")))
        path.chop(4);
    url.setPath(path + QStringLiteral("/share/") + key);
    url.setQuery(QString());
    url.setFragment(QString());
    url.setUserName(QString());
    url.setPassword(QString());
    return url;
}

void ImmichClient::createSharedLink(const QString &assetId, const QUuid &requestId, const QString &password)
{
    if (!isConfigured() || !isValidImmichId(assetId)) {
        emit sharedLinkFailed(requestId, tr("Connect to Immich and choose an uploaded photo or video."));
        return;
    }
    if (sharedLinkUrl(QStringLiteral("validate")).isEmpty()) {
        emit sharedLinkFailed(requestId, tr("Set a valid Immich URL in server settings first."));
        return;
    }
    if (!m_online) {
        emit sharedLinkFailed(requestId, tr("You are offline. Connect to Immich to create a share link."));
        return;
    }
    QJsonObject body{{QStringLiteral("type"), QStringLiteral("INDIVIDUAL")},
                     {QStringLiteral("assetIds"), QJsonArray{assetId}},
                     {QStringLiteral("allowUpload"), false},
                     {QStringLiteral("allowDownload"), true},
                     {QStringLiteral("showMetadata"), false}};
    if (!password.isEmpty())
        body.insert(QStringLiteral("password"), password);
    const QUrl publicBase = sharedLinkUrl(QStringLiteral("placeholder"));
    auto request = authenticatedRequest(apiUrl(QStringLiteral("/shared-links")));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setRawHeader("Accept", "application/json");
    request.setTransferTimeout(20000);
    auto *reply = m_network->post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply, requestId, publicBase] {
        const QByteArray response = reply->readAll();
        if (reply->error() != QNetworkReply::NoError) {
            emit sharedLinkFailed(requestId, errorMessage(reply, response));
            reply->deleteLater();
            return;
        }
        const QString key = QJsonDocument::fromJson(response).object().value(QStringLiteral("key")).toString();
        static const QRegularExpression validKey(QStringLiteral("^[A-Za-z0-9_-]+$"));
        if (!validKey.match(key).hasMatch()) {
            emit sharedLinkFailed(requestId, tr("Immich returned an invalid share link."));
        } else {
            QUrl url = publicBase;
            QString path = url.path();
            path.chop(QStringLiteral("placeholder").size());
            url.setPath(path + key);
            emit sharedLinkCreated(requestId, url);
        }
        reply->deleteLater();
    });
}

void ImmichClient::loadImageAsync(const QString &assetId, const QString &resultSize,
                                  ImageSource source, FetchPriority fetchPriority)
{
    if (!ensureConfigured(tr("Load image")))
        return;
    if (!isValidImmichId(assetId)) {
        emit imageLoadFailed(assetId, resultSize, tr("The asset identifier is invalid."));
        return;
    }

    const bool cacheOnly = source == ImageSource::CacheOnly;
    const QString pendingKey =
        (cacheOnly ? QStringLiteral("cached:") : QString()) + resultSize + u':' + assetId;
    // Cache reads are quick and always run first, so what's on disk never waits
    // behind server fetches. The viewer's preview comes next, then on-screen
    // tiles, then the rest; within each, newest first, so what's in view now
    // beats tiles scrolled past earlier.
    constexpr int kOnScreenBand = 1 << 29;
    if (m_imageRequestSerial >= kOnScreenBand - 1)
        m_imageRequestSerial = 0;
    int priority = INT_MAX;
    if (!cacheOnly && resultSize == QStringLiteral("preview"))
        priority = INT_MAX - 1;
    else if (!cacheOnly)
        priority = ++m_imageRequestSerial +
                   (fetchPriority == FetchPriority::OnScreen ? kOnScreenBand : 0);

    // Already queued: only queue again to move an off-screen prefetch up once its
    // tile is on screen. The later job finds the first one's result in the cache.
    const auto pending = m_pendingImages.constFind(pendingKey);
    if (pending != m_pendingImages.cend() &&
        (pending.value() >= kOnScreenBand || priority < kOnScreenBand))
        return;
    m_pendingImages.insert(pendingKey, priority);

    const ImmichConnectionSettings connection = m_connection;
    const QString serverUrl = activeServerUrl();
    // Captured on the main thread: the worker must not touch m_connection/m_usingLocalEndpoint.
    const QSslConfiguration sslConfig = sslConfigForActiveEndpoint();
    QPointer<ImmichClient> self(this);

    const bool tileImage = resultSize.startsWith(QStringLiteral("tile:"));
    const int tileDimension = tileImage ? resultSize.mid(5).toInt() : 0;
    const bool usesPreview = tileImage || resultSize == QStringLiteral("preview");

    m_imagePool.start([this, self, assetId, resultSize, pendingKey, connection, serverUrl,
                       sslConfig, tileImage, tileDimension, usesPreview, cacheOnly] {
        auto finishFail = [self, assetId, resultSize, pendingKey](const QString &message) {
            if (!self)
                return;
            QMetaObject::invokeMethod(self, [self, assetId, resultSize, pendingKey, message] {
                if (!self)
                    return;
                self->m_pendingImages.remove(pendingKey);
                if (resultSize == QStringLiteral("thumbnail"))
                    self->notePrefetchThumbnailDone(assetId, false);
                emit self->imageLoadFailed(assetId, resultSize, message);
                emit self->requestFailed(ImmichClient::tr("Load image"), message);
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
                                                   ImmichClient::tr("Unsupported image format."));
                        return;
                    }
                    if (resultSize == QStringLiteral("thumbnail")) {
                        self->m_thumbnailCache.store(assetId, cacheBytes, pixmap);
                        emit self->thumbnailLoaded(assetId, pixmap);
                        self->notePrefetchThumbnailDone(assetId, true);
                    } else if (resultSize.startsWith(QStringLiteral("tile:"))) {
                        self->m_previewCache.store(assetId, cacheBytes, pixmap);
                        emit self->thumbnailLoaded(assetId, pixmap);
                    } else {
                        if (resultSize == QStringLiteral("preview"))
                            self->m_previewCache.store(assetId, cacheBytes, pixmap);
                        emit self->previewLoaded(assetId, pixmap);
                    }
                },
                Qt::QueuedConnection);
        };

        if (resultSize == QStringLiteral("thumbnail")) {
            const QByteArray cachedBytes = self->m_thumbnailCache.readDisk(assetId);
            if (!cachedBytes.isEmpty()) {
                const QImage image = self->decodeImage(cachedBytes, 256);
                if (!image.isNull()) {
                    // Rewrite old cache entries that may contain a full-size
                    // response into a bounded, compressed thumbnail.
                    const bool normalizedJpeg =
                        cachedBytes.size() <= 256 * 1024 &&
                        cachedBytes.startsWith(QByteArray::fromHex("ffd8"));
                    finishImage(image, normalizedJpeg
                                           ? QByteArray()
                                           : encodeThumbnail(image));
                    return;
                }
            }
        } else if (usesPreview) {
            const QByteArray cachedBytes = self->m_previewCache.readDisk(assetId);
            if (!cachedBytes.isEmpty()) {
                const QImage image =
                    self->decodeImage(cachedBytes, tileImage ? tileDimension : 1920);
                if (!image.isNull()) {
                    finishImage(image, QByteArray());
                    return;
                }
            }
        }

        if (cacheOnly) {
            QMetaObject::invokeMethod(self, [self, pendingKey] {
                if (self)
                    self->m_pendingImages.remove(pendingKey);
            }, Qt::QueuedConnection);
            return;
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
                                 QNetworkRequest::SameOriginRedirectPolicy);
            request.setTransferTimeout(15000);
            request.setSslConfiguration(sslConfig);
            CustomHeaders::applyTo(request, connection.customHeaders);
            QNetworkReply *reply = network.get(request);
            constexpr qsizetype maximumImageBytes = 32 * 1024 * 1024;
            QByteArray body;
            bool responseTooLarge = false;
            QObject::connect(reply, &QNetworkReply::readyRead, reply, [&] {
                const QByteArray chunk = reply->readAll();
                if (body.size() + chunk.size() > maximumImageBytes) {
                    responseTooLarge = true;
                    body.clear();
                    reply->abort();
                    return;
                }
                body.append(chunk);
            });
            QEventLoop loop;
            QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
            loop.exec();
            const QByteArray remaining = reply->readAll();
            if (body.size() + remaining.size() > maximumImageBytes) {
                responseTooLarge = true;
                body.clear();
            } else {
                body.append(remaining);
            }
            if (responseTooLarge) {
                reply->deleteLater();
                if (reportError)
                    finishFail(QObject::tr("The server returned an image larger than 32 MB."));
                return {};
            }
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

        QByteArray bytes = fetchSize(usesPreview ? QStringLiteral("preview")
                                                 : QStringLiteral("thumbnail"),
                                     true);
        if (bytes.isEmpty())
            return;

        // Keep timeline thumbnails small so main-thread QPixmap conversion stays cheap.
        const int maximumDimension = resultSize == QStringLiteral("thumbnail") ? 256
                                     : tileImage                            ? tileDimension
                                                                            : 1920;
        QImage image = decodeImage(bytes, maximumDimension);
        // Tile images cache the server preview itself so the viewer can reuse it.
        const QByteArray previewBytes = tileImage && !image.isNull() ? bytes : QByteArray();
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
        if (resultSize == QStringLiteral("thumbnail") || resultSize == QStringLiteral("preview"))
            cacheBytes = encodeThumbnail(image);
        else if (tileImage)
            cacheBytes = previewBytes;
        finishImage(image, cacheBytes);
    }, priority);
}

} // namespace Aurora
