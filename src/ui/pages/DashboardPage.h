#pragma once

#include <QWidget>

namespace Aurora {

class DashboardPage final : public QWidget {
    Q_OBJECT

public:
    explicit DashboardPage(QWidget *parent = nullptr);
};

} // namespace Aurora
