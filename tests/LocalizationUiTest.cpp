#include "core/AppSettings.h"
#include "core/FolderSyncService.h"
#include "core/ImmichClient.h"
#include "core/Localization.h"
#include "core/ThemeManager.h"
#include "core/UpdateManager.h"
#include "ui/MainWindow.h"
#include "ui/MainWindowHost.h"
#include "ui/pages/AppearancePage.h"

#include <QComboBox>
#include <QLabel>
#include <QPointer>
#include <QPushButton>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest>

class LocalizationUiTest : public QObject {
    Q_OBJECT

private slots:
    void changeLanguageFromSettings()
    {
        QTemporaryDir settings;
        QVERIFY(settings.isValid());
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settings.path());
        QStandardPaths::setTestModeEnabled(true);
        Aurora::Localization localization;
        localization.initialize(QStringLiteral("en"));
        Aurora::ThemeManager theme;
        theme.initialize();
        Aurora::UpdateManager updates;
        Aurora::ImmichClient client;
        Aurora::FolderSyncService sync(&client);
        QPointer<Aurora::ImmichClient> originalClient(&client);
        QPointer<Aurora::FolderSyncService> originalSync(&sync);
        Aurora::MainWindowHost host(&localization, &theme, &updates, &client, &sync);
        host.window()->resize(900, 620);
        host.window()->showAppearanceSettings();
        host.window()->show();

        for (const auto &language : {QStringLiteral("de"), QStringLiteral("fr"), QStringLiteral("pl"), QStringLiteral("zh_CN"), QStringLiteral("en")}) {
            QPointer<Aurora::MainWindow> oldWindow(host.window());
            auto *combo = host.window()->findChild<QComboBox *>(QStringLiteral("languageCombo"));
            QVERIFY(combo);
            const int index = combo->findData(language);
            QVERIFY(index >= 0);
            combo->setCurrentIndex(index);
            // The old UI must survive signal delivery, then refresh on the event loop.
            QVERIFY(oldWindow);
            QTRY_VERIFY(oldWindow.isNull());
            QCOMPARE(localization.language(), language);
            QCOMPARE(Aurora::AppSettings().loadLanguage(), language);
            QVERIFY(host.window()->isVisible());
            QCOMPARE(host.window()->size(), QSize(900, 620));
            auto *appearance = host.window()->findChild<Aurora::AppearancePage *>();
            QVERIFY(appearance);
            QTRY_VERIFY(appearance->isVisible());
            const QString expected = language == QStringLiteral("de") ? QStringLiteral("Darstellung")
                : language == QStringLiteral("fr") ? QStringLiteral("Apparence")
                : language == QStringLiteral("pl") ? QStringLiteral("Wygląd")
                : language == QStringLiteral("zh_CN") ? QStringLiteral("外观") : QStringLiteral("Appearance");
            bool headingFound = false;
            for (const auto *label : appearance->findChildren<QLabel *>())
                headingFound |= label->text() == expected;
            QVERIFY2(headingFound, qPrintable(expected));
            const QString upload = language == QStringLiteral("de") ? QStringLiteral("Hochladen")
                : language == QStringLiteral("fr") ? QStringLiteral("Importer")
                : language == QStringLiteral("pl") ? QStringLiteral("Wyślij")
                : language == QStringLiteral("zh_CN") ? QStringLiteral("上传") : QStringLiteral("Upload");
            bool uploadFound = false;
            for (const auto *button : host.window()->findChildren<QPushButton *>())
                uploadFound |= button->accessibleName() == upload && button->toolTip() == upload;
            QVERIFY2(uploadFound, qPrintable(upload));
            QVERIFY(originalClient);
            QVERIFY(originalSync);
            if (language == QStringLiteral("pl")) {
                const QString retry = QStringLiteral("Przerwano wysyłanie — nastąpi ponowna próba.");
                emit client.requestFailed(Aurora::ImmichClient::tr("Upload"), retry, true);
                bool retryFound = false;
                for (const auto *label : host.window()->findChildren<QLabel *>())
                    retryFound |= label->text() == retry + QStringLiteral(" (pozostało: 0)");
                QVERIFY(retryFound);
            }
        }
    }
};

QTEST_MAIN(LocalizationUiTest)
#include "LocalizationUiTest.moc"
