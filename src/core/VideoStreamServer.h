#pragma once

#include <QByteArray>
#include <QHash>
#include <QList>
#include <QObject>
#include <QPair>
#include <QSslConfiguration>
#include <QString>
#include <QUrl>

class QNetworkAccessManager;
class QTcpServer;
class QTcpSocket;

namespace Aurora {

class VideoStreamServer final : public QObject {
    Q_OBJECT

public:
    explicit VideoStreamServer(QObject *parent = nullptr);

    bool start();
    void setCredentials(const QUrl &apiBaseUrl, const QString &apiKey,
                        const QSslConfiguration &sslConfig,
                        const QList<QPair<QString, QString>> &customHeaders = {});
    QUrl streamUrl(const QString &assetId) const;

    // True when url is the Immich origin (same host, https): the only hop that
    // gets the mTLS/custom-CA config. CDN/S3 redirect hosts return false.
    static bool isOriginHost(const QUrl &url, const QUrl &apiBase);

    // True when url targets the Immich API host, regardless of scheme/port.
    // Keeps custom headers on the Immich origin while excluding CDN/S3 hops.
    static bool isApiHost(const QUrl &url, const QUrl &apiBase);

private:
    struct Session {
        QTcpSocket *client = nullptr;
        QByteArray requestBuffer;
        bool headersComplete = false;
    };

    void handleNewConnection();
    void handleClientReadyRead(QTcpSocket *client);
    void serveRequest(QTcpSocket *client, const QByteArray &request);
    void startUpstream(QTcpSocket *client, const QUrl &url, const QByteArray &rangeHeader,
                       bool isHead, bool sendApiKey, int redirects);
    QUrl playbackUrl(const QString &assetId) const;

    QTcpServer *m_server;
    QNetworkAccessManager *m_network;
    QUrl m_apiBaseUrl;
    QString m_apiKey;
    QSslConfiguration m_sslConfig;
    QList<QPair<QString, QString>> m_customHeaders;
    QString m_sessionToken;
    QHash<QTcpSocket *, Session> m_sessions;
};

} // namespace Aurora
