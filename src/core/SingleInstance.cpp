#include "core/SingleInstance.h"

#include <QByteArray>
#include <QCryptographicHash>
#include <QLocalServer>
#include <QLocalSocket>
#include <QSysInfo>

namespace Aurora {
namespace {

constexpr char kActivateMessage[] = "activate\n";

} // namespace

SingleInstance::SingleInstance(const QString &applicationKey, QObject *parent)
    : QObject(parent)
    , m_serverName(serverNameForKey(applicationKey))
{
}

SingleInstance::~SingleInstance()
{
    if (m_server) {
        m_server->close();
        QLocalServer::removeServer(m_serverName);
    }
}

QString SingleInstance::serverNameForKey(const QString &applicationKey)
{
    QString identity = applicationKey;
    identity += QLatin1Char('-');
    identity += QSysInfo::machineHostName();
    identity += QLatin1Char('-');
#ifdef Q_OS_WIN
    identity += QString::fromLocal8Bit(qgetenv("USERNAME"));
#else
    identity += QString::fromLocal8Bit(qgetenv("USER"));
#endif
    const QByteArray hash =
        QCryptographicHash::hash(identity.toUtf8(), QCryptographicHash::Sha1).toHex().left(16);
    return QStringLiteral("immich-desktop-%1").arg(QString::fromLatin1(hash));
}

bool SingleInstance::activateExistingInstance()
{
    QLocalSocket socket;
    socket.connectToServer(m_serverName);
    if (!socket.waitForConnected(500)) {
        startPrimaryServer();
        return false;
    }

    socket.write(kActivateMessage);
    socket.flush();
    socket.waitForBytesWritten(500);
    socket.disconnectFromServer();
    if (socket.state() != QLocalSocket::UnconnectedState)
        socket.waitForDisconnected(500);
    return true;
}

void SingleInstance::startPrimaryServer()
{
    m_server = new QLocalServer(this);
    m_server->setSocketOptions(QLocalServer::UserAccessOption);
    connect(m_server, &QLocalServer::newConnection, this, &SingleInstance::handleNewConnection);

    if (!m_server->listen(m_serverName)) {
        // Stale socket left by a crashed previous run.
        QLocalServer::removeServer(m_serverName);
        if (!m_server->listen(m_serverName)) {
            delete m_server;
            m_server = nullptr;
        }
    }
}

void SingleInstance::handleNewConnection()
{
    while (m_server && m_server->hasPendingConnections()) {
        QLocalSocket *client = m_server->nextPendingConnection();
        if (!client)
            continue;
        connect(client, &QLocalSocket::readyRead, this, [this, client] {
            const QByteArray payload = client->readAll();
            if (payload == kActivateMessage)
                emit activationRequested();
        });
        connect(client, &QLocalSocket::disconnected, client, &QLocalSocket::deleteLater);
        // Message may already be buffered before readyRead connects.
        if (client->bytesAvailable() > 0) {
            const QByteArray payload = client->readAll();
            if (payload == kActivateMessage)
                emit activationRequested();
        }
    }
}

} // namespace Aurora
