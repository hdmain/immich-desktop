#include "ui/pages/ConnectionPage.h"
#include "ui/IconUtils.h"

#include "core/CustomHeaders.h"
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
#include <QScrollArea>
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
    , m_enableCustomHeaders(new QCheckBox(tr("Enable custom HTTP headers"), this))
    , m_enableClientCert(new QCheckBox(tr("Enable client certificate"), this))
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
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto *scrollContent = new QWidget(scroll);
    auto *scrollLayout = new QVBoxLayout(scrollContent);
    scrollLayout->setContentsMargins(0, 12, 8, 12);
    scrollLayout->setSpacing(14);

    auto *card = new QFrame(scrollContent);
    card->setProperty("card", true);
    auto *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(22, 20, 22, 22);
    cardLayout->setSpacing(14);

    auto *title = new QLabel(tr("Immich server"), card);
    title->setProperty("section", true);
    auto *description = new QLabel(
        tr("Connect this desktop client to your Immich instance. The API key needs "
           "user.read, asset.read, asset.view, asset.upload, asset.download, "
           "asset.delete, and person.read permissions; album.read is optional and lists a "
           "photo's albums. An optional local URL is "
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

    m_enableCustomHeaders->setChecked(!current.customHeaders.isEmpty());
    m_enableClientCert->setChecked(current.hasClientCert());
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
    m_clientKeyRow = makePathRow(m_clientKeyPath, tr("Select client key"),
                                 [pemFilter] { return pemFilter; });
    auto *caCertRow = makePathRow(m_caCertPath, tr("Select CA certificate"),
                                  [pemFilter] { return pemFilter; });

    auto *form = new QFormLayout;
    form->setContentsMargins(0, 4, 0, 0);
    form->setHorizontalSpacing(18);
    form->setVerticalSpacing(12);
    form->addRow(tr("Immich URL"), m_serverUrl);
    form->addRow(tr("Local URL"), m_localServerUrl);
    form->addRow(tr("API Key"), m_apiKey);

    form->addRow(QString(), m_enableCustomHeaders);

    m_headerFields = new QWidget(card);
    auto *headersSectionLayout = new QVBoxLayout(m_headerFields);
    headersSectionLayout->setContentsMargins(0, 0, 0, 0);
    headersSectionLayout->setSpacing(8);

    auto *headersHint = new QLabel(
        tr("Sent with every request to your Immich server, including the local URL "
           "and video playback, but not to third-party CDN addresses. Values are "
           "stored locally in this app's plaintext settings file."),
        m_headerFields);
    headersHint->setProperty("subheading", true);
    headersHint->setWordWrap(true);

    m_headerRowsLayout = new QVBoxLayout;
    m_headerRowsLayout->setContentsMargins(0, 0, 0, 0);
    m_headerRowsLayout->setSpacing(8);

    auto *addHeader = new QPushButton(tr("Add header"), m_headerFields);
    configureActionButton(addHeader, QStringLiteral(":/ui/plus.svg"));
    addHeader->setCursor(Qt::PointingHandCursor);

    headersSectionLayout->addWidget(headersHint);
    headersSectionLayout->addLayout(m_headerRowsLayout);
    auto *addHeaderRowLayout = new QHBoxLayout;
    addHeaderRowLayout->addWidget(addHeader);
    addHeaderRowLayout->addStretch();
    headersSectionLayout->addLayout(addHeaderRowLayout);
    form->addRow(m_headerFields);

    if (current.customHeaders.isEmpty()) {
        addHeaderRow();
    } else {
        for (const auto &header : current.customHeaders)
            addHeaderRow(header.first, header.second);
    }

    connect(addHeader, &QPushButton::clicked, this, [this] {
        this->addHeaderRow();
        if (!m_headerRows.isEmpty())
            m_headerRows.last().first->setFocus();
    });

    connect(m_enableCustomHeaders, &QCheckBox::toggled, this,
            &ConnectionPage::updateHeaderVisibility);
    updateHeaderVisibility();

    form->addRow(QString(), m_enableClientCert);

    m_clientCertFields = new QWidget(card);
    auto *certForm = new QFormLayout(m_clientCertFields);
    certForm->setContentsMargins(0, 0, 0, 0);
    certForm->setHorizontalSpacing(18);
    certForm->setVerticalSpacing(12);
    certForm->addRow(tr("Format"), m_certFormat);
    certForm->addRow(tr("Client cert"), clientCertRow);
    certForm->addRow(m_clientKeyLabel, m_clientKeyRow);
    certForm->addRow(tr("Server CA"), caCertRow);
    auto *caHint = new QLabel(
        tr("Certificate that signed the server (or its reverse proxy). Needed when "
           "the server uses a private/internal CA. This is separate from the client "
           "certificate above and is not taken from the bundle."),
        m_clientCertFields);
    caHint->setWordWrap(true);
    caHint->setProperty("hint", true);
    certForm->addRow(QString(), caHint);
    certForm->addRow(tr("Key password"), m_keyPassword);
    certForm->addRow(QString(), m_rememberKeyPassword);
    form->addRow(m_clientCertFields);

    connect(m_enableClientCert, &QCheckBox::toggled, this,
            &ConnectionPage::updateClientCertVisibility);
    connect(m_certFormat, &QComboBox::currentIndexChanged, this,
            &ConnectionPage::updateClientCertVisibility);
    updateClientCertVisibility();

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
    scrollLayout->addWidget(card);
    scrollLayout->addStretch();
    scroll->setWidget(scrollContent);
    root->addWidget(scroll, 1);

    connect(m_saveButton, &QPushButton::clicked, this, &ConnectionPage::saveAndTest);
    connect(m_serverUrl, &QLineEdit::returnPressed, this, &ConnectionPage::saveAndTest);
    connect(m_localServerUrl, &QLineEdit::returnPressed, this, &ConnectionPage::saveAndTest);
    connect(m_apiKey, &QLineEdit::returnPressed, this, &ConnectionPage::saveAndTest);
    connect(m_client, &ImmichClient::connectionTested,
            this, &ConnectionPage::showConnectionResult);
    connect(m_client, &ImmichClient::activeEndpointChanged,
            this, &ConnectionPage::showActiveEndpoint);
}

void ConnectionPage::addHeaderRow(const QString &name, const QString &value)
{
    auto *row = new QWidget(this);
    auto *layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    auto *nameEdit = new QLineEdit(name, row);
    nameEdit->setPlaceholderText(QStringLiteral("X-Custom-Header"));
    nameEdit->setAccessibleName(tr("Header name"));
    nameEdit->setClearButtonEnabled(true);

    auto *valueEdit = new QLineEdit(value, row);
    valueEdit->setPlaceholderText(tr("value"));
    valueEdit->setAccessibleName(tr("Header value"));
    valueEdit->setClearButtonEnabled(true);

    auto *remove = new QPushButton(row);
    remove->setObjectName(QStringLiteral("removeHeaderButton"));
    configureActionButton(remove, QStringLiteral(":/icons/trash.svg"));
    remove->setCursor(Qt::PointingHandCursor);
    remove->setToolTip(tr("Remove header"));
    remove->setAccessibleName(tr("Remove header"));

    layout->addWidget(nameEdit, 2);
    layout->addWidget(valueEdit, 3);
    layout->addWidget(remove);
    m_headerRowsLayout->addWidget(row);
    m_headerRows.append(qMakePair(nameEdit, valueEdit));

    connect(nameEdit, &QLineEdit::returnPressed, this, &ConnectionPage::saveAndTest);
    connect(valueEdit, &QLineEdit::returnPressed, this, &ConnectionPage::saveAndTest);
    connect(remove, &QPushButton::clicked, this, [this, row, nameEdit, valueEdit] {
        for (qsizetype i = 0; i < m_headerRows.size(); ++i) {
            if (m_headerRows.at(i).first == nameEdit &&
                m_headerRows.at(i).second == valueEdit) {
                m_headerRows.removeAt(i);
                break;
            }
        }
        m_headerRowsLayout->removeWidget(row);
        row->hide();
        row->deleteLater();
        if (m_headerRows.isEmpty())
            addHeaderRow();
    });
}

QList<QPair<QString, QString>> ConnectionPage::headerRows() const
{
    QList<QPair<QString, QString>> headers;
    headers.reserve(m_headerRows.size());
    for (const auto &row : m_headerRows) {
        // Keep fully empty rows so validationError() reports the row number
        // the user sees; normalize() drops them before persisting.
        headers.append(qMakePair(row.first->text().trimmed(),
                                 row.second->text().trimmed()));
    }
    return headers;
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
    if (m_enableClientCert->isChecked()) {
        settings.certFormat = static_cast<TlsCertFormat>(m_certFormat->currentIndex());
        settings.clientCertPath = m_clientCertPath->text().trimmed();
        settings.clientKeyPath = m_clientKeyPath->text().trimmed();
        settings.caCertPath = m_caCertPath->text().trimmed();
        settings.keyPassword = m_keyPassword->text();
        settings.rememberKeyPassword = m_rememberKeyPassword->isChecked();
    }

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

    if (m_enableCustomHeaders->isChecked()) {
        const auto headers = headerRows();
        const QString headerError = CustomHeaders::validationError(headers);
        if (!headerError.isEmpty()) {
            m_status->setText(headerError);
            return;
        }
        settings.customHeaders = CustomHeaders::normalize(headers);
    }

    m_serverUrl->setText(settings.serverUrl);
    m_client->setConnection(settings);
    m_saveButton->setEnabled(false);
    m_status->setText(tr("Testing connection…"));
    m_client->testConnection();
}

void ConnectionPage::updateHeaderVisibility()
{
    m_headerFields->setVisible(m_enableCustomHeaders->isChecked());
}

void ConnectionPage::updateClientCertVisibility()
{
    const bool enabled = m_enableClientCert->isChecked();
    m_clientCertFields->setVisible(enabled);
    if (!enabled)
        return;
    // PEM keeps a separate key file; PKCS#12 embeds it in the bundle.
    const bool pem = m_certFormat->currentIndex() == static_cast<int>(TlsCertFormat::Pem);
    m_clientKeyLabel->setVisible(pem);
    m_clientKeyRow->setVisible(pem);
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
