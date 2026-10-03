#pragma once

#include <QWidget>

class QCheckBox;
class QLabel;
class QLineEdit;
class QPushButton;

namespace Aurora {

class FolderSyncService;
class ThemeManager;

class SyncFolderPage final : public QWidget {
    Q_OBJECT

public:
    explicit SyncFolderPage(FolderSyncService *syncService, ThemeManager *themeManager,
                            QWidget *parent = nullptr);

private slots:
    void chooseFolder();
    void saveSettings();
    void refreshStatus();
    void openFolder();

private:
    FolderSyncService *m_syncService = nullptr;
    ThemeManager *m_themeManager = nullptr;
    QCheckBox *m_enabled = nullptr;
    QLineEdit *m_folderPath = nullptr;
    QPushButton *m_browseButton = nullptr;
    QPushButton *m_openButton = nullptr;
    QPushButton *m_scanButton = nullptr;
    QLabel *m_statusIcon = nullptr;
    QLabel *m_statusText = nullptr;
};

} // namespace Aurora
