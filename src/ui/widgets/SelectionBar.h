#pragma once

#include <QFrame>

class QLabel;
class QPushButton;

namespace Aurora {

// Replaces a grid's toolbar while photos are selected: the count and what can be
// done with them. Which actions show depends on the view (normal, archive, trash).
class SelectionBar final : public QFrame {
    Q_OBJECT

public:
    enum class Mode { Normal, Archive, Trash };

    explicit SelectionBar(QWidget *parent = nullptr);

    void setMode(Mode mode);
    void setCount(int count);
    // Favorite turns into "Remove from favorites" when every selected photo is one.
    void setAllFavorite(bool allFavorite);

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

private:
    void updateButtons();

    Mode m_mode = Mode::Normal;
    bool m_allFavorite = false;
    QLabel *m_count;
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
