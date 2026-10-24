#include "core/AppSettings.h"
#include "core/TlsConfig.h"
#include "core/VideoStreamServer.h"

#include <QProcess>
#include <QSslConfiguration>
#include <QSslKey>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest>

using namespace Aurora;

class TlsConfigTest : public QObject {
    Q_OBJECT

private:
    QTemporaryDir m_dir;
    QString m_p12;     // client cert + key, password-protected PKCS#12
    QString m_pemCert; // client cert PEM
    QString m_pemKey;  // client key PEM (unencrypted)
    QString m_pemKeyEnc; // client key PEM (encrypted with "secret")
    QString m_caCert;  // standalone CA cert PEM
    static constexpr const char *kPassword = "secret";

    bool openssl(const QStringList &args)
    {
        QProcess proc;
        proc.start(QStringLiteral("openssl"), args);
        if (!proc.waitForFinished(15000)) {
            return false;
        }
        return proc.exitStatus() == QProcess::NormalExit && proc.exitCode() == 0;
    }

    QString path(const QString &name) const { return m_dir.filePath(name); }

private slots:
    void initTestCase()
    {
        if (QStandardPaths::findExecutable(QStringLiteral("openssl")).isEmpty()) {
            QSKIP("openssl not found on PATH");
        }
        QVERIFY(m_dir.isValid());

        m_pemCert = path(QStringLiteral("client.crt"));
        m_pemKey = path(QStringLiteral("client.key"));
        m_pemKeyEnc = path(QStringLiteral("client.enc.key"));
        m_p12 = path(QStringLiteral("client.p12"));
        m_caCert = path(QStringLiteral("ca.crt"));

        // Unencrypted client key + self-signed cert.
        QVERIFY2(openssl({QStringLiteral("req"), QStringLiteral("-x509"),
                          QStringLiteral("-newkey"), QStringLiteral("rsa:2048"),
                          QStringLiteral("-nodes"),
                          QStringLiteral("-keyout"), m_pemKey,
                          QStringLiteral("-out"), m_pemCert,
                          QStringLiteral("-days"), QStringLiteral("1"),
                          QStringLiteral("-subj"), QStringLiteral("/CN=immich-client")}),
                 "failed to generate client cert/key");

        // Encrypted copy of the client key.
        QVERIFY2(openssl({QStringLiteral("rsa"),
                          QStringLiteral("-in"), m_pemKey,
                          QStringLiteral("-aes256"),
                          QStringLiteral("-passout"), QStringLiteral("pass:%1").arg(kPassword),
                          QStringLiteral("-out"), m_pemKeyEnc}),
                 "failed to encrypt client key");

        // PKCS#12 bundle (cert + key) protected by the password.
        QVERIFY2(openssl({QStringLiteral("pkcs12"), QStringLiteral("-export"),
                          QStringLiteral("-inkey"), m_pemKey,
                          QStringLiteral("-in"), m_pemCert,
                          QStringLiteral("-out"), m_p12,
                          QStringLiteral("-passout"), QStringLiteral("pass:%1").arg(kPassword)}),
                 "failed to build pkcs12 bundle");

        // Standalone CA cert (a second self-signed cert).
        QVERIFY2(openssl({QStringLiteral("req"), QStringLiteral("-x509"),
                          QStringLiteral("-newkey"), QStringLiteral("rsa:2048"),
                          QStringLiteral("-nodes"),
                          QStringLiteral("-keyout"), path(QStringLiteral("ca.key")),
                          QStringLiteral("-out"), m_caCert,
                          QStringLiteral("-days"), QStringLiteral("1"),
                          QStringLiteral("-subj"), QStringLiteral("/CN=immich-test-ca")}),
                 "failed to generate CA cert");
    }

    void noCertNoCa_returnsDefaultOkConfig()
    {
        ImmichConnectionSettings conn;
        const auto result = TlsConfig::build(conn, QString());
        QVERIFY(result.ok);
        QVERIFY(result.config.localCertificate().isNull());
        QCOMPARE(result.config.caCertificates().size(),
                 QSslConfiguration::defaultConfiguration().caCertificates().size());
    }

