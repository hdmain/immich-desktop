# immich desktop

A scalable Qt 6 Widgets foundation for a modern commercial desktop application.

> **Unofficial project:** This is a fan-made desktop client and is not
> affiliated with, maintained by, or endorsed by the official Immich project.
> Immich and its logo belong to their respective owners.

## Included

- Modular main window with sidebar, top bar, animated page transitions, and module workspace
- Central `ThemeManager` with live light, dark, and custom themes
- Editable background, panel, button, and accent colors
- Local `QSettings` persistence for theme and custom palette
- Reusable animated navigation and color controls

## Requirements

- CMake 3.21 or newer
- Qt 6.5 or newer with the Widgets and SVG components
- A C++20 compiler

## Build

On Windows, use the included launcher. It detects the installed Qt MinGW kit,
builds the project, deploys the required Qt DLLs, and starts the application:

```powershell
powershell -ExecutionPolicy Bypass -File .\run.ps1
```

After the first build, `.\run.ps1 -NoBuild` skips compilation.

For other toolchains, configure Qt's location manually:

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:/Qt/6.8.0/msvc2022_64"
cmake --build build --config Release
```

The CMake Windows build automatically runs `windeployqt`, so the generated
executable can start without a separately configured Qt `PATH`.

### Linux

Install the build dependencies on Debian or Ubuntu:

```bash
sudo apt install build-essential cmake ninja-build qt6-base-dev libqt6svg6-dev
```

On Fedora:

```bash
sudo dnf install gcc-c++ cmake ninja-build qt6-qtbase-devel qt6-qtsvg-devel
```

Then build and run on X11 or Wayland:

```bash
chmod +x run.sh
./run.sh
```

Use `./run.sh --no-build` after the first build. To install the executable,
desktop entry, and application icon:

```bash
cmake --install build-linux --prefix ~/.local
```

## Structure

- `src/core` — palettes, settings persistence, and centralized appearance management
- `src/ui` — application shell and composition
- `src/ui/pages` — independently extensible application pages
- `src/ui/widgets` — reusable modern controls
- `resources` — application assets and future fonts/icons

Appearance settings are stored in the current user's platform-specific settings location.
