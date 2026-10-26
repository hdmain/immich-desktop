#pragma once

#include <QFrame>

class QLabel;
class QPropertyAnimation;
class QTimer;

namespace Aurora {

// A short message floating near the bottom of a window, for things that need
// noticing more than a status line (e.g. an action that failed).
class Toast final : public QFrame {
    Q_OBJECT

public:
    // Reuses the window's toast if one is already showing.
    static void showMessage(QWidget *window, const QString &message,
                            const QString &iconResource = QStringLiteral(":/icons/circle-alert.svg"));

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    explicit Toast(QWidget *window);
    void setMessage(const QString &message, const QString &iconResource);
    void reposition();
    void fadeOut();

    QLabel *m_icon;
    QLabel *m_text;
    QTimer *m_timer;
    QPropertyAnimation *m_fade;
};

} // namespace Aurora
