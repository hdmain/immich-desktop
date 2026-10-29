#pragma once

#include "core/ImmichTypes.h"

#include <QList>
#include <QPushButton>

class QActionGroup;

namespace Aurora {

// Icon button with a menu for ordering a photo grid. The menu lists the orders
// the current view offers, then the direction (newest/oldest first and so on).
class SortButton final : public QPushButton {
    Q_OBJECT

public:
    explicit SortButton(QWidget *parent = nullptr);

    AssetSort sort() const { return m_sort; }
    // The orders to offer, in menu order; a sort that isn't offered falls back
    // to the first one.
    void setFields(const QList<AssetSort::Field> &fields);
    void setSort(const AssetSort &sort);
    // While shuffled, any choice in the menu (even the current one) sorts again.
    void setShuffled(bool shuffled);

    static QString fieldName(AssetSort::Field field);

signals:
    void sortChanged(const Aurora::AssetSort &sort);

private:
    void rebuildMenu();
    void choose(const AssetSort &sort);

    QList<AssetSort::Field> m_fields{AssetSort::Field::TakenAt};
    AssetSort m_sort;
    bool m_shuffled = false;
};

} // namespace Aurora
