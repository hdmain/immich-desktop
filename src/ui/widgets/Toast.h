#pragma once

#include <QFrame>

#include <functional>

class QLabel;
class QPropertyAnimation;
class QPushButton;
class QTimer;

namespace Aurora {

// A message floating near the bottom of a window, for things that need noticing
// more than a status line. Messages fade after a few seconds; a progress toast
// (e.g. a download) stays, with a button such as Cancel, until it's finished.
class Toast final : public QFrame {
    Q_OBJECT

public:
    // Something went wrong. Reuses the window's message toast if one is showing.
    static void showMessage(QWidget *window, const QString &message,
                            const QString &iconResource = QStringLiteral(":/icons/circle-alert.svg"));
    // Something finished, like a download.
    static void showInfo(QWidget *window, const QString &message);
    // Something short is on its way (e.g. a copy); stays until the next message replaces it.
    static void showPending(QWidget *window, const QString &message, const QString &iconResource);
    // Stays until finishProgress(); update it with setProgressText(). The action
    // button runs action (e.g. cancel) and is hidden when actionText is empty.
    static void showProgress(QWidget *window, const QString &message, const QString &actionText,
                             std::function<void()> action);
    static void setProgressText(QWidget *window, const QString &message);
    static void finishProgress(QWidget *window);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    enum class Kind { Message, Progress };
    enum class Style { Error, Info, Progress };

    Toast(QWidget *window, Kind kind);
    static Toast *find(QWidget *window, Kind kind);
    void show(const QString &message, const QString &iconResource, Style style);
    void reposition();
    void fadeOut();

    Kind m_kind;
    QLabel *m_icon;
    QLabel *m_text;
    QPushButton *m_action;
    std::function<void()> m_onAction;
    QTimer *m_timer;
    QPropertyAnimation *m_fade;
};

} // namespace Aurora