    void pkcs12_correctPassword_loadsCertAndKey()
    {
        ImmichConnectionSettings conn;
        conn.certFormat = TlsCertFormat::Pkcs12;
        conn.clientCertPath = m_p12;
        const auto result = TlsConfig::build(conn, QString::fromLatin1(kPassword));
        QVERIFY2(result.ok, qPrintable(result.error));
        QVERIFY(!result.config.localCertificate().isNull());
        QVERIFY(!result.config.privateKey().isNull());
    }

    void pkcs12_wrongPassword_fails()
    {
        ImmichConnectionSettings conn;
        conn.certFormat = TlsCertFormat::Pkcs12;
        conn.clientCertPath = m_p12;
        const auto result = TlsConfig::build(conn, QStringLiteral("wrong-password"));
        QVERIFY(!result.ok);
        QVERIFY2(result.error.contains(QStringLiteral("password"), Qt::CaseInsensitive),
                 qPrintable(result.error));
        QVERIFY(!result.needsPassword);
        QVERIFY(result.badPassword); // re-prompt, not toast
    }

    void pkcs12_emptyPassword_needsPassword()
    {
        ImmichConnectionSettings conn;
        conn.certFormat = TlsCertFormat::Pkcs12;
        conn.clientCertPath = m_p12;
        const auto result = TlsConfig::build(conn, QString());
        QVERIFY(!result.ok);
        QVERIFY(result.needsPassword);
    }

    void pem_certPlusKey_loads()
    {
        ImmichConnectionSettings conn;
        conn.certFormat = TlsCertFormat::Pem;
        conn.clientCertPath = m_pemCert;
        conn.clientKeyPath = m_pemKey;
        const auto result = TlsConfig::build(conn, QString());
        QVERIFY2(result.ok, qPrintable(result.error));
        QVERIFY(!result.config.localCertificate().isNull());
        QVERIFY(!result.config.privateKey().isNull());
    }

    void pemEncryptedKey_wrongPassword_fails()
    {
        ImmichConnectionSettings conn;
        conn.certFormat = TlsCertFormat::Pem;
        conn.clientCertPath = m_pemCert;
        conn.clientKeyPath = m_pemKeyEnc;
        const auto result = TlsConfig::build(conn, QStringLiteral("wrong-password"));
        QVERIFY(!result.ok);
        QVERIFY2(result.error.contains(QStringLiteral("key"), Qt::CaseInsensitive),
                 qPrintable(result.error));
        QVERIFY(result.badPassword); // re-prompt, not toast
    }

    void pemEncryptedKey_emptyPassword_needsPassword()
    {
        ImmichConnectionSettings conn;
        conn.certFormat = TlsCertFormat::Pem;
        conn.clientCertPath = m_pemCert;
        conn.clientKeyPath = m_pemKeyEnc;
        const auto result = TlsConfig::build(conn, QString());
        QVERIFY(!result.ok);
        QVERIFY(result.needsPassword);
    }

    void pemCertNoKey_fails()
    {
        ImmichConnectionSettings conn;
        conn.certFormat = TlsCertFormat::Pem;
        conn.clientCertPath = m_pemCert;
        conn.clientKeyPath = QString(); // cert configured, no key
        const auto result = TlsConfig::build(conn, QString());
        QVERIFY(!result.ok);
        QVERIFY(!result.error.isEmpty());
    }

    void missingCertFile_fails()
    {
        ImmichConnectionSettings conn;
        conn.certFormat = TlsCertFormat::Pem;
        conn.clientCertPath = path(QStringLiteral("does-not-exist.crt"));
        conn.clientKeyPath = m_pemKey;
        const auto result = TlsConfig::build(conn, QString());
        QVERIFY(!result.ok);
        QVERIFY2(result.error.contains(QStringLiteral("does-not-exist.crt")),
                 qPrintable(result.error));
        QVERIFY(!result.needsPassword); // missing file toasts, never re-prompts
        QVERIFY(!result.badPassword);
    }

