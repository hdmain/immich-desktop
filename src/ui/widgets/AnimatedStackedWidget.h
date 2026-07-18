#pragma once

#include <QStackedWidget>

namespace Aurora {

class AnimatedStackedWidget final : public QStackedWidget {
    Q_OBJECT

public:
    explicit AnimatedStackedWidget(QWidget *parent = nullptr);

public slots:
    void setCurrentIndexAnimated(int index);

private:
    bool m_animating = false;
};

} // namespace Aurora
