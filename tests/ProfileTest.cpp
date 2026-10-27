#include "core/AppSettings.h"
#include "core/OfflineStore.h"
#include "core/ImmichClient.h"
#include "ui/pages/ConnectionPage.h"
#include <QComboBox>
#include <QScrollArea>
#include "core/ThumbnailCache.h"
#include "core/UploadQueueStore.h"
#include "ui/widgets/TopBar.h"
#include "ui/FontLoader.h"
#include <QFont>
#include "core/ThemeManager.h"

#include <QDir>
#include <QFile>
#include <QPushButton>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest>

using namespace Aurora;

class ProfileTest final : public QObject {
    Q_OBJECT
    QTemporaryDir m_settings;

private slots:
    void initTestCase()
    {
        QVERIFY(m_settings.isValid());
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_settings.path());
        QStandardPaths::setTestModeEnabled(true);
    }

    void migrateAndIsolateAccounts()
    {
        QSettings legacy(QSettings::IniFormat, QSettings::UserScope,
                         QStringLiteral("Immich"), QStringLiteral("immich"));
        legacy.setValue("immich/serverUrl", "https://same.example");
        legacy.setValue("immich/apiKey", "old-account-key");
        legacy.setValue("immich/localServerUrl", "http://local.example");
        legacy.setValue("folderSync/enabled", true);
        legacy.setValue("folderSync/folderPath", "old-folder");
        legacy.setValue("appearance/minimalistUi", true);
        legacy.sync();

        AppSettings personal;
        QCOMPARE(AppSettings::profiles().size(), 1);
        QCOMPARE(personal.loadImmichConnection().apiKey, QStringLiteral("old-account-key"));
        QCOMPARE(personal.loadImmichConnection().localServerUrl, QStringLiteral("http://local.example"));
        QCOMPARE(personal.loadFolderSync().folderPaths, QStringList{"old-folder"});
        QVERIFY(personal.loadFolderSync().enabled);
        QVERIFY(!legacy.contains("immich/apiKey"));
        const QString personalId = AppSettings::activeProfileId();
        const QString personalCache = AppSettings::profileCacheDirectory();
        const QString workId = AppSettings::createProfile(QStringLiteral("Work"));
        QVERIFY(!workId.isEmpty());
        QVERIFY(AppSettings::setActiveProfile(workId));
        AppSettings work;
        QVERIFY(!work.loadImmichConnection().isConfigured());
        QVERIFY(!work.loadFolderSync().enabled);
        QVERIFY(work.loadFolderSync().folderPaths.isEmpty());
        QVERIFY(work.loadAppearance().minimalistUi); // Appearance is application-wide.
        QVERIFY(AppSettings::profileCacheDirectory() != personalCache);
        auto connection = work.loadImmichConnection();
        connection.serverUrl = QStringLiteral("https://same.example");
        connection.localServerUrl = QStringLiteral("http://work.local");
        connection.apiKey = QStringLiteral("work-account-key");
        connection.customHeaders.append({QStringLiteral("X-Account"), QStringLiteral("work")});
        connection.certFormat = TlsCertFormat::Pem;
        connection.clientCertPath = QStringLiteral("work.pem");
        connection.clientKeyPath = QStringLiteral("work.key");
        connection.caCertPath = QStringLiteral("work-ca.pem");
        connection.keyPassword = QStringLiteral("password");
        connection.rememberKeyPassword = true;
        work.saveImmichConnection(connection);
        FolderSyncSettings sync;
        sync.folderPaths = {QStringLiteral("work-folder")};
        work.saveFolderSync(sync);
        QCOMPARE(AppSettings().loadImmichConnection().apiKey, connection.apiKey);
        QCOMPARE(AppSettings().loadImmichConnection().customHeaders, connection.customHeaders);
        QCOMPARE(AppSettings().loadImmichConnection().clientCertPath, connection.clientCertPath);
        QCOMPARE(AppSettings().loadImmichConnection().clientKeyPath, connection.clientKeyPath);
        QCOMPARE(AppSettings().loadImmichConnection().caCertPath, connection.caCertPath);
        QCOMPARE(AppSettings().loadImmichConnection().keyPassword, connection.keyPassword);
        connection.rememberKeyPassword = false;
        work.saveImmichConnection(connection);
        QVERIFY(AppSettings().loadImmichConnection().keyPassword.isEmpty());
        // An existing settings object stays bound to its original profile.
        QCOMPARE(personal.loadImmichConnection().apiKey, QStringLiteral("old-account-key"));
        QCOMPARE(personal.loadFolderSync().folderPaths, QStringList{"old-folder"});
        QVERIFY(AppSettings::setDefaultProfile(workId));
        QVERIFY(AppSettings::setActiveProfile(personalId));
        QCOMPARE(AppSettings::defaultProfileId(), workId);
        AppSettings::activateDefaultProfile();
        QCOMPARE(AppSettings::activeProfileId(), workId);
        QVERIFY(AppSettings::renameProfile(workId, QStringLiteral("Office")));
        QVERIFY(!AppSettings::deleteProfile(workId));
        QVERIFY(!AppSettings::setActiveProfile(QStringLiteral("../invalid")));
        QVERIFY(AppSettings::createProfile(QStringLiteral(" ")).isEmpty());
        QVERIFY(!AppSettings::renameProfile(workId, QStringLiteral(" ")));
        QVERIFY(AppSettings::setActiveProfile(personalId));
        QVERIFY(AppSettings::deleteProfile(workId));
        QCOMPARE(AppSettings::defaultProfileId(), personalId);
        QVERIFY(!AppSettings::setActiveProfile(workId));
        QCOMPARE(AppSettings::profiles().size(), 1);
        // Migration never repeats and does not resurrect a deleted account.
        QCOMPARE(AppSettings().loadImmichConnection().apiKey, QStringLiteral("old-account-key"));
    }

    void isolateCachesAndQueuesOnTheSameServer()
    {
        const QString original = AppSettings::activeProfileId();
        const QString first = AppSettings::createProfile(QStringLiteral("First"));
        const QString second = AppSettings::createProfile(QStringLiteral("Second"));
        QVERIFY(AppSettings::setActiveProfile(first));
        const QString firstData = AppSettings::profileDataDirectory();
        const QString firstCache = AppSettings::profileCacheDirectory();
        UploadQueueStore firstQueue;
        const QString queuedPath = m_settings.filePath(QStringLiteral("first.jpg"));
        QFile queuedFile(queuedPath);
        QVERIFY(queuedFile.open(QIODevice::WriteOnly));
        queuedFile.close();
        firstQueue.save({queuedPath});
        ThumbnailCache firstThumb;
        firstThumb.store(QStringLiteral("shared-id"), QByteArray("first-image"), QPixmap(2, 2));
        OfflineStore firstOffline;
        ImmichAsset asset;
        asset.id = QStringLiteral("first-only");
        firstOffline.saveLibrary(QStringLiteral("https://same.example"), {asset});

        QVERIFY(AppSettings::setActiveProfile(second));
        const QString secondData = AppSettings::profileDataDirectory();
        const QString secondCache = AppSettings::profileCacheDirectory();
        UploadQueueStore secondQueue;
        QVERIFY(secondQueue.load().isEmpty());
        ThumbnailCache secondThumb;
        QVERIFY(secondThumb.readDisk(QStringLiteral("shared-id")).isEmpty());
        secondThumb.store(QStringLiteral("shared-id"), QByteArray("second-image"), QPixmap(2, 2));
        OfflineStore secondOffline;
        QList<ImmichAsset> assets;
        QVERIFY(!secondOffline.loadLibrary(QStringLiteral("https://same.example"), &assets));
        QVERIFY(firstOffline.loadLibrary(QStringLiteral("https://same.example"), &assets));
        QCOMPARE(assets.first().id, asset.id);
        QCOMPARE(firstThumb.readDisk(QStringLiteral("shared-id")), QByteArray("first-image"));
        QCOMPARE(firstQueue.load(), QStringList{queuedPath});
        QVERIFY(AppSettings::setActiveProfile(first));
        QCOMPARE(UploadQueueStore().load(), QStringList{queuedPath});
        QCOMPARE(ThumbnailCache().readDisk(QStringLiteral("shared-id")), QByteArray("first-image"));
        QVERIFY(AppSettings::setActiveProfile(original));
        QVERIFY(AppSettings::deleteProfile(first));
        QVERIFY(AppSettings::deleteProfile(second));
        // Only UUID-scoped test directories are removed; never touch legacy data.
        QVERIFY(QDir(firstData).removeRecursively());
        QVERIFY(QDir(firstCache).removeRecursively());
        QVERIFY(QDir(secondData).removeRecursively());
        QVERIFY(QDir(secondCache).removeRecursively());
    }

    void connectionProfileControls()
    {
        const QString original = AppSettings::activeProfileId();
        const QString empty = AppSettings::createProfile(QStringLiteral("Test account"));
        QVERIFY(AppSettings::setActiveProfile(empty));
        {
            const auto families = loadApplicationFonts();
            QVERIFY(!families.isEmpty());
            qApp->setFont(QFont(families.first(), 10));
            ThemeManager theme;
            theme.initialize();
            ImmichClient client;
            ConnectionPage page(&client);
            page.resize(850, 620);
            page.show();
            auto *combo = page.findChild<QComboBox *>(QStringLiteral("connectionProfileCombo"));
            QVERIFY(combo);
            QCOMPARE(combo->currentData().toString(), empty);
            auto *create = page.findChild<QPushButton *>(QStringLiteral("newProfileButton"));
            QVERIFY(create);
            QTest::qWait(30);
            QVERIFY(create->isVisible());
            if (qEnvironmentVariableIsSet("IMMICH_PROFILE_SCREENSHOT"))
                QVERIFY(page.grab().save(qEnvironmentVariable("IMMICH_PROFILE_SCREENSHOT")));
        }
        QVERIFY(AppSettings::setActiveProfile(original));
        QVERIFY(AppSettings::deleteProfile(empty));
    }

    void deferSwitchUntilRuntimeTeardown()
    {
        const QString original = AppSettings::activeProfileId();
        const QString next = AppSettings::createProfile(QStringLiteral("Next"));
        QVERIFY(!next.isEmpty());
        QTimer::singleShot(0, this, [next] { AppSettings::requestProfileSwitch(next); });
        QTimer deadline;
        deadline.setSingleShot(true);
        connect(&deadline, &QTimer::timeout, this, [] { QCoreApplication::exit(-1); });
        deadline.start(3000);
        QCOMPARE(qApp->exec(), AppSettings::ProfileSwitchExitCode);
        deadline.stop();
        QCOMPARE(AppSettings::activeProfileId(), original);
        QCOMPARE(qApp->property("nextProfile").toString(), next);
        QVERIFY(AppSettings::setActiveProfile(next));
        ThemeManager theme;
        theme.initialize();
        TopBar bar(&theme);
        auto *button = bar.findChild<QPushButton *>(QStringLiteral("profileMenuButton"));
        QVERIFY(button);
        QCOMPARE(button->text(), QStringLiteral("Next"));
        QVERIFY(AppSettings::renameProfile(next, QStringLiteral("Renamed")));
        bar.refreshProfiles();
        QCOMPARE(button->text(), QStringLiteral("Renamed"));
        QVERIFY(AppSettings::setActiveProfile(original));
        QVERIFY(AppSettings::deleteProfile(next));
    }
};

QTEST_MAIN(ProfileTest)
#include "ProfileTest.moc"
