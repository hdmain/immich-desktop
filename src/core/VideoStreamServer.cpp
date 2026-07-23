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

QByteArray httpStatusLine(int status)
{
    QByteArray line = "HTTP/1.1 " + QByteArray::number(status);
    if (status == 206)
        line += " Partial Content\r\n";
    else if (status >= 200 && status < 300)
        line += " OK\r\n";
    else if (status == 302)
        line += " Found\r\n";
    else
        line += " Error\r\n";
    return line;
}

QByteArray contentTypeHeader(QNetworkReply *reply)
{
    QByteArray contentType =
        reply->header(QNetworkRequest::ContentTypeHeader).toString().toUtf8();
    if (contentType.isEmpty())
        contentType = reply->rawHeader("Content-Type");
    if (contentType.isEmpty())
        contentType = "video/mp4";
    return contentType;
}

QByteArray contentLengthHeader(QNetworkReply *reply)
{
    const QVariant length = reply->header(QNetworkRequest::ContentLengthHeader);
    if (length.isValid())
        return QByteArray::number(length.toLongLong());
    const QByteArray raw = reply->rawHeader("Content-Length");
    return raw;
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
    m_apiBaseUrl = apiBaseUrl;
    m_apiKey = apiKey;
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

void VideoStreamServer::handleNewConnection()
{
    while (auto *client = m_server->nextPendingConnection()) {
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
    if (!match.hasMatch() || token != m_sessionToken) {
        client->write("HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n");
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

    const QUrl assetUrl = playbackUrl(match.captured(1));
    startUpstream(client, assetUrl, rangeHeader, isHead, /*sendApiKey=*/true, /*redirects=*/0);
}

void VideoStreamServer::startUpstream(QTcpSocket *client, const QUrl &url,
                                      const QByteArray &rangeHeader, bool isHead,
                                      bool sendApiKey, int redirects)
{
    constexpr int kMaxRedirects = 5;
    if (redirects > kMaxRedirects) {
        client->write("HTTP/1.1 502 Bad Gateway\r\nConnection: close\r\n\r\n");
        client->disconnectFromHost();
        return;
    }

    QNetworkRequest upstream(url);
    upstream.setRawHeader("Accept", "*/*");
    if (sendApiKey && !m_apiKey.isEmpty())
        upstream.setRawHeader("x-api-key", m_apiKey.toUtf8());
    if (!rangeHeader.isEmpty())
        upstream.setRawHeader("Range", rangeHeader);
    // Follow Immich → CDN/S3 ourselves so we can drop the API key on the
    // final hop and avoid FFmpeg seeing a broken redirect/error body.
    upstream.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                          QNetworkRequest::ManualRedirectPolicy);
    upstream.setTransferTimeout(120000);

    auto *reply = isHead ? m_network->head(upstream) : m_network->get(upstream);
    reply->setReadBufferSize(512 * 1024);
    const QPointer<QTcpSocket> guardedClient(client);

    connect(reply, &QNetworkReply::metaDataChanged, this,
            [this, guardedClient, reply, rangeHeader, isHead, redirects] {
        if (!guardedClient ||
            guardedClient->state() != QAbstractSocket::ConnectedState)
            return;
        if (reply->property("headersSent").toBool() ||
            reply->property("redirecting").toBool())
            return;

        const int status =
            reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status >= 301 && status <= 308) {
            const QUrl location =
                reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
            const QUrl next = reply->url().resolved(location);
            if (!next.isValid())
                return;
            reply->setProperty("redirecting", true);
            reply->abort();
            reply->deleteLater();
            // Pre-signed CDN URLs must not carry the Immich API key.
            startUpstream(guardedClient, next, rangeHeader, isHead,
                          /*sendApiKey=*/false, redirects + 1);
            return;
        }

        QByteArray header = httpStatusLine(status > 0 ? status : 200);
        header += "Content-Type: " + contentTypeHeader(reply) + "\r\n";
        header += "Accept-Ranges: bytes\r\n";
        header += "Connection: close\r\n";
        const QByteArray contentLength = contentLengthHeader(reply);
        if (!contentLength.isEmpty())
            header += "Content-Length: " + contentLength + "\r\n";
        const QByteArray contentRange = reply->rawHeader("Content-Range");
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
        if (reply->property("redirecting").toBool())
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
        if (reply->property("redirecting").toBool())
            return;
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
