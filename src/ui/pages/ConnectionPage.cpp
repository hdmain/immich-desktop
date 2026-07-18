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
           "user.read, asset.read, and asset.view permissions."),
        card);
    description->setProperty("subheading", true);
    description->setWordWrap(true);

    const ImmichConnectionSettings current = m_client->connection();
    m_serverUrl->setText(current.serverUrl);
    m_serverUrl->setPlaceholderText(QStringLiteral("https://photos.example.com"));
    m_serverUrl->setClearButtonEnabled(true);
    m_apiKey->setText(current.apiKey);
    m_apiKey->setPlaceholderText(tr("Paste an Immich API key"));
    m_apiKey->setEchoMode(QLineEdit::Password);
    m_apiKey->setClearButtonEnabled(true);

    auto *form = new QFormLayout;
    form->setContentsMargins(0, 4, 0, 0);
    form->setHorizontalSpacing(18);
    form->setVerticalSpacing(12);
    form->addRow(tr("Immich URL"), m_serverUrl);
    form->addRow(tr("API Key"), m_apiKey);

    auto *privacy = new QLabel(
        tr("The API key is stored locally in this app's settings and is never sent "
           "anywhere except your configured Immich server."),
        card);
    privacy->setProperty("subheading", true);
    privacy->setWordWrap(true);

    m_saveButton->setProperty("primary", true);
    m_status->setProperty("subheading", true);
    m_status->setWordWrap(true);
    if (current.isConfigured())
        m_status->setText(tr("Connection details are saved. Test them to verify access."));
    else
        m_status->setText(tr("Enter your server details to load the media library."));

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
    connect(m_apiKey, &QLineEdit::returnPressed, this, &ConnectionPage::saveAndTest);
    connect(m_client, &ImmichClient::connectionTested,
            this, &ConnectionPage::showConnectionResult);
}

void ConnectionPage::saveAndTest()
{
    QUrl url = QUrl::fromUserInput(m_serverUrl->text().trimmed());
    if (!url.isValid() || url.host().isEmpty()) {
        m_status->setText(tr("Enter a valid Immich URL, including the host."));
        return;
    }
    if (m_apiKey->text().trimmed().isEmpty()) {
        m_status->setText(tr("Enter an API key."));
        return;
    }

    m_serverUrl->setText(url.toString(QUrl::RemoveQuery | QUrl::RemoveFragment));
    m_client->setConnection({m_serverUrl->text(), m_apiKey->text()});
    m_saveButton->setEnabled(false);
    m_status->setText(tr("Testing connection…"));
    m_client->testConnection();
}

void ConnectionPage::showConnectionResult(bool success, const QString &message)
{
    m_saveButton->setEnabled(true);
    m_status->setText(success ? tr("✓ %1").arg(message) : tr("Could not connect: %1").arg(message));
}

} // namespace Aurora
