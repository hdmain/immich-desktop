#pragma once

#include <QStringList>

namespace Aurora {

// The deepest folder shared by every path, used as the root when showing the
// folder tree. It hides the server's own storage prefix (e.g. /data/library/<user>)
// and never swallows a folder that itself holds photos.
inline QString folderTreeRoot(const QStringList &paths)
{
    if (paths.isEmpty())
        return {};
    QStringList common = paths.first().split(u'/', Qt::SkipEmptyParts);
    for (const QString &path : paths) {
        const QStringList parts = path.split(u'/', Qt::SkipEmptyParts);
        qsizetype shared = 0;
        while (shared < common.size() && shared < parts.size() &&
               common.at(shared) == parts.at(shared))
            ++shared;
        common = common.mid(0, shared);
    }
    for (const QString &path : paths) {
        if (path.split(u'/', Qt::SkipEmptyParts).size() == common.size() && !common.isEmpty()) {
            common.removeLast();
            break;
        }
    }
    return u'/' + common.join(u'/');
}

// path relative to root, e.g. "2025/2025-07-12"; empty for the root itself or an
// unknown root.
inline QString folderRelativePath(const QString &path, const QString &root)
{
    if (root.isEmpty())
        return {};
    const QString prefix = root.endsWith(u'/') ? root : root + u'/';
    return path.startsWith(prefix) ? path.mid(prefix.size()) : QString();
}

inline QString folderPathJoin(const QString &parent, const QString &name)
{
    return parent.endsWith(u'/') ? parent + name : parent + u'/' + name;
}

} // namespace Aurora
