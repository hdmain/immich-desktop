#pragma once

#include <QDateTime>
#include <QList>
#include <QString>

namespace Aurora {

enum class LocalSyncState {
    None,
    Unsaved,
    Saving,
    Error,
};

struct ImmichAsset {
    QString id;
    QString type;
    QString fileName;
    QString duration;
    QDateTime takenAt;
    qreal aspectRatio = 1.0;
    bool favorite = false;
    // Set for Folder Sync items shown in Library before Immich has them.
    QString localPath;
    LocalSyncState localSyncState = LocalSyncState::None;

    bool isVideo() const { return type.compare(QStringLiteral("VIDEO"), Qt::CaseInsensitive) == 0; }
    bool isLocalPending() const { return !localPath.isEmpty(); }
};

struct ImmichPerson {
    QString id;
    QString name;
    bool favorite = false;
    bool hidden = false;
};

struct ImmichPlace {
    QString city;
    ImmichAsset sampleAsset;
};

struct ImmichExploreData {
    QList<ImmichPerson> people;
    QList<ImmichPlace> places;
    QList<ImmichAsset> recentAssets;
};

struct TimeBucketInfo {
    QDate month; // Always the 1st of the month.
    int count = 0;
};

} // namespace Aurora
