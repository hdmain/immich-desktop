#include "core/TlsConfig.h"

#include "core/AppSettings.h"

#include <QFile>
#include <QList>
#include <QSslCertificate>
#include <QSslKey>

namespace Aurora::TlsConfig {

namespace {

TlsConfigResult fail(const QString &error, bool needsPassword = false, bool badPassword = false)
{
    return TlsConfigResult{false, QSslConfiguration(), error, needsPassword, badPassword};
}

// Reads a file fully; reports a load error naming the file on failure.
bool readFile(const QString &path, QByteArray &out, QString &error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        error = QStringLiteral("Could not open %1: %2").arg(path, file.errorString());
        return false;
    }
    out = file.readAll();
    return true;
}

TlsConfigResult buildPkcs12(const ImmichConnectionSettings &conn, const QString &password,
                            QSslConfiguration config)
{
    if (password.isEmpty()) {
        return fail(QStringLiteral("A password is required to open the PKCS#12 bundle."), true);
    }

    QFile file(conn.clientCertPath);
    if (!file.open(QIODevice::ReadOnly)) {
        return fail(QStringLiteral("Could not open %1: %2")
                        .arg(conn.clientCertPath, file.errorString()));
    }

    QSslKey key;
    QSslCertificate cert;
    QList<QSslCertificate> caCerts;
    const bool imported =
        QSslCertificate::importPkcs12(&file, &key, &cert, &caCerts, password.toUtf8());
    if (!imported) {
        return fail(QStringLiteral("Could not read the PKCS#12 bundle. The password may be "
                                   "wrong, or the file is not a valid bundle."),
                    false, /*badPassword=*/true);
    }

    // Present the FULL client chain, not just the leaf. importPkcs12 splits the
    // bundle into the leaf (cert) and the rest (caCerts = issuing CA/intermediates).
    // Setting only the leaf makes Qt/OpenSSL fail to build the local chain
    // (error 11: issuer not found) before it can present anything. The extra
    // certs belong on the CLIENT chain we send — NOT in the server-verification
    // trust store (putting a private client CA there breaks public-root servers).
    QList<QSslCertificate> clientChain;
    clientChain.append(cert);
    clientChain.append(caCerts);
    config.setLocalCertificateChain(clientChain);
    config.setPrivateKey(key);
    return TlsConfigResult{true, config, QString(), false};
}

TlsConfigResult buildPem(const ImmichConnectionSettings &conn, const QString &password,
                         QSslConfiguration config)
{
    const auto certs = QSslCertificate::fromPath(conn.clientCertPath, QSsl::Pem);
    if (certs.isEmpty() || certs.first().isNull()) {
        return fail(QStringLiteral("Could not read a client certificate from %1.")
                        .arg(conn.clientCertPath));
    }

    QByteArray keyData;
    QString error;
    if (!readFile(conn.clientKeyPath, keyData, error)) {
        return fail(error);
    }

    QSslKey key(keyData, QSsl::Rsa, QSsl::Pem, QSsl::PrivateKey, password.toUtf8());
    if (key.isNull()) {
        key = QSslKey(keyData, QSsl::Ec, QSsl::Pem, QSsl::PrivateKey, password.toUtf8());
    }
    if (key.isNull()) {
        const bool needsPassword = password.isEmpty();
        return fail(QStringLiteral("Could not read the private key from %1. The key may be "
                                   "encrypted and need a password, or the password is wrong.")
                        .arg(conn.clientKeyPath),
                    needsPassword, /*badPassword=*/!needsPassword);
    }

    config.setLocalCertificate(certs.first());
    config.setPrivateKey(key);
    return TlsConfigResult{true, config, QString(), false};
}

} // namespace

TlsConfigResult build(const ImmichConnectionSettings &conn, const QString &password)
{
    QSslConfiguration config = QSslConfiguration::defaultConfiguration();

    TlsConfigResult result{true, config, QString(), false};

    if (conn.hasClientCert()) {
        result = conn.certFormat == TlsCertFormat::Pkcs12
                     ? buildPkcs12(conn, password, config)
                     : buildPem(conn, password, config);
        if (!result.ok) {
            return result;
        }
        config = result.config;
    }

    if (!conn.caCertPath.trimmed().isEmpty()) {
        // Accept both PEM and DER: .cer/.crt files are frequently DER-encoded,
        // and QSsl::Pem alone silently yields nothing for them.
        QByteArray caBytes;
        QString readError;
        if (!readFile(conn.caCertPath.trimmed(), caBytes, readError)) {
            return fail(readError);
        }
        QList<QSslCertificate> cas = QSslCertificate::fromData(caBytes, QSsl::Pem);
        if (cas.isEmpty() || cas.first().isNull()) {
            cas = QSslCertificate::fromData(caBytes, QSsl::Der);
        }
        if (cas.isEmpty() || cas.first().isNull()) {
            return fail(QStringLiteral("Could not read a CA certificate from %1 "
                                       "(tried PEM and DER).")
                            .arg(conn.caCertPath));
        }
        auto merged = config.caCertificates();
        merged.append(cas);
        config.setCaCertificates(merged);
        result.config = config;
    }

    return result;
}

} // namespace Aurora::TlsConfig
