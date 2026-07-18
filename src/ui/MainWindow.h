#pragma once

#include <QList>
#include <QMainWindow>

class QEvent;
class QResizeEvent;
class QShowEvent;

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

protected:
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;
    void changeEvent(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    void applyWindowCorners();
    void ensureResizableFrame();
    void selectPage(int index);
    void updateResizeHandles();
    void scheduleAutoUpdateCheck();
    void notifyUpdateAvailable();

    TopBar *m_topBar;
    AnimatedStackedWidget *m_pages;
    SettingsPage *m_settingsPage;
    Sidebar *m_sidebar;
    UpdateManager *m_updateManager;
    QList<ResizeHandle *> m_resizeHandles;
    bool m_autoCheckScheduled = false;
};

} // namespace Aurora
