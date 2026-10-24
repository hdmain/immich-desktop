#include "core/CustomHeaders.h"

#include <QNetworkRequest>
#include <QtTest>

#include <initializer_list>

using Aurora::CustomHeaders::HeaderList;

namespace {

HeaderList headerList(std::initializer_list<QPair<QString, QString>> entries)
{
    HeaderList list;
    for (const auto &entry : entries)
        list.append(entry);
    return list;
}

} // namespace

class CustomHeadersTest final : public QObject {
    Q_OBJECT

private slots:
    void acceptsValidNames_data()
    {
        QTest::addColumn<QString>("name");
        QTest::newRow("plain") << QStringLiteral("X-Auth-Token");
        QTest::newRow("cloudflare") << QStringLiteral("CF-Access-Client-Id");
        QTest::newRow("tokens") << QStringLiteral("X_Test!#$%&'*+-.^`|~");
    }

    void acceptsValidNames()
    {
        QFETCH(QString, name);
        QVERIFY(Aurora::CustomHeaders::isValidName(name));
    }

    void rejectsInvalidNames_data()
    {
        QTest::addColumn<QString>("name");
        QTest::newRow("empty") << QString();
        QTest::newRow("space") << QStringLiteral("X Auth");
        QTest::newRow("colon") << QStringLiteral("X-Auth:");
        QTest::newRow("lineBreak") << QStringLiteral("X-Auth\r\nInjected");
        QTest::newRow("trailingLineFeed") << QStringLiteral("X-Auth\n");
        QTest::newRow("nonAscii") << QStringLiteral("X-认证");
    }

    void rejectsInvalidNames()
    {
        QFETCH(QString, name);
        QVERIFY(!Aurora::CustomHeaders::isValidName(name));
    }

    void rejectsLineBreaksInValues()
    {
        QVERIFY(Aurora::CustomHeaders::isValidValue(QStringLiteral("Bearer abc")));
        QVERIFY(!Aurora::CustomHeaders::isValidValue(QStringLiteral("a\rb")));
        QVERIFY(!Aurora::CustomHeaders::isValidValue(QStringLiteral("a\nb")));
    }

    void rejectsReservedNames()
    {
        QVERIFY(Aurora::CustomHeaders::isReservedName(QStringLiteral("Host")));
        QVERIFY(Aurora::CustomHeaders::isReservedName(QStringLiteral("content-length")));
        QVERIFY(Aurora::CustomHeaders::isReservedName(QStringLiteral("Connection")));
        QVERIFY(Aurora::CustomHeaders::isReservedName(QStringLiteral("Transfer-Encoding")));
        QVERIFY(Aurora::CustomHeaders::isReservedName(QStringLiteral("Range")));
        QVERIFY(Aurora::CustomHeaders::isReservedName(QStringLiteral("x-api-key")));
        QVERIFY(!Aurora::CustomHeaders::isReservedName(QStringLiteral("X-Host")));
    }

    void normalizeTrimsAndDropsEmptyRows()
    {
        const HeaderList input = headerList({
            {QStringLiteral("  X-Auth  "), QStringLiteral("  token  ")},
            {QString(), QString()},
            {QStringLiteral("X-Empty"), QString()},
        });
        const HeaderList normalized = Aurora::CustomHeaders::normalize(input);
        QCOMPARE(normalized.size(), qsizetype(2));
        QCOMPARE(normalized.at(0).first, QStringLiteral("X-Auth"));
        QCOMPARE(normalized.at(0).second, QStringLiteral("token"));
        QCOMPARE(normalized.at(1).first, QStringLiteral("X-Empty"));
        QVERIFY(normalized.at(1).second.isEmpty());
    }

    void normalizeKeepsLastDuplicate()
    {
        const HeaderList input = headerList({
            {QStringLiteral("X-Auth"), QStringLiteral("old")},
            {QStringLiteral("x-auth"), QStringLiteral("new")},
        });
        const HeaderList normalized = Aurora::CustomHeaders::normalize(input);
        QCOMPARE(normalized.size(), qsizetype(1));
        QCOMPARE(normalized.first().second, QStringLiteral("new"));
    }

