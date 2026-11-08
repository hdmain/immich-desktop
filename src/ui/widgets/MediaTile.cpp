#include "ui/widgets/MediaTile.h"

#include "ui/IconUtils.h"
#include "ui/widgets/VideoHoverPreview.h"

#include <QContextMenuEvent>
#include <QEasingCurve>
#include <QEnterEvent>
#include <QKeyEvent>
#include <QKeySequence>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QPropertyAnimation>
#include <QResizeEvent>

namespace Aurora {
namespace {

QColor s_selectionColor(QStringLiteral("#818CF8"));

} // namespace

MediaTile::MediaTile(const ImmichAsset &asset, QWidget *parent)
    : QWidget(parent)
    , m_asset(asset)
    , m_resolvedAspectRatio(asset.aspectRatio > 0.01 ? asset.aspectRatio : 1.0)
{
    setObjectName(QStringLiteral("mediaTile"));
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setAttribute(Qt::WA_OpaquePaintEvent);
    setMouseTracking(true);
    setToolTip(asset.fileName);
}

const ImmichAsset &MediaTile::asset() const
{
    return m_asset;
}

qreal MediaTile::aspectRatio() const
{
    return m_resolvedAspectRatio;
}

void MediaTile::setTrashed(bool trashed)
{
    m_trashed = trashed;
}

bool MediaTile::hasThumbnail() const
{
    return !m_thumbnail.isNull();
}

int MediaTile::thumbnailDimension() const
{
    return qMax(m_thumbnail.width(), m_thumbnail.height());
}

bool MediaTile::hasThumbnailError() const
{
    return m_hasError;
}

void MediaTile::setHoverPreview(VideoHoverPreview *preview)
{
    m_hoverPreview = preview;
}

void MediaTile::endHoverPreview()
{
    if (!m_hoverPreviewActive)
        return;
    m_hoverPreviewActive = false;
    update();
}

void MediaTile::setThumbnail(const QPixmap &thumbnail)
{
    // A late small thumbnail must not replace a sharper image loaded for zoom.
    if (qMax(thumbnail.width(), thumbnail.height()) < thumbnailDimension())
        return;
    m_thumbnail = thumbnail;
    if (thumbnail.height() > 0)
        m_resolvedAspectRatio = qreal(thumbnail.width()) / qreal(thumbnail.height());
    m_hasError = false;
    m_error.clear();
    update();
}

void MediaTile::clearThumbnailError()
{
    if (!m_hasError)
        return;
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

void MediaTile::clearThumbnail()
{
    if (m_thumbnail.isNull())
        return;
    m_thumbnail = QPixmap();
    update();
}

void MediaTile::updateAsset(const ImmichAsset &asset)
{
    const bool badgeChanged = m_asset.localSyncState != asset.localSyncState ||
                              m_asset.favorite != asset.favorite;
    m_asset = asset;
    setToolTip(asset.fileName);
    if (badgeChanged)
        update();
}

void MediaTile::setSelected(bool selected)
{
    if (m_selected == selected)
        return;
    m_selected = selected;
    if (!m_selectionAnimation) {
        m_selectionAnimation = new QPropertyAnimation(this, "selectionProgress", this);
        m_selectionAnimation->setDuration(150);
        m_selectionAnimation->setEasingCurve(QEasingCurve::OutCubic);
    }
    m_selectionAnimation->stop();
    m_selectionAnimation->setStartValue(m_selectionProgress);
    m_selectionAnimation->setEndValue(selected ? 1.0 : 0.0);
    m_selectionAnimation->start();
}

void MediaTile::setSelectionProgress(qreal progress)
{
    progress = qBound(0.0, progress, 1.0);
    if (qFuzzyCompare(m_selectionProgress, progress))
        return;
    m_selectionProgress = progress;
    update();
}

void MediaTile::setSelecting(bool selecting)
{
    if (m_selecting == selecting)
        return;
    m_selecting = selecting;
    // The check circle sits where a video's hover preview would cover it.
    if (selecting)
        endHoverPreview();
    update();
}

void MediaTile::setSelectionColor(const QColor &color)
{
    s_selectionColor = color;
}

QColor MediaTile::selectionColor()
{
    return s_selectionColor;
}

QRect MediaTile::checkRect() const
{
    return QRect(8, 8, 24, 24);
}

QRect MediaTile::heartRect() const
{
    return QRect(6, height() - 28, 22, 22);
}

void MediaTile::setTileSize(const QSize &size)
{
    setFixedSize(size);
    if (m_hoverPreview)
        m_hoverPreview->updateTileGeometry(this);
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
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.fillRect(rect(), QColor(20, 20, 20));

    if (!m_thumbnail.isNull()) {
        // Crop+scale in the draw call so we don't keep a second full pixmap
        // per tile (scaled cache roughly doubled RAM for the timeline).
        const QSizeF tileSize = size();
        const QSizeF srcSize = m_thumbnail.size();
        const qreal scale =
            qMax(tileSize.width() / srcSize.width(), tileSize.height() / srcSize.height());
        const qreal srcW = tileSize.width() / scale;
        const qreal srcH = tileSize.height() / scale;
        const QRectF source((srcSize.width() - srcW) * 0.5, (srcSize.height() - srcH) * 0.5,
                            srcW, srcH);
        painter.drawPixmap(QRectF(rect()), m_thumbnail, source);
    } else {
        painter.setPen(QColor(150, 150, 150));
        painter.drawText(rect().adjusted(8, 8, -8, -8),
                         Qt::AlignCenter | Qt::TextWordWrap,
                         m_hasError ? m_error : tr("…"));
    }

    if (m_asset.isVideo() && !m_hoverPreviewActive) {
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

    // Favorites always show a filled heart; on hover an outline one offers to add it.
    if ((m_asset.favorite && !m_trashed) || (m_hovered && favoritable())) {
        const QPixmap heart = renderSvgIcon(m_asset.favorite ? QStringLiteral(":/icons/heart-filled.svg")
                                                             : QStringLiteral(":/icons/heart.svg"),
                                            Qt::white, QSize(16, 16), devicePixelRatioF());
        if (!heart.isNull()) {
            const QRect bg = heartRect();
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(0, 0, 0, m_hovered ? 150 : 110));
            painter.drawEllipse(bg);
            painter.drawPixmap(bg.x() + 3, bg.y() + 3, heart);
        }
    }

    if (m_asset.localSyncState != LocalSyncState::None) {
        QString iconPath = QStringLiteral(":/icons/cloud-off.svg");
        if (m_asset.localSyncState == LocalSyncState::Saving)
            iconPath = QStringLiteral(":/icons/cloud-upload.svg");
        else if (m_asset.localSyncState == LocalSyncState::Error)
            iconPath = QStringLiteral(":/icons/cloud-alert.svg");
        else if (m_asset.localSyncState == LocalSyncState::Unsaved)
            iconPath = QStringLiteral(":/icons/cloud-off.svg");

        const QPixmap badge =
            renderSvgIcon(iconPath, Qt::white, QSize(16, 16));
        if (!badge.isNull()) {
            constexpr int pad = 6;
            const QRect bg(pad, height() - pad - 22, 22, 22);
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(0, 0, 0, 150));
            painter.drawRoundedRect(bg, 5, 5);
            painter.drawPixmap(bg.x() + 3, bg.y() + 3, badge);
        }
    }

    if (hasFocus()) {
        painter.setPen(QPen(QColor(166, 133, 226), 2));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(rect().adjusted(1, 1, -1, -1));
    }

    if (m_selectionProgress > 0.01) {
        QColor border = s_selectionColor;
        border.setAlphaF(m_selectionProgress);
        painter.setPen(QPen(border, 2.0 + 2.0 * m_selectionProgress));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(rect().adjusted(2, 2, -2, -2));
    }
    // The check circle shows on hover, and on every tile while selecting.
    if (selectable() && (m_selectionProgress > 0.01 || m_selecting || m_hovered)) {
        const qreal pop = 0.82 + 0.18 * m_selectionProgress;
        QRectF circle = QRectF(checkRect()).adjusted(1, 1, -1, -1);
        if (m_selectionProgress > 0.01) {
            const QPointF c = circle.center();
            circle.setSize(circle.size() * pop);
            circle.moveCenter(c);
        }
        painter.setOpacity(m_selectionProgress > 0.01
                               ? (0.35 + 0.65 * m_selectionProgress)
                               : 1.0);
        painter.setPen(QPen(Qt::white, 2));
        QColor fill = s_selectionColor;
        if (m_selectionProgress <= 0.01)
            fill = QColor(0, 0, 0, 90);
        else
            fill.setAlphaF(m_selectionProgress);
        painter.setBrush(fill);
        painter.drawEllipse(circle);
        if (m_selectionProgress > 0.2) {
            QPainterPath check;
            check.moveTo(circle.left() + circle.width() * 0.28, circle.center().y());
            check.lineTo(circle.left() + circle.width() * 0.44, circle.top() + circle.height() * 0.66);
            check.lineTo(circle.left() + circle.width() * 0.72, circle.top() + circle.height() * 0.34);
            QColor tick = Qt::white;
            tick.setAlphaF(qMin(1.0, (m_selectionProgress - 0.2) / 0.8));
            painter.setPen(QPen(tick, 2.2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            painter.setBrush(Qt::NoBrush);
            painter.drawPath(check);
        }
        painter.setOpacity(1.0);
    }
}

void MediaTile::enterEvent(QEnterEvent *event)
{
    emit highlighted(m_asset);
    m_hovered = true;
    update();
    if (m_hoverPreview && m_asset.isVideo() && !m_asset.isLocalPending() && !m_selecting) {
        m_hoverPreviewActive = true;
        m_hoverPreview->showForTile(this);
        update();
    }
    QWidget::enterEvent(event);
}

void MediaTile::leaveEvent(QEvent *event)
{
    m_hovered = false;
    update();
    if (m_hoverPreview && m_hoverPreviewActive) {
        m_hoverPreviewActive = false;
        m_hoverPreview->hideForTile(this);
        update();
    }
    QWidget::leaveEvent(event);
}

void MediaTile::resizeEvent(QResizeEvent *event)
{
    if (m_hoverPreview)
        m_hoverPreview->updateTileGeometry(this);
    QWidget::resizeEvent(event);
}

void MediaTile::keyPressEvent(QKeyEvent *event)
{
    if (event->matches(QKeySequence::Copy)) {
        emit copyRequested(m_asset);
        event->accept();
        return;
    }
    switch (event->key()) {
    case Qt::Key_Left:
    case Qt::Key_Right:
    case Qt::Key_Up:
    case Qt::Key_Down:
    case Qt::Key_Home:
    case Qt::Key_End:
        emit navigationRequested(event->key(), event->modifiers());
        event->accept();
        return;
    case Qt::Key_Space:
        // Ctrl+Space toggles, as does Space while selecting.
        if (selectable() && (m_selecting || (event->modifiers() & Qt::ControlModifier))) {
            emit selectionClicked(m_asset, Qt::ControlModifier);
            event->accept();
            return;
        }
        [[fallthrough]];
    case Qt::Key_Return:
    case Qt::Key_Enter:
        emit activated(m_asset);
        event->accept();
        return;
    default:
        break;
    }
    QWidget::keyPressEvent(event);
}

void MediaTile::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_leftPressed = true;
        m_pressBecameDrag = false;
        m_pressPos = event->position().toPoint();
        m_pressOnCheck = selectable() && checkRect().adjusted(-4, -4, 4, 4).contains(m_pressPos);
        m_pressOnHeart = !m_pressOnCheck && favoritable() &&
                         heartRect().adjusted(-4, -4, 4, 4).contains(m_pressPos);
        setFocus(Qt::MouseFocusReason);
        emit highlighted(m_asset);
    }
    QWidget::mousePressEvent(event);
}

void MediaTile::mouseMoveEvent(QMouseEvent *event)
{
    if (m_leftPressed && !m_pressBecameDrag) {
        constexpr int kDragThresholdPx = 8;
        if ((event->position().toPoint() - m_pressPos).manhattanLength() >= kDragThresholdPx)
            m_pressBecameDrag = true;
    }
    QWidget::mouseMoveEvent(event);
}

void MediaTile::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        // Ignore release after a drag/scroll so timeline scrolling does not open media.
        if (m_leftPressed && !m_pressBecameDrag &&
            rect().contains(event->position().toPoint())) {
            Qt::KeyboardModifiers modifiers = event->modifiers();
            if (m_pressOnHeart) {
                emit favoriteRequested(m_asset, !m_asset.favorite);
                m_leftPressed = false;
                QWidget::mouseReleaseEvent(event);
                return;
            }
            if (m_pressOnCheck)
                modifiers |= Qt::ControlModifier; // The circle toggles, like Ctrl-click.
            if (selectable() &&
                (m_pressOnCheck || m_selecting ||
                 (modifiers & (Qt::ControlModifier | Qt::ShiftModifier))))
                emit selectionClicked(m_asset, modifiers);
            else
                emit activated(m_asset);
        }
        m_leftPressed = false;
        m_pressBecameDrag = false;
    }
    QWidget::mouseReleaseEvent(event);
}

