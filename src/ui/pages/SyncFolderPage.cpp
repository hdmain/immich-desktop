#include "ui/pages/SyncFolderPage.h"

#include "core/FolderSyncService.h"
#include "core/ImmichClient.h"
#include "core/ThemeManager.h"
#include "ui/IconUtils.h"
#include "ui/widgets/NoWheelComboBox.h"

#include <QCheckBox>
#include <QDesktopServices>
#include <QFileDialog>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QUrl>
#include <QVBoxLayout>

namespace Aurora {

SyncFolderPage::SyncFolderPage(FolderSyncService *syncService, ImmichClient *immichClient,
                               ThemeManager *themeManager, QWidget *parent)
    : QWidget(parent)
    , m_syncService(syncService)
    , m_immichClient(immichClient)
    , m_themeManager(themeManager)
    , m_enabled(new QCheckBox(tr("Watch folders and upload new media"), this))
    , m_localNetworkOnly(new QCheckBox(
          tr("Upload only when Immich is available on the local network"), this))
    , m_addFolderButton(new QPushButton(this))
    , m_scanButton(new QPushButton(tr("Scan now"), this))
    , m_statusIcon(new QLabel(this))
    , m_statusText(new QLabel(this))
{
    auto *root = new QVBoxLayout(this);
    configureActionButton(m_addFolderButton, QStringLiteral(":/ui/plus.svg"));
    m_addFolderButton->setObjectName(QStringLiteral("addSyncFolderButton"));
    m_addFolderButton->setToolTip(tr("Add another folder"));
    m_addFolderButton->setAccessibleName(tr("Add another folder"));
    m_addFolderButton->setCursor(Qt::PointingHandCursor);
    configureActionButton(m_scanButton, QStringLiteral(":/icons/refresh-cw.svg"));
    root->setContentsMargins(0, 12, 0, 0);
    root->setSpacing(14);
    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    auto *content = new QWidget(scroll);
    auto *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(0, 0, 8, 0);

    auto *card = new QFrame(this);
    card->setProperty("card", true);
    auto *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(22, 20, 22, 22);
    cardLayout->setSpacing(14);

    auto *title = new QLabel(tr("Folder sync"), card);
    title->setProperty("section", true);
    auto *description = new QLabel(
        tr("Choose folders on this computer. Photos and videos dropped into them "
           "(including subfolders) are uploaded to Immich automatically on Windows, "
           "macOS, and Linux. Optionally pick an album per folder. Files stay on disk "
           "after upload."),
        card);
    description->setProperty("subheading", true);
    description->setWordWrap(true);

    const FolderSyncSettings current = m_syncService->settings();
    m_enabled->setChecked(current.enabled);
    m_enabled->setCursor(Qt::PointingHandCursor);
    m_localNetworkOnly->setChecked(current.localNetworkOnly);
    m_localNetworkOnly->setCursor(Qt::PointingHandCursor);
    m_localNetworkOnly->setToolTip(
        tr("Uses your Local URL from Immich Server settings. Files still appear in "
           "Library with an unsaved badge until the local server is reachable."));
    auto *foldersHeader = new QHBoxLayout;
    auto *foldersLabel = new QLabel(tr("Folders"), card);
    foldersLabel->setProperty("section", true);
    foldersHeader->addWidget(foldersLabel);
    foldersHeader->addStretch();
    foldersHeader->addWidget(m_addFolderButton);
    m_foldersLayout = new QVBoxLayout;
    m_foldersLayout->setSpacing(10);
    loadFolderRows(current.folders);

    auto *actions = new QHBoxLayout;
    actions->setSpacing(8);
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
    cardLayout->addWidget(m_localNetworkOnly);
    cardLayout->addLayout(foldersHeader);
    cardLayout->addLayout(m_foldersLayout);
    cardLayout->addLayout(actions);
    cardLayout->addLayout(statusRow);
    cardLayout->addWidget(legend);
    cardLayout->addLayout(legendIcons);
    contentLayout->addWidget(card);
    contentLayout->addStretch();
    scroll->setWidget(content);
    root->addWidget(scroll);

    connect(m_enabled, &QCheckBox::toggled, this, &SyncFolderPage::saveSettings);
    connect(m_localNetworkOnly, &QCheckBox::toggled, this, &SyncFolderPage::saveSettings);
    connect(m_addFolderButton, &QPushButton::clicked, this, &SyncFolderPage::addFolder);
    connect(m_scanButton, &QPushButton::clicked, this, [this] {
        saveSettings();
        m_syncService->scanNow();
    });
    connect(m_syncService, &FolderSyncService::statusChanged, this,
            &SyncFolderPage::refreshStatus);
    connect(m_syncService, &FolderSyncService::settingsChanged, this,
            [this](const FolderSyncSettings &settings) {
                const QSignalBlocker enabledBlocker(m_enabled);
                const QSignalBlocker localOnlyBlocker(m_localNetworkOnly);
                m_enabled->setChecked(settings.enabled);
                m_localNetworkOnly->setChecked(settings.localNetworkOnly);
                if (selectedFolders() != settings.folders)
                    loadFolderRows(settings.folders);
                refreshStatus();
            });
    connect(m_themeManager, &ThemeManager::appearanceChanged, this,
            &SyncFolderPage::refreshStatus);
    connect(m_immichClient, &ImmichClient::albumsLoaded, this,
            &SyncFolderPage::handleAlbumsLoaded);
    connect(m_immichClient, &ImmichClient::configurationChanged, this, [this](bool configured) {
        if (configured)
            m_immichClient->loadAlbums();
        else {
            m_albums.clear();
            refreshAlbumCombos();
        }
    });
    if (m_immichClient->isConfigured())
        m_immichClient->loadAlbums();

    refreshStatus();
}

void SyncFolderPage::chooseFolder(QLineEdit *field)
{
    const QString folder = QFileDialog::getExistingDirectory(
        this, tr("Choose sync folder"), field->text().trimmed(), QFileDialog::ShowDirsOnly);
    if (folder.isEmpty())
        return;
    field->setText(folder);
    saveSettings();
}

void SyncFolderPage::saveSettings()
{
    FolderSyncSettings settings;
    settings.enabled = m_enabled->isChecked();
    settings.localNetworkOnly = m_localNetworkOnly->isChecked();
    settings.folders = selectedFolders();
    m_syncService->setSettings(settings);
}

QList<FolderSyncFolder> SyncFolderPage::selectedFolders() const
{
    QList<FolderSyncFolder> folders;
    for (const FolderRow &row : m_folderRows) {
        const QString path = row.pathField->text().trimmed();
        if (path.isEmpty())
            continue;
        FolderSyncFolder folder;
        folder.path = path;
        folder.albumId = row.albumCombo->currentData().toString();
        folders.append(folder);
    }
    return folders;
}

void SyncFolderPage::addFolder()
{
    const QString folder = QFileDialog::getExistingDirectory(
        this, tr("Add sync folder"), selectedFolders().value(0).path, QFileDialog::ShowDirsOnly);
    if (folder.isEmpty())
        return;
    if (m_folderRows.size() == 1 && m_folderRows.first().pathField->text().trimmed().isEmpty())
        m_folderRows.first().pathField->setText(folder);
    else
        addFolderRow(folder);
    saveSettings();
}

void SyncFolderPage::loadFolderRows(const QList<FolderSyncFolder> &folders)
{
    m_folderRows.clear();
    while (auto *item = m_foldersLayout->takeAt(0)) {
        if (auto *widget = item->widget()) {
            widget->hide();
            widget->deleteLater();
        }
        delete item;
    }
    if (folders.isEmpty())
        addFolderRow(QString());
    else
        for (const FolderSyncFolder &folder : folders)
            addFolderRow(folder.path, folder.albumId);
}

void SyncFolderPage::addFolderRow(const QString &path, const QString &albumId)
{
    auto *row = new QWidget(this);
    auto *layout = new QVBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    auto *pathRow = new QWidget(row);
    auto *pathLayout = new QHBoxLayout(pathRow);
    pathLayout->setContentsMargins(0, 0, 0, 0);
    pathLayout->setSpacing(8);
    auto *field = new QLineEdit(path, pathRow);
    field->setObjectName(QStringLiteral("syncFolderPath"));
    field->setAccessibleName(tr("Sync folder path"));
    field->setPlaceholderText(tr("Choose a folder to sync…"));
    field->setClearButtonEnabled(true);
    pathLayout->addWidget(field, 1);
    auto *browse = new QPushButton(tr("Browse…"), pathRow);
    auto *open = new QPushButton(tr("Open folder"), pathRow);
    auto *remove = new QPushButton(pathRow);
    remove->setObjectName(QStringLiteral("removeSyncFolderButton"));
    configureActionButton(browse, QStringLiteral(":/icons/folder.svg"));
    configureActionButton(open, QStringLiteral(":/icons/external-link.svg"));
    configureActionButton(remove, QStringLiteral(":/icons/trash.svg"));
    remove->setToolTip(tr("Remove folder from sync"));
    remove->setAccessibleName(tr("Remove folder from sync"));
    open->setEnabled(!path.trimmed().isEmpty());
    for (auto *button : {browse, open, remove}) {
        button->setCursor(Qt::PointingHandCursor);
        pathLayout->addWidget(button);
    }

    auto *albumRow = new QHBoxLayout;
    albumRow->setSpacing(8);
    auto *albumLabel = new QLabel(tr("Upload to album"), row);
    albumLabel->setProperty("subheading", true);
    auto *albumCombo = new NoWheelComboBox(row);
    albumCombo->setObjectName(QStringLiteral("syncFolderAlbum"));
    albumCombo->setAccessibleName(tr("Upload to album"));
    albumCombo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    albumCombo->setMinimumContentsLength(18);
    populateAlbumCombo(albumCombo, albumId);
    albumRow->addWidget(albumLabel);
    albumRow->addWidget(albumCombo, 1);

    layout->addWidget(pathRow);
    layout->addLayout(albumRow);
    m_foldersLayout->addWidget(row);

    FolderRow folderRow;
    folderRow.pathField = field;
    folderRow.albumCombo = albumCombo;
    m_folderRows.append(folderRow);

    connect(field, &QLineEdit::editingFinished, this, &SyncFolderPage::saveSettings);
    connect(field, &QLineEdit::textChanged, this, [this, open](const QString &text) {
        open->setEnabled(!text.trimmed().isEmpty());
        refreshStatus();
    });
    connect(albumCombo, &QComboBox::currentIndexChanged, this, &SyncFolderPage::saveSettings);
    connect(browse, &QPushButton::clicked, this, [this, field] { chooseFolder(field); });
    connect(open, &QPushButton::clicked, this, [field] {
        if (!field->text().trimmed().isEmpty())
            QDesktopServices::openUrl(QUrl::fromLocalFile(field->text().trimmed()));
    });
    connect(remove, &QPushButton::clicked, this, [this, row, field] {
        m_folderRows.removeIf([field](const FolderRow &entry) { return entry.pathField == field; });
        m_foldersLayout->removeWidget(row);
        row->hide();
        row->deleteLater();
        if (m_folderRows.isEmpty())
            addFolderRow(QString());
        saveSettings();
    });
}

void SyncFolderPage::populateAlbumCombo(NoWheelComboBox *combo, const QString &selectedAlbumId) const
{
    const QSignalBlocker blocker(combo);
    combo->clear();
    combo->addItem(tr("Library only (no album)"), QString());
    int selectedIndex = 0;
    for (const ImmichAlbum &album : m_albums) {
        combo->addItem(album.name, album.id);
        if (!selectedAlbumId.isEmpty() && album.id == selectedAlbumId)
            selectedIndex = combo->count() - 1;
    }
    if (!selectedAlbumId.isEmpty() && selectedIndex == 0) {
        combo->addItem(tr("Album unavailable (%1)").arg(selectedAlbumId.left(8)), selectedAlbumId);
        selectedIndex = combo->count() - 1;
    }
    combo->setCurrentIndex(selectedIndex);
}

void SyncFolderPage::refreshAlbumCombos()
{
    for (const FolderRow &row : m_folderRows) {
        const QString selected = row.albumCombo->currentData().toString();
        populateAlbumCombo(row.albumCombo, selected);
    }
}

void SyncFolderPage::handleAlbumsLoaded(const QList<ImmichAlbum> &albums)
{
    m_albums = albums;
    refreshAlbumCombos();
}

void SyncFolderPage::refreshStatus()
{
    const QColor color = m_themeManager->palette().text;
    m_statusIcon->setPixmap(renderSvgIcon(m_syncService->statusIconPath(), color,
                                          QSize(24, 24), devicePixelRatioF()));
    m_statusText->setText(m_syncService->statusMessage());
    m_scanButton->setEnabled(m_enabled->isChecked() && !selectedFolders().isEmpty());
}

} // namespace Aurora
