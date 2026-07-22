#pragma once

#include <QPixmap>
#include <QPointF>
#include <QWidget>

class QMouseEvent;
class QPaintEvent;
class QResizeEvent;
class QWheelEvent;

namespace Aurora {

class ZoomPanWidget final : public QWidget {
    Q_OBJECT

public:
    explicit ZoomPanWidget(QWidget *parent = nullptr);

    void setPixmap(const QPixmap &pixmap);
    void setHostedWidget(QWidget *widget);
    void setPlaceholderText(const QString &text);
    void resetTransform();

signals:
    void transformChanged();

protected:
    void paintEvent(QPaintEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    QSize contentSize() const;
    qreal effectiveScale() const;
    void clampOffset();
    void updateHostedGeometry();
    void zoomAt(const QPointF &viewportPoint, qreal factor);

    QPixmap m_pixmap;
    QWidget *m_hostedWidget = nullptr;
    QString m_placeholder;
    qreal m_fitScale = 1.0;
    qreal m_userScale = 1.0;
    QPointF m_offset;
    bool m_dragging = false;
    QPoint m_lastDragPos;
};

} // namespace Aurora
