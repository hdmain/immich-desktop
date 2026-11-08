#include "ui/TileNavigation.h"

#include "ui/widgets/MediaTile.h"

#include <QScrollArea>

#include <limits>

namespace Aurora {

namespace {

QRect windowGeometry(const MediaTile *tile)
{
    return QRect(tile->mapTo(tile->window(), QPoint(0, 0)), tile->size());
}

} // namespace

MediaTile *neighbourTile(const QList<MediaTile *> &tiles, const MediaTile *from, int key)
{
    QList<MediaTile *> visible;
    for (MediaTile *tile : tiles) {
        if (tile && tile->isVisible())
            visible.append(tile);
    }
    const qsizetype index = visible.indexOf(const_cast<MediaTile *>(from));
    if (visible.isEmpty() || index < 0)
        return nullptr;

    switch (key) {
    case Qt::Key_Left:
        return index > 0 ? visible.at(index - 1) : nullptr;
    case Qt::Key_Right:
        return index + 1 < visible.size() ? visible.at(index + 1) : nullptr;
    case Qt::Key_Home:
        return visible.first();
    case Qt::Key_End:
        return visible.last();
    case Qt::Key_Up:
    case Qt::Key_Down:
        break;
    default:
        return nullptr;
    }

    // Rows are justified, so tiles don't line up in columns: take the closest
    // row in that direction, then the tile in it nearest horizontally.
    const bool up = key == Qt::Key_Up;
    const QRect origin = windowGeometry(from);
    int rowY = up ? std::numeric_limits<int>::min() : std::numeric_limits<int>::max();
    QList<QPair<MediaTile *, QRect>> row;
    for (MediaTile *tile : visible) {
        const QRect geometry = windowGeometry(tile);
        const bool beyond = up ? geometry.bottom() < origin.top() : geometry.top() > origin.bottom();
        if (!beyond)
            continue;
        const int y = up ? geometry.bottom() : geometry.top();
        if (up ? y > rowY : y < rowY) {
            rowY = y;
            row.clear();
        }
        if (y == rowY)
            row.append({tile, geometry});
    }
    MediaTile *best = nullptr;
    int bestDistance = std::numeric_limits<int>::max();
    for (const auto &[tile, geometry] : row) {
        const int distance = qAbs(geometry.center().x() - origin.center().x());
        if (distance < bestDistance) {
            bestDistance = distance;
            best = tile;
        }
    }
    return best;
}

void focusTile(MediaTile *tile)
{
    if (!tile)
        return;
    tile->setFocus(Qt::TabFocusReason);
    for (QWidget *parent = tile->parentWidget(); parent; parent = parent->parentWidget()) {
        if (auto *area = qobject_cast<QScrollArea *>(parent)) {
            area->ensureWidgetVisible(tile, 0, 24);
            break;
        }
    }
}

} // namespace Aurora
