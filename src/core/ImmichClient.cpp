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
#include <QNetworkInformation>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPointer>
#include <QRegularExpression>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>

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
    return asset;
}

QList<ImmichAsset> assetsFromSearchBody(const QByteArray &body, QString *nextPage)
{
    const QJsonObject assetsObject =
        QJsonDocument::fromJson(body).object().value(QStringLiteral("assets")).toObject();
    if (nextPage)
        *nextPage = assetsObject.value(QStringLiteral("nextPage")).toVariant().toString();

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

bool isValidImmichId(const QString &id)
{
    static const QRegularExpression pattern(
        QStringLiteral("^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-"
                       "[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$"));
    return pattern.match(id).hasMatch();
}

} // namespace

ImmichClient::ImmichClient(QObject *parent)
    : QObject(parent)
    , m_connection(m_store.loadImmichConnection())
    , m_activeServerUrl(normalizeServerUrl(m_connection.serverUrl))
    , m_network(new QNetworkAccessManager(this))
    , m_endpointProbeTimer(new QTimer(this))
    , m_reachabilityTimer(new QTimer(this))
{
    m_imagePool.setMaxThreadCount(8);
    m_endpointProbeTimer->setInterval(12 * 1000);
    connect(m_endpointProbeTimer, &QTimer::timeout, this, &ImmichClient::probeEndpoints);
    m_reachabilityTimer->setInterval(15 * 1000);
    connect(m_reachabilityTimer, &QTimer::timeout, this, &ImmichClient::probeReachability);
    setupNetworkMonitoring();
    restoreUploadQueue();
    if (isConfigured()) {
        m_endpointProbeTimer->start();
        m_reachabilityTimer->start();
        QTimer::singleShot(0, this, &ImmichClient::probeEndpoints);
        QTimer::singleShot(0, this, &ImmichClient::probeReachability);
        QTimer::singleShot(2000, this, &ImmichClient::processUploadQueue);
    }
}

