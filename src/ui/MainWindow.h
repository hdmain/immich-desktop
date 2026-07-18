#pragma once

#include <QList>
#include <QMainWindow>

class QEvent;
class QResizeEvent;
class QShowEvent;

namespace Aurora {

class AnimatedStackedWidget;
class ResizeHandle;
class ThemeManager;
class TopBar;

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(ThemeManager *themeManager, QWidget *parent = nullptr);

protected:
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;
    void changeEvent(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    void applyWindowCorners();
    void selectPage(int index);
    void updateResizeHandles();

    TopBar *m_topBar;
    AnimatedStackedWidget *m_pages;
    QList<ResizeHandle *> m_resizeHandles;
};

} // namespace Aurora
