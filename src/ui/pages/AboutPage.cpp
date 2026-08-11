#include "ui/pages/AboutPage.h"

#include "AppVersion.h"

#include <QClipboard>
#include <QDesktopServices>
#include <QFrame>
#include <QGuiApplication>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>

namespace Aurora {

namespace {

QFrame *card(QWidget *parent, QVBoxLayout **out = nullptr)
{
    auto *c = new QFrame(parent);
    c->setProperty("card", true);
    auto *l = new QVBoxLayout(c);
    l->setContentsMargins(20, 18, 20, 20);
    l->setSpacing(10);
    if (out)
        *out = l;
    return c;
}

QLabel *headingLabel(const QString &text, QWidget *parent)
{
    auto *w = new QLabel(text, parent);
    w->setProperty("section", true);
    w->setWordWrap(true);
    return w;
}

QLabel *bodyLabel(const QString &text, QWidget *parent, bool richText = false)
{
    auto *w = new QLabel(text, parent);
    w->setProperty("subheading", true);
    w->setWordWrap(true);
    w->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse);
    if (richText) {
        w->setTextFormat(Qt::RichText);
        w->setOpenExternalLinks(true);
    }
    return w;
}

QLabel *monoLabel(const QString &text, QWidget *parent)
{
    auto *w = new QLabel(text, parent);
    w->setProperty("subheading", true);
    w->setWordWrap(true);
    w->setTextInteractionFlags(Qt::TextSelectableByMouse);
    w->setStyleSheet(QStringLiteral("font-family: monospace;"));
    return w;
}

} // namespace

