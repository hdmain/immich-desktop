#pragma once

#include <QList>
#include <QMainWindow>

class QCloseEvent;
class QEvent;
class QResizeEvent;
class QShowEvent;
class QSystemTrayIcon;

namespace Aurora {

class AnimatedStackedWidget;
class ImmichClient;
class ResizeHandle;
class SettingsPage;
class Sidebar;
class ThemeManager;
class TopBar;
class UpdateManager;

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(ThemeManager *themeManager, UpdateManager *updateManager,
                        ImmichClient *immichClient,
                        QWidget *parent = nullptr);

    void raiseToFront();
    void scheduleStartupUpdateCheck();
    void scheduleGitHubStarPrompt();

protected:
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;
    void changeEvent(QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    void setupTrayIcon();
    void quitApplication();
    bool closeToTrayEnabled() const;

    void applyWindowCorners();
    void ensureResizableFrame();
    void selectPage(int index);
    void updateResizeHandles();
    void notifyUpdateAvailable();
    void maybeShowGitHubStarPrompt();
    void refreshTrayIcon();

    TopBar *m_topBar;
    AnimatedStackedWidget *m_pages;
    SettingsPage *m_settingsPage;
    Sidebar *m_sidebar;
    ThemeManager *m_themeManager;
    UpdateManager *m_updateManager;
    ImmichClient *m_immichClient = nullptr;
    QSystemTrayIcon *m_trayIcon = nullptr;
    QList<ResizeHandle *> m_resizeHandles;
    bool m_autoCheckScheduled = false;
    bool m_starPromptScheduled = false;
    bool m_forceQuit = false;
};

} // namespace Aurora
