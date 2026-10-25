#pragma once

#include <QLayout>
#include <QList>

namespace Aurora {

// Lays widgets out left to right, wrapping onto new rows (Qt's FlowLayout example).
class FlowLayout final : public QLayout {
public:
    explicit FlowLayout(QWidget *parent = nullptr, int spacing = 12);
    ~FlowLayout() override;

    void addItem(QLayoutItem *item) override;
    int count() const override;
    QLayoutItem *itemAt(int index) const override;
    QLayoutItem *takeAt(int index) override;
    Qt::Orientations expandingDirections() const override;
    bool hasHeightForWidth() const override;
    int heightForWidth(int width) const override;
    QSize minimumSize() const override;
    QSize sizeHint() const override;
    void setGeometry(const QRect &rect) override;

private:
    int doLayout(const QRect &rect, bool testOnly) const;

    QList<QLayoutItem *> m_items;
};

} // namespace Aurora
