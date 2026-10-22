#include "ui/pages/ConnectionPage.h"
#include "ui/IconUtils.h"

#include "core/ImmichClient.h"
#include "core/TlsConfig.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>

#include <functional>

namespace Aurora {

ConnectionPage::ConnectionPage(ImmichClient *client, QWidget *parent)
    : QWidget(parent)
    , m_client(client)
    , m_serverUrl(new QLineEdit(this))
    , m_localServerUrl(new QLineEdit(this))
    , m_apiKey(new QLineEdit(this))
    , m_certFormat(new QComboBox(this))
    , m_clientCertPath(new QLineEdit(this))
    , m_clientKeyPath(new QLineEdit(this))
    , m_clientKeyLabel(new QLabel(tr("Client key"), this))
    , m_caCertPath(new QLineEdit(this))
    , m_keyPassword(new QLineEdit(this))
    , m_rememberKeyPassword(new QCheckBox(tr("Remember password"), this))
    , m_status(new QLabel(this))
    , m_saveButton(new QPushButton(tr("Save & test connection"), this))
{
    configureActionButton(m_saveButton, QStringLiteral(":/icons/save.svg"));
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 12, 0, 0);
    root->setSpacing(14);

    auto *card = new QFrame(this);
    card->setProperty("card", true);
    auto *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(22, 20, 22, 22);
    cardLayout->setSpacing(14);

    auto *title = new QLabel(tr("Immich server"), card);
    title->setProperty("section", true);
    auto *description = new QLabel(
        tr("Connect this desktop client to your Immich instance. The API key needs "
           "user.read, asset.read, asset.view, asset.upload, asset.download, "
           "asset.delete, and person.read permissions. An optional local URL is "
           "preferred automatically when reachable."),
        card);
    description->setProperty("subheading", true);
    description->setWordWrap(true);

    const ImmichConnectionSettings current = m_client->connection();
    m_serverUrl->setText(current.serverUrl);
    m_serverUrl->setPlaceholderText(QStringLiteral("https://photos.example.com"));
    m_serverUrl->setClearButtonEnabled(true);
    m_localServerUrl->setText(current.localServerUrl);
    m_localServerUrl->setPlaceholderText(
        tr("Optional - e.g. http://192.168.1.10:2283"));
    m_localServerUrl->setClearButtonEnabled(true);
    m_apiKey->setText(current.apiKey);
    m_apiKey->setPlaceholderText(tr("Paste an Immich API key"));
    m_apiKey->setEchoMode(QLineEdit::Password);
    m_apiKey->setClearButtonEnabled(true);

    m_certFormat->addItem(tr("PKCS#12 (.p12/.pfx)"));
    m_certFormat->addItem(tr("PEM certificate + key"));
    m_certFormat->setCurrentIndex(static_cast<int>(current.certFormat));
    m_clientCertPath->setText(current.clientCertPath);
    m_clientCertPath->setClearButtonEnabled(true);
    m_clientKeyPath->setText(current.clientKeyPath);
    m_clientKeyPath->setClearButtonEnabled(true);
    m_caCertPath->setText(current.caCertPath);
    m_caCertPath->setClearButtonEnabled(true);
    m_keyPassword->setText(current.keyPassword);
    m_keyPassword->setEchoMode(QLineEdit::Password);
    m_keyPassword->setClearButtonEnabled(true);
    m_rememberKeyPassword->setChecked(current.rememberKeyPassword);

    const QString pemFilter = tr("PEM files (*.pem *.crt *.cer *.key)");
    // Pairs a line edit with a Browse… button that seeds its dialog from the
    // current path's directory and the given filter.
    const auto makePathRow = [this](QLineEdit *edit, const QString &caption,
                                    std::function<QString()> filter) {
        auto *container = new QWidget(this);
        auto *row = new QHBoxLayout(container);
        row->setContentsMargins(0, 0, 0, 0);
        row->setSpacing(8);
        auto *browse = new QPushButton(tr("Browse…"), this);
        connect(browse, &QPushButton::clicked, this, [this, edit, caption, filter] {
            const QString dir = QFileInfo(edit->text().trimmed()).absolutePath();
            const QString picked =
                QFileDialog::getOpenFileName(this, caption, dir, filter());
            if (!picked.isEmpty())
                edit->setText(picked);
        });
        row->addWidget(edit, 1);
        row->addWidget(browse);
        return container;
    };

    auto *clientCertRow = makePathRow(
        m_clientCertPath, tr("Select client certificate"), [this, pemFilter] {
            return m_certFormat->currentIndex() == static_cast<int>(TlsCertFormat::Pkcs12)
                       ? tr("PKCS#12 bundles (*.p12 *.pfx)")
                       : pemFilter;
        });
    auto *clientKeyRow =
        makePathRow(m_clientKeyPath, tr("Select client key"),
                    [pemFilter] { return pemFilter; });
    auto *caCertRow = makePathRow(m_caCertPath, tr("Select CA certificate"),
                                  [pemFilter] { return pemFilter; });

    // The client key only exists as a separate file in PEM mode; in PKCS#12 it
    // lives inside the bundle, so hide the whole row (label + field + button)
    // there. clientKeyRow is a container widget, so hiding it hides the Browse
    // button too — hiding only the line edit would strand the button.
    const auto applyKeyRowVisibility = [this, clientKeyRow] {
        const bool pem = m_certFormat->currentIndex() == static_cast<int>(TlsCertFormat::Pem);
        m_clientKeyLabel->setVisible(pem);
        clientKeyRow->setVisible(pem);
    };
    connect(m_certFormat, &QComboBox::currentIndexChanged, this,
            [applyKeyRowVisibility] { applyKeyRowVisibility(); });

