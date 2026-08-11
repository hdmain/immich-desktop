#pragma once

#include <QWidget>

namespace Aurora {

class AboutPage final : public QWidget {
    Q_OBJECT

public:
    explicit AboutPage(QWidget *parent = nullptr);
};

} // namespace Aurora
