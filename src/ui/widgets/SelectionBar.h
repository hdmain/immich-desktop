#pragma once

#include <QFrame>

class QGraphicsOpacityEffect;
class QLabel;
class QPropertyAnimation;
class QPushButton;
class QResizeEvent;
class QShowEvent;

namespace Aurora {

// Replaces a grid's toolbar while photos are selected: the count and what can be
// done with them. Which actions show depends on the view (normal, archive, trash).
// Labels shrink with the bar's own width: full text, short text, or icons only.
// Appears and disappears with a short fade.
class SelectionBar final : public QFrame {
    Q_OBJECT

public:
    enum class Mode { Normal, Archive, Trash };

    explicit SelectionBar(QWidget *parent = nullptr);

    void setMode(Mode mode);
    void setCount(int count);
    // Favorite turns into "Remove from favorites" when every selected photo is one.
    void setAllFavorite(bool allFavorite);
    // Shows or hides with a short fade; prefer this over setVisible while selecting.
    void setActive(bool active);
    bool isActive() const { return m_active; }

signals:
    void clearRequested();
    void selectAllRequested();
    void openRequested();
    void downloadRequested();
    void favoriteRequested(bool favorite);
    void archiveRequested(bool archive);
    void trashRequested();
    void deleteRequested();
    void restoreRequested();

protected:
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    enum class Density { Full, Short, Icons };

    void updateButtons();
    void fitDensity();
    void applyDensity(Density density);
    void setButtonText(QPushButton *button, const QString &visible, const QString &fullName);

    Mode m_mode = Mode::Normal;
    bool m_allFavorite = false;
    Density m_density = Density::Full;
    bool m_fitting = false;
    bool m_active = false;
    QGraphicsOpacityEffect *m_opacity;
    QPropertyAnimation *m_fade;
    QLabel *m_count;
    QPushButton *m_selectAll;
    QPushButton *m_open;
    QPushButton *m_download;
    QPushButton *m_favorite;
    QPushButton *m_archive;
    QPushButton *m_trash;
    QPushButton *m_delete;
    QPushButton *m_restore;
};

// Asks before favoriting or archiving a lot of photos at once, since Select all
// can reach thousands of them. True when it's few enough or the user agreed.
bool confirmBulkChange(QWidget *parent, int count, const QString &title, const QString &question);

} // namespace Aurora
