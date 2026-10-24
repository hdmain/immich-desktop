#include "core/CustomHeaders.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QObject>
#include <QRegularExpression>
#include <QSet>

namespace Aurora::CustomHeaders {

bool isValidName(const QString &name)
{
    static const QRegularExpression pattern(
        QStringLiteral("\\A[!#$%&'*+\\-.^_`|~0-9A-Za-z]+\\z"));
    return pattern.match(name).hasMatch();
}

bool isValidValue(const QString &value)
{
    return !value.contains(u'\r') && !value.contains(u'\n');
}

bool isReservedName(const QString &name)
{
    static const QSet<QString> reserved = {
        QStringLiteral("host"),
        QStringLiteral("content-length"),
        QStringLiteral("transfer-encoding"),
        QStringLiteral("connection"),
        QStringLiteral("range"),
        QStringLiteral("x-api-key"),
    };
    return reserved.contains(name.trimmed().toLower());
}

QString validationError(const HeaderList &headers)
{
    for (qsizetype i = 0; i < headers.size(); ++i) {
        const QString name = headers.at(i).first.trimmed();
        const QString value = headers.at(i).second.trimmed();
        if (name.isEmpty()) {
            if (!value.isEmpty())
                return QObject::tr("Row %1: header name is required.").arg(i + 1);
            continue;
        }
        if (!isValidName(name))
            return QObject::tr("Row %1: \"%2\" is not a valid header name.")
                .arg(i + 1)
                .arg(name);
        if (isReservedName(name))
            return QObject::tr("Row %1: \"%2\" is managed by the app and cannot be "
                               "overridden.")
                .arg(i + 1)
                .arg(name);
        if (!isValidValue(value))
            return QObject::tr("Row %1: header values cannot contain line breaks.")
                .arg(i + 1);
    }
    return {};
}

HeaderList normalize(const HeaderList &headers)
{
    HeaderList result;
    for (const auto &header : headers) {
        const QString name = header.first.trimmed();
        const QString value = header.second.trimmed();
        if (name.isEmpty() && value.isEmpty())
            continue;
        bool replaced = false;
        for (qsizetype i = 0; i < result.size(); ++i) {
            if (result.at(i).first.compare(name, Qt::CaseInsensitive) == 0) {
                result[i] = {name, value};
                replaced = true;
                break;
            }
        }
        if (!replaced)
            result.append({name, value});
    }
    return result;
}

QString serialize(const HeaderList &headers)
{
    QJsonArray array;
    for (const auto &header : headers) {
        QJsonObject object;
        object.insert(QStringLiteral("name"), header.first);
        object.insert(QStringLiteral("value"), header.second);
        array.append(object);
    }
    return QString::fromUtf8(QJsonDocument(array).toJson(QJsonDocument::Compact));
}

HeaderList deserialize(const QString &serialized)
{
    HeaderList result;
    if (serialized.trimmed().isEmpty())
        return result;
    const QJsonDocument document = QJsonDocument::fromJson(serialized.toUtf8());
    if (!document.isArray())
        return result;
    for (const QJsonValue &entry : document.array()) {
        if (!entry.isObject())
            return {};
        const QJsonObject object = entry.toObject();
        const QJsonValue name = object.value(QStringLiteral("name"));
        const QJsonValue value = object.value(QStringLiteral("value"));
        if (!name.isString() || !value.isString())
            return {};
        result.append({name.toString(), value.toString()});
    }
    return result;
}

void applyTo(QNetworkRequest &request, const HeaderList &headers)
{
    for (const auto &header : headers) {
        const QString name = header.first.trimmed();
        const QString value = header.second.trimmed();
        if (!isValidName(name) || isReservedName(name) || !isValidValue(value))
            continue;
        const QByteArray rawValue = value.isNull() ? QByteArray("") : value.toUtf8();
        request.setRawHeader(name.toLatin1(), rawValue);
    }
}

} // namespace Aurora::CustomHeaders