AboutPage::AboutPage(QWidget *parent)
    : QWidget(parent)
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(4, 6, 4, 4);
    root->setSpacing(8);

    auto *title = new QLabel(tr("About"), this);
    title->setProperty("heading", true);
    auto *subtitle = new QLabel(
        tr("Unofficial, fan-made desktop client for your self-hosted Immich library."),
        this);
    subtitle->setProperty("subheading", true);
    subtitle->setWordWrap(true);
    root->addWidget(title);
    root->addWidget(subtitle);
    root->addSpacing(10);

    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setFrameShape(QFrame::NoFrame);

    auto *content = new QWidget(scroll);
    auto *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(0, 0, 8, 0);
    contentLayout->setSpacing(14);

    {
        QVBoxLayout *l = nullptr;
        auto *c = card(content, &l);
        l->addWidget(headingLabel(tr("What is this?"), c));
        l->addWidget(bodyLabel(
            tr("immich desktop is an unofficial, fan-made desktop app for "
               "<a href=\"https://immich.app\">Immich</a> — the self-hosted photo and video manager. "
               "It connects to your own Immich server via an API key and lets you browse your timeline, "
               "search, explore people &amp; places, upload/download, stream video, and keep browsing "
               "offline through a local thumbnail cache and queued uploads. "
               "It is not affiliated with, maintained by, or endorsed by the official Immich project."),
            c, true));
        contentLayout->addWidget(c);
    }

    {
        QVBoxLayout *l = nullptr;
        auto *c = card(content, &l);
        l->addWidget(headingLabel(tr("Who makes it?"), c));
        l->addWidget(bodyLabel(
            tr("Built and maintained by the Immich Desktop community, led by "
               "<a href=\"https://github.com/hdmain\">hdmain</a>. "
               "Contributions, bug reports, and ideas are welcome on GitHub."),
            c, true));
        auto *links = new QLabel(content);
        links->setProperty("subheading", true);
        links->setWordWrap(true);
        links->setTextFormat(Qt::RichText);
        links->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse);
        links->setOpenExternalLinks(true);
        links->setText(
            tr("<a href=\"https://github.com/%1\">github.com/%1</a> · "
               "<a href=\"https://github.com/%1/issues\">Issues</a> · "
               "<a href=\"https://github.com/%1/releases\">Releases</a>")
                .arg(QString::fromLatin1(Config::GitHubRepository)));
        l->addWidget(links);
        contentLayout->addWidget(c);
    }

    {
        QVBoxLayout *l = nullptr;
        auto *c = card(content, &l);
        l->addWidget(headingLabel(tr("Tech stack"), c));
        l->addWidget(bodyLabel(
            tr("Everything this build is made with — frameworks, packaging, and key libraries:"),
            c));
        const QStringList items = {
            QStringLiteral("Qt 6 — Core, Gui, Widgets, Network, Svg"),
            QStringLiteral("Qt Multimedia (+ MultimediaWidgets) — video playback & audio (QMediaPlayer, QVideoSink)"),
            QStringLiteral("GStreamer / FFmpeg — media backends behind Qt Multimedia (selected at runtime)"),
            QStringLiteral("CMake + Ninja — build system"),
            QStringLiteral("C++20 — language standard"),
            QStringLiteral("QSettings (INI) — AppSettings persistence (appearance, updates, server, playback)"),
            QStringLiteral("QSystemTrayIcon — tray, notifications, close-to-tray"),
            QStringLiteral("QLocalServer / QLocalSocket — SingleInstance (single-instance + raise)"),
            QStringLiteral("Inter font family — bundled UI typeface (via FontLoader)"),
            QStringLiteral("Lucide icons — SVG icon set"),
            QStringLiteral("linuxdeploy + linuxdeploy-plugin-qt — AppImage bundling"),
            QStringLiteral("CPack — DEB / NSIS / WiX (MSI) installers"),
            QStringLiteral("Snapcraft (core24 + gnome extension) — Snap package"),
            QStringLiteral("Flatpak (org.kde.Platform / Sdk) — Flatpak manifest"),
            QStringLiteral("GitHub Actions — CI for Windows/Linux/Snap + tagged releases"),
            QStringLiteral("NSIS & WiX Toolset — Windows installers"),
        };
        auto *bullets = new QLabel(c);
        bullets->setProperty("subheading", true);
        bullets->setWordWrap(true);
        bullets->setTextInteractionFlags(Qt::TextSelectableByMouse);
        QString html = QStringLiteral("<ul style=\"margin:0; padding-left:18px;\">");
        for (const auto &it : items)
            html += QStringLiteral("<li>") + it.toHtmlEscaped() + QStringLiteral("</li>");
        html += QStringLiteral("</ul>");
        bullets->setText(html);
        bullets->setTextFormat(Qt::RichText);
        l->addWidget(bullets);
        contentLayout->addWidget(c);
    }

    {
        QVBoxLayout *l = nullptr;
        auto *c = card(content, &l);
        l->addWidget(headingLabel(tr("Get in touch & collaborate"), c));
        l->addWidget(bodyLabel(
            tr("Want to build this together, report a bug, or propose a feature? "
               "Reach out on GitHub or Discord — contributions of all sizes are welcome."),
            c));
        auto *discordRow = new QLabel(c);
        discordRow->setProperty("subheading", true);
        discordRow->setWordWrap(true);
        discordRow->setTextInteractionFlags(Qt::TextSelectableByMouse);
        discordRow->setText(tr("Discord: diegosanche3"));
        l->addWidget(discordRow);

        auto *copyBtn = new QPushButton(tr("Copy Discord handle"), c);
        copyBtn->setCursor(Qt::PointingHandCursor);
        QObject::connect(copyBtn, &QPushButton::clicked, copyBtn, [copyBtn] {
            QGuiApplication::clipboard()->setText(QStringLiteral("diegosanche3"));
            const QString orig = copyBtn->text();
            copyBtn->setText(QObject::tr("Copied!"));
            QTimer::singleShot(1500, copyBtn, [copyBtn, orig] { copyBtn->setText(orig); });
        });

        auto *ghBtn = new QPushButton(tr("Open GitHub"), c);
        ghBtn->setCursor(Qt::PointingHandCursor);
        ghBtn->setProperty("primary", true);
        QObject::connect(ghBtn, &QPushButton::clicked, c, [] {
            QDesktopServices::openUrl(
                QUrl(QStringLiteral("https://github.com/%1")
                         .arg(QString::fromLatin1(Config::GitHubRepository))));
        });

        auto *btnRow = new QHBoxLayout;
        btnRow->setSpacing(10);
        btnRow->addWidget(ghBtn);
        btnRow->addWidget(copyBtn);
        btnRow->addStretch();
        l->addLayout(btnRow);

        l->addWidget(monoLabel(QStringLiteral("Discord handle: diegosanche3"), c));
        contentLayout->addWidget(c);
    }

    {
        QVBoxLayout *l = nullptr;
        auto *c = card(content, &l);
        l->addWidget(headingLabel(tr("Version"), c));
        const QString ver = QString::fromLatin1(Config::ApplicationVersion);
        const QString snapVer = qEnvironmentVariable("SNAP_VERSION");
        const QString shown = !snapVer.isEmpty()
                                  ? tr("v%1 (Snap %2)").arg(ver, snapVer)
                                  : tr("v%1").arg(ver);
        l->addWidget(bodyLabel(shown, c));
        l->addWidget(bodyLabel(
            tr("Licensed under MIT. See LICENSE.txt. Immich is a trademark of its owners; "
               "this app is an independent project and does not redistribute official Immich branding."),
            c));
        contentLayout->addWidget(c);
    }

    contentLayout->addStretch();
    scroll->setWidget(content);
    root->addWidget(scroll, 1);
}

} // namespace Aurora
