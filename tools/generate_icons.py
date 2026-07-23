#!/usr/bin/env python3
"""Rasterize brand SVGs into the PNG/ICO filenames used by the app."""

from __future__ import annotations

import shutil
import sys
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont
from PyQt6.QtCore import QByteArray, QRectF, QSize, Qt
from PyQt6.QtGui import QImage, QPainter
from PyQt6.QtSvg import QSvgRenderer
from PyQt6.QtWidgets import QApplication


ROOT = Path(__file__).resolve().parents[1]
RESOURCES = ROOT / "resources"

# Dark-bg mark = light/white artwork (for dark UI).
# Light-bg mark = dark artwork (for light UI).
SVG_ON_DARK = RESOURCES / "icon_colored_dark_bg.svg"
SVG_ON_LIGHT = RESOURCES / "icon_colored.svg"
LOGO_SVG_CANON = RESOURCES / "immich-logo.svg"


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


def fill_square(img: Image.Image, size: int, padding_ratio: float = 0.01) -> Image.Image:
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


def load_banner_font(size: int) -> ImageFont.ImageFont:
    candidates = [
        RESOURCES / "fonts" / "Inter-SemiBold.ttf",
        RESOURCES / "fonts" / "Inter-Bold.ttf",
        RESOURCES / "fonts" / "Inter-Medium.ttf",
    ]
    for path in candidates:
        if path.exists():
            return ImageFont.truetype(str(path), size=size)
    return ImageFont.load_default()


def make_inline_banner(
    icon: Image.Image, height: int, text_rgba: tuple[int, int, int, int]
) -> Image.Image:
    mark = icon.resize((height, height), Image.Resampling.LANCZOS)
    text = "immich desktop"
    font = load_banner_font(int(height * 0.42))
    scratch = Image.new("RGBA", (1, 1), (0, 0, 0, 0))
    draw = ImageDraw.Draw(scratch)
    bbox = draw.textbbox((0, 0), text, font=font)
    text_w = bbox[2] - bbox[0]
    text_h = bbox[3] - bbox[1]

    gap = int(height * 0.18)
    width = mark.width + gap + text_w + int(height * 0.08)
    banner = Image.new("RGBA", (width, height), (0, 0, 0, 0))
    banner.paste(mark, (0, 0), mark)

    draw = ImageDraw.Draw(banner)
    x = mark.width + gap
    y = (height - text_h) // 2 - bbox[1]
    draw.text((x, y), text, font=font, fill=text_rgba)
    return banner


def write_icon_set(master: Image.Image, variant: str = "") -> None:
    """variant '' -> immich-logo.png; 'on-light' -> immich-logo-on-light.png"""
    if variant:
        master_name = f"immich-logo-{variant}.png"
        size_fmt = f"immich-logo-{variant}-{{size}}.png"
    else:
        master_name = "immich-logo.png"
        size_fmt = "immich-logo-{size}.png"

    master.save(RESOURCES / master_name, format="PNG")
    print(f"Wrote {master_name}")

    for size in (16, 32, 48, 64, 128, 256, 512):
        resized = master.resize((size, size), Image.Resampling.LANCZOS)
        out = RESOURCES / size_fmt.format(size=size)
        resized.save(out, format="PNG")
        print(f"Wrote {out.name}")


def main() -> int:
    app = QApplication(sys.argv)

    if not SVG_ON_DARK.exists():
        raise SystemExit(f"Missing {SVG_ON_DARK}")
    if not SVG_ON_LIGHT.exists():
        raise SystemExit(f"Missing {SVG_ON_LIGHT}")

    # Canonical packaging icon = on-dark (white mark).
    shutil.copyfile(SVG_ON_DARK, LOGO_SVG_CANON)
    print(f"Synced {SVG_ON_DARK.name} -> {LOGO_SVG_CANON.name}")

    old_inline = RESOURCES / "immich-logo-inline.svg"
    if old_inline.exists():
        old_inline.unlink()
        print(f"Removed {old_inline.name}")

    master_dark_ui = fill_square(render_svg(SVG_ON_DARK, 2048, 2048), 1024)
    master_light_ui = fill_square(render_svg(SVG_ON_LIGHT, 2048, 2048), 1024)

    # Default / packaging set = for dark UI (white icon).
    write_icon_set(master_dark_ui, "")
    master_dark_ui.save(
        RESOURCES / "immich.ico",
        format="ICO",
        sizes=[(16, 16), (24, 24), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)],
    )
    print("Wrote immich.ico")

    # Light UI set = dark icon.
    write_icon_set(master_light_ui, "on-light")

    # Banners: light text on dark UI, dark text on light UI.
    banner_on_dark = make_inline_banner(master_dark_ui, 256, (245, 247, 251, 255))
    banner_on_light = make_inline_banner(master_light_ui, 256, (23, 32, 51, 255))
    banner_on_dark.save(RESOURCES / "immich-logo-inline-on-dark.png", format="PNG")
    banner_on_light.save(RESOURCES / "immich-logo-inline-on-light.png", format="PNG")
    shutil.copyfile(
        RESOURCES / "immich-logo-inline-on-dark.png",
        RESOURCES / "immich-logo-inline-light.png",
    )
    print(
        f"Wrote banners "
        f"({banner_on_dark.width}x{banner_on_dark.height}, "
        f"{banner_on_light.width}x{banner_on_light.height})"
    )

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
