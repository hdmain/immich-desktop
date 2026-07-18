#pragma once

#include <QWidget>

namespace Aurora {

class AnimatedStackedWidget;
class ThemeManager;
class UpdateManager;

class SettingsPage final : public QWidget {
    Q_OBJECT

public:
    explicit SettingsPage(ThemeManager *themeManager, UpdateManager *updateManager,
                          QWidget *parent = nullptr);

    bool isShowingUpdates() const;

public slots:
    void showAppearance();
    void showUpdates();

private:
    AnimatedStackedWidget *m_sections;
};

} // namespace Aurora
