#include "ui/widgets/SortButton.h"

#include "ui/IconUtils.h"

#include <QActionGroup>
#include <QMenu>

namespace Aurora {
namespace {

// Direction labels that fit the field: newest/oldest, largest/smallest, ...
QPair<QString, QString> directionNames(AssetSort::Field field)
{
    switch (field) {
    case AssetSort::Field::Size:
        return {SortButton::tr("Largest first"), SortButton::tr("Smallest first")};
    case AssetSort::Field::Rating:
        return {SortButton::tr("Highest first"), SortButton::tr("Lowest first")};
    case AssetSort::Field::Name:
        return {SortButton::tr("Z to A"), SortButton::tr("A to Z")};
    case AssetSort::Field::Type:
        return {SortButton::tr("Videos first"), SortButton::tr("Photos first")};
    default:
        return {SortButton::tr("Newest first"), SortButton::tr("Oldest first")};
    }
}

} // namespace

SortButton::SortButton(QWidget *parent)
    : QPushButton(parent)
{
    configureActionButton(this, QStringLiteral(":/icons/arrow-up-down.svg"));
    setCursor(Qt::PointingHandCursor);
    setMenu(new QMenu(this));
    rebuildMenu();
}

QString SortButton::fieldName(AssetSort::Field field)
{
    switch (field) {
    case AssetSort::Field::TakenAt: return tr("Date taken");
    case AssetSort::Field::AddedAt: return tr("Date added");
    case AssetSort::Field::Size: return tr("File size");
    case AssetSort::Field::Rating: return tr("Rating");
    case AssetSort::Field::Relevance: return tr("Relevance");
    case AssetSort::Field::Activity: return tr("Recent likes and comments");
    case AssetSort::Field::Deleted: return tr("Date deleted");
    case AssetSort::Field::Viewed: return tr("Last viewed");
    case AssetSort::Field::Name: return tr("Name");
    case AssetSort::Field::Modified: return tr("Date modified");
    case AssetSort::Field::Type: return tr("Type");
    }
    return {};
}

void SortButton::setFields(const QList<AssetSort::Field> &fields)
{
    m_fields = fields.isEmpty() ? QList<AssetSort::Field>{AssetSort::Field::TakenAt} : fields;
    if (!m_fields.contains(m_sort.field))
        m_sort = {m_fields.first(), false};
    rebuildMenu();
}

void SortButton::setSort(const AssetSort &sort)
{
    m_sort = m_fields.contains(sort.field) ? sort : AssetSort{m_fields.first(), false};
    rebuildMenu();
}

void SortButton::setShuffled(bool shuffled)
{
    m_shuffled = shuffled;
    rebuildMenu();
}

void SortButton::choose(const AssetSort &sort)
{
    if (sort == m_sort && !m_shuffled)
        return;
    m_sort = sort;
    m_shuffled = false;
    rebuildMenu();
    emit sortChanged(m_sort);
}

void SortButton::rebuildMenu()
{
    QMenu *menu = this->menu();
    menu->clear();
    menu->addSection(tr("Sort by"));
    auto *fieldGroup = new QActionGroup(menu);
    for (AssetSort::Field field : std::as_const(m_fields)) {
        QAction *action = menu->addAction(fieldName(field));
        action->setCheckable(true);
        action->setChecked(field == m_sort.field);
        fieldGroup->addAction(action);
        // Names and types start A to Z / photos first, the rest newest/largest first.
        connect(action, &QAction::triggered, this, [this, field] {
            const bool startsAscending =
                field == AssetSort::Field::Name || field == AssetSort::Field::Type;
            choose({field, field == m_sort.field ? m_sort.ascending : startsAscending});
        });
    }

    const auto [descending, ascending] = directionNames(m_sort.field);
    if (m_sort.field != AssetSort::Field::Relevance) {
        menu->addSection(tr("Order"));
        auto *orderGroup = new QActionGroup(menu);
        const bool ascendingFirst =
            m_sort.field == AssetSort::Field::Name || m_sort.field == AssetSort::Field::Type;
        for (const bool up : {ascendingFirst, !ascendingFirst}) {
            QAction *action = menu->addAction(up ? ascending : descending);
            action->setCheckable(true);
            action->setChecked(m_sort.ascending == up);
            orderGroup->addAction(action);
            connect(action, &QAction::triggered, this,
                    [this, up] { choose({m_sort.field, up}); });
        }
    }

    const QString field = fieldName(m_sort.field).toLower();
    const QString tip =
        m_shuffled ? tr("Shuffled; pick an order to sort again")
        : m_sort.field == AssetSort::Field::Relevance
            ? tr("Sorted by relevance")
            : tr("Sorted by %1, %2")
                  .arg(field, (m_sort.ascending ? ascending : descending).toLower());
    setActionButtonLabel(this, QString());
    setToolTip(tip);
    setAccessibleName(tip);
}

} // namespace Aurora
