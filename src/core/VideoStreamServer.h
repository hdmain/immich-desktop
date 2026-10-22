#pragma once

#include <QByteArray>
#include <QHash>
#include <QObject>
#include <QSslConfiguration>
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
                        const QSslConfiguration &sslConfig);
    QUrl streamUrl(const QString &assetId) const;

    // True when url is the Immich origin (same host, https): the only hop that
    // gets the mTLS/custom-CA config. CDN/S3 redirect hosts return false.
    static bool isOriginHost(const QUrl &url, const QUrl &apiBase);

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
    QString m_sessionToken;
    QHash<QTcpSocket *, Session> m_sessions;
};

} // namespace Aurora
