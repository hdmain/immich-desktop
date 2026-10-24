#pragma once

#include <QList>
#include <QPair>
#include <QWidget>

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QVBoxLayout;

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
    void updateClientCertVisibility();
    void updateHeaderVisibility();
    void addHeaderRow(const QString &name = {}, const QString &value = {});
    QList<QPair<QString, QString>> headerRows() const;

    ImmichClient *m_client;
    QLineEdit *m_serverUrl;
    QLineEdit *m_localServerUrl;
    QLineEdit *m_apiKey;
    QCheckBox *m_enableCustomHeaders;
    QWidget *m_headerFields = nullptr;
    QCheckBox *m_enableClientCert;
    QWidget *m_clientCertFields = nullptr;
    QComboBox *m_certFormat;
    QLineEdit *m_clientCertPath;
    QLineEdit *m_clientKeyPath;
    QLabel *m_clientKeyLabel;
    QWidget *m_clientKeyRow = nullptr;
    QLineEdit *m_caCertPath;
    QLineEdit *m_keyPassword;
    QCheckBox *m_rememberKeyPassword;
    QLabel *m_status;
    QPushButton *m_saveButton;
    QVBoxLayout *m_headerRowsLayout = nullptr;
    QList<QPair<QLineEdit *, QLineEdit *>> m_headerRows;
};

} // namespace Aurora
