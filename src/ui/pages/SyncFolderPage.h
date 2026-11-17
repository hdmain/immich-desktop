#pragma once

#include "core/AppSettings.h"
#include "core/ImmichTypes.h"

#include <QWidget>
#include <QList>

class QCheckBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QVBoxLayout;

namespace Aurora {

class FolderSyncService;
class ImmichClient;
class NoWheelComboBox;
class ThemeManager;

class SyncFolderPage final : public QWidget {
    Q_OBJECT

public:
    explicit SyncFolderPage(FolderSyncService *syncService, ImmichClient *immichClient,
                            ThemeManager *themeManager, QWidget *parent = nullptr);

private slots:
    void addFolder();
    void saveSettings();
    void refreshStatus();
    void handleAlbumsLoaded(const QList<Aurora::ImmichAlbum> &albums);

private:
    struct FolderRow {
        QLineEdit *pathField = nullptr;
        NoWheelComboBox *albumCombo = nullptr;
    };

    void addFolderRow(const QString &path, const QString &albumId = {});
    void loadFolderRows(const QList<FolderSyncFolder> &folders);
    QList<FolderSyncFolder> selectedFolders() const;
    void chooseFolder(QLineEdit *field);
    void populateAlbumCombo(NoWheelComboBox *combo, const QString &selectedAlbumId) const;
    void refreshAlbumCombos();

    FolderSyncService *m_syncService = nullptr;
    ImmichClient *m_immichClient = nullptr;
    ThemeManager *m_themeManager = nullptr;
    QCheckBox *m_enabled = nullptr;
    QCheckBox *m_localNetworkOnly = nullptr;
    QList<FolderRow> m_folderRows;
    QList<ImmichAlbum> m_albums;
    QVBoxLayout *m_foldersLayout = nullptr;
    QPushButton *m_addFolderButton = nullptr;
    QPushButton *m_scanButton = nullptr;
    QLabel *m_statusIcon = nullptr;
    QLabel *m_statusText = nullptr;
};

} // namespace Aurora
