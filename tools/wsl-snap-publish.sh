#!/usr/bin/env bash
set -euo pipefail

# Helper to verify Snapcraft credentials in WSL and export a copy for
# GitHub Actions secret SNAPCRAFT_STORE_CREDENTIALS.
#
# Usage (from WSL):
#   bash tools/wsl-snap-publish.sh

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
CREDS_JSON="${HOME}/.local/share/snapcraft/credentials.json"
EXPORT_FILE="${HOME}/snapcraft-immich-desktop.creds"

if [[ ! -f "$CREDS_JSON" ]]; then
  echo "Missing $CREDS_JSON"
  echo "Log in first:"
  echo "  snapcraft login"
  echo "  snapcraft export-login --snaps=immich-desktop \\"
  echo "    --acls package_access,package_push,package_update,package_release \\"
  echo "    $CREDS_JSON"
  exit 1
fi

echo "== credentials =="
python3 - <<'PY'
from pathlib import Path
import json
p = Path.home() / ".local/share/snapcraft" / "credentials.json"
text = p.read_text()
print("size", len(text))
data = json.loads(text)
print("keys", sorted(data.keys()))
PY

cp -f "$CREDS_JSON" "$EXPORT_FILE"
# Also drop a copy next to the Windows repo for pasting into GitHub secrets.
if [[ "$ROOT" == /mnt/* ]]; then
  cp -f "$CREDS_JSON" "$ROOT/.snapcraft-store-credentials.json"
  echo "Wrote $ROOT/.snapcraft-store-credentials.json"
fi
echo "Wrote $EXPORT_FILE"
echo
echo "Next:"
echo "  1) Put the file contents into GitHub secret SNAPCRAFT_STORE_CREDENTIALS"
echo "  2) Tag a release (vX.Y.Z) — CI builds and publishes amd64+arm64 to stable"
echo "  3) Or locally: bash tools/build-and-publish-snap.sh stable"