ImmichClient::~ImmichClient()
{
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
    if (m_online)
        processUploadQueue();
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
        if (!m_reachabilityTimer->isActive())
            m_reachabilityTimer->start();
        probeEndpoints();
        probeReachability();
    } else {
        m_endpointProbeTimer->stop();
        m_reachabilityTimer->stop();
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
                         QNetworkRequest::SameOriginRedirectPolicy);

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

        if (reachable && pong) {
            setActiveServerUrl(m_connection.localServerUrl, true);
            setOnline(true);
        } else {
            setActiveServerUrl(m_connection.serverUrl, false);
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
    m_streamServer->setCredentials(apiBase, m_connection.apiKey);
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

    if (!pollOnly && !m_online) {
        if (page <= 1 && emitCachedLibrary(trimmedQuery))
            return;
        emit requestFailed(smartSearch ? tr("Search") : tr("Load library"),
                           tr("You are offline and no cached library is available."));
        return;
    }

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
    searchRequest.setTransferTimeout(20000);
    auto *reply = m_network->post(searchRequest, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this,
            [this, reply, pollOnly, smartSearch, trimmedQuery, page] {
        const QByteArray body = reply->readAll();
        if (pollOnly)
            m_pollInFlight = false;

        if (reply->error() != QNetworkReply::NoError) {
            if (isTransientNetworkError(reply))
                setOnline(false);
            if (!pollOnly) {
                if (page <= 1 && emitCachedLibrary(trimmedQuery)) {
                    reply->deleteLater();
                    return;
                }
                emit requestFailed(smartSearch ? tr("Search") : tr("Load library"),
                                   errorMessage(reply, body));
            }
            reply->deleteLater();
            return;
        }

        setOnline(true);
        QString nextPage;
        QList<ImmichAsset> assets = assetsFromSearchBody(body, &nextPage);

        if (pollOnly) {
            emit newestAssetsPolled(assets);
        } else {
            if (!smartSearch) {
                if (page <= 1)
                    m_offlineStore.saveLibrary(m_connection.serverUrl, assets, trimmedQuery);
                else
                    m_offlineStore.mergeLibrary(m_connection.serverUrl, assets);
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
        finishExploreLoad(true, false, error);
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
        finishExploreLoad(false, true, error);
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

void ImmichClient::loadAssetsForPerson(const QString &personId, int page, int pageSize)
{
    searchFilteredAssets(QStringLiteral("person"), personId, page, pageSize);
}

void ImmichClient::loadAssetsForCity(const QString &city, int page, int pageSize)
{
    searchFilteredAssets(QStringLiteral("city"), city, page, pageSize);
}

void ImmichClient::searchFilteredAssets(const QString &filterKind, const QString &filterValue,
                                        int page, int pageSize)
{
    if (!ensureConfigured(tr("Load explore")) || filterValue.trimmed().isEmpty())
        return;

    QJsonObject body;
    body.insert(QStringLiteral("page"), qMax(1, page));
    body.insert(QStringLiteral("size"), qBound(1, pageSize, 250));
    body.insert(QStringLiteral("order"), QStringLiteral("desc"));
    body.insert(QStringLiteral("withExif"), true);
    body.insert(QStringLiteral("withStacked"), true);
    if (filterKind == QStringLiteral("person")) {
        QJsonArray personIds;
        personIds.append(filterValue);
        body.insert(QStringLiteral("personIds"), personIds);
    } else if (filterKind == QStringLiteral("city")) {
        body.insert(QStringLiteral("city"), filterValue);
    } else {
        return;
    }

    QNetworkRequest request = authenticatedRequest(apiUrl(QStringLiteral("/search/metadata")));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setRawHeader("Accept", "application/json");
    auto *reply = m_network->post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this,
            [this, reply, filterKind, filterValue] {
                const QByteArray body = reply->readAll();
                if (reply->error() != QNetworkReply::NoError) {
                    emit requestFailed(tr("Load explore"), errorMessage(reply, body));
                    reply->deleteLater();
                    return;
                }
                QString nextPage;
                const QList<ImmichAsset> assets = assetsFromSearchBody(body, &nextPage);
                emit filteredAssetsLoaded(filterKind, filterValue, assets, nextPage);
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
                             tr("Offline — person photo not cached."));
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
        emit requestFailed(tr("Upload"), tr("File not found: %1").arg(filePath));
        return false;
    }

    if (info.size() <= 0) {
        m_uploadRetryCounts.remove(filePath);
        m_uploadQueueStore.remove(filePath);
        emit requestFailed(tr("Upload"),
                           tr("File is empty: %1").arg(info.fileName()));
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
            emit requestFailed(
                tr("Upload"),
                tr("Could not open %1 after several tries: %2")
                    .arg(info.fileName(), message));
        } else {
            // Defer retry — never recurse into processUploadQueue here.
            requeueUpload(filePath, false);
            scheduleUploadRetry(2000 * attempts);
            emit requestFailed(
                tr("Upload"),
                tr("Could not open %1: %2 (will retry)")
                    .arg(info.fileName(), message));
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
                    tr("%1 interrupted — queued for retry when online.")
                        .arg(QFileInfo(filePath).fileName()));
            } else {
                m_uploadRetryCounts.remove(filePath);
                m_uploadQueueStore.remove(filePath);
                persistUploadQueue();
                emit requestFailed(tr("Upload"),
                                   tr("%1: %2").arg(QFileInfo(filePath).fileName(),
                                                    errorMessage(reply, body)));
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
            emit assetsDeleted(ids, permanent);
        }
        reply->deleteLater();
    });
}

void ImmichClient::loadImageAsync(const QString &assetId, const QString &resultSize)
{
    if (!ensureConfigured(tr("Load image")))
        return;
    if (!isValidImmichId(assetId)) {
        emit imageLoadFailed(assetId, resultSize, tr("The asset identifier is invalid."));
        return;
    }

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
                                 QNetworkRequest::SameOriginRedirectPolicy);
            request.setTransferTimeout(15000);
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
