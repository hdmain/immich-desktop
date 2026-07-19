<p align="center">
  <img src="resources/immich-logo-512.png" alt="immich desktop" width="128" height="128">
</p>

<h1 align="center">immich desktop</h1>

<p align="center">
  <strong>Unofficial Qt desktop client for Immich</strong><br>
  Browse, search, upload, download, and stream your self-hosted photo library.
</p>

<p align="center">
  <a href="https://github.com/hdmain/immich-desktop/releases/latest"><img src="https://img.shields.io/github/v/release/hdmain/immich-desktop?style=flat-square&label=release" alt="Release"></a>
  <a href="https://github.com/hdmain/immich-desktop/stargazers"><img src="https://img.shields.io/github/stars/hdmain/immich-desktop?style=flat-square" alt="Stars"></a>
  <a href="https://github.com/hdmain/immich-desktop/network/members"><img src="https://img.shields.io/github/forks/hdmain/immich-desktop?style=flat-square" alt="Forks"></a>
  <a href="https://github.com/hdmain/immich-desktop/issues"><img src="https://img.shields.io/github/issues/hdmain/immich-desktop?style=flat-square" alt="Issues"></a>
  <a href="https://github.com/hdmain/immich-desktop/releases"><img src="https://img.shields.io/github/downloads/hdmain/immich-desktop/total?style=flat-square" alt="Downloads"></a>
  <a href="https://snapcraft.io/immich-desktop"><img src="https://img.shields.io/badge/snap-immich--desktop-E95420?style=flat-square&logo=snapcraft&logoColor=white" alt="Snap"></a>
  <a href="LICENSE.txt"><img src="https://img.shields.io/badge/license-MIT-blue?style=flat-square" alt="License"></a>
</p>

> **Unofficial project.** Fan-made desktop client — not affiliated with, maintained by, or endorsed by the official Immich project. Immich and its logo belong to their respective owners.

## Showcase

![Library view](docs/screenshots/immich-desktop-library.png)

## Features

- **Library** — Immich-style timeline with compact rows and grouped days
- **Explore** — people, places, and discovery views
- **Search** — find photos and videos across your library
- **Upload & download** — send media to the server or save it locally
- **Video streaming** — built-in player with seek, volume, and buffering
- **Offline mode** — keep browsing with a local thumbnail/disk cache
- **Themes** — light, dark, and custom palettes
- **Desktop extras** — system tray, close-to-tray, autostart, single-instance

## Install

### Linux (Snap)

```bash
sudo snap install immich-desktop
```

<p align="center">
  <a href="https://snapcraft.io/immich-desktop">
    <img alt="Get it from the Snap Store" src="https://snapcraft.io/static/images/badges/en/snap-store-black.svg">
  </a>
</p>

### Other packages

Grab Windows (`.exe` / `.msi`), Linux `.deb`, or AppImage from the
[latest release](https://github.com/hdmain/immich-desktop/releases/latest).

## How to run

1. Install Immich Desktop from Snap or a [GitHub release](https://github.com/hdmain/immich-desktop/releases/latest).
2. Open the app and go to **Settings → Immich Server**.
3. Enter your Immich server URL and an API key with at least `user.read`, `asset.read`, and `asset.view`.
4. Test the connection, save, then browse **Library** or **Explore**.

```bash
# Snap
immich-desktop

# AppImage
chmod +x immich-desktop-x86_64.AppImage
./immich-desktop-x86_64.AppImage
```

## Roadmap

Plans can shift — track progress and ideas in
[Issues](https://github.com/hdmain/immich-desktop/issues).

### Shipped

- Library timeline with search, preview, trash, upload & download
- Explore: people and places
- Video streaming player
- Offline browsing via local disk cache + queued uploads when offline
- Themes (light / dark / custom), system tray, autostart, single-instance
- Packaging: Windows installers, `.deb`, AppImage, Snap Store

### Near term

- **Albums** — browse, create, add/remove assets, cover photos
- **Bulk actions** — multi-select favorite, archive, download, trash, album assign
- **Upload reliability** — pause/resume, per-file progress, clearer failure recovery
- **Explore polish** — map improvements, people naming/merge hooks, better empty states
- **Keyboard & UX** — shortcuts, smoother timeline scrolling, denser grid options

### Next

- **Sharing** — album links, partner sharing, copy public URLs from the desktop
- **Memories & faces** — Immich memories feed and richer face/person management
- **Smart library tools** — duplicates, archive views, advanced filters (type, camera, date)
- **Notifications** — tray alerts for finished uploads and available updates
- **Sync health** — connection status, last sync time, cache size controls

### Later

- Multi-account / multi-server profiles
- Full-resolution offline packs for selected albums
- External editor / “open with” workflows
- Wider Immich API parity as the server evolves
- Broader packaging (Flatpak / more architectures) as demand appears

## Project layout

- `src/core` — settings, Immich client, updates, tray helpers
- `src/ui` — shell, pages, and widgets
- `resources` — icons, fonts, desktop metadata
- `snap` — Snap packaging

## License

MIT — see [LICENSE.txt](LICENSE.txt).
