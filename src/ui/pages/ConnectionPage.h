#pragma once

#include <QWidget>

class QLabel;
class QLineEdit;
class QPushButton;

namespace Aurora {

class ImmichClient;

class ConnectionPage final : public QWidget {
    Q_OBJECT

public:
    explicit ConnectionPage(ImmichClient *client, QWidget *parent = nullptr);

private slots:
    void saveAndTest();
    void showConnectionResult(bool success, const QString &message);

private:
    ImmichClient *m_client;
    QLineEdit *m_serverUrl;
    QLineEdit *m_apiKey;
    QLabel *m_status;
    QPushButton *m_saveButton;
};

} // namespace Aurora
