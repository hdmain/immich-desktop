#!/usr/bin/env bash
set -euo pipefail

SNAP="/mnt/c/Users/makss/Desktop/python/immich-desktop/dist-snap/immich-desktop_0.1.7_amd64.snap"
CHANNEL="${1:-stable}"

if [ ! -f "$SNAP" ]; then
  echo "Snap not found: $SNAP" >&2
  exit 1
fi

echo "Uploading $(basename "$SNAP") to $CHANNEL"
snapcraft upload "$SNAP" --release="$CHANNEL"
echo
echo "== Store status =="
snapcraft status immich-desktop
echo
echo "== Revisions =="
snapcraft revisions immich-desktop | head -10
