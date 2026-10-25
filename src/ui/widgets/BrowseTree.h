#pragma once

#include "core/ImmichTypes.h"
#include "ui/BrowseSection.h"

#include <QHash>
#include <QTreeWidget>

namespace Aurora {

// The albums, tags or folders listed under their sidebar button. It grows to fit
// its rows so the sidebar scrolls as a whole instead of nesting scroll areas.
class BrowseTree final : public QTreeWidget {
    Q_OBJECT

public:
    explicit BrowseTree(BrowseSection section, QWidget *parent = nullptr);

    BrowseSection section() const;
    void setAlbums(const QList<ImmichAlbum> &albums);
    void setTags(const QList<ImmichTag> &tags);
    void setFolderPaths(const QStringList &paths);
    void setUnavailable(bool permissionDenied);
    void showLoading();
    void selectValue(const QString &value);
    QRect selectedRect() const; // In viewport coordinates.
    QSize sizeHint() const override;

signals:
    void albumRequested(const Aurora::ImmichAlbum &album);
    void tagRequested(const Aurora::ImmichTag &tag);
    void folderRequested(const QString &path, const QString &title);
    void reloadRequested();
    void itemSelected();

private:
    void clearItems();
    QTreeWidgetItem *addNote(const QString &text);
    void activate(QTreeWidgetItem *item);
    void updateHeight();

    BrowseSection m_section;
    QHash<QString, ImmichAlbum> m_albums;
    QHash<QString, ImmichTag> m_tags;
    QString m_selectedValue; // Reapplied when the list reloads.
};

} // namespace Aurora
