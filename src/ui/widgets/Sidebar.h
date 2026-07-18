#pragma once

#include <QWidget>

class QButtonGroup;

namespace Aurora {

class ThemeManager;

class Sidebar final : public QWidget {
    Q_OBJECT

public:
    explicit Sidebar(ThemeManager *themeManager, QWidget *parent = nullptr);

signals:
    void pageRequested(int index);
};

} // namespace Aurora
