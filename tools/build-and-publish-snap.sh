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
# Destructive mode builds only one platform at a time; pick the host arch.
HOST_ARCH="$(dpkg --print-architecture 2>/dev/null || uname -m)"
case "$HOST_ARCH" in
  x86_64) HOST_ARCH=amd64 ;;
  aarch64) HOST_ARCH=arm64 ;;
esac
snapcraft pack --destructive-mode --platform "$HOST_ARCH"

mapfile -t SNAPS < <(ls -1 immich-desktop_*.snap)
echo "Built: ${SNAPS[*]}"

echo "== Uploading to $CHANNEL =="
for SNAP in "${SNAPS[@]}"; do
  snapcraft upload "$SNAP" --release="$CHANNEL"
done

echo "== Store status =="
snapcraft status immich-desktop
snapcraft revisions immich-desktop | head -10
