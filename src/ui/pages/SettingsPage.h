#pragma once

#include <QWidget>

namespace Aurora {

class AnimatedStackedWidget;
class FolderSyncService;
class ImmichClient;
class ThemeManager;
class UpdateManager;

class SettingsPage final : public QWidget {
    Q_OBJECT

public:
    explicit SettingsPage(ThemeManager *themeManager, UpdateManager *updateManager,
                          ImmichClient *immichClient, FolderSyncService *folderSync,
                          QWidget *parent = nullptr);

    bool isShowingUpdates() const;

public slots:
    void showConnection();
    void showFolderSync();
    void showAppearance();
    void showUpdates();
    void showAbout();

private:
    AnimatedStackedWidget *m_sections;
};

} // namespace Aurora
