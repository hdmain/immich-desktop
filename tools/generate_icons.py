#!/usr/bin/env python3
"""Rasterize brand SVGs into the PNG/ICO filenames used by the app."""

from __future__ import annotations

import sys
from pathlib import Path

from PIL import Image
from PyQt6.QtCore import QByteArray, QRectF, QSize, Qt
from PyQt6.QtGui import QImage, QPainter
from PyQt6.QtSvg import QSvgRenderer
from PyQt6.QtWidgets import QApplication


ROOT = Path(__file__).resolve().parents[1]
RESOURCES = ROOT / "resources"

LOGO_SVG = RESOURCES / "immich-computer-logo-v3.svg"
INLINE_SVG = RESOURCES / "immich-inline-computer.svg"

# Canonical names after rename (preferred if present).
LOGO_SVG_CANON = RESOURCES / "immich-logo.svg"
INLINE_SVG_CANON = RESOURCES / "immich-logo-inline.svg"


def render_svg(svg_path: Path, width: int, height: int) -> Image.Image:
    data = QByteArray(svg_path.read_bytes())
    renderer = QSvgRenderer(data)
    if not renderer.isValid():
        raise RuntimeError(f"Invalid SVG: {svg_path}")

    image = QImage(QSize(width, height), QImage.Format.Format_ARGB32_Premultiplied)
    image.fill(Qt.GlobalColor.transparent)
    painter = QPainter(image)
    painter.setRenderHint(QPainter.RenderHint.Antialiasing, True)
    painter.setRenderHint(QPainter.RenderHint.SmoothPixmapTransform, True)
    renderer.render(painter, QRectF(0, 0, width, height))
    painter.end()

    bits = image.bits()
    bits.setsize(image.sizeInBytes())
    raw = bytes(bits)
    return Image.frombuffer(
        "RGBA", (width, height), raw, "raw", "BGRA", image.bytesPerLine(), 1
    ).copy()


def fill_square(img: Image.Image, size: int, padding_ratio: float = 0.02) -> Image.Image:
    """Crop transparent margins and scale content to fill a square canvas."""
    alpha = img.getchannel("A")
    bbox = alpha.getbbox()
    if not bbox:
        return Image.new("RGBA", (size, size), (0, 0, 0, 0))

    cropped = img.crop(bbox)
    pad = max(1, int(round(size * padding_ratio)))
    target = max(1, size - 2 * pad)
    scale = min(target / cropped.width, target / cropped.height)
    new_w = max(1, int(round(cropped.width * scale)))
    new_h = max(1, int(round(cropped.height * scale)))
    scaled = cropped.resize((new_w, new_h), Image.Resampling.LANCZOS)

    canvas = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    canvas.paste(scaled, ((size - new_w) // 2, (size - new_h) // 2), scaled)
    return canvas


def main() -> int:
    app = QApplication(sys.argv)

    logo_svg = LOGO_SVG_CANON if LOGO_SVG_CANON.exists() else LOGO_SVG
    inline_svg = INLINE_SVG_CANON if INLINE_SVG_CANON.exists() else INLINE_SVG

    if not logo_svg.exists():
        raise SystemExit(f"Missing logo SVG: {logo_svg}")
    if not inline_svg.exists():
        raise SystemExit(f"Missing inline SVG: {inline_svg}")

    # Rename sources to canonical names used going forward.
    if logo_svg == LOGO_SVG and not LOGO_SVG_CANON.exists():
        LOGO_SVG.replace(LOGO_SVG_CANON)
        logo_svg = LOGO_SVG_CANON
        print(f"Renamed {LOGO_SVG.name} -> {LOGO_SVG_CANON.name}")
    if inline_svg == INLINE_SVG and not INLINE_SVG_CANON.exists():
        INLINE_SVG.replace(INLINE_SVG_CANON)
        inline_svg = INLINE_SVG_CANON
        print(f"Renamed {INLINE_SVG.name} -> {INLINE_SVG_CANON.name}")

    # App icon (square) — crop padding so the mark fills the canvas.
    master = fill_square(render_svg(logo_svg, 2048, 2048), 1024, padding_ratio=0.02)
    master.save(RESOURCES / "immich-logo.png", format="PNG")
    print("Wrote immich-logo.png")

    for size in (16, 32, 48, 64, 128, 256, 512):
        resized = master.resize((size, size), Image.Resampling.LANCZOS)
        out = RESOURCES / f"immich-logo-{size}.png"
        resized.save(out, format="PNG")
        print(f"Wrote {out.name}")

    ico_sizes = [(16, 16), (24, 24), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)]
    master.save(
        RESOURCES / "immich.ico",
        format="ICO",
        sizes=ico_sizes,
    )
    print("Wrote immich.ico")

    # Top-bar banner — crop padding so the mark fills the available height.
    banner_src = render_svg(inline_svg, 1584, 532)
    alpha = banner_src.getchannel("A")
    bbox = alpha.getbbox()
    if bbox:
        banner_src = banner_src.crop(bbox)
    banner_h = 256
    banner_w = max(1, int(round(banner_src.width * (banner_h / banner_src.height))))
    banner = banner_src.resize((banner_w, banner_h), Image.Resampling.LANCZOS)
    banner.save(RESOURCES / "immich-logo-inline-light.png", format="PNG")
    print(f"Wrote immich-logo-inline-light.png ({banner_w}x{banner_h})")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
