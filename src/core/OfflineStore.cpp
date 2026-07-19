#include "core/OfflineStore.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <QStandardPaths>

namespace Aurora {
namespace {

QJsonObject assetToJson(const ImmichAsset &asset)
{
    QJsonObject object;
    object.insert(QStringLiteral("id"), asset.id);
    object.insert(QStringLiteral("type"), asset.type);
    object.insert(QStringLiteral("fileName"), asset.fileName);
    object.insert(QStringLiteral("duration"), asset.duration);
    object.insert(QStringLiteral("favorite"), asset.favorite);
    object.insert(QStringLiteral("aspectRatio"), asset.aspectRatio);
    if (asset.takenAt.isValid())
        object.insert(QStringLiteral("takenAt"), asset.takenAt.toString(Qt::ISODate));
    return object;
}

ImmichAsset assetFromJson(const QJsonObject &object)
{
    ImmichAsset asset;
    asset.id = object.value(QStringLiteral("id")).toString();
    asset.type = object.value(QStringLiteral("type")).toString();
    asset.fileName = object.value(QStringLiteral("fileName")).toString();
    asset.duration = object.value(QStringLiteral("duration")).toString();
    asset.favorite = object.value(QStringLiteral("favorite")).toBool();
    asset.aspectRatio = object.value(QStringLiteral("aspectRatio")).toDouble(1.0);
    asset.takenAt = QDateTime::fromString(
        object.value(QStringLiteral("takenAt")).toString(), Qt::ISODate);
    return asset;
}

bool writeJsonFile(const QString &path, const QJsonObject &object)
{
    QFileInfo info(path);
    QDir().mkpath(info.absolutePath());
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    file.write(QJsonDocument(object).toJson(QJsonDocument::Compact));
    return true;
}

QJsonObject readJsonFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return {};
    return QJsonDocument::fromJson(file.readAll()).object();
}

} // namespace

OfflineStore::OfflineStore()
    : m_directory(QStandardPaths::writableLocation(QStandardPaths::CacheLocation) +
                  QStringLiteral("/offline"))
{
    QDir().mkpath(m_directory);
}

QString OfflineStore::directory() const
{
    return m_directory;
}

QString OfflineStore::serverKey(const QString &serverUrl)
{
    return QString::fromLatin1(
        QCryptographicHash::hash(serverUrl.toUtf8(), QCryptographicHash::Sha1).toHex());
}

QString OfflineStore::libraryPath(const QString &serverUrl) const
{
    return m_directory + u'/' + serverKey(serverUrl) + QStringLiteral("-library.json");
}

QString OfflineStore::explorePath(const QString &serverUrl) const
{
    return m_directory + u'/' + serverKey(serverUrl) + QStringLiteral("-explore.json");
}

void OfflineStore::saveLibrary(const QString &serverUrl, const QList<ImmichAsset> &assets,
                               const QString &query)
{
    if (serverUrl.isEmpty())
        return;

    QJsonArray items;
    for (const ImmichAsset &asset : assets)
        items.append(assetToJson(asset));

    QJsonObject root;
    root.insert(QStringLiteral("serverUrl"), serverUrl);
    root.insert(QStringLiteral("savedAt"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    root.insert(QStringLiteral("query"), query);
    root.insert(QStringLiteral("assets"), items);
    writeJsonFile(libraryPath(serverUrl), root);
}

void OfflineStore::mergeLibrary(const QString &serverUrl, const QList<ImmichAsset> &assets)
{
    QList<ImmichAsset> existing;
    QString query;
    loadLibrary(serverUrl, &existing, &query);
    QSet<QString> seen;
    for (const ImmichAsset &asset : existing)
        seen.insert(asset.id);
    for (const ImmichAsset &asset : assets) {
        if (seen.contains(asset.id))
            continue;
        existing.append(asset);
        seen.insert(asset.id);
    }
    saveLibrary(serverUrl, existing, query);
}

bool OfflineStore::loadLibrary(const QString &serverUrl, QList<ImmichAsset> *assets,
                               QString *query) const
{
    if (!assets || serverUrl.isEmpty())
        return false;
    const QJsonObject root = readJsonFile(libraryPath(serverUrl));
    if (root.isEmpty())
        return false;
    if (query)
        *query = root.value(QStringLiteral("query")).toString();
    assets->clear();
    const QJsonArray items = root.value(QStringLiteral("assets")).toArray();
    assets->reserve(items.size());
    for (const QJsonValue &value : items) {
        ImmichAsset asset = assetFromJson(value.toObject());
        if (!asset.id.isEmpty())
            assets->append(asset);
    }
    return !assets->isEmpty();
}

void OfflineStore::saveExplore(const QString &serverUrl, const ImmichExploreData &data)
{
    if (serverUrl.isEmpty())
        return;

    QJsonArray people;
    for (const ImmichPerson &person : data.people) {
        QJsonObject object;
        object.insert(QStringLiteral("id"), person.id);
        object.insert(QStringLiteral("name"), person.name);
        object.insert(QStringLiteral("favorite"), person.favorite);
        object.insert(QStringLiteral("hidden"), person.hidden);
        people.append(object);
    }

    QJsonArray places;
    for (const ImmichPlace &place : data.places) {
        QJsonObject object;
        object.insert(QStringLiteral("city"), place.city);
        object.insert(QStringLiteral("sampleAsset"), assetToJson(place.sampleAsset));
        places.append(object);
    }

    QJsonArray recent;
    for (const ImmichAsset &asset : data.recentAssets)
        recent.append(assetToJson(asset));

    QJsonObject root;
    root.insert(QStringLiteral("serverUrl"), serverUrl);
    root.insert(QStringLiteral("savedAt"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    root.insert(QStringLiteral("people"), people);
    root.insert(QStringLiteral("places"), places);
    root.insert(QStringLiteral("recentAssets"), recent);
    writeJsonFile(explorePath(serverUrl), root);
}

bool OfflineStore::loadExplore(const QString &serverUrl, ImmichExploreData *data) const
{
    if (!data || serverUrl.isEmpty())
        return false;
    const QJsonObject root = readJsonFile(explorePath(serverUrl));
    if (root.isEmpty())
        return false;

    data->people.clear();
    data->places.clear();
    data->recentAssets.clear();

    for (const QJsonValue &value : root.value(QStringLiteral("people")).toArray()) {
        const QJsonObject object = value.toObject();
        ImmichPerson person;
        person.id = object.value(QStringLiteral("id")).toString();
        person.name = object.value(QStringLiteral("name")).toString();
        person.favorite = object.value(QStringLiteral("favorite")).toBool();
        person.hidden = object.value(QStringLiteral("hidden")).toBool();
        if (!person.id.isEmpty())
            data->people.append(person);
    }
    for (const QJsonValue &value : root.value(QStringLiteral("places")).toArray()) {
        const QJsonObject object = value.toObject();
        ImmichPlace place;
        place.city = object.value(QStringLiteral("city")).toString();
        place.sampleAsset =
            assetFromJson(object.value(QStringLiteral("sampleAsset")).toObject());
        if (!place.city.isEmpty())
            data->places.append(place);
    }
    for (const QJsonValue &value : root.value(QStringLiteral("recentAssets")).toArray()) {
        ImmichAsset asset = assetFromJson(value.toObject());
        if (!asset.id.isEmpty())
            data->recentAssets.append(asset);
    }
    return !data->people.isEmpty() || !data->places.isEmpty() || !data->recentAssets.isEmpty();
}

} // namespace Aurora
