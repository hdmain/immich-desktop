#!/usr/bin/env python3
"""Copy Snapcraft store credentials into the repo for GitHub secret setup."""
from pathlib import Path
import json
import os

src = Path.home() / ".local/share/snapcraft" / "credentials.json"
# Prefer repo root next to this script; fall back to env override.
repo = Path(__file__).resolve().parent.parent
dst = Path(os.environ.get("SNAPCRAFT_CREDS_OUT", repo / ".snapcraft-store-credentials.json"))

text = src.read_text()
data = json.loads(text)
print("keys", sorted(data.keys()))
print("size", len(text))
dst.write_text(text)
print("wrote", dst)
