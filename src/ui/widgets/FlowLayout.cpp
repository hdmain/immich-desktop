#include "ui/widgets/FlowLayout.h"

#include <QWidget>

namespace Aurora {

FlowLayout::FlowLayout(QWidget *parent, int spacing)
    : QLayout(parent)
{
    setSpacing(spacing);
    setContentsMargins(0, 0, 0, 0);
}

FlowLayout::~FlowLayout()
{
    while (QLayoutItem *item = takeAt(0))
        delete item;
}

void FlowLayout::addItem(QLayoutItem *item)
{
    m_items.append(item);
}

int FlowLayout::count() const
{
    return m_items.size();
}

QLayoutItem *FlowLayout::itemAt(int index) const
{
    return m_items.value(index);
}

QLayoutItem *FlowLayout::takeAt(int index)
{
    return index >= 0 && index < m_items.size() ? m_items.takeAt(index) : nullptr;
}

Qt::Orientations FlowLayout::expandingDirections() const
{
    return {};
}

bool FlowLayout::hasHeightForWidth() const
{
    return true;
}

int FlowLayout::heightForWidth(int width) const
{
    return doLayout(QRect(0, 0, width, 0), true);
}

QSize FlowLayout::minimumSize() const
{
    QSize size;
    for (const QLayoutItem *item : m_items)
        size = size.expandedTo(item->minimumSize());
    const QMargins margins = contentsMargins();
    return size + QSize(margins.left() + margins.right(), margins.top() + margins.bottom());
}

QSize FlowLayout::sizeHint() const
{
    return minimumSize();
}

void FlowLayout::setGeometry(const QRect &rect)
{
    QLayout::setGeometry(rect);
    doLayout(rect, false);
}

int FlowLayout::doLayout(const QRect &rect, bool testOnly) const
{
    const QMargins margins = contentsMargins();
    const QRect area = rect.adjusted(margins.left(), margins.top(), -margins.right(),
                                     -margins.bottom());
    int x = area.x();
    int y = area.y();
    int rowHeight = 0;
    for (QLayoutItem *item : m_items) {
        if (item->widget() && item->widget()->isHidden())
            continue;
        const QSize size = item->sizeHint();
        if (x + size.width() > area.right() + 1 && rowHeight > 0) {
            x = area.x();
            y += rowHeight + spacing();
            rowHeight = 0;
        }
        if (!testOnly)
            item->setGeometry(QRect(QPoint(x, y), size));
        x += size.width() + spacing();
        rowHeight = qMax(rowHeight, size.height());
    }
    return y + rowHeight - rect.y() + margins.bottom();
}

} // namespace Aurora
