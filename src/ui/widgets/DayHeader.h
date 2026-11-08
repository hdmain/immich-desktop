#pragma once

#include <QLabel>

namespace Aurora {

// A day's date above its photos, with a check circle after it that selects or
// clears the whole day. The circle shows on hover, and on every day while selecting.
class DayHeader final : public QLabel {
    Q_OBJECT

public:
    DayHeader(const QString &text, QWidget *parent = nullptr);

    void setSelecting(bool selecting);
    // Every photo of the day is selected.
    void setChecked(bool checked);

signals:
    void toggled();

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QRect circleRect() const;
    bool overCircle(const QPoint &pos) const;

    bool m_selecting = false;
    bool m_checked = false;
    bool m_hovered = false;
    bool m_pressOnCircle = false;
};

} // namespace Aurora
