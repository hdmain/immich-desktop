#pragma once

#include <QObject>
#include <QString>

class QLocalServer;

namespace Aurora {

// Ensures only one process runs. Secondary launches ask the primary to raise.
class SingleInstance final : public QObject {
    Q_OBJECT

public:
    explicit SingleInstance(const QString &applicationKey, QObject *parent = nullptr);
    ~SingleInstance() override;

    // Returns true if another instance was found and activated (caller should exit).
    bool activateExistingInstance();

    bool isPrimary() const { return m_server != nullptr; }

signals:
    void activationRequested();

private:
    void startPrimaryServer();
    void handleNewConnection();
    static QString serverNameForKey(const QString &applicationKey);

    QString m_serverName;
    QLocalServer *m_server = nullptr;
};

} // namespace Aurora
