#pragma once

#include <QList>

namespace Aurora {

class MediaTile;

// The tile an arrow key (or Home/End) moves to from `from` in a photo grid.
// tiles is the grid in display order; Left/Right step through it, Up/Down go to
// the nearest tile in the row above or below. Null when there's nowhere to go.
MediaTile *neighbourTile(const QList<MediaTile *> &tiles, const MediaTile *from, int key);

// Focuses the tile and scrolls its grid so it's in view.
void focusTile(MediaTile *tile);

} // namespace Aurora
