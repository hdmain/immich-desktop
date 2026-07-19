#pragma once

#include <QString>
#include <QStringList>

namespace Aurora {

class UploadQueueStore final {
public:
    UploadQueueStore();

    QStringList load() const;
    void save(const QStringList &paths) const;
    void enqueue(const QStringList &paths);
    void remove(const QString &path);
    void clear();

    QString filePath() const;

private:
    QString m_filePath;
};

} // namespace Aurora
