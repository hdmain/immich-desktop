#pragma once

#include <QWidget>

class QLabel;
class QMouseEvent;
class QPushButton;

namespace Aurora {

class ThemeManager;

class TopBar final : public QWidget {
    Q_OBJECT

public:
    explicit TopBar(ThemeManager *themeManager, QWidget *parent = nullptr);
    void setPageTitle(const QString &title);
    void setUpdateAvailable(bool available, const QString &version = {});

signals:
    void updatesRequested();

protected:
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    void toggleMaximized();
    void refreshIcons();

    ThemeManager *m_themeManager;
    QLabel *m_title;
    QPushButton *m_updateButton;
    QPushButton *m_minimizeButton;
    QPushButton *m_maximizeButton;
    QPushButton *m_closeButton;
};

} // namespace Aurora
