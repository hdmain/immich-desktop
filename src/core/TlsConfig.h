#pragma once

#include <QSslConfiguration>
#include <QString>

namespace Aurora {

enum class TlsCertFormat { Pkcs12, Pem };

struct TlsConfigResult {
    bool ok = false;
    QSslConfiguration config;
    QString error;
    bool needsPassword = false; // empty password, but an encrypted cert needs one
    bool badPassword = false;   // a non-empty password failed to decrypt the cert
};

struct ImmichConnectionSettings; // defined in core/AppSettings.h

namespace TlsConfig {

// Builds a QSslConfiguration from the connection's mTLS settings. Pure: it only
// assembles the config object, it never applies it to a socket or request.
TlsConfigResult build(const ImmichConnectionSettings &conn, const QString &password);

} // namespace TlsConfig

} // namespace Aurora
