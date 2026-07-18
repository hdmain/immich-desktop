#include "core/VideoStreamServer.h"

#include <QHostAddress>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPointer>
#include <QRegularExpression>
#include <QTcpServer>
#include <QTcpSocket>

#include <functional>
#include <memory>

namespace Aurora {

VideoStreamServer::VideoStreamServer(QObject *parent)
    : QObject(parent)
    , m_server(new QTcpServer(this))
    , m_network(new QNetworkAccessManager(this))
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

    const QString path = QString::fromUtf8(parts.at(1));
    static const QRegularExpression pathPattern(
        QStringLiteral("^/video/([0-9a-fA-F-]{36})$"));
    const auto match = pathPattern.match(path);
    if (!match.hasMatch()) {
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

    QNetworkRequest upstream(playbackUrl(match.captured(1)));
    upstream.setRawHeader("Accept", "*/*");
    upstream.setRawHeader("x-api-key", m_apiKey.toUtf8());
    if (!rangeHeader.isEmpty())
        upstream.setRawHeader("Range", rangeHeader);
    upstream.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                          QNetworkRequest::NoLessSafeRedirectPolicy);

    auto *reply = isHead ? m_network->head(upstream) : m_network->get(upstream);
    reply->setReadBufferSize(512 * 1024);
    const QPointer<QTcpSocket> guardedClient(client);
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

        const auto contentType = reply->header(QNetworkRequest::ContentTypeHeader).toByteArray();
        header += "Content-Type: " +
                  (contentType.isEmpty() ? QByteArray("video/mp4") : contentType) + "\r\n";
        header += "Accept-Ranges: bytes\r\n";
        header += "Connection: close\r\n";
        header += "Access-Control-Allow-Origin: *\r\n";

        const auto contentLength =
            reply->header(QNetworkRequest::ContentLengthHeader).toByteArray();
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
