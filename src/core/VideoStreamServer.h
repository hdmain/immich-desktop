#pragma once

#include <QByteArray>
#include <QHash>
#include <QObject>
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
    void setCredentials(const QUrl &apiBaseUrl, const QString &apiKey);
    QUrl streamUrl(const QString &assetId) const;

private:
    struct Session {
        QTcpSocket *client = nullptr;
        QByteArray requestBuffer;
        bool headersComplete = false;
    };

    void handleNewConnection();
    void handleClientReadyRead(QTcpSocket *client);
    void serveRequest(QTcpSocket *client, const QByteArray &request);
    QUrl playbackUrl(const QString &assetId) const;

    QTcpServer *m_server;
    QNetworkAccessManager *m_network;
    QUrl m_apiBaseUrl;
    QString m_apiKey;
    QHash<QTcpSocket *, Session> m_sessions;
};

} // namespace Aurora
