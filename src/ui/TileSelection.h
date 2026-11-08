#pragma once

#include <QObject>
#include <QSet>
#include <QStringList>

namespace Aurora {

// Which photos in a grid are selected. Ctrl-click (or any click while something
// is selected) toggles a photo; Shift-click or Shift+arrow selects everything
// from the last toggled photo to this one, in the grid's order.
class TileSelection final : public QObject {
    Q_OBJECT

public:
    explicit TileSelection(QObject *parent = nullptr);

    // order: the grid's asset ids in display order, for Shift ranges.
    void click(const QString &assetId, Qt::KeyboardModifiers modifiers, const QStringList &order);
    // Shift+arrow: selects from the anchor to assetId. Without an anchor yet,
    // anchorIfNone (the tile the cursor left) becomes it.
    void extendTo(const QString &assetId, const QStringList &order, const QString &anchorIfNone);
    void selectAll(const QStringList &order);
    // Selects or clears these photos together, like a day from its header.
    void setSelected(const QStringList &assetIds, bool selected);
    void clear();
    // Photos that left the grid (deleted, restored, unfavorited on Favorites…).
    void remove(const QStringList &assetIds);

    bool isEmpty() const { return m_ids.isEmpty(); }
    int count() const { return int(m_ids.size()); }
    bool contains(const QString &assetId) const { return m_ids.contains(assetId); }
    // The selected ids in display order.
    QStringList ordered(const QStringList &order) const;

signals:
    void changed();

private:
    QSet<QString> m_ids;
    QString m_anchor;
    QSet<QString> m_base; // The selection before the current Shift range.
};

} // namespace Aurora