void MediaTile::contextMenuEvent(QContextMenuEvent *event)
{
    if (m_selected && m_selecting) {
        emit selectionMenuRequested(event->globalPos());
        return;
    }
    QMenu menu(this);
    menu.addAction(tr("Open"), this, [this] { emit activated(m_asset); });
    if (m_trashed) {
        menu.addAction(tr("Info"), this, [this] { emit infoRequested(m_asset); });
        menu.addAction(tr("Download"), this, [this] { emit downloadRequested(m_asset); });
        menu.addSeparator();
        menu.addAction(tr("Restore"), this, [this] { emit restoreRequested(m_asset); });
        menu.addAction(tr("Delete permanently"), this,
                       [this] { emit deleteRequested(m_asset); });
    } else if (!m_asset.isLocalPending()) {
        menu.addAction(tr("Info"), this, [this] { emit infoRequested(m_asset); });
        if (!m_asset.isVideo())
            menu.addAction(tr("Copy"), this, [this] { emit copyRequested(m_asset); });
        menu.addAction(tr("Download"), this, [this] { emit downloadRequested(m_asset); });
        menu.addAction(tr("Share link…"), this, [this] { emit shareRequested(m_asset); });
        menu.addAction(m_asset.favorite ? tr("Remove from favorites") : tr("Add to favorites"),
                       this, [this] { emit favoriteRequested(m_asset, !m_asset.favorite); });
        menu.addSeparator();
        menu.addAction(tr("Move to trash"), this, [this] { emit trashRequested(m_asset); });
        menu.addAction(tr("Delete permanently"), this,
                       [this] { emit deleteRequested(m_asset); });
    }
    menu.exec(event->globalPos());
}

} // namespace Aurora
