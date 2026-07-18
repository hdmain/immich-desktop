#include "ui/widgets/AnimatedStackedWidget.h"

#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>

namespace Aurora {

AnimatedStackedWidget::AnimatedStackedWidget(QWidget *parent)
    : QStackedWidget(parent)
{
}

void AnimatedStackedWidget::setCurrentIndexAnimated(int index)
{
    if (index < 0 || index >= count() || index == currentIndex() || m_animating)
        return;

    m_animating = true;
    auto *oldPage = currentWidget();
    auto *fadeOutEffect = new QGraphicsOpacityEffect(oldPage);
    oldPage->setGraphicsEffect(fadeOutEffect);
    auto *fadeOut = new QPropertyAnimation(fadeOutEffect, "opacity", oldPage);
    fadeOut->setDuration(120);
    fadeOut->setStartValue(1.0);
    fadeOut->setEndValue(0.0);

    connect(fadeOut, &QPropertyAnimation::finished, this, [this, index, oldPage] {
        oldPage->setGraphicsEffect(nullptr);
        setCurrentIndex(index);

        auto *newPage = currentWidget();
        auto *fadeInEffect = new QGraphicsOpacityEffect(newPage);
        newPage->setGraphicsEffect(fadeInEffect);
        auto *fadeIn = new QPropertyAnimation(fadeInEffect, "opacity", newPage);
        fadeIn->setDuration(180);
        fadeIn->setStartValue(0.0);
        fadeIn->setEndValue(1.0);
        fadeIn->setEasingCurve(QEasingCurve::OutCubic);
        connect(fadeIn, &QPropertyAnimation::finished, this, [this, newPage] {
            newPage->setGraphicsEffect(nullptr);
            m_animating = false;
        });
        fadeIn->start(QAbstractAnimation::DeleteWhenStopped);
    });
    fadeOut->start(QAbstractAnimation::DeleteWhenStopped);
}

} // namespace Aurora
