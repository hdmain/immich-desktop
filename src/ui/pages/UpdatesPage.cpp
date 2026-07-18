#include "ui/pages/UpdatesPage.h"

#include "AppVersion.h"
#include "core/UpdateManager.h"

#include <QCheckBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTextEdit>
#include <QVBoxLayout>

namespace Aurora {
namespace {

QFrame *sectionCard(const QString &title, const QString &description, QWidget *parent,
                    QVBoxLayout **contentLayout)
{
    auto *card = new QFrame(parent);
    card->setProperty("card", true);
    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(20, 18, 20, 20);
    layout->setSpacing(12);

    auto *titleLabel = new QLabel(title, card);
    titleLabel->setProperty("section", true);
    auto *descriptionLabel = new QLabel(description, card);
    descriptionLabel->setProperty("subheading", true);
    descriptionLabel->setWordWrap(true);
    layout->addWidget(titleLabel);
    layout->addWidget(descriptionLabel);
    layout->addSpacing(4);
    *contentLayout = layout;
    return card;
}

} // namespace

UpdatesPage::UpdatesPage(UpdateManager *updateManager, QWidget *parent)
    : QWidget(parent)
    , m_updateManager(updateManager)
    , m_statusLabel(new QLabel(this))
    , m_detailLabel(new QLabel(this))
    , m_packageLabel(new QLabel(this))
    , m_notesView(new QTextEdit(this))
    , m_progress(new QProgressBar(this))
    , m_autoCheck(new QCheckBox(tr("Check for updates automatically"), this))
    , m_checkButton(new QPushButton(tr("Check for updates"), this))
    , m_downloadButton(new QPushButton(tr("Download update"), this))
    , m_installButton(new QPushButton(tr("Install and restart"), this))
    , m_skipButton(new QPushButton(tr("Skip this version"), this))
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(4, 6, 4, 4);
    root->setSpacing(8);

    auto *heading = new QLabel(tr("Updates"), this);
    heading->setProperty("heading", true);
    auto *subheading = new QLabel(
        tr("Detect new releases and install the package that matches this platform."),
        this);
    subheading->setProperty("subheading", true);
    root->addWidget(heading);
    root->addWidget(subheading);
    root->addSpacing(18);

    QVBoxLayout *statusLayout = nullptr;
    auto *statusCard = sectionCard(
        tr("Status"),
        tr("Current version: v%1").arg(QString::fromLatin1(Config::ApplicationVersion)),
        this, &statusLayout);
    m_statusLabel->setProperty("section", true);
    m_detailLabel->setProperty("subheading", true);
    m_detailLabel->setWordWrap(true);
    m_packageLabel->setProperty("subheading", true);
    m_packageLabel->setWordWrap(true);
    m_progress->setRange(0, 100);
    m_progress->setValue(0);
    m_progress->setTextVisible(true);
    m_progress->setVisible(false);
    statusLayout->addWidget(m_statusLabel);
    statusLayout->addWidget(m_detailLabel);
    statusLayout->addWidget(m_packageLabel);
    statusLayout->addWidget(m_progress);
    root->addWidget(statusCard);

    QVBoxLayout *actionLayout = nullptr;
    auto *actionCard = sectionCard(
        tr("Actions"),
        tr("Updates are downloaded safely, then installed with the matching package type."),
        this, &actionLayout);
    m_checkButton->setProperty("primary", true);
    m_checkButton->setCursor(Qt::PointingHandCursor);
    m_downloadButton->setCursor(Qt::PointingHandCursor);
    m_installButton->setCursor(Qt::PointingHandCursor);
    m_installButton->setProperty("primary", true);
    m_skipButton->setCursor(Qt::PointingHandCursor);
    m_autoCheck->setCursor(Qt::PointingHandCursor);

    auto *buttonRow = new QHBoxLayout;
    buttonRow->setSpacing(10);
    buttonRow->addWidget(m_checkButton);
    buttonRow->addWidget(m_downloadButton);
    buttonRow->addWidget(m_installButton);
    buttonRow->addWidget(m_skipButton);
    buttonRow->addStretch();
    actionLayout->addLayout(buttonRow);
    actionLayout->addWidget(m_autoCheck);
    root->addWidget(actionCard);

    QVBoxLayout *notesLayout = nullptr;
    auto *notesCard = sectionCard(
        tr("Release notes"),
        tr("Notes from the latest GitHub release appear here when an update is available."),
        this, &notesLayout);
    m_notesView->setReadOnly(true);
    m_notesView->setMinimumHeight(160);
    m_notesView->setPlaceholderText(tr("No release notes yet."));
    notesLayout->addWidget(m_notesView);
    root->addWidget(notesCard, 1);

    connect(m_checkButton, &QPushButton::clicked, this, [this] {
        m_updateManager->checkForUpdates(false);
    });
    connect(m_downloadButton, &QPushButton::clicked, m_updateManager, &UpdateManager::downloadUpdate);
    connect(m_installButton, &QPushButton::clicked, m_updateManager, &UpdateManager::installUpdate);
    connect(m_skipButton, &QPushButton::clicked, m_updateManager, &UpdateManager::skipCurrentUpdate);
    connect(m_autoCheck, &QCheckBox::toggled, m_updateManager, &UpdateManager::setAutoCheckEnabled);
    connect(m_updateManager, &UpdateManager::stateChanged, this, [this](UpdateState) {
        refreshUi();
    });
    connect(m_updateManager, &UpdateManager::updateAvailable, this, &UpdatesPage::onUpdateAvailable);
    connect(m_updateManager, &UpdateManager::upToDate, this, [this] { refreshUi(); });
    connect(m_updateManager, &UpdateManager::errorOccurred, this, [this](const QString &) {
        refreshUi();
    });
    connect(m_updateManager, &UpdateManager::downloadProgress, this,
            [this](qint64 received, qint64 total) {
                m_progress->setVisible(true);
                if (total > 0) {
                    m_progress->setRange(0, 100);
                    m_progress->setValue(static_cast<int>((received * 100) / total));
                    m_progress->setFormat(
                        tr("%p%  ·  %1 / %2")
                            .arg(formatBytes(received), formatBytes(total)));
                } else {
                    m_progress->setRange(0, 0);
                }
            });

