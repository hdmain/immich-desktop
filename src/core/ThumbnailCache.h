#pragma once

#include <QByteArray>
#include <QCache>
#include <QMutex>
#include <QPixmap>
#include <QString>

namespace Aurora {

class ThumbnailCache final {
public:
    ThumbnailCache();

    QPixmap memoryPixmap(const QString &assetId) const;
    QByteArray readDisk(const QString &assetId) const;
    void store(const QString &assetId, const QByteArray &bytes, const QPixmap &pixmap);
    void clearMemory();

    QString directory() const;

private:
    QString filePath(const QString &assetId) const;

    mutable QMutex m_mutex;
    mutable QCache<QString, QPixmap> m_memory;
    QString m_directory;
};

} // namespace Aurora
