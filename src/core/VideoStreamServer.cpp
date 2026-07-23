#include "core/VideoStreamServer.h"

#include <QHostAddress>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPointer>
#include <QRegularExpression>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUrlQuery>
#include <QUuid>

#include <functional>
#include <memory>

namespace Aurora {
namespace {

bool containsControlOrSeparator(const QByteArray &value)
{
    for (const char ch : value) {
        const auto byte = static_cast<unsigned char>(ch);
        if (byte < 0x20 || byte == 0x7f)
            return true;
    }
    return false;
}

} // namespace

VideoStreamServer::VideoStreamServer(QObject *parent)
    : QObject(parent)
    , m_server(new QTcpServer(this))
    , m_network(new QNetworkAccessManager(this))
    , m_sessionToken(QUuid::createUuid().toString(QUuid::WithoutBraces))
{
    connect(m_server, &QTcpServer::newConnection, this, &VideoStreamServer::handleNewConnection);
}

bool VideoStreamServer::start()
{
    if (m_server->isListening())
        return true;
    return m_server->listen(QHostAddress::LocalHost, 0);
}

void VideoStreamServer::setCredentials(const QUrl &apiBaseUrl, const QString &apiKey)
{
    const bool credentialsChanged =
        apiBaseUrl != m_apiBaseUrl || apiKey != m_apiKey;
    m_apiBaseUrl = apiBaseUrl;
    m_apiKey = apiKey;
    // Rotate the bearer so a previous session token cannot keep streaming after
    // the user switches servers or rotates their API key.
    if (credentialsChanged)
        m_sessionToken = QUuid::createUuid().toString(QUuid::WithoutBraces);
}

QUrl VideoStreamServer::streamUrl(const QString &assetId) const
{
    QUrl url;
    url.setScheme(QStringLiteral("http"));
    url.setHost(QStringLiteral("127.0.0.1"));
    url.setPort(m_server->serverPort());
    url.setPath(QStringLiteral("/video/%1").arg(assetId));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("token"), m_sessionToken);
    url.setQuery(query);
    return url;
}

QUrl VideoStreamServer::playbackUrl(const QString &assetId) const
{
    QUrl url = m_apiBaseUrl;
    QString path = url.path();
    while (path.endsWith(u'/'))
        path.chop(1);
    url.setPath(path + QStringLiteral("/assets/%1/video/playback").arg(assetId));
    url.setQuery(QString());
    url.setFragment({});
    return url;
}

bool VideoStreamServer::constantTimeEquals(const QString &left, const QString &right)
{
    const QByteArray a = left.toUtf8();
    const QByteArray b = right.toUtf8();
    if (a.size() != b.size())
        return false;
    volatile unsigned char diff = 0;
    for (int i = 0; i < a.size(); ++i)
        diff = static_cast<unsigned char>(diff | (a.at(i) ^ b.at(i)));
    return diff == 0;
}

QByteArray VideoStreamServer::sanitizedHeaderValue(const QByteArray &value)
{
    if (value.isEmpty() || containsControlOrSeparator(value))
        return {};
    return value;
}

QByteArray VideoStreamServer::sanitizedContentType(const QByteArray &value)
{
    const QByteArray cleaned = sanitizedHeaderValue(value);
    if (cleaned.isEmpty())
        return {};
    static const QRegularExpression pattern(
        QStringLiteral("^[A-Za-z0-9][A-Za-z0-9!#$&^_.+-]{0,126}"
                       "/[A-Za-z0-9][A-Za-z0-9!#$&^_.+-]{0,126}"
                       "(?:[ \\t]*;[ \\t]*[A-Za-z0-9!#$&^_.+-]+=[^\\r\\n;]+)*$"));
    if (!pattern.match(QString::fromLatin1(cleaned)).hasMatch())
        return {};
    return cleaned;
}

QByteArray VideoStreamServer::sanitizedContentLength(const QByteArray &value)
{
    const QByteArray cleaned = sanitizedHeaderValue(value);
    if (cleaned.isEmpty())
        return {};
    for (const char ch : cleaned) {
        if (ch < '0' || ch > '9')
            return {};
    }
    return cleaned;
}