    auto *certTitle = new QLabel(tr("Client certificate (optional)"), card);
    certTitle->setProperty("section", true);

    auto *form = new QFormLayout;
    form->setContentsMargins(0, 4, 0, 0);
    form->setHorizontalSpacing(18);
    form->setVerticalSpacing(12);
    form->addRow(tr("Immich URL"), m_serverUrl);
    form->addRow(tr("Local URL"), m_localServerUrl);
    form->addRow(tr("API Key"), m_apiKey);
    form->addRow(certTitle);
    form->addRow(tr("Format"), m_certFormat);
    form->addRow(tr("Client cert"), clientCertRow);
    form->addRow(m_clientKeyLabel, clientKeyRow);
    form->addRow(tr("Server CA"), caCertRow);
    auto *caHint = new QLabel(
        tr("Certificate that signed the server (or its reverse proxy). Needed when "
           "the server uses a private/internal CA. This is separate from the client "
           "certificate above and is not taken from the bundle."),
        card);
    caHint->setWordWrap(true);
    caHint->setProperty("hint", true);
    form->addRow(QString(), caHint);
    form->addRow(tr("Key password"), m_keyPassword);
    form->addRow(QString(), m_rememberKeyPassword);

    applyKeyRowVisibility();

    auto *privacy = new QLabel(
        tr("The API key is stored locally in this app's settings and is never sent "
           "anywhere except your configured Immich server. When a local URL is set, "
           "the app probes it every few seconds and uses it while available."),
        card);
    privacy->setProperty("subheading", true);
    privacy->setWordWrap(true);

    m_saveButton->setProperty("primary", true);
    m_status->setProperty("subheading", true);
    m_status->setWordWrap(true);
    if (current.isConfigured()) {
        showActiveEndpoint(m_client->usingLocalEndpoint(), m_client->activeServerUrl());
    } else {
        m_status->setText(tr("Enter your server details to load the media library."));
    }

    auto *actions = new QHBoxLayout;
    actions->addWidget(m_status, 1);
    actions->addWidget(m_saveButton);

    cardLayout->addWidget(title);
    cardLayout->addWidget(description);
    cardLayout->addLayout(form);
    cardLayout->addWidget(privacy);
    cardLayout->addLayout(actions);
    root->addWidget(card);
    root->addStretch();

    connect(m_saveButton, &QPushButton::clicked, this, &ConnectionPage::saveAndTest);
    connect(m_serverUrl, &QLineEdit::returnPressed, this, &ConnectionPage::saveAndTest);
    connect(m_localServerUrl, &QLineEdit::returnPressed, this, &ConnectionPage::saveAndTest);
    connect(m_apiKey, &QLineEdit::returnPressed, this, &ConnectionPage::saveAndTest);
    connect(m_client, &ImmichClient::connectionTested,
            this, &ConnectionPage::showConnectionResult);
    connect(m_client, &ImmichClient::activeEndpointChanged,
            this, &ConnectionPage::showActiveEndpoint);
}

void ConnectionPage::saveAndTest()
{
    QUrl url = QUrl::fromUserInput(m_serverUrl->text().trimmed());
    const auto isAllowedServerUrl = [](const QUrl &candidate) {
        const QString scheme = candidate.scheme().toLower();
        return candidate.isValid() && !candidate.host().isEmpty() &&
               (scheme == QStringLiteral("https") || scheme == QStringLiteral("http")) &&
               candidate.userName().isEmpty() && candidate.password().isEmpty();
    };
    if (!isAllowedServerUrl(url)) {
        m_status->setText(
            tr("Enter an HTTP(S) Immich URL without embedded credentials."));
        return;
    }
    if (m_apiKey->text().trimmed().isEmpty()) {
        m_status->setText(tr("Enter an API key."));
        return;
    }

    ImmichConnectionSettings settings;
    settings.serverUrl = url.toString(QUrl::RemoveQuery | QUrl::RemoveFragment);
    settings.apiKey = m_apiKey->text();
    settings.certFormat = static_cast<TlsCertFormat>(m_certFormat->currentIndex());
    settings.clientCertPath = m_clientCertPath->text().trimmed();
    settings.clientKeyPath = m_clientKeyPath->text().trimmed();
    settings.caCertPath = m_caCertPath->text().trimmed();
    settings.keyPassword = m_keyPassword->text();
    settings.rememberKeyPassword = m_rememberKeyPassword->isChecked();

    const QString localText = m_localServerUrl->text().trimmed();
    if (!localText.isEmpty()) {
        QUrl localUrl = QUrl::fromUserInput(localText);
        if (!isAllowedServerUrl(localUrl)) {
            m_status->setText(
                tr("Local URL must be HTTP(S) and must not contain embedded credentials."));
            return;
        }
        settings.localServerUrl =
            localUrl.toString(QUrl::RemoveQuery | QUrl::RemoveFragment);
        m_localServerUrl->setText(settings.localServerUrl);
    }

    m_serverUrl->setText(settings.serverUrl);
    m_client->setConnection(settings);
    m_saveButton->setEnabled(false);
    m_status->setText(tr("Testing connection…"));
    m_client->testConnection();
}

void ConnectionPage::showConnectionResult(bool success, const QString &message)
{
    m_saveButton->setEnabled(true);
    m_status->setText(success ? tr("✓ %1").arg(message) : tr("Could not connect: %1").arg(message));
}

void ConnectionPage::showActiveEndpoint(bool usingLocal, const QString &activeUrl)
{
    if (!m_client->isConfigured() || activeUrl.isEmpty())
        return;
    if (m_saveButton->isEnabled() == false)
        return;
    m_status->setText(usingLocal
                          ? tr("Using local endpoint: %1").arg(activeUrl)
                          : tr("Using remote endpoint: %1").arg(activeUrl));
}

} // namespace Aurora
