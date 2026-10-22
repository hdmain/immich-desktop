#pragma once

#include <QWidget>

class QCheckBox;
class QComboBox;
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
    void showActiveEndpoint(bool usingLocal, const QString &activeUrl);

private:
    ImmichClient *m_client;
    QLineEdit *m_serverUrl;
    QLineEdit *m_localServerUrl;
    QLineEdit *m_apiKey;
    QComboBox *m_certFormat;
    QLineEdit *m_clientCertPath;
    QLineEdit *m_clientKeyPath;
    QLabel *m_clientKeyLabel;
    QLineEdit *m_caCertPath;
    QLineEdit *m_keyPassword;
    QCheckBox *m_rememberKeyPassword;
    QLabel *m_status;
    QPushButton *m_saveButton;
};

} // namespace Aurora