QByteArray VideoStreamServer::sanitizedContentRange(const QByteArray &value)
{
    const QByteArray cleaned = sanitizedHeaderValue(value);
    if (cleaned.isEmpty())
        return {};
    static const QRegularExpression pattern(
        QStringLiteral("^bytes (?:\\d+-\\d+/\\d+|\\*/\\d+|\\d+-\\d+/\\*)$"),
        QRegularExpression::CaseInsensitiveOption);
    if (!pattern.match(QString::fromLatin1(cleaned)).hasMatch())
        return {};
    return cleaned;
}

void VideoStreamServer::handleNewConnection()
{
    while (auto *client = m_server->nextPendingConnection()) {
        if (m_sessions.size() >= kMaxConcurrentUpstream * 2) {
            client->write("HTTP/1.1 503 Service Unavailable\r\nConnection: close\r\n\r\n");
            client->disconnectFromHost();
            client->deleteLater();
            continue;
        }
        Session session;
        session.client = client;
        m_sessions.insert(client, session);
        connect(client, &QTcpSocket::readyRead, this, [this, client] {
            handleClientReadyRead(client);
        });
        connect(client, &QTcpSocket::disconnected, this, [this, client] {
            m_sessions.remove(client);
            client->deleteLater();
        });
    }
}

void VideoStreamServer::handleClientReadyRead(QTcpSocket *client)
{
    auto it = m_sessions.find(client);
    if (it == m_sessions.end() || it->headersComplete)
        return;

    it->requestBuffer.append(client->readAll());
    constexpr qsizetype maximumHeaderBytes = 32 * 1024;
    if (it->requestBuffer.size() > maximumHeaderBytes) {
        client->write(
            "HTTP/1.1 431 Request Header Fields Too Large\r\nConnection: close\r\n\r\n");
        client->disconnectFromHost();
        return;
    }
    const int headerEnd = it->requestBuffer.indexOf("\r\n\r\n");
    if (headerEnd < 0)
        return;

    it->headersComplete = true;
    const QByteArray request = it->requestBuffer.left(headerEnd + 4);
    serveRequest(client, request);
}

