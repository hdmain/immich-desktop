#pragma once

#include <QDateTime>
#include <QList>
#include <QString>
#include <QStringList>

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

struct ImmichAlbum {
    QString id;
    QString name;
    int assetCount = 0;
    QString thumbnailAssetId;
};

struct ImmichTag {
    QString id;
    QString name;
    QString path; // Full hierarchy, e.g. "Cars/BMW".
    QString parentId;
};

struct ImmichAssetDetails {
    ImmichAsset asset;
    QDateTime localDateTime; // Wall-clock time where the photo was taken, held as UTC.
    QString timeZone;
    int width = 0;
    int height = 0;
    qint64 fileSize = 0;
    QString make;
    QString model;
    QString lens;
    double fNumber = 0.0;
    double focalLength = 0.0;
    int iso = 0;
    QString exposureTime;
    bool hasLocation = false;
    double latitude = 0.0;
    double longitude = 0.0;
    QString city;
    QString state;
    QString country;
    QString description;
    QString folder;     // For display: the part below the server's storage root.
    QString folderPath; // Full server path, for opening the folder.
    QList<ImmichPerson> people;
    QList<ImmichTag> tags;
    QString ownerId;
    QString ownerName;
    bool sharedByOtherUser = false;
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
