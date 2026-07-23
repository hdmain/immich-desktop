#include "AppVersion.h"
#include "core/AppSettings.h"
#include "core/AutoStart.h"
#include "core/ImmichClient.h"
#include "core/SingleInstance.h"
#include "core/ThemeManager.h"
#include "core/UpdateManager.h"
#include "ui/AppIcon.h"
#include "ui/FontLoader.h"
#include "ui/MainWindow.h"

#include <QApplication>
#include <QFont>
#include <QGuiApplication>
#include <QStyleFactory>
#include <QSystemTrayIcon>

int main(int argc, char *argv[])
{
    // Avoid rounded 100/125/150% steps that make fonts and icons look soft/pixelated.
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    QApplication application(argc, argv);
    application.setOrganizationName(QStringLiteral("Immich"));
    application.setOrganizationDomain(QStringLiteral("immich.app"));
    application.setApplicationName(QStringLiteral("immich"));
    application.setApplicationVersion(
        QString::fromLatin1(Aurora::Config::ApplicationVersion));
#if defined(Q_OS_LINUX)
    QGuiApplication::setDesktopFileName(QStringLiteral("immich-desktop"));
#endif

    application.setWindowIcon(Aurora::applicationIcon());
    application.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));

    Aurora::SingleInstance singleInstance(QStringLiteral("immich-desktop"));
    if (singleInstance.activateExistingInstance())
        return 0;

    const bool launchedFromAutoStart =
        application.arguments().contains(QStringLiteral("--autostart"));

    // Keep the OS autostart command pointed at the current executable after updates.
    if (Aurora::AppSettings().loadWindow().autoStart && Aurora::AutoStart::isSupported())
        Aurora::AutoStart::setEnabled(true);

    const QStringList fontFamilies = Aurora::loadApplicationFonts();
    QFont appFont(fontFamilies.contains(QStringLiteral("Inter"))
                      ? QStringLiteral("Inter")
                      : (fontFamilies.isEmpty() ? QStringLiteral("Segoe UI")
                                                : fontFamilies.first()));
    appFont.setStyleHint(QFont::SansSerif, QFont::PreferAntialias);
    appFont.setHintingPreference(QFont::PreferNoHinting);
    appFont.setStyleStrategy(static_cast<QFont::StyleStrategy>(
        QFont::PreferAntialias | QFont::PreferQuality));
    appFont.setPointSizeF(10.5);
    application.setFont(appFont);

    Aurora::ThemeManager themeManager;
    themeManager.initialize();

    Aurora::UpdateManager updateManager;
    Aurora::ImmichClient immichClient;

    Aurora::MainWindow window(&themeManager, &updateManager, &immichClient);
    QObject::connect(&singleInstance, &Aurora::SingleInstance::activationRequested, &window,
                     &Aurora::MainWindow::raiseToFront);

    const bool startHidden =
        launchedFromAutoStart && Aurora::AppSettings().loadWindow().closeToTray &&
        QSystemTrayIcon::isSystemTrayAvailable();
    if (startHidden)
        window.hide();
    else
        window.show();

    window.scheduleStartupUpdateCheck();
    window.scheduleGitHubStarPrompt();

    return application.exec();
}
