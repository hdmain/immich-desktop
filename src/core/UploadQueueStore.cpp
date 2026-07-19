#include "core/UploadQueueStore.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

namespace Aurora {

UploadQueueStore::UploadQueueStore()
    : m_filePath(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
                 QStringLiteral("/upload-queue.json"))
{
    QDir().mkpath(QFileInfo(m_filePath).absolutePath());
}

QString UploadQueueStore::filePath() const
{
    return m_filePath;
}

QStringList UploadQueueStore::load() const
{
    QFile file(m_filePath);
    if (!file.open(QIODevice::ReadOnly))
        return {};

    const QJsonArray items = QJsonDocument::fromJson(file.readAll())
                                 .object()
                                 .value(QStringLiteral("paths"))
                                 .toArray();
    QStringList paths;
    paths.reserve(items.size());
    for (const QJsonValue &value : items) {
        const QString path = value.toString();
        if (!path.isEmpty() && QFileInfo::exists(path))
            paths.append(QFileInfo(path).absoluteFilePath());
    }
    return paths;
}

void UploadQueueStore::save(const QStringList &paths) const
{
    QJsonArray items;
    for (const QString &path : paths) {
        if (!path.isEmpty())
            items.append(path);
    }
    QJsonObject root;
    root.insert(QStringLiteral("paths"), items);

    QFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return;
    file.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
}

void UploadQueueStore::enqueue(const QStringList &paths)
{
    QStringList current = load();
    for (const QString &path : paths) {
        const QString absolute = QFileInfo(path).absoluteFilePath();
        if (absolute.isEmpty() || current.contains(absolute))
            continue;
        current.append(absolute);
    }
    save(current);
}

void UploadQueueStore::remove(const QString &path)
{
    QStringList current = load();
    const QString absolute = QFileInfo(path).absoluteFilePath();
    current.removeAll(absolute);
    save(current);
}

void UploadQueueStore::clear()
{
    save({});
}

} // namespace Aurora
