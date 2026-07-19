#include "AppVersion.h"
#include "core/ImmichClient.h"
#include "core/SingleInstance.h"
#include "core/ThemeManager.h"
#include "core/UpdateManager.h"
#include "ui/AppIcon.h"
#include "ui/FontLoader.h"
#include "ui/MainWindow.h"

#include <QApplication>
#include <QFont>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    application.setOrganizationName(QStringLiteral("Immich"));
    application.setOrganizationDomain(QStringLiteral("immich.app"));
    application.setApplicationName(QStringLiteral("immich"));
    application.setApplicationVersion(
        QString::fromLatin1(Aurora::Config::ApplicationVersion));
    application.setWindowIcon(Aurora::applicationIcon());
    application.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));

    Aurora::SingleInstance singleInstance(QStringLiteral("immich-desktop"));
    if (singleInstance.activateExistingInstance())
        return 0;

    const QStringList fontFamilies = Aurora::loadApplicationFonts();
    QFont appFont(fontFamilies.contains(QStringLiteral("Inter"))
                      ? QStringLiteral("Inter")
                      : (fontFamilies.isEmpty() ? QStringLiteral("Sans Serif")
                                                : fontFamilies.first()));
    appFont.setStyleHint(QFont::SansSerif);
    appFont.setPointSize(10);
    application.setFont(appFont);

    Aurora::ThemeManager themeManager;
    themeManager.initialize();

    Aurora::UpdateManager updateManager;
    Aurora::ImmichClient immichClient;

    Aurora::MainWindow window(&themeManager, &updateManager, &immichClient);
    QObject::connect(&singleInstance, &Aurora::SingleInstance::activationRequested, &window,
                     &Aurora::MainWindow::raiseToFront);
    window.show();

    return application.exec();
}
