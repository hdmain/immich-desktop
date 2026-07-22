#include "ui/widgets/ZoomPanWidget.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QWheelEvent>
#include <QtMath>

namespace Aurora {

namespace {
constexpr qreal kMinUserScale = 1.0;
constexpr qreal kMaxUserScale = 8.0;
constexpr qreal kWheelFactor = 1.12;
} // namespace

ZoomPanWidget::ZoomPanWidget(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_OpaquePaintEvent, false);
    setMouseTracking(true);
    setFocusPolicy(Qt::WheelFocus);
}

void ZoomPanWidget::setPixmap(const QPixmap &pixmap)
{
    if (m_hostedWidget) {
        m_hostedWidget->hide();
        m_hostedWidget = nullptr;
    }
    m_pixmap = pixmap;
    resetTransform();
}

void ZoomPanWidget::setHostedWidget(QWidget *widget)
{
    m_pixmap = QPixmap();
    m_hostedWidget = widget;
    if (m_hostedWidget) {
        m_hostedWidget->setParent(this);
        m_hostedWidget->show();
    }
    resetTransform();
}

void ZoomPanWidget::setPlaceholderText(const QString &text)
{
    m_placeholder = text;
    update();
}

void ZoomPanWidget::resetTransform()
{
    m_userScale = 1.0;
    m_offset = QPointF(0, 0);

    const QSize content = contentSize();
    if (!content.isEmpty() && !size().isEmpty()) {
        m_fitScale = qMin(qreal(width()) / content.width(),
                          qreal(height()) / content.height());
    } else {
        m_fitScale = 1.0;
    }

    clampOffset();
    updateHostedGeometry();
    update();
    emit transformChanged();
}

QSize ZoomPanWidget::contentSize() const
{
    if (m_hostedWidget)
        return size().isValid() ? size() : QSize(640, 360);
    if (!m_pixmap.isNull())
        return m_pixmap.size();
    return QSize();
}

qreal ZoomPanWidget::effectiveScale() const
{
    return m_hostedWidget ? m_userScale : m_fitScale * m_userScale;
}

void ZoomPanWidget::clampOffset()
{
    const QSize content = contentSize();
    if (content.isEmpty() || size().isEmpty())
        return;

    const qreal scale = effectiveScale();
    const qreal contentWidth = content.width() * scale;
    const qreal contentHeight = content.height() * scale;

    if (contentWidth <= width())
        m_offset.setX((width() - contentWidth) / 2.0);
    else
        m_offset.setX(qBound(width() - contentWidth, m_offset.x(), 0.0));

    if (contentHeight <= height())
        m_offset.setY((height() - contentHeight) / 2.0);
    else
        m_offset.setY(qBound(height() - contentHeight, m_offset.y(), 0.0));
}

void ZoomPanWidget::updateHostedGeometry()
{
    if (!m_hostedWidget)
        return;

    const QSize base = size();
    const QSize scaled(qMax(1, qRound(base.width() * m_userScale)),
                       qMax(1, qRound(base.height() * m_userScale)));
    m_hostedWidget->setGeometry(QRect(m_offset.toPoint(), scaled));
}

void ZoomPanWidget::zoomAt(const QPointF &viewportPoint, qreal factor)
{
    const qreal oldScale = effectiveScale();
    const qreal newUserScale = qBound(kMinUserScale, m_userScale * factor, kMaxUserScale);
    if (qFuzzyCompare(newUserScale, m_userScale))
        return;

    const QPointF contentPoint((viewportPoint.x() - m_offset.x()) / oldScale,
                               (viewportPoint.y() - m_offset.y()) / oldScale);

    m_userScale = newUserScale;
    const qreal newScale = effectiveScale();
    m_offset = viewportPoint - contentPoint * newScale;
    clampOffset();
    updateHostedGeometry();
    update();
    emit transformChanged();
}

void ZoomPanWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.fillRect(rect(), QColor(12, 12, 12));

    if (m_hostedWidget)
        return;

    if (m_pixmap.isNull()) {
        if (!m_placeholder.isEmpty()) {
            painter.setPen(QColor(150, 150, 150));
            painter.drawText(rect(), Qt::AlignCenter, m_placeholder);
        }
        return;
    }

    const qreal scale = effectiveScale();
    const QSizeF drawSize(m_pixmap.width() * scale, m_pixmap.height() * scale);
    const QRectF target(m_offset.x(), m_offset.y(), drawSize.width(), drawSize.height());

    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.drawPixmap(target, m_pixmap, QRectF(m_pixmap.rect()));
    Q_UNUSED(event);
}

void ZoomPanWidget::wheelEvent(QWheelEvent *event)
{
    if (contentSize().isEmpty()) {
        event->ignore();
        return;
    }

    const int delta = event->angleDelta().y();
    if (delta == 0) {
        event->ignore();
        return;
    }

    zoomAt(event->position(), delta > 0 ? kWheelFactor : 1.0 / kWheelFactor);
    event->accept();
}

void ZoomPanWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_userScale > 1.01) {
        m_dragging = true;
        m_lastDragPos = event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void ZoomPanWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging) {
        m_offset += event->pos() - m_lastDragPos;
        m_lastDragPos = event->pos();
        clampOffset();
        updateHostedGeometry();
        update();
        event->accept();
        return;
    }

    if (m_userScale > 1.01)
        setCursor(Qt::OpenHandCursor);
    else
        unsetCursor();

    QWidget::mouseMoveEvent(event);
}

void ZoomPanWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_dragging && event->button() == Qt::LeftButton) {
        m_dragging = false;
        unsetCursor();
        event->accept();
        return;
    }
    QWidget::mouseReleaseEvent(event);
}

void ZoomPanWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        resetTransform();
        event->accept();
        return;
    }
    QWidget::mouseDoubleClickEvent(event);
}

void ZoomPanWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    if (!m_pixmap.isNull()) {
        const QSize content = m_pixmap.size();
        if (!content.isEmpty()) {
            m_fitScale = qMin(qreal(width()) / content.width(),
                              qreal(height()) / content.height());
        }
    }

    clampOffset();
    updateHostedGeometry();
}

} // namespace Aurora
