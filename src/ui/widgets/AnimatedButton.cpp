#include "ui/widgets/AnimatedButton.h"

#include "core/ThemeManager.h"
#include "ui/IconUtils.h"

#include <QEnterEvent>
#include <QFont>
#include <QPainter>
#include <QPropertyAnimation>

namespace Aurora {

AnimatedButton::AnimatedButton(const QString &text, const QString &iconResource,
                               ThemeManager *themeManager, QWidget *parent)
    : QPushButton(text, parent)
    , m_iconResource(iconResource)
    , m_themeManager(themeManager)
{
    setCheckable(true);
    setCursor(Qt::PointingHandCursor);
    setMinimumHeight(44);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setStyleSheet(QStringLiteral("background: transparent; border: none; padding: 0;"));
    connect(m_themeManager, &ThemeManager::appearanceChanged, this,
            [this] { update(); });
}

qreal AnimatedButton::hoverProgress() const { return m_hoverProgress; }

void AnimatedButton::setHoverProgress(qreal progress)
{
    m_hoverProgress = progress;
    update();
}

void AnimatedButton::enterEvent(QEnterEvent *event)
{
    animateTo(1.0);
    QPushButton::enterEvent(event);
}

void AnimatedButton::leaveEvent(QEvent *event)
{
    animateTo(0.0);
    QPushButton::leaveEvent(event);
}

void AnimatedButton::paintEvent(QPaintEvent *)
{
    const auto &theme = m_themeManager->palette();
    QColor base = theme.button;
    QColor target = isChecked() ? theme.accent : theme.panel;
    const qreal amount = isChecked() ? 0.82 : m_hoverProgress * 0.65;

    QColor fill(
        qRound(base.red() + (target.red() - base.red()) * amount),
        qRound(base.green() + (target.green() - base.green()) * amount),
        qRound(base.blue() + (target.blue() - base.blue()) * amount));

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(fill);
    painter.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 11, 11);

    painter.setPen(isChecked() ? QColor(Qt::white) : theme.text);
    QFont labelFont = font();
    labelFont.setHintingPreference(QFont::PreferNoHinting);
    labelFont.setStyleStrategy(static_cast<QFont::StyleStrategy>(
        QFont::PreferAntialias | QFont::PreferQuality));
    labelFont.setWeight(isChecked() ? QFont::DemiBold : QFont::Medium);
    painter.setFont(labelFont);
    const QColor iconColor = isChecked() ? QColor(Qt::white) : theme.mutedText;
    const QPixmap icon = renderSvgIcon(m_iconResource, iconColor, QSize(19, 19),
                                       devicePixelRatioF());
    painter.drawPixmap(QRect(15, (height() - 19) / 2, 19, 19), icon);
    painter.drawText(rect().adjusted(46, 0, -10, 0),
                     Qt::AlignVCenter | Qt::AlignLeft, text());
}

void AnimatedButton::animateTo(qreal endValue)
{
    auto *animation = new QPropertyAnimation(this, "hoverProgress", this);
    animation->setDuration(160);
    animation->setStartValue(m_hoverProgress);
    animation->setEndValue(endValue);
    animation->setEasingCurve(QEasingCurve::OutCubic);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

} // namespace Aurora
