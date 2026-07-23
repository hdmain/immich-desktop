#include "core/AutoStart.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTextStream>

#ifdef Q_OS_WIN
#include <QSettings>
#endif

namespace Aurora {

QString &AutoStart::errorSlot()
{
    static QString error;
    return error;
}

QString AutoStart::lastError()
{
    return errorSlot();
}

QString AutoStart::registryValueName()
{
    return QStringLiteral("ImmichDesktop");
}

QString AutoStart::launchCommand()
{
#ifndef Q_OS_WIN
    // Prefer the stable snap launcher over the revision-specific binary path.
    const QString snapName = qEnvironmentVariable("SNAP_NAME");
    if (!snapName.isEmpty())
        return QStringLiteral("/snap/bin/%1 --autostart").arg(snapName);
    QString exe = QCoreApplication::applicationFilePath();
    exe.replace(u'\\', QStringLiteral("\\\\"));
    exe.replace(u'"', QStringLiteral("\\\""));
    return QStringLiteral("\"%1\" --autostart").arg(exe);
#else
    const QString exe = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
    return QStringLiteral("\"%1\" --autostart").arg(exe);
#endif
}

QString AutoStart::desktopFilePath()
{
    const QString config = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    return QDir(config).filePath(QStringLiteral("autostart/immich-desktop.desktop"));
}

bool AutoStart::isSupported()
{
#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
    return true;
#else
    return false;
#endif
}

bool AutoStart::isEnabled()
{
    errorSlot().clear();
#ifdef Q_OS_WIN
    QSettings run(QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\"
                                 "CurrentVersion\\Run"),
                  QSettings::NativeFormat);
    return run.contains(registryValueName());
#elif defined(Q_OS_LINUX)
    QFile file(desktopFilePath());
    if (!file.exists())
        return false;
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;
    const QString contents = QString::fromUtf8(file.readAll());
    return !contents.contains(QStringLiteral("Hidden=true"), Qt::CaseInsensitive) &&
           !contents.contains(QStringLiteral("X-GNOME-Autostart-enabled=false"),
                              Qt::CaseInsensitive);
#else
    return false;
#endif
}

bool AutoStart::setEnabled(bool enabled)
{
    errorSlot().clear();
    if (!isSupported()) {
        errorSlot() = QCoreApplication::translate("AutoStart",
                                                  "Autostart is not supported on this platform.");
        return false;
    }

#ifdef Q_OS_WIN
    QSettings run(QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\"
                                 "CurrentVersion\\Run"),
                  QSettings::NativeFormat);
    if (enabled) {
        run.setValue(registryValueName(), launchCommand());
    } else {
        run.remove(registryValueName());
    }
    run.sync();
    if (run.status() != QSettings::NoError) {
        errorSlot() = QCoreApplication::translate("AutoStart",
                                                  "Could not update Windows startup settings.");
        return false;
    }
    return true;
#elif defined(Q_OS_LINUX)
    const QString path = desktopFilePath();
    if (!enabled) {
        if (QFile::exists(path) && !QFile::remove(path)) {
            errorSlot() = QCoreApplication::translate("AutoStart",
                                                      "Could not remove autostart entry.");
            return false;
        }
        return true;
    }

    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        errorSlot() = QCoreApplication::translate("AutoStart",
                                                  "Could not create autostart entry.");
        return false;
    }

    const QString exePath = launchCommand();

    QTextStream out(&file);
    out << QStringLiteral("[Desktop Entry]\n");
    out << QStringLiteral("Type=Application\n");
    out << QStringLiteral("Version=1.0\n");
    out << QStringLiteral("Name=immich desktop\n");
    out << QStringLiteral("Comment=Immich desktop client\n");
    out << QStringLiteral("Exec=%1\n").arg(exePath);
    out << QStringLiteral("Icon=immich-desktop\n");
    out << QStringLiteral("Terminal=false\n");
    out << QStringLiteral("Categories=Graphics;Photography;\n");
    out << QStringLiteral("X-GNOME-Autostart-enabled=true\n");
    out << QStringLiteral("Hidden=false\n");
    file.close();
    return true;
#else
    Q_UNUSED(enabled);
    return false;
#endif
}

} // namespace Aurora
