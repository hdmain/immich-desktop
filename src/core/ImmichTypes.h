#pragma once

#include <QDateTime>
#include <QList>
#include <QString>

namespace Aurora {

struct ImmichAsset {
    QString id;
    QString type;
    QString fileName;
    QString duration;
    QDateTime takenAt;
    qreal aspectRatio = 1.0;
    bool favorite = false;

    bool isVideo() const { return type.compare(QStringLiteral("VIDEO"), Qt::CaseInsensitive) == 0; }
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

} // namespace Aurora