    void customCa_isAddedToCaSet()
    {
        ImmichConnectionSettings conn;
        conn.caCertPath = m_caCert;
        const auto result = TlsConfig::build(conn, QString());
        QVERIFY2(result.ok, qPrintable(result.error));

        const auto defaultSize =
            QSslConfiguration::defaultConfiguration().caCertificates().size();
        QVERIFY(result.config.caCertificates().size() > defaultSize);

        const auto expected = QSslCertificate::fromPath(m_caCert);
        QVERIFY(!expected.isEmpty());
        QVERIFY(result.config.caCertificates().contains(expected.first()));
    }

    // --- origin host-match: the mTLS/custom-CA config applies to the Immich
    // origin only, never to a CDN/S3 redirect hop (spec Verification gate a). ---

    void originHost_matchesSameHttpsHost()
    {
        const QUrl api(QStringLiteral("https://immich.example.com"));
        QVERIFY(VideoStreamServer::isOriginHost(
            QUrl(QStringLiteral("https://immich.example.com/api/assets/x/video")), api));
    }

    void originHost_cdnHost_notMatched()
    {
        const QUrl api(QStringLiteral("https://immich.example.com"));
        // Pre-signed CDN/S3 redirect target — different host, must NOT get the CA.
        QVERIFY(!VideoStreamServer::isOriginHost(
            QUrl(QStringLiteral("https://s3.amazonaws.com/bucket/object?sig=abc")), api));
    }

    void originHost_httpScheme_notMatched()
    {
        const QUrl api(QStringLiteral("https://immich.example.com"));
        QVERIFY(!VideoStreamServer::isOriginHost(
            QUrl(QStringLiteral("http://immich.example.com/api")), api));
    }

    // --- API host-match: custom headers follow the Immich host even over plain
    // HTTP, but must never reach a CDN/S3 redirect hop (spec Verification gate a). ---

    void apiHost_sameHostHttp_matched()
    {
        const QUrl api(QStringLiteral("https://immich.example.com"));
        QVERIFY(VideoStreamServer::isApiHost(
            QUrl(QStringLiteral("http://immich.example.com/api/assets/x")), api));
    }

    void apiHost_sameHostHttps_matched()
    {
        const QUrl api(QStringLiteral("https://immich.example.com"));
        QVERIFY(VideoStreamServer::isApiHost(
            QUrl(QStringLiteral("https://immich.example.com/api/x")), api));
    }

    void apiHost_mixedCase_matched()
    {
        const QUrl api(QStringLiteral("https://immich.example.com"));
        QVERIFY(VideoStreamServer::isApiHost(
            QUrl(QStringLiteral("https://IMMICH.Example.COM/x")), api));
    }

    void apiHost_cdnHost_notMatched()
    {
        const QUrl api(QStringLiteral("https://immich.example.com"));
        QVERIFY(!VideoStreamServer::isApiHost(
            QUrl(QStringLiteral("https://s3.amazonaws.com/bucket/object")), api));
    }

    void apiHost_suffixHost_notMatched()
    {
        const QUrl api(QStringLiteral("https://immich.example.com"));
        QVERIFY(!VideoStreamServer::isApiHost(
            QUrl(QStringLiteral("https://immich.example.com.evil.com/x")), api));
    }

    void apiHost_userInfoTrick_notMatched()
    {
        const QUrl api(QStringLiteral("https://immich.example.com"));
        QVERIFY(!VideoStreamServer::isApiHost(
            QUrl(QStringLiteral("https://immich.example.com@evil.com/x")), api));
    }

    void apiHost_emptyBase_notMatched()
    {
        QVERIFY(!VideoStreamServer::isApiHost(
            QUrl(QStringLiteral("https://immich.example.com/x")), QUrl()));
    }
};

QTEST_MAIN(TlsConfigTest)
#include "TlsConfigTest.moc"
