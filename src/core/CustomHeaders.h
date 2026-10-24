#pragma once

#include <QList>
#include <QPair>
#include <QString>

class QNetworkRequest;

namespace Aurora::CustomHeaders {

using HeaderList = QList<QPair<QString, QString>>;

bool isValidName(const QString &name);
bool isValidValue(const QString &value);
bool isReservedName(const QString &name);

// Returns an empty string when all headers are valid; otherwise a user-facing error message.
QString validationError(const HeaderList &headers);

// Trims names and values, drops fully empty rows, and lets later duplicates override earlier ones.
HeaderList normalize(const HeaderList &headers);

QString serialize(const HeaderList &headers);
HeaderList deserialize(const QString &serialized);

void applyTo(QNetworkRequest &request, const HeaderList &headers);

} // namespace Aurora::CustomHeaders
