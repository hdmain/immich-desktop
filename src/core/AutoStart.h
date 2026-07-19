#pragma once

#include <QString>

namespace Aurora {

class AutoStart final {
public:
    static bool isSupported();
    static bool isEnabled();
    // Registers or removes OS login autostart. Returns false on failure.
    static bool setEnabled(bool enabled);
    static QString lastError();

private:
    static QString &errorSlot();
    static QString launchCommand();
    static QString registryValueName();
    static QString desktopFilePath();
};

} // namespace Aurora
