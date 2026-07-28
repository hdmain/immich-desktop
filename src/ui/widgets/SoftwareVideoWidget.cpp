#include "ui/widgets/SoftwareVideoWidget.h"

#include <QPainter>
#include <QPaintEvent>

namespace Aurora {

SoftwareVideoWidget::SoftwareVideoWidget(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_OpaquePaintEvent);
    setStyleSheet(QStringLiteral("background: black;"));
}

void SoftwareVideoWidget::setFrame(QImage frame)
{
    m_frame = std::move(frame);
    update();
}

void SoftwareVideoWidget::clearFrame()
{
    if (m_frame.isNull())
        return;
    m_frame = QImage();
    update();
}

void SoftwareVideoWidget::setScaleMode(ScaleMode mode)
{
    if (m_scaleMode == mode)
        return;
    m_scaleMode = mode;
    update();
}

void SoftwareVideoWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.fillRect(rect(), Qt::black);
    if (m_frame.isNull())
        return;

    if (m_scaleMode == ScaleMode::Cover) {
        const QImage scaled = m_frame.scaled(
            size(), Qt::KeepAspectRatioByExpanding, Qt::FastTransformation);
        const int x = (scaled.width() - width()) / 2;
        const int y = (scaled.height() - height()) / 2;
        painter.drawImage(0, 0, scaled, x, y, width(), height());
        return;
    }

    const QImage scaled = m_frame.scaled(
        size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    const int x = (width() - scaled.width()) / 2;
    const int y = (height() - scaled.height()) / 2;
    painter.drawImage(x, y, scaled);
}

} // namespace Aurora
