#include "AppVersion.h"
#include "core/ThemeManager.h"
#include "core/UpdateManager.h"
#include "ui/MainWindow.h"

#include <QApplication>
#include <QIcon>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    application.setOrganizationName(QStringLiteral("Immich"));
    application.setOrganizationDomain(QStringLiteral("immich.app"));
    application.setApplicationName(QStringLiteral("immich"));
    application.setApplicationVersion(
        QString::fromLatin1(Aurora::Config::ApplicationVersion));
    application.setWindowIcon(QIcon(QStringLiteral(":/branding/immich-logo.png")));
    application.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));

    Aurora::ThemeManager themeManager;
    themeManager.initialize();

    Aurora::UpdateManager updateManager;

    Aurora::MainWindow window(&themeManager, &updateManager);
    window.show();

    return application.exec();
}
