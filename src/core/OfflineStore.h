#pragma once

#include "core/ImmichTypes.h"

#include <QList>
#include <QString>

namespace Aurora {

class OfflineStore final {
public:
    OfflineStore();

    void saveLibrary(const QString &serverUrl, const QList<ImmichAsset> &assets,
                     const QString &query = {});
    void mergeLibrary(const QString &serverUrl, const QList<ImmichAsset> &assets);
    bool loadLibrary(const QString &serverUrl, QList<ImmichAsset> *assets,
                     QString *query = nullptr) const;

    void saveExplore(const QString &serverUrl, const ImmichExploreData &data);
    bool loadExplore(const QString &serverUrl, ImmichExploreData *data) const;

    QString directory() const;

private:
    QString libraryPath(const QString &serverUrl) const;
    QString explorePath(const QString &serverUrl) const;
    static QString serverKey(const QString &serverUrl);

    QString m_directory;
};

} // namespace Aurora
