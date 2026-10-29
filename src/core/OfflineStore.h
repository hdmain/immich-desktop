#pragma once

#include "core/ImmichTypes.h"

#include <QHash>
#include <QList>
#include <QString>

namespace Aurora {

// Every view of one photo, for "most viewed" lists later on.
struct AssetViewStats {
    ImmichAsset asset; // As last opened, so lists can show it without a fetch.
    QList<qint64> viewedAt; // UTC seconds since the epoch, oldest first.
};

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

    // Photos opened in the viewer, most recent first.
    void saveViewHistory(const QString &serverUrl, const QList<ImmichAsset> &assets);
    QList<ImmichAsset> loadViewHistory(const QString &serverUrl) const;
    // How often and when each photo was viewed; not capped like the history.
    void saveViewStats(const QString &serverUrl, const QHash<QString, AssetViewStats> &stats);
    QHash<QString, AssetViewStats> loadViewStats(const QString &serverUrl) const;

    QString directory() const;

private:
    QString libraryPath(const QString &serverUrl) const;
    QString explorePath(const QString &serverUrl) const;
    QString viewHistoryPath(const QString &serverUrl) const;
    QString viewStatsPath(const QString &serverUrl) const;
    static QString serverKey(const QString &serverUrl);

    QString m_directory;
};

} // namespace Aurora