    void serializationRoundTrip()
    {
        const HeaderList headers = headerList({
            {QStringLiteral("X-Auth"), QStringLiteral("a:b,c")},
            {QStringLiteral("X-Chinese"), QStringLiteral("值")},
        });
        const HeaderList parsed = Aurora::CustomHeaders::deserialize(
            Aurora::CustomHeaders::serialize(headers));
        QVERIFY(parsed == headers);
    }

    void deserializesMalformedJsonAsEmpty()
    {
        QVERIFY(Aurora::CustomHeaders::deserialize(QStringLiteral("not json")).isEmpty());
        QVERIFY(Aurora::CustomHeaders::deserialize(
                    QStringLiteral("{\"name\":\"x\"}")).isEmpty());
        QVERIFY(Aurora::CustomHeaders::deserialize(QString()).isEmpty());
        QVERIFY(Aurora::CustomHeaders::deserialize(
                    QStringLiteral("[{\"name\":\"x\",\"value\":\"y\"}, 3]")).isEmpty());
    }

    void validationReportsProblems()
    {
        QVERIFY(Aurora::CustomHeaders::validationError(headerList({
            {QStringLiteral("X-Ok"), QStringLiteral("1")}})).isEmpty());
        QVERIFY(!Aurora::CustomHeaders::validationError(headerList({
            {QString(), QStringLiteral("1")}})).isEmpty());
        QVERIFY(!Aurora::CustomHeaders::validationError(headerList({
            {QStringLiteral("Bad Name"), QStringLiteral("1")}})).isEmpty());
        QVERIFY(!Aurora::CustomHeaders::validationError(headerList({
            {QStringLiteral("Host"), QStringLiteral("1")}})).isEmpty());
        QVERIFY(!Aurora::CustomHeaders::validationError(headerList({
            {QStringLiteral("X-Ok"), QStringLiteral("a\nb")}})).isEmpty());
        QVERIFY(!Aurora::CustomHeaders::validationError(headerList({
            {QStringLiteral("x-api-key"), QStringLiteral("1")}})).isEmpty());
    }

    void applyToSetsHeadersAndSkipsReserved()
    {
        QNetworkRequest request(QUrl(QStringLiteral("https://example.com")));
        Aurora::CustomHeaders::applyTo(request, headerList({
            {QStringLiteral("X-Auth"), QStringLiteral("token")},
            {QStringLiteral("Host"), QStringLiteral("evil.example")},
        }));
        QCOMPARE(request.rawHeader("X-Auth"), QByteArray("token"));
        QVERIFY(request.rawHeader("Host").isEmpty());

        Aurora::CustomHeaders::applyTo(request, headerList({
            {QStringLiteral("X-Embedded"), QStringLiteral("a\rb")},
        }));
        QVERIFY(request.rawHeader("X-Embedded").isEmpty());

        Aurora::CustomHeaders::applyTo(request, headerList({
            {QStringLiteral("Bad Name"), QStringLiteral("x")},
        }));
        QVERIFY(!request.hasRawHeader("Bad Name"));

        Aurora::CustomHeaders::applyTo(request, headerList({
            {QStringLiteral("x-api-key"), QStringLiteral("evil")},
        }));
        QVERIFY(!request.hasRawHeader("x-api-key"));
    }

    void applyToSetsEmptyValue()
    {
        QNetworkRequest request(QUrl(QStringLiteral("https://example.com")));
        Aurora::CustomHeaders::applyTo(request, headerList({
            {QStringLiteral("X-Empty"), QString()},
        }));
        QVERIFY(request.hasRawHeader("X-Empty"));
        QVERIFY(request.rawHeader("X-Empty").isEmpty());
    }
};

QTEST_GUILESS_MAIN(CustomHeadersTest)
#include "CustomHeadersTest.moc"
