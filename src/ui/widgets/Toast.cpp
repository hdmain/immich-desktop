#include "ui/widgets/Toast.h"

#include "ui/IconUtils.h"

#include <QEvent>
#include <QGraphicsOpacityEffect>
#include <QHash>
#include <QHBoxLayout>
#include <QLabel>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QStyle>
#include <QTimer>

namespace Aurora {

void Toast::showMessage(QWidget *window, const QString &message, const QString &iconResource)
{
    if (!window)
        return;
    Toast *toast = find(window, Kind::Message);
    if (!toast)
        toast = new Toast(window->window(), Kind::Message);
    toast->show(message, iconResource, Style::Error);
}

void Toast::showInfo(QWidget *window, const QString &message)
{
    if (!window)
        return;
    Toast *toast = find(window, Kind::Message);
    if (!toast)
        toast = new Toast(window->window(), Kind::Message);
    toast->show(message, QStringLiteral(":/icons/circle-check.svg"), Style::Info);
}

void Toast::showProgress(QWidget *window, const QString &message, const QString &actionText,
                         std::function<void()> action)
{
    if (!window)
        return;
    Toast *toast = find(window, Kind::Progress);
    if (!toast)
        toast = new Toast(window->window(), Kind::Progress);
    toast->m_onAction = std::move(action);
    toast->m_action->setText(actionText);
    toast->m_action->setVisible(!actionText.isEmpty());
    toast->show(message, QStringLiteral(":/icons/download.svg"), Style::Progress);
}

void Toast::setProgressText(QWidget *window, const QString &message)
{
    if (Toast *toast = window ? find(window, Kind::Progress) : nullptr) {
        toast->m_text->setText(message);
        toast->adjustSize();
        toast->reposition();
    }
}

void Toast::finishProgress(QWidget *window)
{
    if (Toast *toast = window ? find(window, Kind::Progress) : nullptr)
        toast->fadeOut();
}

Toast *Toast::find(QWidget *window, Kind kind)
{
    for (Toast *toast : window->window()->findChildren<Toast *>(QString(), Qt::FindDirectChildrenOnly)) {
        if (toast->m_kind == kind)
            return toast;
    }
    return nullptr;
}

Toast::Toast(QWidget *window, Kind kind)
    : QFrame(window)
    , m_kind(kind)
    , m_icon(new QLabel(this))
    , m_text(new QLabel(this))
    , m_action(new QPushButton(this))
    , m_timer(new QTimer(this))
{
    setObjectName(QStringLiteral("toast"));
    setAttribute(Qt::WA_StyledBackground, true);
    if (kind == Kind::Message) {
        setCursor(Qt::PointingHandCursor);
        setToolTip(tr("Click to dismiss"));
    }

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(14, 10, 16, 10);
    layout->setSpacing(10);
    m_icon->setFixedSize(18, 18);
    m_text->setWordWrap(true);
    m_text->setMaximumWidth(460);
    m_action->setCursor(Qt::PointingHandCursor);
    m_action->hide();
    layout->addWidget(m_icon, 0, Qt::AlignTop);
    layout->addWidget(m_text, 1);
    layout->addWidget(m_action, 0, Qt::AlignVCenter);
    connect(m_action, &QPushButton::clicked, this, [this] {
        if (m_onAction)
            m_onAction();
    });

    auto *opacity = new QGraphicsOpacityEffect(this);
    setGraphicsEffect(opacity);
    m_fade = new QPropertyAnimation(opacity, "opacity", this);
    m_fade->setDuration(220);
    connect(m_fade, &QPropertyAnimation::finished, this, [this] {
        if (m_fade->endValue().toReal() == 0.0) {
            hide();
            // A message toast above this one can move back down.
            for (Toast *other : parentWidget()->findChildren<Toast *>(QString(), Qt::FindDirectChildrenOnly))
                other->reposition();
        }
    });

    m_timer->setSingleShot(true);
    m_timer->setInterval(6000);
    connect(m_timer, &QTimer::timeout, this, &Toast::fadeOut);
    window->installEventFilter(this);
}

void Toast::show(const QString &message, const QString &iconResource, Style style)
{
    static const QHash<Style, QString> styleNames = {{Style::Error, QStringLiteral("error")},
                                                     {Style::Info, QStringLiteral("info")},
                                                     {Style::Progress, QStringLiteral("progress")}};
    setProperty("toastStyle", styleNames.value(style));
    this->style()->unpolish(this);
    this->style()->polish(this);

    m_text->setText(message);
    // Wrap only messages too long for one line (measured with the styled font).
    m_text->ensurePolished();
    m_text->setMinimumWidth(qMin(m_text->fontMetrics().horizontalAdvance(message) + 2,
                                 m_text->maximumWidth()));
    const QColor iconColor = style == Style::Error ? QColor(QStringLiteral("#E5484D"))
                             : style == Style::Info ? QColor(QStringLiteral("#46A758"))
                                                    : palette().color(QPalette::PlaceholderText);
    m_icon->setPixmap(renderSvgIcon(iconResource, iconColor, QSize(18, 18), devicePixelRatioF()));
    adjustSize();
    reposition();
    raise();
    QFrame::show();
    m_fade->stop();
    m_fade->setStartValue(m_fade->targetObject()->property("opacity"));
    m_fade->setEndValue(1.0);
    m_fade->start();
    // Progress stays until finished; messages fade on their own.
    if (m_kind == Kind::Message)
        m_timer->start();
    for (Toast *other : parentWidget()->findChildren<Toast *>(QString(), Qt::FindDirectChildrenOnly))
        other->reposition();
}

void Toast::reposition()
{
    const QWidget *window = parentWidget();
    int bottom = window->height() - 32;
    // A message sits above a progress toast rather than on top of it.
    if (m_kind == Kind::Message) {
        if (const Toast *progress = find(const_cast<QWidget *>(window), Kind::Progress);
            progress && progress->isVisible())
            bottom = progress->y() - 10;
    }
    move((window->width() - width()) / 2, bottom - height());
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
    if (m_kind == Kind::Message)
        fadeOut();
}

} // namespace Aurora
