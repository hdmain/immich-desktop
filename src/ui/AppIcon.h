#pragma once

#include <QIcon>

namespace Aurora {

struct ThemePalette;

QIcon applicationIcon();
QIcon applicationIconForPalette(const ThemePalette &palette);

} // namespace Aurora
