#include "core/Localization.h"

#include <QCoreApplication>
#include <QtTest>

class LocalizationTest : public QObject {
    Q_OBJECT

private slots:
    void availableCatalogs()
    {
        const auto languages = Aurora::Localization::availableLanguages();
        QVERIFY(languages.contains(QStringLiteral("en")));
        QVERIFY(languages.contains(QStringLiteral("de")));
        QVERIFY(languages.contains(QStringLiteral("fr")));
        QVERIFY(languages.contains(QStringLiteral("pl")));
        QVERIFY(languages.contains(QStringLiteral("zh_CN")));
    }

    void selection_data()
    {
        QTest::addColumn<QString>("preference");
        QTest::addColumn<QStringList>("system");
        QTest::addColumn<QString>("expected");
        QTest::addColumn<QString>("library");
        QTest::newRow("regional system") << QString() << QStringList{"de-CH", "en"}
            << QString("de") << QString("Mediathek");
        QTest::newRow("script system") << QString() << QStringList{"fr-Latn-CA", "en"}
            << QString("fr") << QString("Photothèque");
        QTest::newRow("explicit override") << QString("fr") << QStringList{"de-DE"}
            << QString("fr") << QString("Photothèque");
        QTest::newRow("explicit English") << QString("en") << QStringList{"de-DE"}
            << QString("en") << QString("Library");
        QTest::newRow("English preferred") << QString() << QStringList{"en-US", "de-DE"}
            << QString("en") << QString("Library");
        QTest::newRow("second preference") << QString() << QStringList{"ja-JP", "fr-FR"}
            << QString("fr") << QString("Photothèque");
        QTest::newRow("unsupported system") << QString() << QStringList{"ja-JP"}
            << QString("en") << QString("Library");
        QTest::newRow("stale setting") << QString("removed") << QStringList{"de-DE"}
            << QString("de") << QString("Mediathek");
        QTest::newRow("Polish system") << QString() << QStringList{"pl-PL", "en"}
            << QString("pl") << QString("Biblioteka");
        QTest::newRow("Polish override") << QString("pl") << QStringList{"en-US"}
            << QString("pl") << QString("Biblioteka");
        QTest::newRow("Chinese system") << QString() << QStringList{"zh-CN", "en"}
            << QString("zh_CN") << QString("媒体库");
        QTest::newRow("Chinese override") << QString("zh_CN") << QStringList{"en-US"}
            << QString("zh_CN") << QString("媒体库");
    }

    void selection()
    {
        QFETCH(QString, preference);
        QFETCH(QStringList, system);
        QFETCH(QString, expected);
        QFETCH(QString, library);
        Aurora::Localization localization;
        localization.initialize(preference, system);
        QCOMPARE(localization.language(), expected);
        QCOMPARE(QCoreApplication::translate("Aurora::Sidebar", "Library"), library);
        if (Aurora::Localization::availableLanguages().contains(preference))
            QCOMPARE(QLocale().language(), QLocale(expected).language());
        else
            QCOMPARE(QLocale().name(), QLocale::system().name());
    }

    void pluralAndFallback()
    {
        Aurora::Localization localization;
        localization.initialize(QStringLiteral("de"));
        QCOMPARE(QCoreApplication::translate("Aurora::LibraryPage", "%n item(s)", nullptr, 1),
                 QStringLiteral("1 Element"));
        QCOMPARE(QCoreApplication::translate("Aurora::LibraryPage", "%n item(s)", nullptr, 2),
                 QStringLiteral("2 Elemente"));
        // An unfinished translation must use the original English source.
        QCOMPARE(QCoreApplication::translate("Aurora::AboutPage", "What is this?"),
                 QStringLiteral("What is this?"));
        localization.initialize(QStringLiteral("en"));
        QCOMPARE(QCoreApplication::translate("Aurora::LibraryPage", "%n item(s)", nullptr, 1),
                 QStringLiteral("1 item"));
        QCOMPARE(QCoreApplication::translate("Aurora::LibraryPage", "%n item(s)", nullptr, 0),
                 QStringLiteral("0 items"));
        QCOMPARE(QCoreApplication::translate("Aurora::Sidebar", "Library"), QStringLiteral("Library"));
    }

    void polishPlurals_data()
    {
        QTest::addColumn<int>("count");
        QTest::addColumn<QString>("expected");
        QTest::newRow("zero") << 0 << QStringLiteral("0 elementów");
        QTest::newRow("one") << 1 << QStringLiteral("1 element");
        QTest::newRow("two") << 2 << QStringLiteral("2 elementy");
        QTest::newRow("five") << 5 << QStringLiteral("5 elementów");
        QTest::newRow("twelve") << 12 << QStringLiteral("12 elementów");
        QTest::newRow("twenty-one") << 21 << QStringLiteral("21 elementów");
        QTest::newRow("twenty-two") << 22 << QStringLiteral("22 elementy");
        QTest::newRow("hundred-twelve") << 112 << QStringLiteral("112 elementów");
    }

    void polishPlurals()
    {
        QFETCH(int, count);
        QFETCH(QString, expected);
        Aurora::Localization localization;
        localization.initialize(QStringLiteral("pl"));
        QCOMPARE(QCoreApplication::translate("Aurora::LibraryPage", "%n item(s)", nullptr, count), expected);
    }
};

QTEST_GUILESS_MAIN(LocalizationTest)
#include "LocalizationTest.moc"