    refreshUi();
}

void UpdatesPage::refreshUi()
{
    const auto state = m_updateManager->state();
    const auto info = m_updateManager->availableUpdate();
    const QSignalBlocker blocker(m_autoCheck);
    m_autoCheck->setChecked(m_updateManager->settings().autoCheck);

    m_downloadButton->setEnabled(state == UpdateState::Available ||
                                 state == UpdateState::ReadyToInstall ||
                                 state == UpdateState::Failed);
    m_installButton->setEnabled(state == UpdateState::ReadyToInstall);
    m_skipButton->setEnabled(state == UpdateState::Available ||
                             state == UpdateState::ReadyToInstall);
    m_checkButton->setEnabled(state != UpdateState::Checking &&
                              state != UpdateState::Downloading &&
                              state != UpdateState::Installing);

    switch (state) {
    case UpdateState::Idle:
        m_statusLabel->setText(tr("Ready"));
        m_detailLabel->setText(tr("Check for updates whenever you want, or leave automatic checks on."));
        m_packageLabel->clear();
        m_progress->setVisible(false);
        break;
    case UpdateState::Checking:
        m_statusLabel->setText(tr("Checking for updates…"));
        m_detailLabel->setText(tr("Contacting GitHub Releases."));
        m_packageLabel->clear();
        m_progress->setVisible(true);
        m_progress->setRange(0, 0);
        break;
    case UpdateState::UpToDate:
        m_statusLabel->setText(tr("You're up to date"));
        m_detailLabel->setText(
            tr("immich desktop v%1 is the latest available release.")
                .arg(QString::fromLatin1(Config::ApplicationVersion)));
        m_packageLabel->clear();
        m_notesView->clear();
        m_progress->setVisible(false);
        break;
    case UpdateState::Available:
        m_statusLabel->setText(tr("Update available: v%1").arg(info.version));
        m_detailLabel->setText(tr("A newer release is ready to download."));
        m_packageLabel->setText(packageLabel(info));
        m_progress->setVisible(false);
        break;
    case UpdateState::Downloading:
        m_statusLabel->setText(tr("Downloading v%1…").arg(info.version));
        m_detailLabel->setText(tr("The installer is being downloaded in the background."));
        m_packageLabel->setText(packageLabel(info));
        m_progress->setVisible(true);
        break;
    case UpdateState::ReadyToInstall:
        m_statusLabel->setText(tr("Ready to install v%1").arg(info.version));
        m_detailLabel->setText(
            tr("The package is ready. Installation will close the app and restart it afterwards."));
        m_packageLabel->setText(packageLabel(info));
        m_progress->setVisible(true);
        m_progress->setRange(0, 100);
        m_progress->setValue(100);
        m_progress->setFormat(tr("Download complete"));
        break;
    case UpdateState::Installing:
        m_statusLabel->setText(tr("Installing update…"));
        m_detailLabel->setText(tr("Launching the installer and restarting immich desktop."));
        m_packageLabel->setText(packageLabel(info));
        break;
    case UpdateState::Failed:
        m_statusLabel->setText(tr("Update failed"));
        m_detailLabel->setText(m_updateManager->errorMessage());
        m_packageLabel->setText(info.assetName.isEmpty() ? QString() : packageLabel(info));
        m_progress->setVisible(false);
        break;
    }
}

void UpdatesPage::onUpdateAvailable(const UpdateInfo &info)
{
    m_notesView->setPlainText(info.releaseNotes.isEmpty()
                                  ? tr("No release notes were published for this version.")
                                  : info.releaseNotes);
    refreshUi();
}

QString UpdatesPage::formatBytes(qint64 bytes)
{
    constexpr double kb = 1024.0;
    constexpr double mb = kb * 1024.0;
    if (bytes >= static_cast<qint64>(mb))
        return QStringLiteral("%1 MB").arg(bytes / mb, 0, 'f', 1);
    if (bytes >= static_cast<qint64>(kb))
        return QStringLiteral("%1 KB").arg(bytes / kb, 0, 'f', 0);
    return QStringLiteral("%1 B").arg(bytes);
}

QString UpdatesPage::packageLabel(const UpdateInfo &info)
{
    QString kind;
    switch (info.installKind) {
    case InstallKind::WindowsExe: kind = QStringLiteral("Windows installer (.exe)"); break;
    case InstallKind::WindowsMsi: kind = QStringLiteral("Windows installer (.msi)"); break;
    case InstallKind::LinuxDeb: kind = QStringLiteral("Debian package (.deb)"); break;
    case InstallKind::LinuxAppImage: kind = QStringLiteral("Linux AppImage"); break;
    case InstallKind::Unknown: kind = QStringLiteral("Package"); break;
    }
    if (info.sizeBytes > 0)
        return QStringLiteral("%1  ·  %2  ·  %3")
            .arg(kind, info.assetName, formatBytes(info.sizeBytes));
    return QStringLiteral("%1  ·  %2").arg(kind, info.assetName);
}

} // namespace Aurora
