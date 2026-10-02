#!/usr/bin/env bash
set -euo pipefail

# Build immich-desktop snap and upload to the Snap Store.
# Usage: tools/build-and-publish-snap.sh [channel]
# Default channel: stable
#
# Intended to run inside WSL/Linux with snapcraft installed.
# Copies the Windows-mounted tree into ~ so snapcraft does not build
# from a slow /mnt/c path.

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
# Prefer a native Linux copy when the repo lives on /mnt/c (WSL).
if [[ "$ROOT" == /mnt/* ]]; then
  DEST="${HOME}/immich-desktop-snap-build"
else
  DEST="$ROOT"
fi
CHANNEL="${1:-stable}"

if [[ "$DEST" != "$ROOT" ]]; then
  echo "== Preparing build tree =="
  echo "Source: $ROOT"
  echo "Build:  $DEST"
  rm -rf "$DEST"
  mkdir -p "$DEST"
  rsync -a --delete \
    --exclude=.git \
    --exclude=build \
    --exclude=build-* \
    --exclude=dist \
    --exclude=dist-* \
    --exclude=.snapcraft-store-credentials.* \
    "$ROOT/" "$DEST/"
fi

cd "$DEST"
echo "Version: $(tr '\n' ' ' < CMakeLists.txt | sed -n 's/.*project([[:space:]]*immich[[:space:]]*VERSION[[:space:]]*\([0-9.]*\).*/\1/p')"

echo "== Building snap =="
# Destructive mode builds only one platform at a time; pick the host arch.
HOST_ARCH="$(dpkg --print-architecture 2>/dev/null || uname -m)"
case "$HOST_ARCH" in
  x86_64) HOST_ARCH=amd64 ;;
  aarch64) HOST_ARCH=arm64 ;;
esac
snapcraft pack --destructive-mode --platform "$HOST_ARCH"

mapfile -t SNAPS < <(ls -1 immich-desktop_*.snap)
echo "Built: ${SNAPS[*]}"

mkdir -p "$ROOT/dist-snap"
for SNAP in "${SNAPS[@]}"; do
  cp -f "$SNAP" "$ROOT/dist-snap/"
done

echo "== Uploading to $CHANNEL =="
for SNAP in "${SNAPS[@]}"; do
  snapcraft upload "$SNAP" --release="$CHANNEL"
done

echo "== Store status =="
snapcraft status immich-desktop
snapcraft revisions immich-desktop | head -10
