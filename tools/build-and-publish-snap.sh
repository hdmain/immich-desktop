#!/usr/bin/env bash
set -euo pipefail

SRC="/mnt/c/Users/makss/Desktop/python/immich-desktop"
DEST="${HOME}/immich-desktop-snap-build"
CHANNEL="${1:-stable}"

echo "== Preparing build tree =="
rm -rf "$DEST"
mkdir -p "$DEST"
rsync -a --delete \
  --exclude=.git \
  --exclude=build \
  --exclude=build-* \
  --exclude=dist \
  --exclude=dist-* \
  --exclude=.snapcraft-store-credentials.* \
  "$SRC/" "$DEST/"

cd "$DEST"
echo "Version: $(tr '\n' ' ' < CMakeLists.txt | sed -n 's/.*project([[:space:]]*immich[[:space:]]*VERSION[[:space:]]*\([0-9.]*\).*/\1/p')"

echo "== Building snap =="
snapcraft pack --destructive-mode

SNAP="$(ls -1 immich-desktop_*.snap | head -1)"
echo "Built: $SNAP ($(du -h "$SNAP" | cut -f1))"

echo "== Uploading to $CHANNEL =="
snapcraft upload "$SNAP" --release="$CHANNEL"

echo "== Store status =="
snapcraft status immich-desktop
snapcraft revisions immich-desktop | head -10
