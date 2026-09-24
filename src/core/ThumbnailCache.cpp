#include "core/ThumbnailCache.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutexLocker>
#include <QSaveFile>
#include <QStandardPaths>

#include <algorithm>

namespace Aurora {

ThumbnailCache::ThumbnailCache(const QString &subdirectory, int memoryBudgetKb,
                               qint64 maxDiskBytes)
    : m_directory(QStandardPaths::writableLocation(QStandardPaths::CacheLocation) +
                  u'/' + subdirectory)
    , m_maxDiskBytes(maxDiskBytes)
{
    // Keep only a small viewport-sized working set in RAM. The full cache lives
    // on disk and is decoded again when an item returns to the viewport.
    m_memory.setMaxCost(memoryBudgetKb);
    QDir().mkpath(m_directory);
}

QString ThumbnailCache::directory() const
{
    return m_directory;
}

QString ThumbnailCache::filePath(const QString &assetId) const
{
    const QByteArray hash =
        QCryptographicHash::hash(assetId.toUtf8(), QCryptographicHash::Sha1).toHex();
    return m_directory + u'/' + QString::fromLatin1(hash) + QStringLiteral(".bin");
}

QPixmap ThumbnailCache::memoryPixmap(const QString &assetId) const
{
    QMutexLocker lock(&m_mutex);
    if (const QPixmap *pixmap = m_memory.object(assetId))
        return *pixmap;
    return {};
}

QByteArray ThumbnailCache::readDisk(const QString &assetId) const
{
    QMutexLocker lock(&m_diskMutex);
    QFile file(filePath(assetId));
    if (!file.open(QIODevice::ReadOnly))
        return {};
    return file.readAll();
}

void ThumbnailCache::store(const QString &assetId, const QByteArray &bytes,
                           const QPixmap &pixmap)
{
    if (assetId.isEmpty() || pixmap.isNull())
        return;

    {
        QMutexLocker lock(&m_mutex);
        const int costKb = qMax(1, pixmap.width() * pixmap.height() *
                                       qMax(1, pixmap.depth()) / 8 / 1024);
        m_memory.insert(assetId, new QPixmap(pixmap), costKb);
    }

    if (bytes.isEmpty())
        return;

    QMutexLocker lock(&m_diskMutex);
    QSaveFile file(filePath(assetId));
    if (!file.open(QIODevice::WriteOnly))
        return;
    if (file.write(bytes) != bytes.size()) {
        file.cancelWriting();
        return;
    }
    file.commit();
    if (m_maxDiskBytes > 0)
        trimDiskLocked();
}

void ThumbnailCache::clearMemory()
{
    QMutexLocker lock(&m_mutex);
    m_memory.clear();
}

void ThumbnailCache::trimDiskLocked()
{
    QDir dir(m_directory);
    QFileInfoList entries = dir.entryInfoList(QDir::Files, QDir::NoSort);

    qint64 total = 0;
    for (const QFileInfo &info : entries)
        total += info.size();
    if (total <= m_maxDiskBytes)
        return;

    std::sort(entries.begin(), entries.end(), [](const QFileInfo &a, const QFileInfo &b) {
        return a.lastModified() < b.lastModified();
    });
    for (const QFileInfo &info : entries) {
        if (total <= m_maxDiskBytes)
            break;
        total -= info.size();
        QFile::remove(info.absoluteFilePath());
    }
}

} // namespace Aurora
