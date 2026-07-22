#include "ui/pages/UpdatesPage.h"

#include "AppVersion.h"
#include "core/UpdateManager.h"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QSignalBlocker>
#include <QVBoxLayout>

namespace Aurora {

UpdatesPage::UpdatesPage(UpdateManager *updateManager, QWidget *parent)
    : QWidget(parent)
    , m_updateManager(updateManager)
    , m_versionLabel(new QLabel(this))
    , m_statusLabel(new QLabel(this))
    , m_progress(new QProgressBar(this))
    , m_autoCheck(new QCheckBox(tr("Check for updates automatically"), this))
    , m_refreshButton(new QPushButton(tr("Refresh"), this))
    , m_updateButton(new QPushButton(tr("Update"), this))
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(16);

    auto *heading = new QLabel(tr("Updates"), this);
    heading->setProperty("heading", true);
    root->addWidget(heading);

    m_versionLabel->setProperty("subheading", true);
    m_versionLabel->setText(
        tr("Current version: v%1").arg(QString::fromLatin1(Config::ApplicationVersion)));
    root->addWidget(m_versionLabel);

    m_statusLabel->setProperty("section", true);
    m_statusLabel->setWordWrap(true);
    root->addWidget(m_statusLabel);

    m_progress->setRange(0, 100);
    m_progress->setValue(0);
    m_progress->setTextVisible(true);
    m_progress->setVisible(false);
    root->addWidget(m_progress);

    auto *buttons = new QHBoxLayout;
    buttons->setSpacing(10);
    m_refreshButton->setCursor(Qt::PointingHandCursor);
    m_updateButton->setCursor(Qt::PointingHandCursor);
    m_updateButton->setProperty("primary", true);
    buttons->addWidget(m_refreshButton);
    buttons->addWidget(m_updateButton);
    buttons->addStretch();
    root->addLayout(buttons);

    m_autoCheck->setCursor(Qt::PointingHandCursor);
    root->addWidget(m_autoCheck);
    root->addStretch();

    connect(m_refreshButton, &QPushButton::clicked, this, [this] {
        m_updateManager->checkForUpdates(false);
    });
    connect(m_updateButton, &QPushButton::clicked, m_updateManager, &UpdateManager::applyUpdate);
    connect(m_autoCheck, &QCheckBox::toggled, m_updateManager, &UpdateManager::setAutoCheckEnabled);
    connect(m_updateManager, &UpdateManager::stateChanged, this, [this](UpdateState) {
        refreshUi();
    });
    connect(m_updateManager, &UpdateManager::updateAvailable, this, [this](const UpdateInfo &) {
        refreshUi();
    });
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

    const bool busy = state == UpdateState::Checking || state == UpdateState::Downloading ||
                      state == UpdateState::Installing;
    m_refreshButton->setEnabled(!busy);
    m_updateButton->setEnabled(state == UpdateState::Available ||
                               state == UpdateState::ReadyToInstall ||
                               state == UpdateState::Failed);

    switch (state) {
    case UpdateState::Idle:
        m_statusLabel->setText(tr("Press Refresh to check for updates."));
        m_progress->setVisible(false);
        break;
    case UpdateState::Checking:
        m_statusLabel->setText(tr("Checking for updates…"));
        m_progress->setVisible(true);
        m_progress->setRange(0, 0);
        break;
    case UpdateState::UpToDate:
        m_statusLabel->setText(tr("You're up to date."));
        m_progress->setVisible(false);
        break;
    case UpdateState::Available:
        m_statusLabel->setText(tr("Update available: v%1").arg(info.version));
        m_progress->setVisible(false);
        break;
    case UpdateState::Downloading:
        m_statusLabel->setText(tr("Downloading v%1…").arg(info.version));
        m_progress->setVisible(true);
        break;
    case UpdateState::ReadyToInstall:
        m_statusLabel->setText(tr("Installing v%1…").arg(info.version));
        m_progress->setVisible(true);
        m_progress->setRange(0, 100);
        m_progress->setValue(100);
        m_progress->setFormat(tr("Download complete"));
        break;
    case UpdateState::Installing:
        m_statusLabel->setText(tr("Installing update…"));
        m_progress->setVisible(true);
        m_progress->setRange(0, 0);
        break;
    case UpdateState::Failed:
        m_statusLabel->setText(tr("Update failed: %1").arg(m_updateManager->errorMessage()));
        m_progress->setVisible(false);
        break;
    }
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

} // namespace Aurora
