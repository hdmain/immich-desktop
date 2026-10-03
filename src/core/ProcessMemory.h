#pragma once

namespace Aurora {

// Ask the OS to return unused heap pages after large pixmap/cache drops.
// Safe to call when the window is hidden, minimized, or idling in the tray.
void trimProcessMemory();

} // namespace Aurora
