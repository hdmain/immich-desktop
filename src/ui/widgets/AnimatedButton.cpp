#include "ui/widgets/AnimatedButton.h"

#include "core/ThemeManager.h"
#include "ui/IconUtils.h"

#include <QEnterEvent>
#include <QFont>
#include <QMouseEvent>
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
    setToolTip(text);
    setAccessibleName(text);
    setStyleSheet(QStringLiteral("background: transparent; border: none; padding: 0;"));
    connect(m_themeManager, &ThemeManager::appearanceChanged, this,
            [this] {
                updateGeometry();
                update();
            });
}

qreal AnimatedButton::hoverProgress() const { return m_hoverProgress; }

void AnimatedButton::setHoverProgress(qreal progress)
{
    m_hoverProgress = progress;
    update();
}

QSize AnimatedButton::sizeHint() const
{
    if (m_themeManager->minimalistUi() || m_iconOnly || text().isEmpty())
        return QSize(44, 44);
    return QSize(46 + fontMetrics().horizontalAdvance(text()) + 14, 44);
}

void AnimatedButton::setIconOnly(bool iconOnly)
{
    m_iconOnly = iconOnly;
    updateGeometry();
    update();
}

void AnimatedButton::setIconResource(const QString &iconResource)
{
    m_iconResource = iconResource;
    update();
}

void AnimatedButton::setExpandable(bool expandable)
{
    m_expandable = expandable;
    update();
}

void AnimatedButton::setExpanded(bool expanded)
{
    m_expanded = expanded;
    update();
}

bool AnimatedButton::isExpanded() const
{
    return m_expanded;
}

bool AnimatedButton::showsChevron() const
{
    return m_expandable && !m_iconOnly && !m_themeManager->minimalistUi() && !text().isEmpty();
}

QRect AnimatedButton::chevronRect() const
{
    return QRect(width() - 34, 0, 34, height());
}

void AnimatedButton::mousePressEvent(QMouseEvent *event)
{
    m_chevronPressed = showsChevron() && event->button() == Qt::LeftButton &&
                       chevronRect().contains(event->position().toPoint());
    if (m_chevronPressed) {
        event->accept();
        return;
    }
    QPushButton::mousePressEvent(event);
}

void AnimatedButton::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_chevronPressed) {
        m_chevronPressed = false;
        if (chevronRect().contains(event->position().toPoint())) {
            setExpanded(!m_expanded);
            emit expandToggled(m_expanded);
        }
        event->accept();
        return;
    }
    QPushButton::mouseReleaseEvent(event);
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
    if (!isEnabled())
        painter.setOpacity(0.4);
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
    const bool minimal = m_themeManager->minimalistUi() || m_iconOnly || text().isEmpty();
    painter.drawPixmap(QRect(minimal ? (width() - 19) / 2 : 15, (height() - 19) / 2, 19, 19), icon);
    if (!minimal)
        painter.drawText(rect().adjusted(46, 0, showsChevron() ? -34 : -10, 0),
                         Qt::AlignVCenter | Qt::AlignLeft, text());
    if (showsChevron()) {
        const QPixmap chevron = renderSvgIcon(
            m_expanded ? QStringLiteral(":/icons/chevron-down.svg")
                       : QStringLiteral(":/icons/chevron-right.svg"),
            iconColor, QSize(14, 14), devicePixelRatioF());
        const QRect area = chevronRect();
        painter.drawPixmap(QRect(area.center().x() - 7, area.center().y() - 7, 14, 14), chevron);
    }
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
