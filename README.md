# immich desktop

A scalable Qt 6 Widgets foundation for a modern commercial desktop application.

> **Unofficial project:** This is a fan-made desktop client and is not
> affiliated with, maintained by, or endorsed by the official Immich project.
> Immich and its logo belong to their respective owners.

## Included

- Modular main window with sidebar, top bar, animated page transitions, and module workspace
- Immich-style dated timeline with compact rows and grouped sparse days
- Built-in streaming video player with seek, volume, and buffering controls
- Viewport-based thumbnail loading with threaded decoding and persistent disk cache
- Immich server URL and API key configuration with a built-in connection test
- Central `ThemeManager` with live light, dark, and custom themes
- Editable background, panel, button, and accent colors
- Local `QSettings` persistence for theme and custom palette
- Automatic update detection from GitHub Releases with platform-aware installation
- Reusable animated navigation and color controls
- Windows installers with application icon and license agreement

## Requirements

- CMake 3.21 or newer
- Qt 6.2 or newer with Widgets, SVG, Network, Multimedia, and MultimediaWidgets
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
sudo apt install build-essential cmake ninja-build libgl1-mesa-dev qt6-base-dev \
  libqt6svg6-dev qt6-image-formats-plugins qt6-multimedia-dev
```

On Fedora:

```bash
sudo dnf install gcc-c++ cmake ninja-build qt6-qtbase-devel qt6-qtsvg-devel \
  qt6-qtmultimedia-devel
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

## Installers and releases

The `Build installers` GitHub Actions workflow automatically creates:

- Windows NSIS installer (`.exe`)
- Windows WiX installer (`.msi`)
- Debian/Ubuntu package (`.deb`)
- Portable Linux AppImage (`.AppImage`)
- Linux Snap package (`.snap`)

Every push to `main` or `master`, every pull request, and every manual workflow
run produces downloadable workflow artifacts. Pushing a version tag also
creates a GitHub Release and attaches the installers:

```bash
git tag v0.1.0
git push origin v0.1.0
```

Keep the tag synchronized with the version declared by
`project(immich VERSION ...)` in `CMakeLists.txt`.

### Snap Store

Builds publish to the Snap Store when the repository secret
`SNAPCRAFT_STORE_CREDENTIALS` is configured:

1. Install Snapcraft and log in: `snapcraft login`
2. Register the name once: `snapcraft register immich-desktop`
3. Export credentials: `snapcraft export-login --snaps=immich-desktop --acls package_access,package_push,package_update,package_release -`
4. Add the exported blob as the GitHub Actions secret `SNAPCRAFT_STORE_CREDENTIALS`

- Pushes to `main` / `master` → **edge** channel
- Version tags `v*` → **stable** channel

Install from the store:

```bash
sudo snap install immich-desktop
```

Or build locally:

```bash
snapcraft
sudo snap install *.snap --dangerous
```

## Updates

The in-app updater checks
[`hdmain/immich-desktop`](https://github.com/hdmain/immich-desktop) GitHub
Releases and selects the matching package automatically:

- Windows: NSIS `.exe`, with `.msi` as fallback
- Linux AppImage builds: replace the running AppImage and relaunch
- Linux Snap builds: refresh via `snap refresh`
- Other Linux installs: Debian `.deb` (via elevated `dpkg`), with AppImage fallback

Automatic checks run once per day on startup and can be toggled on the Updates
page. Download, skip, and install-and-restart are available there as well.

## Structure

- `src/core` — palettes, settings persistence, and centralized appearance management
- `src/ui` — application shell and composition
- `src/ui/pages` — independently extensible application pages
- `src/ui/widgets` — reusable modern controls
- `resources` — application assets and future fonts/icons

Appearance settings are stored in the current user's platform-specific settings location.
Immich connection details are stored in the same local settings store. Create an API
key in Immich with `user.read`, `asset.read`, and `asset.view` permissions, then enter
the server URL and key under **Settings → Immich Server**.
