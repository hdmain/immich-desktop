#pragma once

#include <QIcon>

namespace Aurora {

struct ThemePalette;

QIcon applicationIcon();
QIcon applicationIconForPalette(const ThemePalette &palette);
QIcon trayUploadIcon(const ThemePalette &palette);
QIcon trayDownloadIcon(const ThemePalette &palette);

} // namespace Aurora
