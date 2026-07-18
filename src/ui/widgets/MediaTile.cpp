#include "ui/widgets/MediaTile.h"

#include "ui/IconUtils.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>

namespace Aurora {

MediaTile::MediaTile(const ImmichAsset &asset, QWidget *parent)
    : QWidget(parent)
    , m_asset(asset)
{
    setObjectName(QStringLiteral("mediaTile"));
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setAttribute(Qt::WA_OpaquePaintEvent);
    setToolTip(asset.fileName);
}

const ImmichAsset &MediaTile::asset() const
{
    return m_asset;
}

qreal MediaTile::aspectRatio() const
{
    if (!m_thumbnail.isNull() && m_thumbnail.height() > 0)
        return qreal(m_thumbnail.width()) / qreal(m_thumbnail.height());
    return m_asset.aspectRatio > 0.01 ? m_asset.aspectRatio : 1.0;
}

void MediaTile::setThumbnail(const QPixmap &thumbnail)
{
    m_thumbnail = thumbnail;
    m_hasError = false;
    m_error.clear();
    update();
}

void MediaTile::setThumbnailError(const QString &message)
{
    m_thumbnail = QPixmap();
    m_hasError = true;
    m_error = message;
    update();
}

void MediaTile::setTileSize(const QSize &size)
{
    setFixedSize(size);
}

QString MediaTile::formatDuration(const QString &raw)
{
    QString duration = raw.section(u'.', 0, 0);
    if (duration.startsWith(QStringLiteral("00:")))
        duration.remove(0, 3);
    while (duration.startsWith(QStringLiteral("0")) && duration.size() > 4 &&
           duration.at(1) != u':')
        duration.remove(0, 1);
    return duration;
}

void MediaTile::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.fillRect(rect(), QColor(20, 20, 20));

    if (!m_thumbnail.isNull()) {
        const QPixmap scaled = m_thumbnail.scaled(
            size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        const int x = (scaled.width() - width()) / 2;
        const int y = (scaled.height() - height()) / 2;
        painter.drawPixmap(0, 0, scaled, x, y, width(), height());
    } else {
        painter.setPen(QColor(150, 150, 150));
        painter.drawText(rect().adjusted(8, 8, -8, -8),
                         Qt::AlignCenter | Qt::TextWordWrap,
                         m_hasError ? m_error : tr("…"));
    }

    if (m_asset.isVideo()) {
        const QString duration = formatDuration(m_asset.duration);
        const QPixmap playIcon =
            renderSvgIcon(QStringLiteral(":/icons/play.svg"), Qt::white, QSize(18, 18));

        int right = width() - 8;
        if (!playIcon.isNull()) {
            const int iconX = right - playIcon.width();
            const int iconY = 8;
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(0, 0, 0, 120));
            painter.drawEllipse(QRect(iconX - 2, iconY - 2,
                                      playIcon.width() + 4, playIcon.height() + 4));
            painter.drawPixmap(iconX, iconY, playIcon);
            right = iconX - 8;
        }

        if (!duration.isEmpty()) {
            const QFontMetrics metrics(painter.font());
            const int textWidth = metrics.horizontalAdvance(duration);
            const QRect textRect(right - textWidth - 10, 8, textWidth + 10, 20);
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(0, 0, 0, 140));
            painter.drawRoundedRect(textRect, 4, 4);
            painter.setPen(Qt::white);
            painter.drawText(textRect, Qt::AlignCenter, duration);
        }
    }

    if (hasFocus()) {
        painter.setPen(QPen(QColor(166, 133, 226), 2));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(rect().adjusted(1, 1, -1, -1));
    }
}

void MediaTile::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter ||
        event->key() == Qt::Key_Space) {
        emit activated(m_asset);
        event->accept();
        return;
    }
    QWidget::keyPressEvent(event);
}

void MediaTile::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && rect().contains(event->position().toPoint()))
        emit activated(m_asset);
    QWidget::mouseReleaseEvent(event);
}

} // namespace Aurora
