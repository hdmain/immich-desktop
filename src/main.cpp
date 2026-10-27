#include "AppVersion.h"
#include "core/AppSettings.h"
#include "core/AutoStart.h"
#include "core/FolderSyncService.h"
#include "core/ImmichClient.h"
#include "core/Localization.h"
#include "core/TranslationUpdater.h"
#include "core/SingleInstance.h"
#include "core/ThemeManager.h"
#include "core/UpdateManager.h"
#include "ui/AppIcon.h"
#include "ui/FontLoader.h"
#include "ui/MainWindow.h"
#include "ui/MainWindowHost.h"

#include <QApplication>
#include <QFont>
#include <QEventLoop>
#include <QProgressDialog>
#include <QTimer>
#include <QGuiApplication>
#include <QStyleFactory>
#include <QSystemTrayIcon>

int main(int argc, char *argv[])
{
    // Avoid rounded 100/125/150% steps that make fonts and icons look soft/pixelated.
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

#if defined(Q_OS_LINUX)
    // Prefer GStreamer on Linux. FFmpeg's setSource blocks the UI thread while
    // probing Immich HTTP streams (freeze on play). Snap's launcher also forces
    // GStreamer. Video output uses QVideoSink software frames so we avoid
    // GStreamer's GL sink (EGL_BAD_CONTEXT on some Wayland/XWayland sessions).
    if (qEnvironmentVariableIsEmpty("QT_MEDIA_BACKEND"))
        qputenv("QT_MEDIA_BACKEND", "gstreamer");
#endif

    QApplication application(argc, argv);
    application.setOrganizationName(QStringLiteral("Immich"));
    application.setOrganizationDomain(QStringLiteral("immich.app"));
    application.setApplicationName(QStringLiteral("immich"));
    application.setApplicationVersion(
        QString::fromLatin1(Aurora::Config::ApplicationVersion));
#if defined(Q_OS_LINUX)
    if (const QString flatpakId = qEnvironmentVariable("FLATPAK_ID"); !flatpakId.isEmpty())
        QGuiApplication::setDesktopFileName(flatpakId);
    else
        QGuiApplication::setDesktopFileName(QStringLiteral("immich-desktop"));
#endif

    Aurora::Localization localization;
    localization.initialize(Aurora::AppSettings().loadLanguage());

    application.setWindowIcon(Aurora::applicationIcon());
    application.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));

    Aurora::SingleInstance singleInstance(QStringLiteral("immich-desktop"));
    if (singleInstance.activateExistingInstance())
        return 0;

    const bool launchedFromAutoStart =
        application.arguments().contains(QStringLiteral("--autostart"));
    Aurora::TranslationUpdater translationUpdater;
    application.setQuitOnLastWindowClosed(false);
    {
        // Fetch before building the UI so this launch uses the latest catalogs.
        // The updater's overall five-second deadline guarantees offline fallback.
        QProgressDialog progress(QObject::tr("Updating translations…"), QString(), 0, 0);
        progress.setWindowTitle(QStringLiteral("immich"));
        progress.setCancelButton(nullptr);
        QEventLoop ready;
        QObject::connect(&translationUpdater, &Aurora::TranslationUpdater::finished,
                         &ready, &QEventLoop::quit);
        if (!launchedFromAutoStart)
            QTimer::singleShot(300, &progress, &QWidget::show);
        translationUpdater.refresh();
        ready.exec();
    }
    application.setQuitOnLastWindowClosed(true);
    localization.initialize(Aurora::AppSettings().loadLanguage());

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

    Aurora::AppSettings::activateDefaultProfile();
    Aurora::UpdateManager updateManager;
    int exitCode = 0;
    bool firstSession = true;
    QByteArray windowGeometry;
    do {
        Aurora::ImmichClient immichClient;
        Aurora::FolderSyncService folderSync(&immichClient);

        Aurora::MainWindowHost windows(&localization, &themeManager, &updateManager,
                                       &immichClient, &folderSync, &translationUpdater);
        QObject::connect(&singleInstance, &Aurora::SingleInstance::activationRequested,
                         &windows, [&windows] { windows.window()->raiseToFront(); });

        const bool startHidden =
            firstSession && launchedFromAutoStart && Aurora::AppSettings().loadWindow().closeToTray &&
            QSystemTrayIcon::isSystemTrayAvailable();
        if (!windowGeometry.isEmpty())
            windows.window()->restoreGeometry(windowGeometry);
        if (startHidden)
            windows.window()->hide();
        else
            windows.window()->show();

        if (firstSession) {
            windows.window()->scheduleStartupUpdateCheck();
            windows.window()->scheduleGitHubStarPrompt();
        }

        exitCode = application.exec();
        windowGeometry = windows.window()->saveGeometry();
        firstSession = false;
    } while ([&] {
        if (exitCode != Aurora::AppSettings::ProfileSwitchExitCode)
            return false;
        return Aurora::AppSettings::setActiveProfile(
            application.property("nextProfile").toString());
    }());
    return exitCode == Aurora::AppSettings::ProfileSwitchExitCode ? 0 : exitCode;
}
