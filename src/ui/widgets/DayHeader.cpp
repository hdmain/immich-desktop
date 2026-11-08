#include "ui/widgets/DayHeader.h"

#include "ui/widgets/MediaTile.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>

namespace Aurora {

DayHeader::DayHeader(const QString &text, QWidget *parent)
    : QLabel(text, parent)
{
    setMouseTracking(true);
}

void DayHeader::setSelecting(bool selecting)
{
    if (m_selecting == selecting)
        return;
    m_selecting = selecting;
    update();
}

void DayHeader::setChecked(bool checked)
{
    if (m_checked == checked)
        return;
    m_checked = checked;
    update();
}

QRect DayHeader::circleRect() const
{
    constexpr int size = 18;
    const QRect content = contentsRect();
    const int x = content.left() + fontMetrics().horizontalAdvance(text()) + 10;
    return QRect(x, content.center().y() - size / 2 + 1, size, size);
}

bool DayHeader::overCircle(const QPoint &pos) const
{
    return circleRect().adjusted(-6, -6, 6, 6).contains(pos);
}

void DayHeader::paintEvent(QPaintEvent *event)
{
    QLabel::paintEvent(event);
    if (!m_checked && !m_selecting && !m_hovered)
        return;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    const QRectF circle = QRectF(circleRect()).adjusted(1, 1, -1, -1);
    const QColor accent = MediaTile::selectionColor();
    if (m_checked) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(accent);
        painter.drawEllipse(circle);
        QPainterPath check;
        check.moveTo(circle.left() + circle.width() * 0.28, circle.center().y());
        check.lineTo(circle.left() + circle.width() * 0.44, circle.top() + circle.height() * 0.66);
        check.lineTo(circle.left() + circle.width() * 0.72, circle.top() + circle.height() * 0.34);
        painter.setPen(QPen(Qt::white, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(check);
    } else {
        QColor ring = palette().color(foregroundRole());
        ring.setAlphaF(m_hovered ? 0.7 : 0.4);
        painter.setPen(QPen(ring, 1.6));
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(circle);
    }
}

void DayHeader::enterEvent(QEnterEvent *event)
{
    m_hovered = true;
    update();
    QLabel::enterEvent(event);
}

void DayHeader::leaveEvent(QEvent *event)
{
    m_hovered = false;
    unsetCursor();
    update();
    QLabel::leaveEvent(event);
}

void DayHeader::mouseMoveEvent(QMouseEvent *event)
{
    if (overCircle(event->position().toPoint()))
        setCursor(Qt::PointingHandCursor);
    else
        unsetCursor();
    QLabel::mouseMoveEvent(event);
}

void DayHeader::mousePressEvent(QMouseEvent *event)
{
    m_pressOnCircle = event->button() == Qt::LeftButton && overCircle(event->position().toPoint());
    QLabel::mousePressEvent(event);
}

void DayHeader::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_pressOnCircle && event->button() == Qt::LeftButton &&
        overCircle(event->position().toPoint()))
        emit toggled();
    m_pressOnCircle = false;
    QLabel::mouseReleaseEvent(event);
}

} // namespace Aurora
