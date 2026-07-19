#!/usr/bin/env bash
set -euo pipefail

CREDS_JSON="${HOME}/.local/share/snapcraft/credentials.json"
EXPORT_FILE="${HOME}/snapcraft-immich-desktop.creds"
SNAP_DIR="${HOME}/immich-desktop-snap"
DOWNLOAD_DIR="${HOME}/immich-desktop-snap-artifacts"

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

# Newer Snapcraft accepts credentials.json via SNAPCRAFT_STORE_CREDENTIALS
# for some flows; also keep a copy for GitHub secret setup from Windows.
cp -f "$CREDS_JSON" "$EXPORT_FILE"
echo "Wrote $EXPORT_FILE"

echo "== waiting not needed; upload helper =="
mkdir -p "$DOWNLOAD_DIR"