void VideoStreamServer::serveRequest(QTcpSocket *client, const QByteArray &request)
{
    const QList<QByteArray> lines = request.split('\n');
    if (lines.isEmpty()) {
        client->disconnectFromHost();
        return;
    }

    const QByteArray requestLine = lines.first().trimmed();
    const QList<QByteArray> parts = requestLine.split(' ');
    const bool isGet = parts.size() >= 2 && parts.at(0) == "GET";
    const bool isHead = parts.size() >= 2 && parts.at(0) == "HEAD";
    if (!isGet && !isHead) {
        client->write("HTTP/1.1 405 Method Not Allowed\r\nConnection: close\r\n\r\n");
        client->disconnectFromHost();
        return;
    }

    const QUrl target = QUrl::fromEncoded(parts.at(1));
    const QString path = target.path();
    static const QRegularExpression pathPattern(
        QStringLiteral("^/video/([0-9a-fA-F]{8}-[0-9a-fA-F]{4}-"
                       "[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12})$"));
    const auto match = pathPattern.match(path);
    const QString token =
        QUrlQuery(target).queryItemValue(QStringLiteral("token"), QUrl::FullyDecoded);
    if (!match.hasMatch() || !constantTimeEquals(token, m_sessionToken)) {
        client->write("HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n");
        client->disconnectFromHost();
        return;
    }

    if (m_activeUpstream >= kMaxConcurrentUpstream) {
        client->write("HTTP/1.1 503 Service Unavailable\r\nConnection: close\r\n\r\n");
        client->disconnectFromHost();
        return;
    }

    QByteArray rangeHeader;
    for (const QByteArray &line : lines) {
        const QByteArray trimmed = line.trimmed();
        if (trimmed.toLower().startsWith("range:")) {
            rangeHeader = trimmed.mid(6).trimmed();
            break;
        }
    }
    static const QRegularExpression rangePattern(
        QStringLiteral("^bytes=(?:\\d+-\\d*|-\\d+)"
                       "(?:,(?:\\d+-\\d*|-\\d+))*$"),
        QRegularExpression::CaseInsensitiveOption);
    if (!rangeHeader.isEmpty() &&
        !rangePattern.match(QString::fromLatin1(rangeHeader)).hasMatch()) {
        client->write("HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n");
        client->disconnectFromHost();
        return;
    }

    QNetworkRequest upstream(playbackUrl(match.captured(1)));
    upstream.setRawHeader("Accept", "*/*");
    upstream.setRawHeader("x-api-key", m_apiKey.toUtf8());
    if (!rangeHeader.isEmpty())
        upstream.setRawHeader("Range", rangeHeader);
    upstream.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                          QNetworkRequest::SameOriginRedirectPolicy);
    upstream.setTransferTimeout(30000);

    auto *reply = isHead ? m_network->head(upstream) : m_network->get(upstream);
    reply->setReadBufferSize(512 * 1024);
    ++m_activeUpstream;
    const QPointer<QTcpSocket> guardedClient(client);
    connect(reply, &QNetworkReply::destroyed, this, [this] {
        if (m_activeUpstream > 0)
            --m_activeUpstream;
    });
    connect(reply, &QNetworkReply::metaDataChanged, this, [guardedClient, reply] {
        if (!guardedClient ||
            guardedClient->state() != QAbstractSocket::ConnectedState)
            return;
        if (reply->property("headersSent").toBool())
            return;

        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status <= 0)
            status = 200;
        QByteArray header = "HTTP/1.1 " + QByteArray::number(status);
        if (status == 206)
            header += " Partial Content\r\n";
        else if (status >= 200 && status < 300)
            header += " OK\r\n";
        else
            header += " Error\r\n";

        const QByteArray contentType = VideoStreamServer::sanitizedContentType(
            reply->header(QNetworkRequest::ContentTypeHeader).toByteArray());
        header += "Content-Type: " +
                  (contentType.isEmpty() ? QByteArray("video/mp4") : contentType) + "\r\n";
        header += "Accept-Ranges: bytes\r\n";
        header += "Connection: close\r\n";
        const QByteArray contentLength = VideoStreamServer::sanitizedContentLength(
            reply->header(QNetworkRequest::ContentLengthHeader).toByteArray());
        if (!contentLength.isEmpty())
            header += "Content-Length: " + contentLength + "\r\n";

        const QByteArray contentRange =
            VideoStreamServer::sanitizedContentRange(reply->rawHeader("Content-Range"));
        if (!contentRange.isEmpty())
            header += "Content-Range: " + contentRange + "\r\n";

        header += "\r\n";
        guardedClient->write(header);
        reply->setProperty("headersSent", true);
    });

    auto pump = std::make_shared<std::function<void()>>();
    *pump = [guardedClient, reply, isHead] {
        if (isHead || !guardedClient ||
            guardedClient->state() != QAbstractSocket::ConnectedState)
            return;
        constexpr qint64 maximumQueuedBytes = 512 * 1024;
        constexpr qint64 chunkSize = 64 * 1024;
        while (reply->bytesAvailable() > 0 &&
               guardedClient->bytesToWrite() < maximumQueuedBytes) {
            const QByteArray chunk = reply->read(chunkSize);
            if (chunk.isEmpty())
                break;
            guardedClient->write(chunk);
        }
    };
    connect(reply, &QNetworkReply::readyRead, this, [pump] { (*pump)(); });
    connect(client, &QTcpSocket::bytesWritten, this,
            [pump](qint64) { (*pump)(); });

    connect(reply, &QNetworkReply::finished, this,
            [guardedClient, reply, pump, isHead] {
        (*pump)();
        if (guardedClient &&
            guardedClient->state() == QAbstractSocket::ConnectedState) {
            if (!reply->property("headersSent").toBool()) {
                guardedClient->write(
                    "HTTP/1.1 502 Bad Gateway\r\nConnection: close\r\n\r\n");
            } else if (!isHead && reply->bytesAvailable() > 0) {
                guardedClient->write(reply->readAll());
            }
            guardedClient->disconnectFromHost();
        }
        reply->deleteLater();
    });

    connect(client, &QTcpSocket::disconnected, reply, &QNetworkReply::abort);
}

} // namespace Aurora
