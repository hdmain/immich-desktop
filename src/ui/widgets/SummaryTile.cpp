#include "ui/widgets/SummaryTile.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>

namespace Aurora {

SummaryTile::SummaryTile(const QString &label, int count, QWidget *parent)
    : QWidget(parent)
    , m_label(label)
    , m_count(count)
{
    setObjectName(QStringLiteral("summaryTile"));
    setCursor(Qt::PointingHandCursor);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setAttribute(Qt::WA_OpaquePaintEvent);
}

void SummaryTile::setThumbnail(const QPixmap &thumbnail)
{
    m_thumbnail = thumbnail;
    update();
}

void SummaryTile::setTileSize(const QSize &size)
{
    setFixedSize(size);
}

void SummaryTile::setShowCount(bool show)
{
    if (m_showCount == show)
        return;
    m_showCount = show;
    update();
}

void SummaryTile::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.fillRect(rect(), QColor(20, 20, 20));

    if (!m_thumbnail.isNull()) {
        const QPixmap scaled = m_thumbnail.scaled(
            size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        const int x = (scaled.width() - width()) / 2;
        const int y = (scaled.height() - height()) / 2;
        painter.drawPixmap(0, 0, scaled, x, y, width(), height());
    }

    const int gradientHeight = qMax(36, height() / 3);
    QLinearGradient gradient(0, height() - gradientHeight, 0, height());
    gradient.setColorAt(0, QColor(0, 0, 0, 0));
    gradient.setColorAt(1, QColor(0, 0, 0, 170));
    painter.fillRect(QRect(0, height() - gradientHeight, width(), gradientHeight), gradient);

    QFont labelFont = painter.font();
    labelFont.setPointSizeF(labelFont.pointSizeF() * 1.15);
    labelFont.setBold(true);
    painter.setFont(labelFont);
    painter.setPen(Qt::white);
    const QRect labelRect(8, height() - gradientHeight + 4, width() - 16, gradientHeight - 20);
    painter.drawText(labelRect, Qt::AlignLeft | Qt::AlignTop, m_label);

    if (m_showCount && m_count > 0) {
        QFont countFont = painter.font();
        countFont.setBold(false);
        countFont.setPointSizeF(countFont.pointSizeF() * 0.85);
        painter.setFont(countFont);
        painter.setPen(QColor(230, 230, 230));
        const QRect countRect(8, height() - 20, width() - 16, 18);
        painter.drawText(countRect, Qt::AlignLeft | Qt::AlignVCenter,
                         tr("%n item(s)", nullptr, m_count));
    }

    if (hasFocus()) {
        painter.setPen(QPen(QColor(166, 133, 226), 2));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(rect().adjusted(1, 1, -1, -1));
    }
}

void SummaryTile::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && rect().contains(event->position().toPoint()))
        emit clicked();
    QWidget::mouseReleaseEvent(event);
}

} // namespace Aurora
