#include "ui/pages/SyncFolderPage.h"

#include "core/FolderSyncService.h"
#include "core/ThemeManager.h"
#include "ui/IconUtils.h"

#include <QCheckBox>
#include <QDesktopServices>
#include <QFileDialog>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QUrl>
#include <QVBoxLayout>

namespace Aurora {

SyncFolderPage::SyncFolderPage(FolderSyncService *syncService, ThemeManager *themeManager,
                               QWidget *parent)
    : QWidget(parent)
    , m_syncService(syncService)
    , m_themeManager(themeManager)
    , m_enabled(new QCheckBox(tr("Watch a local folder and upload new media"), this))
    , m_folderPath(new QLineEdit(this))
    , m_browseButton(new QPushButton(tr("Browse…"), this))
    , m_openButton(new QPushButton(tr("Open folder"), this))
    , m_scanButton(new QPushButton(tr("Scan now"), this))
    , m_statusIcon(new QLabel(this))
    , m_statusText(new QLabel(this))
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 12, 0, 0);
    root->setSpacing(14);

    auto *card = new QFrame(this);
    card->setProperty("card", true);
    auto *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(22, 20, 22, 22);
    cardLayout->setSpacing(14);

    auto *title = new QLabel(tr("Folder sync"), card);
    title->setProperty("section", true);
    auto *description = new QLabel(
        tr("Pick a folder on this computer. Photos and videos dropped into it "
           "(including subfolders) are uploaded to Immich automatically on Windows, "
           "macOS, and Linux. Files stay on disk after upload."),
        card);
    description->setProperty("subheading", true);
    description->setWordWrap(true);

    const FolderSyncSettings current = m_syncService->settings();
    m_enabled->setChecked(current.enabled);
    m_enabled->setCursor(Qt::PointingHandCursor);
    m_folderPath->setText(current.folderPath);
    m_folderPath->setPlaceholderText(tr("Choose a folder to sync…"));
    m_folderPath->setClearButtonEnabled(true);

    auto *pathRow = new QHBoxLayout;
    pathRow->setSpacing(8);
    pathRow->addWidget(m_folderPath, 1);
    pathRow->addWidget(m_browseButton);

    auto *actions = new QHBoxLayout;
    actions->setSpacing(8);
    actions->addWidget(m_openButton);
    actions->addWidget(m_scanButton);
    actions->addStretch();

    auto *statusRow = new QHBoxLayout;
    statusRow->setSpacing(10);
    m_statusIcon->setFixedSize(28, 28);
    m_statusIcon->setAlignment(Qt::AlignCenter);
    m_statusText->setProperty("subheading", true);
    m_statusText->setWordWrap(true);
    statusRow->addWidget(m_statusIcon, 0, Qt::AlignTop);
    statusRow->addWidget(m_statusText, 1);

    auto *legend = new QLabel(
        tr("Status icons: saved · saving · unsaved · error saving"), card);
    legend->setProperty("subheading", true);
    legend->setWordWrap(true);

    auto *legendIcons = new QHBoxLayout;
    legendIcons->setSpacing(16);
    const auto addLegend = [this, legendIcons](const QString &path, const QString &label) {
        auto *icon = new QLabel(this);
        icon->setFixedSize(22, 22);
        icon->setPixmap(renderSvgIcon(path, m_themeManager->palette().mutedText, QSize(20, 20),
                                      devicePixelRatioF()));
        auto *text = new QLabel(label, this);
        text->setProperty("subheading", true);
        auto *row = new QHBoxLayout;
        row->setSpacing(6);
        row->addWidget(icon);
        row->addWidget(text);
        legendIcons->addLayout(row);
    };
    addLegend(QStringLiteral(":/icons/cloud-check.svg"), tr("Saved"));
    addLegend(QStringLiteral(":/icons/cloud-upload.svg"), tr("Saving"));
    addLegend(QStringLiteral(":/icons/cloud-off.svg"), tr("Unsaved"));
    addLegend(QStringLiteral(":/icons/cloud-alert.svg"), tr("Error"));
    legendIcons->addStretch();

    cardLayout->addWidget(title);
    cardLayout->addWidget(description);
    cardLayout->addWidget(m_enabled);
    cardLayout->addLayout(pathRow);
    cardLayout->addLayout(actions);
    cardLayout->addLayout(statusRow);
    cardLayout->addWidget(legend);
    cardLayout->addLayout(legendIcons);
    root->addWidget(card);
    root->addStretch();

    connect(m_enabled, &QCheckBox::toggled, this, &SyncFolderPage::saveSettings);
    connect(m_folderPath, &QLineEdit::editingFinished, this, &SyncFolderPage::saveSettings);
    connect(m_browseButton, &QPushButton::clicked, this, &SyncFolderPage::chooseFolder);
    connect(m_openButton, &QPushButton::clicked, this, &SyncFolderPage::openFolder);
    connect(m_scanButton, &QPushButton::clicked, this, [this] {
        saveSettings();
        m_syncService->scanNow();
    });
    connect(m_syncService, &FolderSyncService::statusChanged, this,
            &SyncFolderPage::refreshStatus);
    connect(m_syncService, &FolderSyncService::settingsChanged, this, [this](const FolderSyncSettings &settings) {
        const QSignalBlocker enabledBlocker(m_enabled);
        const QSignalBlocker pathBlocker(m_folderPath);
        m_enabled->setChecked(settings.enabled);
        m_folderPath->setText(settings.folderPath);
        refreshStatus();
    });
    connect(m_themeManager, &ThemeManager::appearanceChanged, this,
            &SyncFolderPage::refreshStatus);

    refreshStatus();
}

void SyncFolderPage::chooseFolder()
{
    const QString start = m_folderPath->text().trimmed().isEmpty()
                              ? QString()
                              : m_folderPath->text().trimmed();
    const QString folder = QFileDialog::getExistingDirectory(
        this, tr("Choose sync folder"), start,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (folder.isEmpty())
        return;
    m_folderPath->setText(folder);
    saveSettings();
}

void SyncFolderPage::saveSettings()
{
    FolderSyncSettings settings;
    settings.enabled = m_enabled->isChecked();
    settings.folderPath = m_folderPath->text().trimmed();
    m_syncService->setSettings(settings);
}

void SyncFolderPage::openFolder()
{
    const QString path = m_folderPath->text().trimmed();
    if (path.isEmpty())
        return;
    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

void SyncFolderPage::refreshStatus()
{
    const QColor color = m_themeManager->palette().text;
    m_statusIcon->setPixmap(renderSvgIcon(m_syncService->statusIconPath(), color,
                                          QSize(24, 24), devicePixelRatioF()));
    m_statusText->setText(m_syncService->statusMessage());
    m_openButton->setEnabled(!m_folderPath->text().trimmed().isEmpty());
    m_scanButton->setEnabled(m_enabled->isChecked() &&
                             !m_folderPath->text().trimmed().isEmpty());
}

} // namespace Aurora
