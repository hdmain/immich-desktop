#include "ui/TileSelection.h"

#include <QtTest>

using namespace Aurora;

class TileSelectionTest final : public QObject {
    Q_OBJECT

    const QStringList m_order{"a", "b", "c", "d", "e", "f"};

private slots:
    void clickToggles()
    {
        TileSelection selection;
        selection.click("b", Qt::NoModifier, m_order);
        selection.click("d", Qt::ControlModifier, m_order);
        QCOMPARE(selection.ordered(m_order), QStringList({"b", "d"}));
        selection.click("b", Qt::NoModifier, m_order);
        QCOMPARE(selection.ordered(m_order), QStringList({"d"}));
    }

    void shiftRangeShrinksBack()
    {
        TileSelection selection;
        selection.click("b", Qt::NoModifier, m_order);
        selection.click("e", Qt::ShiftModifier, m_order);
        QCOMPARE(selection.ordered(m_order), QStringList({"b", "c", "d", "e"}));
        // Same anchor: the range is replaced, not added to.
        selection.click("c", Qt::ShiftModifier, m_order);
        QCOMPARE(selection.ordered(m_order), QStringList({"b", "c"}));
        selection.click("a", Qt::ShiftModifier, m_order);
        QCOMPARE(selection.ordered(m_order), QStringList({"a", "b"}));
    }

    void rangeKeepsEarlierSelection()
    {
        TileSelection selection;
        selection.click("a", Qt::NoModifier, m_order);
        selection.click("d", Qt::ControlModifier, m_order);
        selection.click("f", Qt::ShiftModifier, m_order);
        QCOMPARE(selection.ordered(m_order), QStringList({"a", "d", "e", "f"}));
        selection.click("e", Qt::ShiftModifier, m_order);
        QCOMPARE(selection.ordered(m_order), QStringList({"a", "d", "e"}));
    }

    void extendWithoutAnchorStartsFromCursor()
    {
        TileSelection selection;
        selection.extendTo("d", m_order, "c");
        QCOMPARE(selection.ordered(m_order), QStringList({"c", "d"}));
        selection.extendTo("e", m_order, "x");
        QCOMPARE(selection.ordered(m_order), QStringList({"c", "d", "e"}));
        selection.extendTo("b", m_order, "x");
        QCOMPARE(selection.ordered(m_order), QStringList({"b", "c"}));
    }

    void dayTogglesTogether()
    {
        TileSelection selection;
        selection.click("a", Qt::NoModifier, m_order);
        selection.setSelected({"c", "d"}, true);
        QCOMPARE(selection.ordered(m_order), QStringList({"a", "c", "d"}));
        // A later Shift range keeps the day.
        selection.click("b", Qt::ShiftModifier, m_order);
        QCOMPARE(selection.ordered(m_order), QStringList({"a", "b", "c", "d"}));
        selection.setSelected({"c", "d"}, false);
        QCOMPARE(selection.ordered(m_order), QStringList({"a", "b"}));
    }

    void removedPhotosStayOutOfRanges()
    {
        TileSelection selection;
        selection.click("a", Qt::NoModifier, m_order);
        selection.click("c", Qt::ControlModifier, m_order);
        selection.remove({"a"});
        selection.click("e", Qt::ShiftModifier, m_order);
        QCOMPARE(selection.ordered(m_order), QStringList({"c", "d", "e"}));
    }
};

QTEST_GUILESS_MAIN(TileSelectionTest)
#include "TileSelectionTest.moc"
