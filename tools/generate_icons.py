from pathlib import Path

from PIL import Image

root = Path(__file__).resolve().parents[1]
src = root / "resources" / "immich-logo.png"
img = Image.open(src).convert("RGBA")

w, h = img.size
side = min(w, h)
left = (w - side) // 2
top = (h - side) // 2
square = img.crop((left, top, left + side, top + side))

sizes = [(16, 16), (24, 24), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)]
square.save(root / "resources" / "immich.ico", format="ICO", sizes=sizes)

for size in (16, 32, 48, 64, 128, 256):
    square.resize((size, size), Image.Resampling.LANCZOS).save(
        root / "resources" / f"immich-logo-{size}.png"
    )

print("Wrote multi-size immich.ico and PNG icons")
