#include "ui/TileSelection.h"

namespace Aurora {

TileSelection::TileSelection(QObject *parent)
    : QObject(parent)
{
}

void TileSelection::click(const QString &assetId, Qt::KeyboardModifiers modifiers,
                          const QStringList &order)
{
    if ((modifiers & Qt::ShiftModifier) && order.contains(m_anchor)) {
        extendTo(assetId, order, m_anchor);
        return;
    }
    if (!m_ids.remove(assetId))
        m_ids.insert(assetId);
    m_anchor = assetId;
    m_base = m_ids;
    emit changed();
}

void TileSelection::extendTo(const QString &assetId, const QStringList &order,
                             const QString &anchorIfNone)
{
    if (!order.contains(m_anchor)) {
        m_anchor = anchorIfNone;
        m_base = m_ids;
    }
    const qsizetype from = order.indexOf(m_anchor);
    const qsizetype to = order.indexOf(assetId);
    if (from < 0 || to < 0)
        return;
    // The range replaces the previous one from this anchor, so going back shrinks it.
    m_ids = m_base;
    for (qsizetype i = qMin(from, to); i <= qMax(from, to); ++i)
        m_ids.insert(order.at(i));
    emit changed();
}

void TileSelection::selectAll(const QStringList &order)
{
    m_ids = QSet<QString>(order.begin(), order.end());
    m_base = m_ids;
    emit changed();
}

void TileSelection::setSelected(const QStringList &assetIds, bool selected)
{
    for (const QString &id : assetIds) {
        if (selected)
            m_ids.insert(id);
        else
            m_ids.remove(id);
    }
    m_base = m_ids;
    emit changed();
}

void TileSelection::clear()
{
    if (m_ids.isEmpty())
        return;
    m_ids.clear();
    m_base.clear();
    m_anchor.clear();
    emit changed();
}

void TileSelection::remove(const QStringList &assetIds)
{
    bool removed = false;
    for (const QString &id : assetIds) {
        m_base.remove(id);
        removed = m_ids.remove(id) || removed;
    }
    if (removed)
        emit changed();
}

QStringList TileSelection::ordered(const QStringList &order) const
{
    QStringList ids;
    for (const QString &id : order) {
        if (m_ids.contains(id))
            ids.append(id);
    }
    return ids;
}

} // namespace Aurora
