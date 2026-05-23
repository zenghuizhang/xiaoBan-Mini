#!/usr/bin/env python3
"""
SVG -> 32x32 RGBA8888 PNG -> LVGL 9 lv_image_dsc_t C source.
Single-color icons; alpha encodes the lucide stroke shape.
"""
import os, sys, subprocess
from pathlib import Path

ICON_SIZE = 32          # final raster size (px)
SRC_DIR   = Path("icons_pack/svg")
PNG_DIR   = Path("icons_pack/png")
C_DIR     = Path("icons_pack/c")

ICONS = [
    ("icon_smile",    "Smile / 表情"),
    ("icon_message",  "MessageCircle / 对话"),
    ("icon_settings", "Settings / 设置"),
    ("icon_palette",  "Palette / 主题"),
    ("icon_puzzle",   "Puzzle / 扩展"),
    ("icon_shuffle",  "Shuffle / 随机"),
]

PNG_DIR.mkdir(parents=True, exist_ok=True)
C_DIR.mkdir(parents=True, exist_ok=True)

# 1) rasterize SVG -> PNG (white stroke on transparent)
# Use Pillow + a tiny custom renderer via cairosvg if available, else fall back to ImageMagick / rsvg.
def rasterize(svg_path: Path, png_path: Path, size: int):
    # Try cairosvg
    try:
        import cairosvg
        # Force currentColor to white so we get a solid alpha mask matching stroke shape
        svg_text = svg_path.read_text(encoding="utf-8")
        svg_text = svg_text.replace('stroke="currentColor"', 'stroke="#FFFFFF"')
        svg_text = svg_text.replace('fill="currentColor"', 'fill="#FFFFFF"')
        cairosvg.svg2png(bytestring=svg_text.encode("utf-8"),
                         output_width=size, output_height=size,
                         write_to=str(png_path))
        return
    except Exception:
        pass
    # Fallback: rsvg-convert
    if subprocess.run(["which", "rsvg-convert"], capture_output=True).returncode == 0:
        # Rewrite to white stroke
        tmp = svg_path.with_suffix(".tmp.svg")
        tmp.write_text(svg_path.read_text().replace('currentColor', '#FFFFFF'))
        subprocess.check_call(["rsvg-convert", "-w", str(size), "-h", str(size),
                               "-o", str(png_path), str(tmp)])
        tmp.unlink()
        return
    raise RuntimeError("Neither cairosvg nor rsvg-convert available")

# 2) PNG (RGBA) -> LVGL 9 C array (LV_COLOR_FORMAT_ARGB8888)
def png_to_c(png_path: Path, c_path: Path, sym_name: str, desc: str):
    from PIL import Image
    img = Image.open(png_path).convert("RGBA")
    w, h = img.size
    pixels = img.load()
    # LVGL 9 ARGB8888 layout in memory: B, G, R, A (little-endian uint32 0xAARRGGBB)
    bytes_per_px = 4
    data = bytearray()
    for y in range(h):
        for x in range(w):
            r, g, b, a = pixels[x, y]
            data += bytes([b, g, r, a])

    lines = []
    lines.append(f"/* AUTO-GENERATED FROM lucide SVG. DO NOT EDIT BY HAND.")
    lines.append(f" * Symbol      : {sym_name}")
    lines.append(f" * Description : {desc}")
    lines.append(f" * Size        : {w}x{h} px")
    lines.append(f" * Format      : LV_COLOR_FORMAT_ARGB8888 (4 bytes/px, B,G,R,A)")
    lines.append(f" * Tint        : white stroke; runtime tint via lv_obj_set_style_image_recolor")
    lines.append(f" */")
    lines.append("")
    lines.append('#include "lvgl.h"')
    lines.append("")
    lines.append("#ifndef LV_ATTRIBUTE_MEM_ALIGN")
    lines.append("#define LV_ATTRIBUTE_MEM_ALIGN")
    lines.append("#endif")
    lines.append("")
    lines.append(f"static const LV_ATTRIBUTE_MEM_ALIGN uint8_t {sym_name}_map[] = {{")
    # 16 bytes per line for readability
    for i in range(0, len(data), 16):
        chunk = data[i:i+16]
        line = "    " + ", ".join(f"0x{b:02X}" for b in chunk) + ","
        lines.append(line)
    lines.append("};")
    lines.append("")
    lines.append(f"const lv_image_dsc_t {sym_name} = {{")
    lines.append("    .header = {")
    lines.append("        .magic     = LV_IMAGE_HEADER_MAGIC,")
    lines.append("        .cf        = LV_COLOR_FORMAT_ARGB8888,")
    lines.append("        .flags     = 0,")
    lines.append(f"        .w         = {w},")
    lines.append(f"        .h         = {h},")
    lines.append("        .stride    = " + f"{w * bytes_per_px},")
    lines.append("        .reserved_2 = 0,")
    lines.append("    },")
    lines.append(f"    .data_size = {len(data)},")
    lines.append(f"    .data      = {sym_name}_map,")
    lines.append("    .reserved  = NULL,")
    lines.append("};")
    lines.append("")
    c_path.write_text("\n".join(lines), encoding="utf-8")

def main():
    for name, desc in ICONS:
        svg = SRC_DIR / f"{name}.svg"
        png = PNG_DIR / f"{name}.png"
        c   = C_DIR  / f"{name}.c"
        rasterize(svg, png, ICON_SIZE)
        png_to_c(png, c, name, desc)
        print(f"[OK] {name}: {svg} -> {png} -> {c}")

if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        print(f"ERROR: {e}", file=sys.stderr)
        sys.exit(1)
