#include "ui/widgets/Toast.h"

#include "ui/IconUtils.h"

#include <QEvent>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QPropertyAnimation>
#include <QTimer>

namespace Aurora {

void Toast::showMessage(QWidget *window, const QString &message, const QString &iconResource)
{
    if (!window)
        return;
    window = window->window();
    auto *toast = window->findChild<Toast *>(QString(), Qt::FindDirectChildrenOnly);
    if (!toast)
        toast = new Toast(window);
    toast->setMessage(message, iconResource);
}

Toast::Toast(QWidget *window)
    : QFrame(window)
    , m_icon(new QLabel(this))
    , m_text(new QLabel(this))
    , m_timer(new QTimer(this))
{
    setObjectName(QStringLiteral("toast"));
    setAttribute(Qt::WA_StyledBackground, true);
    setCursor(Qt::PointingHandCursor);
    setToolTip(tr("Click to dismiss"));

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(14, 10, 16, 10);
    layout->setSpacing(10);
    m_icon->setFixedSize(18, 18);
    m_text->setWordWrap(true);
    m_text->setMaximumWidth(460);
    layout->addWidget(m_icon, 0, Qt::AlignTop);
    layout->addWidget(m_text, 1);

    auto *opacity = new QGraphicsOpacityEffect(this);
    setGraphicsEffect(opacity);
    m_fade = new QPropertyAnimation(opacity, "opacity", this);
    m_fade->setDuration(220);
    connect(m_fade, &QPropertyAnimation::finished, this, [this] {
        if (m_fade->endValue().toReal() == 0.0)
            hide();
    });

    m_timer->setSingleShot(true);
    m_timer->setInterval(6000);
    connect(m_timer, &QTimer::timeout, this, &Toast::fadeOut);
    window->installEventFilter(this);
}

void Toast::setMessage(const QString &message, const QString &iconResource)
{
    m_text->setText(message);
    // Wrap only messages too long for one line (measured with the styled font).
    m_text->ensurePolished();
    m_text->setMinimumWidth(qMin(m_text->fontMetrics().horizontalAdvance(message) + 2,
                                 m_text->maximumWidth()));
    m_icon->setPixmap(renderSvgIcon(iconResource, QColor(QStringLiteral("#E5484D")),
                                    QSize(18, 18), devicePixelRatioF()));
    adjustSize();
    reposition();
    raise();
    show();
    m_fade->stop();
    m_fade->setStartValue(m_fade->targetObject()->property("opacity"));
    m_fade->setEndValue(1.0);
    m_fade->start();
    m_timer->start();
}

void Toast::reposition()
{
    const QWidget *window = parentWidget();
    move((window->width() - width()) / 2, window->height() - height() - 32);
}

void Toast::fadeOut()
{
    m_timer->stop();
    m_fade->stop();
    m_fade->setStartValue(m_fade->targetObject()->property("opacity"));
    m_fade->setEndValue(0.0);
    m_fade->start();
}

bool Toast::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == parentWidget() && event->type() == QEvent::Resize)
        reposition();
    return QFrame::eventFilter(watched, event);
}

void Toast::mousePressEvent(QMouseEvent *)
{
    fadeOut();
}

} // namespace Aurora
