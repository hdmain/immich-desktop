#include "core/ThumbnailCache.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QMutexLocker>
#include <QSaveFile>
#include <QStandardPaths>

namespace Aurora {

ThumbnailCache::ThumbnailCache()
    : m_directory(QStandardPaths::writableLocation(QStandardPaths::CacheLocation) +
                  QStringLiteral("/thumbnails"))
{
    // Keep only a small viewport-sized working set in RAM. The full cache lives
    // on disk and is decoded again when an item returns to the viewport.
    m_memory.setMaxCost(32 * 1024);
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
    constexpr qint64 maximumThumbnailBytes = 8LL * 1024 * 1024;
    if (file.size() < 0 || file.size() > maximumThumbnailBytes)
        return {};
    const QByteArray bytes = file.read(maximumThumbnailBytes + 1);
    if (bytes.size() > maximumThumbnailBytes)
        return {};
    return bytes;
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
    if (file.write(bytes) != bytes.size())
        file.cancelWriting();
    else
        file.commit();
}

void ThumbnailCache::clearMemory()
{
    QMutexLocker lock(&m_mutex);
    m_memory.clear();
}

} // namespace Aurora
