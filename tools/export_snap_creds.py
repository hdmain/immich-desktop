#!/usr/bin/env python3
from pathlib import Path
import json
src = Path.home() / ".local/share/snapcraft" / "credentials.json"
dst = Path("/mnt/c/Users/makss/Desktop/python/immich-desktop/.snapcraft-store-credentials.json")
text = src.read_text()
data = json.loads(text)
print("keys", sorted(data.keys()))
print("size", len(text))
dst.write_text(text)
print("wrote", dst)
