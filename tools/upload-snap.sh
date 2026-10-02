#!/usr/bin/env bash
set -euo pipefail

# Upload one or more local snap packages to the Snap Store.
# Usage:
#   tools/upload-snap.sh [channel] [snap-file...]
# Defaults to stable and every immich-desktop_*.snap under dist-snap/.

CHANNEL="${1:-stable}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"

if [[ $# -gt 1 ]]; then
  shift
  SNAPS=("$@")
else
  mapfile -t SNAPS < <(compgen -G "${ROOT}/dist-snap/immich-desktop_"*.snap | sort || true)
fi

if [[ ${#SNAPS[@]} -eq 0 ]]; then
  echo "No snap packages found under dist-snap/" >&2
  exit 1
fi

for SNAP in "${SNAPS[@]}"; do
  if [[ ! -f "$SNAP" ]]; then
    echo "Snap not found: $SNAP" >&2
    exit 1
  fi
  echo "Uploading $(basename "$SNAP") to $CHANNEL"
  snapcraft upload "$SNAP" --release="$CHANNEL"
done

echo
echo "== Store status =="
snapcraft status immich-desktop
echo
echo "== Revisions =="
snapcraft revisions immich-desktop | head -20
