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

protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    void animateTo(qreal endValue);

    QString m_iconResource;
    ThemeManager *m_themeManager;
    qreal m_hoverProgress = 0.0;
};

} // namespace Aurora
