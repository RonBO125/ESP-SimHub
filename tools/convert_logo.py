#!/usr/bin/env python3
"""
Convert a PNG/JPG logo to a C PROGMEM bitmap for LovyanGFX drawBitmap().

Usage:
    pip install Pillow
    python tools/convert_logo.py lamborghini.png

Output: src/logo.h  (overwrite and recompile)

The script inverts the image so dark pixels become the logo (drawn in gold)
and light pixels become the background (black).
Target size defaults to 240x320 to fit a 240x320 TFT.
"""

import sys
import os
from PIL import Image


def convert(src_path: str, w: int = 240, h: int = 320) -> None:
    img = Image.open(src_path).convert("L")
    img = img.resize((w, h), Image.LANCZOS)

    bytes_per_row = (w + 7) // 8
    flat: list[int] = []

    for y in range(h):
        for bx in range(bytes_per_row):
            byte = 0
            for bit in range(8):
                x = bx * 8 + bit
                if x < w:
                    pixel = img.getpixel((x, y))
                    if pixel < 128:          # dark pixel → logo → set bit
                        byte |= 0x80 >> bit
            flat.append(byte)

    total = len(flat)
    out_path = os.path.join(os.path.dirname(__file__), "..", "src", "logo.h")
    out_path = os.path.normpath(out_path)

    lines = [
        "#pragma once",
        "#include <pgmspace.h>",
        "",
        f"// Generated from {os.path.basename(src_path)}  ({w}x{h} px)",
        f"static const int16_t LOGO_W = {w};",
        f"static const int16_t LOGO_H = {h};",
        "",
        f"// {bytes_per_row} bytes/row × {h} rows = {total} bytes",
        f"static const uint8_t LOGO_BITMAP[{total}] PROGMEM = {{",
    ]

    chunk = 16
    for i in range(0, total, chunk):
        row = flat[i : i + chunk]
        suffix = "," if i + chunk < total else ""
        lines.append("  " + ", ".join(f"0x{b:02X}" for b in row) + suffix)

    lines += ["};", ""]

    with open(out_path, "w", encoding="utf-8") as f:
        f.write("\n".join(lines))

    print(f"Written {out_path}  ({total} bytes, {w}×{h} px)")
    print("Recompile with: pio run -e esp32")


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(1)

    src   = sys.argv[1]
    width  = int(sys.argv[2]) if len(sys.argv) > 2 else 240
    height = int(sys.argv[3]) if len(sys.argv) > 3 else 320
    convert(src, width, height)
