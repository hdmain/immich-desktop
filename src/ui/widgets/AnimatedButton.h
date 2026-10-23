#pragma once

#include <QPushButton>

namespace Aurora {

class ThemeManager;

class AnimatedButton final : public QPushButton {
    Q_OBJECT
    Q_PROPERTY(qreal hoverProgress READ hoverProgress WRITE setHoverProgress)

public:
    explicit AnimatedButton(const QString &text, const QString &iconResource,
                            ThemeManager *themeManager,
                            QWidget *parent = nullptr);

    qreal hoverProgress() const;
    void setHoverProgress(qreal progress);
    QSize sizeHint() const override;
    void setIconOnly(bool iconOnly);
    void setIconResource(const QString &iconResource);
    // Expandable buttons draw a chevron; clicking it toggles instead of clicking.
    void setExpandable(bool expandable);
    void setExpanded(bool expanded);
    bool isExpanded() const;

signals:
    void expandToggled(bool expanded);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    void animateTo(qreal endValue);
    bool showsChevron() const;
    QRect chevronRect() const;

    QString m_iconResource;
    ThemeManager *m_themeManager;
    qreal m_hoverProgress = 0.0;
    bool m_iconOnly = false;
    bool m_expandable = false;
    bool m_expanded = false;
    bool m_chevronPressed = false;
};

} // namespace Aurora
