#pragma once

#include <QByteArray>
#include <QCache>
#include <QMutex>
#include <QPixmap>
#include <QString>

namespace Aurora {

class ThumbnailCache final {
public:
    // subdirectory: cache namespace under the app cache dir (keeps unrelated
    // image variants, e.g. thumbnails vs previews, from colliding by asset id).
    // maxDiskBytes: 0 means unbounded (existing thumbnail cache behavior);
    // a positive value enables LRU-by-mtime trimming after each disk write.
    explicit ThumbnailCache(const QString &subdirectory = QStringLiteral("thumbnails"),
                            int memoryBudgetKb = 32 * 1024, qint64 maxDiskBytes = 0);

    QPixmap memoryPixmap(const QString &assetId) const;
    QByteArray readDisk(const QString &assetId) const;
    void store(const QString &assetId, const QByteArray &bytes, const QPixmap &pixmap);
    void clearMemory();

    QString directory() const;

private:
    QString filePath(const QString &assetId) const;
    void trimDiskLocked();

    mutable QMutex m_mutex;
    mutable QMutex m_diskMutex;
    mutable QCache<QString, QPixmap> m_memory;
    QString m_directory;
    qint64 m_maxDiskBytes = 0;
};

} // namespace Aurora
