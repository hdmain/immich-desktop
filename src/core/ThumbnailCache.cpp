#include "core/ThumbnailCache.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QMutexLocker>
#include <QStandardPaths>

namespace Aurora {

ThumbnailCache::ThumbnailCache()
    : m_directory(QStandardPaths::writableLocation(QStandardPaths::CacheLocation) +
                  QStringLiteral("/thumbnails"))
{
    m_memory.setMaxCost(192 * 1024);
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

    QFile file(filePath(assetId));
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        file.write(bytes);
}

void ThumbnailCache::clearMemory()
{
    QMutexLocker lock(&m_mutex);
    m_memory.clear();
}

} // namespace Aurora
