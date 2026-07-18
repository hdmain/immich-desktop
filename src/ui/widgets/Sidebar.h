#pragma once

#include <QWidget>

class QButtonGroup;
class QLabel;
class QVBoxLayout;

namespace Aurora {

class ThemeManager;

class Sidebar final : public QWidget {
    Q_OBJECT

public:
    explicit Sidebar(ThemeManager *themeManager, QWidget *parent = nullptr);
    void setCurrentPage(int index);

signals:
    void pageRequested(int index);

private:
    void addNavigation(const QString &label, const QString &icon, int index,
                       bool checkable = true);
    void clearNavigation();
    void showMainNavigation();
    void showSettingsNavigation();

    ThemeManager *m_themeManager;
    QLabel *m_navigationLabel;
    QVBoxLayout *m_navigationLayout;
    QButtonGroup *m_navigationGroup;
    bool m_settingsMode = false;
};

} // namespace Aurora
