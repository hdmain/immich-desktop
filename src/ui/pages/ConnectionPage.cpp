#include "ui/pages/ConnectionPage.h"

#include "core/ImmichClient.h"

#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>

namespace Aurora {

ConnectionPage::ConnectionPage(ImmichClient *client, QWidget *parent)
    : QWidget(parent)
    , m_client(client)
    , m_serverUrl(new QLineEdit(this))
    , m_localServerUrl(new QLineEdit(this))
    , m_apiKey(new QLineEdit(this))
    , m_status(new QLabel(this))
    , m_saveButton(new QPushButton(tr("Save & test connection"), this))
{
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
        tr("Optional — e.g. http://192.168.1.10:2283"));
    m_localServerUrl->setClearButtonEnabled(true);
    m_apiKey->setText(current.apiKey);
    m_apiKey->setPlaceholderText(tr("Paste an Immich API key"));
    m_apiKey->setEchoMode(QLineEdit::Password);
    m_apiKey->setClearButtonEnabled(true);

    auto *form = new QFormLayout;
    form->setContentsMargins(0, 4, 0, 0);
    form->setHorizontalSpacing(18);
    form->setVerticalSpacing(12);
    form->addRow(tr("Immich URL"), m_serverUrl);
    form->addRow(tr("Local URL"), m_localServerUrl);
    form->addRow(tr("API Key"), m_apiKey);

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
