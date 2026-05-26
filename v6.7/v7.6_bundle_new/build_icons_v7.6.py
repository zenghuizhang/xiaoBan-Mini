#!/usr/bin/env python3
# build_icons_v7.6.py — 6 menu sector icons, high-res standalone SVGs.
# Each icon: 256x256, centered, accent_hi stroke 8px, rounded line caps.
# Geometry mirrors the React lucide-react icons used in the prototype:
#   对话 = MessageCircle
#   模型 = Cpu
#   主题 = Palette
#   设置 = Settings
#   人格 = User
#   记忆 = BookOpen
# Color = accent_hi of each of the 4 themes, plus a neutral white set.

import os, pathlib

OUT = pathlib.Path(__file__).resolve().parent / "icons_v7.6"
OUT.mkdir(parents=True, exist_ok=True)

THEMES = {
    "tech":     ("#33C5FF", "#000000"),   # accent_hi, bg
    "lavender": ("#A855F7", "#FAF5FF"),
    "child":    ("#FFAA78", "#FFF9E6"),
    "cocoa":    ("#FDA4AF", "#2D1B0E"),
    "mono":     ("#FFFFFF", "transparent"),
}

# All icon paths centered in 256x256 viewBox.
# Stroke: 8, linecap/linejoin: round, fill: none.
ICONS = {
    # 对话 (MessageCircle)
    "01_chat": '''
        <path d="M40 128 a88 88 0 1 1 36 71 L40 216 l17 -36 A88 88 0 0 1 40 128z"/>
    ''',
    # 模型 (Cpu)
    "02_model": '''
        <rect x="64"  y="64"  width="128" height="128" rx="16"/>
        <rect x="96"  y="96"  width="64"  height="64"  rx="6"/>
        <line x1="40"  y1="96"  x2="64"  y2="96"/>
        <line x1="40"  y1="128" x2="64"  y2="128"/>
        <line x1="40"  y1="160" x2="64"  y2="160"/>
        <line x1="192" y1="96"  x2="216" y2="96"/>
        <line x1="192" y1="128" x2="216" y2="128"/>
        <line x1="192" y1="160" x2="216" y2="160"/>
        <line x1="96"  y1="40"  x2="96"  y2="64"/>
        <line x1="128" y1="40"  x2="128" y2="64"/>
        <line x1="160" y1="40"  x2="160" y2="64"/>
        <line x1="96"  y1="192" x2="96"  y2="216"/>
        <line x1="128" y1="192" x2="128" y2="216"/>
        <line x1="160" y1="192" x2="160" y2="216"/>
    ''',
    # 主题 (Palette) — 4-color dots inside a palette outline
    "03_theme": '''
        <path d="M128 32 C72 32 32 72 32 128 c0 52 40 96 96 96 12 0 22 -10 22 -22 0 -6 -2 -11 -6 -15 -4 -4 -6 -9 -6 -15 0 -12 10 -22 22 -22 h28 c34 0 60 -26 60 -60 0 -50 -44 -90 -120 -90z"/>
        <circle cx="80"  cy="120" r="10" fill="currentColor"/>
        <circle cx="112" cy="76"  r="10" fill="currentColor"/>
        <circle cx="160" cy="76"  r="10" fill="currentColor"/>
        <circle cx="192" cy="120" r="10" fill="currentColor"/>
    ''',
    # 设置 (Settings) — gear
    "04_settings": '''
        <circle cx="128" cy="128" r="32"/>
        <path d="M128 16 v32 M128 208 v32
                 M16 128 h32 M208 128 h32
                 M48 48 l24 24 M184 184 l24 24
                 M208 48 l-24 24 M72 184 l-24 24"/>
    ''',
    # 人格 (User)
    "05_persona": '''
        <circle cx="128" cy="92" r="44"/>
        <path d="M48 224 c0 -44 36 -80 80 -80 s80 36 80 80"/>
    ''',
    # 记忆 (BookOpen)
    "06_memory": '''
        <path d="M40 56 h72 c18 0 32 14 32 32 v136
                 M216 56 h-72 c-18 0 -32 14 -32 32 v136
                 M40 56 v144 h72
                 M216 56 v144 h-72"/>
    ''',
}

LABELS = {
    "01_chat":     ("对话",  "MessageCircle"),
    "02_model":    ("模型",  "Cpu"),
    "03_theme":    ("主题",  "Palette"),
    "04_settings": ("设置",  "Settings"),
    "05_persona":  ("人格",  "User"),
    "06_memory":   ("记忆",  "BookOpen"),
}

def make_svg(stroke: str, bg: str, body: str, with_bg: bool = True) -> str:
    bgrect = (f'<rect width="256" height="256" rx="32" fill="{bg}"/>'
              if with_bg and bg != "transparent" else "")
    # fill is forced "none" for strokes; inner <circle fill="currentColor"/> uses stroke color
    return f'''<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 256 256" width="256" height="256">
  {bgrect}
  <g fill="none" stroke="{stroke}" stroke-width="10"
     stroke-linecap="round" stroke-linejoin="round"
     style="color:{stroke}">
    {body}
  </g>
</svg>
'''

# ---- 1) Per-icon × per-theme files ------------------------------------------
for theme_name, (stroke, bg) in THEMES.items():
    tdir = OUT / theme_name
    tdir.mkdir(exist_ok=True)
    for key, body in ICONS.items():
        (tdir / f"menu_{key}_{theme_name}.svg").write_text(
            make_svg(stroke, bg, body, with_bg=(theme_name != "mono"))
        )

# ---- 2) Combined 6-up overview (one per theme) ------------------------------
def make_overview(theme_name: str, stroke: str, bg: str) -> str:
    # 3 cols × 2 rows, each cell 320x320 (icon 256 + label 64)
    W, H, C = 960, 720, 320
    text_color = "#FFFFFF" if theme_name in ("tech", "cocoa") else "#000000"
    if theme_name == "mono":
        text_color = "#FFFFFF"
    cells = []
    for i, (key, body) in enumerate(ICONS.items()):
        cx = (i % 3) * C + 32
        cy = (i // 3) * C + 32
        cn, en = LABELS[key]
        cells.append(f'''
        <g transform="translate({cx},{cy})">
          <rect width="256" height="256" rx="28" fill="{bg if bg != 'transparent' else '#111'}"
                stroke="{stroke}" stroke-width="1" opacity="0.95"/>
          <g fill="none" stroke="{stroke}" stroke-width="10"
             stroke-linecap="round" stroke-linejoin="round"
             style="color:{stroke}">{body}</g>
          <text x="128" y="296" text-anchor="middle"
                font-family="PingFang SC, Noto Sans CJK SC, sans-serif"
                font-size="22" fill="{text_color}">{cn} · {en}</text>
        </g>''')
    bgfill = bg if bg != "transparent" else "#0a0a0a"
    return f'''<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {W} {H}" width="{W}" height="{H}">
  <rect width="{W}" height="{H}" fill="{bgfill}"/>
  <text x="32" y="36" font-family="PingFang SC, Noto Sans, sans-serif"
        font-size="22" fill="{stroke}">menu sectors · theme: {theme_name}</text>
  {''.join(cells)}
</svg>
'''

for theme_name, (stroke, bg) in THEMES.items():
    (OUT / f"OVERVIEW_menu_icons_{theme_name}.svg").write_text(
        make_overview(theme_name, stroke, bg))

# ---- 3) Single master overview (Tech + accent_hi labels) --------------------
print("Generated:")
for p in sorted(OUT.rglob("*.svg")):
    print("  ", p.relative_to(OUT))
print(f"Total files: {len(list(OUT.rglob('*.svg')))}")
