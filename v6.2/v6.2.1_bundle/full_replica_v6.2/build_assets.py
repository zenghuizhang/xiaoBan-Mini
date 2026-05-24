"""
Full v6.2 replica generator.
Outputs into full_replica_v6.2/:
  - assets/icons/        (80+ SVG icons)
  - assets/themes/theme_tokens.h (7 themes + high-contrast)
  - assets/anims/*.json  (10 AVG/Lottie-style timelines)
  - mock_renders/*.png   (8 pages × 7 themes + boot)
  - src/pages/*.c        (LVGL 9 page implementations)
  - src/widgets/*.c      (statusbar, bubble, dot_loading, qr_shimmer, ota_strip)
  - src/core/*.{c,h}     (theme, event_router, face_engine_stub, app_main)
  - docs/README.md, INTEGRATION.md, COVERAGE.md
"""
import os, math, json
from PIL import Image, ImageDraw, ImageFont, ImageFilter

ROOT = os.path.dirname(os.path.abspath(__file__))
A = lambda *p: os.path.join(ROOT, *p)

# ---------------- 1. THEMES (7 + high-contrast) ----------------
# tuple = (bg, panel, accent, accent_dim, text, text_dim, border, danger, success)
THEMES = {
    "tech":   ((  0,  0,  0),(10,30,40),(51,197,255),(15,90,120),(180,235,255),(110,170,200),(20,80,110),(244, 63, 94),( 34,197,94)),
    "child":  ((255,249,230),(255,255,255),(255,127,80),(255,170,120),(200,90,40),(210,140,90),(255,200,160),(244, 63, 94),( 34,197,94)),
    "dev":    ((  0,  0,  0),(10,30,15),(34,197,94),(15,90,40),(180,255,200),(110,200,140),(20,80,30),(244, 63, 94),(120,200,80)),
    "orange": (( 24, 16,  8),(40,28,20),(255,140, 66),(120, 70, 30),(255,228,200),(200,160,120),(80, 50, 30),(244, 63, 94),( 34,197,94)),
    "blue":   ((  8, 14, 24),(20, 30, 50),( 74,144,226),( 30, 70,140),(220,232,255),(140,170,210),( 40, 70,120),(244, 63, 94),( 34,197,94)),
    "pink":   (( 24, 14, 20),(50, 28, 40),(255,107,157),(140, 60, 90),(255,220,235),(210,160,185),( 90, 50, 70),(244, 63, 94),( 34,197,94)),
    "green":  ((  8, 20, 12),(20, 40, 24),( 91,184, 92),( 40, 90, 50),(220,245,222),(150,200,160),( 40, 80, 50),(244, 63, 94),( 91,184, 92)),
    "purple": (( 18, 10, 28),(36, 22, 56),(155, 89,182),( 80, 45,100),(232,220,250),(180,160,210),( 60, 40, 90),(244, 63, 94),( 34,197,94)),
    "yellow": (( 30, 26,  8),(50, 44, 20),(241,196, 15),(130,100, 10),(255,248,210),(210,190,120),(100, 80, 20),(244, 63, 94),( 34,197,94)),
    "mono":   ((  0,  0,  0),(40, 40, 40),(255,255,255),(180,180,180),(255,255,255),(200,200,200),(120,120,120),(255, 90, 90),(120,255,120)),  # high-contrast
}

ic_dir = A("assets","icons")
os.makedirs(ic_dir, exist_ok=True)

# ---------------- 2. ICONS (lucide-derived single-color SVG) ----------------
# Each value is the inner SVG body; all share viewBox 0 0 16 16, stroke=currentColor, sw=1.4
ICONS = {
    # status bar
    "battery":      '<rect x="1.5" y="5" width="11" height="6" rx="1"/><rect x="13" y="6.5" width="1.5" height="3"/><rect x="3" y="6.5" width="6" height="3" fill="currentColor"/>',
    "battery_low":  '<rect x="1.5" y="5" width="11" height="6" rx="1"/><rect x="13" y="6.5" width="1.5" height="3"/><rect x="3" y="6.5" width="2" height="3" fill="currentColor"/>',
    "wifi":         '<path d="M1 5.5c4-3.5 10-3.5 14 0"/><path d="M3 8.5c3-2.5 7-2.5 10 0"/><path d="M5.5 11.5c1.5-1.2 3.5-1.2 5 0"/><circle cx="8" cy="13.5" r="0.8" fill="currentColor"/>',
    "wifi_off":     '<path d="M2 2l12 12"/><path d="M5.5 11.5c1.5-1.2 3.5-1.2 5 0"/><circle cx="8" cy="13.5" r="0.8" fill="currentColor"/>',
    "ai_dot":       '<circle cx="8" cy="8" r="3" fill="currentColor"/><circle cx="8" cy="8" r="6"/>',
    "plug":         '<path d="M5 1.5v3M11 1.5v3M3.5 4.5h9v3a4.5 4.5 0 0 1-9 0z"/><path d="M8 12v3"/>',
    # menu sectors
    "smile":        '<circle cx="8" cy="8" r="6.5"/><circle cx="6" cy="7" r="0.7" fill="currentColor"/><circle cx="10" cy="7" r="0.7" fill="currentColor"/><path d="M5.5 10c1 1.2 4 1.2 5 0"/>',
    "msg":          '<path d="M2 4a1.5 1.5 0 0 1 1.5-1.5h9A1.5 1.5 0 0 1 14 4v6a1.5 1.5 0 0 1-1.5 1.5H6L3 14v-2.5A1.5 1.5 0 0 1 2 10z"/>',
    "settings":     '<circle cx="8" cy="8" r="2.2"/><path d="M8 1.5v1.8M8 12.7v1.8M14.5 8h-1.8M3.3 8H1.5M12.6 3.4l-1.3 1.3M4.7 11.3l-1.3 1.3M12.6 12.6l-1.3-1.3M4.7 4.7L3.4 3.4"/>',
    "palette":      '<path d="M8 1.5C4.5 1.5 1.5 4.5 1.5 8c0 3 2.5 5 5 5 0.5 0 1-0.5 1-1 0-1 1-1.5 2-1.5h2c2 0 3-1 3-3 0-3-2.5-6-6.5-6z"/><circle cx="5" cy="6.5" r="0.8" fill="currentColor"/><circle cx="8" cy="4.5" r="0.8" fill="currentColor"/><circle cx="11.5" cy="6.5" r="0.8" fill="currentColor"/>',
    "puzzle":       '<path d="M5.5 2v2.5H3v3a1.5 1.5 0 0 0 0 3H5.5V13h3v-2.5a1.5 1.5 0 0 0 3 0V13h2.5V8a1.5 1.5 0 0 0 0-3H11V2.5H8a1.5 1.5 0 0 0-3 0z"/>',
    "shuffle":      '<path d="M2 4h2.5l3.5 8H10M2 12h2.5l3.5-8H10"/><path d="M11 2.5L13.5 4 11 5.5M11 10.5L13.5 12 11 13.5"/>',
    # settings rows
    "volume":       '<path d="M3 6h2l4-3v10l-4-3H3z"/><path d="M11 5c1.5 1.5 1.5 4.5 0 6"/><path d="M13 3c2.5 2.5 2.5 7.5 0 10"/>',
    "sun":          '<circle cx="8" cy="8" r="3"/><path d="M8 1v2M8 13v2M1 8h2M13 8h2M3.5 3.5l1.4 1.4M11.1 11.1l1.4 1.4M3.5 12.5l1.4-1.4M11.1 4.9l1.4-1.4"/>',
    "mic":          '<rect x="6" y="2" width="4" height="8" rx="2"/><path d="M3.5 8a4.5 4.5 0 0 0 9 0M8 12.5V14M5.5 14h5"/>',
    "clock":        '<circle cx="8" cy="8" r="6.5"/><path d="M8 4v4l2.5 1.5"/>',
    "user":         '<circle cx="8" cy="5.5" r="2.8"/><path d="M2.5 13.5c1.2-2.5 3.2-3.5 5.5-3.5s4.3 1 5.5 3.5"/>',
    "refresh":      '<path d="M2 8a6 6 0 0 1 10.5-4M14 8a6 6 0 0 1-10.5 4"/><path d="M12 1.5V4.5H9M4 14.5V11.5H7"/>',
    "alert":        '<path d="M8 1.5L14.5 13.5h-13z"/><path d="M8 6v4M8 11.5v0.5"/>',
    "info":         '<circle cx="8" cy="8" r="6.5"/><path d="M8 7v4M8 5v0.5"/>',
    "shield":       '<path d="M8 1.5L13.5 4v4.5c0 3-2.5 5.5-5.5 6-3-0.5-5.5-3-5.5-6V4z"/>',
    "cpu":          '<rect x="3.5" y="3.5" width="9" height="9" rx="1"/><rect x="6" y="6" width="4" height="4"/><path d="M6 1.5v2M10 1.5v2M6 12.5v2M10 12.5v2M1.5 6h2M1.5 10h2M12.5 6h2M12.5 10h2"/>',
    "globe":        '<circle cx="8" cy="8" r="6.5"/><path d="M1.5 8h13M8 1.5c2 2 3 4 3 6.5s-1 4.5-3 6.5M8 1.5c-2 2-3 4-3 6.5s1 4.5 3 6.5"/>',
    "trash":        '<path d="M3 4.5h10M6 4.5V3a1 1 0 0 1 1-1h2a1 1 0 0 1 1 1v1.5M5 4.5v8.5a1 1 0 0 0 1 1h4a1 1 0 0 0 1-1V4.5"/>',
    "lock":         '<rect x="3" y="7" width="10" height="7" rx="1.5"/><path d="M5.5 7V5a2.5 2.5 0 0 1 5 0v2"/>',
    "key":          '<circle cx="11" cy="6" r="2.5"/><path d="M9 7L3 13M5.5 11l1.5 1.5M4 12.5l1 1"/>',
    # nav / actions
    "chevron_left": '<path d="M10 3L5 8l5 5"/>',
    "chevron_right":'<path d="M6 3l5 5-5 5"/>',
    "chevron_up":   '<path d="M3 10l5-5 5 5"/>',
    "chevron_down": '<path d="M3 6l5 5 5-5"/>',
    "close":        '<path d="M3 3l10 10M13 3L3 13"/>',
    "check":        '<path d="M3 8l3.5 3.5L13 5"/>',
    "plus":         '<path d="M8 3v10M3 8h10"/>',
    "minus":        '<path d="M3 8h10"/>',
    "send":         '<path d="M14 2L2 7l5 2 2 5z"/>',
    "stop":         '<rect x="4" y="4" width="8" height="8" rx="1"/>',
    "play":         '<polygon points="5,3 12,8 5,13"/>',
    "pause":        '<rect x="4" y="3" width="3" height="10"/><rect x="9" y="3" width="3" height="10"/>',
    "search":       '<circle cx="7" cy="7" r="4.5"/><path d="M10.5 10.5L14 14"/>',
    "download":     '<path d="M8 2v9M4 7l4 4 4-4M2 14h12"/>',
    "upload":       '<path d="M8 14V5M4 9l4-4 4 4M2 2h12"/>',
    "qrcode":       '<rect x="2" y="2" width="4" height="4"/><rect x="10" y="2" width="4" height="4"/><rect x="2" y="10" width="4" height="4"/><rect x="8" y="8" width="2" height="2" fill="currentColor"/><rect x="11" y="8" width="2" height="2" fill="currentColor"/><rect x="8" y="11" width="2" height="2" fill="currentColor"/><rect x="11" y="11" width="2" height="2" fill="currentColor"/>',
    "loader":       '<path d="M8 1.5v3M8 11.5v3M1.5 8h3M11.5 8h3M3.5 3.5l2 2M10.5 10.5l2 2M3.5 12.5l2-2M10.5 5.5l2-2"/>',
    "thumbs_up":    '<path d="M3 7h2v6H3zM5 7l2-5a1.5 1.5 0 0 1 3 0v3h3a1.5 1.5 0 0 1 1.5 1.5l-1 4A1.5 1.5 0 0 1 12 13H5z"/>',
    "thumbs_down":  '<path d="M3 9h2V3H3zM5 9l2 5a1.5 1.5 0 0 0 3 0v-3h3a1.5 1.5 0 0 0 1.5-1.5l-1-4A1.5 1.5 0 0 0 12 3H5z"/>',
    # category / feature
    "puzzle_dot":   '<circle cx="8" cy="8" r="6.5"/><circle cx="8" cy="8" r="2" fill="currentColor"/>',
    "weather":      '<circle cx="6" cy="6" r="2.5"/><path d="M5 11h7a2 2 0 0 0 0-4 3 3 0 0 0-5.5-1"/>',
    "alarm":        '<circle cx="8" cy="9" r="5"/><path d="M2.5 4.5L4.5 2.5M13.5 4.5L11.5 2.5M8 7v2l1.5 1"/>',
    "tomato":       '<circle cx="8" cy="9" r="5"/><path d="M6 4l2-2 2 2"/>',
    "translate":    '<path d="M2 4h6M5 2v2M3 6c0 2 2 4 4 4M9 8c0 2-2 4-4 4"/><path d="M9 14L12 6l3 8M10 12h4"/>',
    "parrot":       '<path d="M5 5a3 3 0 1 1 6 0v2c0 2-1.5 3.5-3 3.5S5 9 5 7z"/><circle cx="9" cy="6" r="0.7" fill="currentColor"/><path d="M11 8L13 6"/>',
    "skill_pkg":    '<path d="M2 5l6-3 6 3-6 3z"/><path d="M2 5v6l6 3M14 5v6l-6 3"/>',
    "store":        '<path d="M2 5l1-2h10l1 2v1a2 2 0 0 1-4 0 2 2 0 0 1-4 0 2 2 0 0 1-4 0z"/><path d="M3 6v7h10V6"/>',
    "fire":         '<path d="M8 1c1 3 4 4 4 7a4 4 0 0 1-8 0c0-2 2-3 2-5 0 2 2 2 2-2z"/>',
    "moon":         '<path d="M13 9.5A6 6 0 0 1 6.5 3 6 6 0 1 0 13 9.5z"/>',
    "sparkle":      '<path d="M8 2v4M8 10v4M2 8h4M10 8h4"/>',
    "memory":       '<rect x="2.5" y="4" width="11" height="8" rx="1"/><path d="M5 4V2M8 4V2M11 4V2M5 14v-2M8 14v-2M11 14v-2"/>',
    "gauge":        '<path d="M2.5 11a6 6 0 0 1 11 0"/><path d="M8 11l3-4"/>',
    "compass":      '<circle cx="8" cy="8" r="6.5"/><polygon points="8,4 10,8 8,12 6,8" fill="currentColor"/>',
    "sliders":      '<path d="M3 4h10M3 8h10M3 12h10"/><circle cx="6" cy="4" r="1.3" fill="currentColor"/><circle cx="10" cy="8" r="1.3" fill="currentColor"/><circle cx="5" cy="12" r="1.3" fill="currentColor"/>',
    # OTA / system
    "package":      '<path d="M2 5l6-3 6 3v6l-6 3-6-3z"/><path d="M2 5l6 3 6-3M8 8v6"/>',
    "shield_check": '<path d="M8 1.5L13.5 4v4.5c0 3-2.5 5.5-5.5 6-3-0.5-5.5-3-5.5-6V4z"/><path d="M5.5 8L7 9.5l3.5-3.5"/>',
    "rotate":       '<path d="M2 8a6 6 0 1 1 6 6"/><path d="M5 11l3 3-3 3"/>',
    "bell":         '<path d="M4 11V8a4 4 0 0 1 8 0v3l1 2H3z"/><path d="M6.5 13.5a1.5 1.5 0 0 0 3 0"/>',
    "bug":          '<rect x="5" y="5" width="6" height="7" rx="2"/><path d="M2 5l3 1M14 5l-3 1M2 8h3M11 8h3M2 11l3-1M14 11l-3-1M6 4l1-2M10 4l-1-2"/>',
    "language":     '<circle cx="8" cy="8" r="6.5"/><path d="M2 8h12M8 2c2 1.5 3 4 3 6s-1 4.5-3 6"/>',
    "sd_card":      '<path d="M4 2h7l3 3v9H4z"/><path d="M6 2v3M9 2v3M12 2v3"/>',
    "factory":      '<path d="M2 14V7l4-2v3l4-2v3l4-2v8z"/><rect x="6" y="11" width="2" height="3"/>',
    "reset":        '<path d="M3 4v3h3"/><path d="M3 7a5 5 0 1 1 0 4"/>',
    # Privacy / network
    "eye":          '<path d="M1 8s2.5-5 7-5 7 5 7 5-2.5 5-7 5-7-5-7-5z"/><circle cx="8" cy="8" r="2"/>',
    "eye_off":      '<path d="M1 8s2.5-5 7-5 7 5 7 5-2.5 5-7 5-7-5-7-5z"/><path d="M2 2l12 12"/>',
    "cloud":        '<path d="M5 11h7a3 3 0 0 0 0-6 4 4 0 0 0-7.5 1A3 3 0 0 0 5 11z"/>',
    "cloud_off":    '<path d="M5 11h7a3 3 0 0 0 0-6 4 4 0 0 0-7.5 1A3 3 0 0 0 5 11z"/><path d="M2 2l12 12"/>',
    "link":         '<path d="M6 8a3 3 0 0 1 3-3h2a3 3 0 0 1 0 6H9"/><path d="M10 8a3 3 0 0 1-3 3H5a3 3 0 0 1 0-6h2"/>',
    "broadcast":    '<circle cx="8" cy="8" r="2" fill="currentColor"/><path d="M5 11a4 4 0 0 1 0-6M11 5a4 4 0 0 1 0 6M3 13a7 7 0 0 1 0-10M13 3a7 7 0 0 1 0 10"/>',
    # icon dot loader (special)
    "dot3":         '<circle cx="3" cy="8" r="1.2" fill="currentColor"/><circle cx="8" cy="8" r="1.2" fill="currentColor"/><circle cx="13" cy="8" r="1.2" fill="currentColor"/>',
    # dialog / chat
    "copy":         '<rect x="5" y="5" width="8" height="9" rx="1"/><path d="M3 11V3a1 1 0 0 1 1-1h7"/>',
    "regen":        '<path d="M2 8a6 6 0 0 1 6-6"/><path d="M8 2l2 2-2 2"/><path d="M14 8a6 6 0 0 1-6 6"/><path d="M8 14l-2-2 2-2"/>',
    "stop_circle":  '<circle cx="8" cy="8" r="6.5"/><rect x="6" y="6" width="4" height="4" fill="currentColor"/>',
    "user_circle":  '<circle cx="8" cy="8" r="6.5"/><circle cx="8" cy="6.5" r="2"/><path d="M3.5 13a4.5 4.5 0 0 1 9 0"/>',
    "robot":        '<rect x="3" y="5" width="10" height="8" rx="2"/><circle cx="6" cy="9" r="1" fill="currentColor"/><circle cx="10" cy="9" r="1" fill="currentColor"/><path d="M8 1v3M5 13v2M11 13v2"/>',
}

SVG_TPL = ('<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 16 16" '
           'fill="none" stroke="currentColor" stroke-width="1.4" '
           'stroke-linecap="round" stroke-linejoin="round">{body}</svg>')
for n, body in ICONS.items():
    with open(os.path.join(ic_dir, f"{n}.svg"), "w") as f:
        f.write(SVG_TPL.format(body=body))
print(f"[icons] wrote {len(ICONS)} svgs")

# ---------------- 3. theme_tokens.h ----------------
hdr_path = A("assets","themes","theme_tokens.h")
os.makedirs(os.path.dirname(hdr_path), exist_ok=True)
out = ["// AUTO-GENERATED theme_tokens.h — DO NOT EDIT BY HAND",
       "// 7 main themes + high-contrast (mono).",
       "#pragma once",
       "#include \"lvgl.h\"", "",
       "typedef struct {",
       "    lv_color_t bg, panel, accent, accent_dim, text, text_dim, border, danger, success;",
       "} theme_t;",""]
for n,c in THEMES.items():
    out.append(f"static const theme_t THEME_{n.upper()} = {{")
    for tup in c:
        out.append(f"    lv_color_make({tup[0]:3d},{tup[1]:3d},{tup[2]:3d}),")
    out.append("};")
out += ["",
        "// usage: switch by theme name string from settings",
        "static inline const theme_t* xb_theme_lookup(const char* name) {",
        "    if(!name) return &THEME_TECH;"]
for n in THEMES:
    out.append(f'    if(strcmp(name,"{n}")==0) return &THEME_{n.upper()};')
out += ["    return &THEME_TECH;","}",""]
open(hdr_path,"w").write("\n".join(out))
print(f"[theme] {hdr_path}")

# ---------------- 4. AVG ANIMS (10 timeline JSON for MI-01..MI-10) ----------------
anims_dir = A("assets","anims")
os.makedirs(anims_dir, exist_ok=True)
anims = {
    "MI-01_dot_loading": {
        "name":"dot-loading","duration_ms":600,"loop":True,
        "tracks":[
            {"target":"dot1","prop":"opacity","keys":[[0,0.3],[200,1.0],[600,0.3]]},
            {"target":"dot2","prop":"opacity","keys":[[0,0.3],[400,1.0],[600,0.3]]},
            {"target":"dot3","prop":"opacity","keys":[[200,0.3],[600,1.0]]}]},
    "MI-02_bubble_pop": {
        "name":"bubble-pop","duration_ms":180,"loop":False,"easing":"ease_out",
        "tracks":[
            {"target":"bubble","prop":"scale","keys":[[0,0.9],[180,1.0]]},
            {"target":"bubble","prop":"opacity","keys":[[0,0.0],[180,1.0]]}]},
    "MI-03_toast": {
        "name":"toast","duration_ms":2200,"loop":False,
        "tracks":[
            {"target":"toast","prop":"y","keys":[[0,20],[200,0],[2000,0],[2200,20]]},
            {"target":"toast","prop":"opacity","keys":[[0,0],[200,1],[2000,1],[2200,0]]}]},
    "MI-04_btn_press": {
        "name":"button-press","duration_ms":80,"loop":False,
        "tracks":[
            {"target":"btn","prop":"scale","keys":[[0,1.0],[40,0.96],[80,1.0]]}]},
    "MI-05_sector_hover": {
        "name":"sector-hover","duration_ms":150,"loop":False,
        "tracks":[
            {"target":"halo","prop":"scale","keys":[[0,0.8],[150,1.4]]},
            {"target":"halo","prop":"opacity","keys":[[0,0.6],[150,0.0]]}]},
    "MI-06_progress_strip": {
        "name":"progress-strip","duration_ms":1200,"loop":True,
        "tracks":[
            {"target":"shimmer","prop":"x","keys":[[0,-40],[1200,320]]},
            {"target":"shimmer","prop":"opacity","keys":[[0,0.0],[200,0.7],[1000,0.7],[1200,0.0]]}]},
    "MI-07_qrcode_shimmer": {
        "name":"qrcode-shimmer","duration_ms":4000,"loop":True,
        "tracks":[
            {"target":"shine","prop":"x","keys":[[0,-30],[4000,180]]},
            {"target":"shine","prop":"opacity","keys":[[0,0],[500,0.5],[3500,0.5],[4000,0]]}]},
    "MI-08_blink_rate": {
        "name":"blink-rate","note":"applied as multiplier on face_engine blink interval",
        "presets":{"thinking":1.5,"sleep":0.3,"talking":0.8,"alert":1.2,"idle":1.0}},
    "MI-09_mcp_pulse": {
        "name":"mcp-pulse","duration_ms":200,"loop":False,
        "tracks":[
            {"target":"plug","prop":"scale","keys":[[0,1.0],[100,1.4],[200,1.0]]}]},
    "MI-10_error_shake": {
        "name":"error-shake","duration_ms":200,"loop":False,
        "tracks":[
            {"target":"obj","prop":"x","keys":[[0,0],[50,-4],[100,4],[150,-4],[200,0]]}]},
}
for k,v in anims.items():
    open(os.path.join(anims_dir,k+".json"),"w").write(json.dumps(v,indent=2,ensure_ascii=False))
print(f"[anims] wrote {len(anims)} timelines")

# ---------------- 5. FONT spec ----------------
open(A("assets","fonts","FONT_SPEC.md"),"w").write("""# Font Spec

LVGL fonts to bundle (UTF-8, GBK overlap allowed):

| Name | Use | Glyphs |
|------|-----|--------|
| `xb_font_pingfang_12` | body, settings labels | ASCII + 4500 GB2312 |
| `xb_font_pingfang_13` | top-bar title | same |
| `xb_font_mono_10` | percentages, code, version | ASCII only |
| `xb_font_mono_20` | pairing code | digits 0-9 |
| `xb_font_mono_48` | pairing code (large) | digits 0-9 |

Generate via lv_font_conv:

```
lv_font_conv --font NotoSansSC-Regular.otf --size 12 --bpp 4 \\
  --range 0x20-0x7F,0x4E00-0x9FA5 --format lvgl -o xb_font_pingfang_12.c
```
""")

# ---------------- 6. MOCK PNG RENDERS for all pages × themes ----------------
W,H = 320,240
FONT_TRIES = ["/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
              "/usr/share/fonts/truetype/noto/NotoSansCJK-Regular.ttc",
              "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"]
def F(sz):
    for p in FONT_TRIES:
        if os.path.exists(p):
            try: return ImageFont.truetype(p,sz)
            except: pass
    return ImageFont.load_default()
F_TITLE,F_BODY,F_SMALL,F_TINY,F_MONO,F_BIG = F(13),F(11),F(9),F(8),F(10),F(28)

def stub_icon(d,x,y,name,color,sz=14):
    cx,cy=x+sz/2,y+sz/2
    if name in ("volume","sun","wifi","cpu","plug","shield","info","clock","user",
                "refresh","alert","mic","puzzle","palette","msg","smile","settings",
                "shuffle","battery","ai_dot"):
        # generic distinctive glyph
        if name=="volume":
            d.polygon([(x+1,y+5),(x+5,y+5),(x+9,y+2),(x+9,y+13),(x+5,y+10),(x+1,y+10)],outline=color)
            d.arc([x+10,y+4,x+15,y+11],300,60,fill=color,width=1)
        elif name=="sun":
            d.ellipse([cx-3,cy-3,cx+3,cy+3],outline=color,width=1)
            for a in range(0,360,45):
                r=math.radians(a); d.line([(cx+5*math.cos(r),cy+5*math.sin(r)),(cx+7*math.cos(r),cy+7*math.sin(r))],fill=color)
        elif name=="wifi":
            for r,o in [(8,1),(5,3),(2,5)]: d.arc([cx-r,cy-r+o,cx+r,cy+r+o],200,340,fill=color,width=1)
            d.ellipse([cx-1,cy+4,cx+1,cy+6],fill=color)
        elif name=="cpu":
            d.rectangle([x+3,y+3,x+sz-3,y+sz-3],outline=color,width=1)
            d.rectangle([cx-2,cy-2,cx+2,cy+2],outline=color,width=1)
        elif name=="plug":
            d.line([x+4,y+1,x+4,y+5],fill=color); d.line([x+11,y+1,x+11,y+5],fill=color)
            d.rounded_rectangle([x+2,y+5,x+13,y+10],1,outline=color,width=1)
            d.arc([x+2,y+7,x+13,y+15],0,180,fill=color,width=1)
        elif name=="shield":
            d.polygon([(cx,y+1),(x+sz-1,y+3),(x+sz-1,y+8),(cx,y+sz-1),(x+1,y+8),(x+1,y+3)],outline=color,width=1)
        elif name=="info":
            d.ellipse([x+1,y+1,x+sz-1,y+sz-1],outline=color,width=1)
            d.line([cx,cy-1,cx,cy+3],fill=color)
        elif name=="clock":
            d.ellipse([x+1,y+1,x+sz-1,y+sz-1],outline=color,width=1)
            d.line([cx,cy,cx,cy-3],fill=color); d.line([cx,cy,cx+2,cy+1],fill=color)
        elif name=="user":
            d.ellipse([cx-3,y+2,cx+3,y+8],outline=color,width=1)
            d.arc([cx-5,y+7,cx+5,y+15],180,360,fill=color,width=1)
        elif name=="refresh":
            d.arc([x+1,y+1,x+sz-1,y+sz-1],200,360,fill=color,width=1)
            d.polygon([(x+11,y+1),(x+15,y+3),(x+12,y+5)],fill=color)
        elif name=="alert":
            d.polygon([(cx,y+1),(x+sz-1,y+sz-1),(x+1,y+sz-1)],outline=color,width=1)
            d.line([cx,y+5,cx,y+10],fill=color)
        elif name=="mic":
            d.rounded_rectangle([cx-2,y+1,cx+2,y+9],2,outline=color,width=1)
            d.arc([cx-4,y+5,cx+4,y+13],0,180,fill=color,width=1)
        elif name=="puzzle":
            d.rectangle([x+2,y+2,x+sz-2,y+sz-2],outline=color,width=1)
            d.rectangle([cx-2,cy-2,cx+2,cy+2],outline=color,width=1)
        elif name=="palette":
            d.ellipse([x+1,y+1,x+sz-1,y+sz-1],outline=color,width=1)
            for px,py in [(x+5,y+5),(x+10,y+5),(x+10,y+10)]: d.ellipse([px-1,py-1,px+1,py+1],fill=color)
        elif name=="msg":
            d.rounded_rectangle([x+1,y+3,x+sz-1,y+sz-3],2,outline=color,width=1)
            d.line([x+4,y+sz-3,x+4,y+sz],fill=color)
        elif name=="smile":
            d.ellipse([x+1,y+1,x+sz-1,y+sz-1],outline=color,width=1)
            d.point([cx-2,cy-1],fill=color); d.point([cx+2,cy-1],fill=color)
            d.arc([cx-3,cy,cx+3,cy+4],0,180,fill=color,width=1)
        elif name=="settings":
            d.ellipse([cx-2,cy-2,cx+2,cy+2],outline=color,width=1)
            for a in range(0,360,45):
                r=math.radians(a); d.line([(cx+4*math.cos(r),cy+4*math.sin(r)),(cx+6*math.cos(r),cy+6*math.sin(r))],fill=color)
        elif name=="shuffle":
            d.line([x+1,y+4,x+sz-3,y+sz-4],fill=color); d.line([x+1,y+sz-4,x+sz-3,y+4],fill=color)
            d.polygon([(x+sz-4,y+2),(x+sz-1,y+4),(x+sz-4,y+6)],fill=color)
        elif name=="battery":
            d.rounded_rectangle([x+1,y+5,x+sz-3,y+sz-3],1,outline=color,width=1)
            d.rectangle([x+sz-3,y+7,x+sz-1,y+sz-5],fill=color)
            d.rectangle([x+2,y+6,x+8,y+sz-4],fill=color)
        elif name=="ai_dot":
            d.ellipse([cx-2,cy-2,cx+2,cy+2],fill=color)
    else:
        d.rectangle([x+2,y+2,x+sz-2,y+sz-2],outline=color)

def card(d, box, fill, border, opa=0.3):
    # blend fill into bg darkness with given alpha
    pass  # we render directly

def render_topbar(d, theme, mode_label="Qwen-7B", show_ai=False, show_mcp=False):
    bg,panel,acc,accd,txt,txtd,bd,*_ = THEMES[theme]
    d.rectangle([0,0,W,28], fill=panel)
    # right side: battery / wifi / ai / mcp
    x = W - 8
    # battery
    x -= 16; stub_icon(d,x-2,7,"battery",txt,14)
    # wifi
    x -= 18; stub_icon(d,x-2,7,"wifi",txt,14)
    if show_ai:
        x -= 18; stub_icon(d,x-2,7,"ai_dot",acc,14)
    if show_mcp:
        x -= 18; stub_icon(d,x-2,7,"plug",acc,14)
    return acc, txt, txtd, bd, panel

def render_home(theme, out):
    bg,panel,acc,accd,txt,txtd,bd,*_ = THEMES[theme]
    img=Image.new("RGB",(W,H),bg); d=ImageDraw.Draw(img)
    render_topbar(d,theme,show_ai=True,show_mcp=True)
    # face placeholder (eyes + mouth)
    cx,cy=W//2,H//2+10
    d.rounded_rectangle([cx-44,cy-30,cx-12,cy+10],10,fill=acc)
    d.rounded_rectangle([cx+12,cy-30,cx+44,cy+10],10,fill=acc)
    d.rounded_rectangle([cx-14,cy+25,cx+14,cy+33],4,fill=acc)
    # bottom hint
    d.text((W/2-60,H-22),"双击进入菜单 · 长按主题切换",fill=txtd,font=F_SMALL)
    img.save(out)

def render_menu(theme, out):
    bg,panel,acc,accd,txt,txtd,bd,*_ = THEMES[theme]
    img=Image.new("RGB",(W,H),bg); d=ImageDraw.Draw(img)
    # darken backdrop simulated by overlay rect
    d.rectangle([0,0,W,H],fill=bg)
    render_topbar(d,theme,show_ai=True)
    cx,cy=W//2,H//2+5
    icons=["smile","msg","settings","palette","puzzle","shuffle"]
    labels=["表情","对话","设置","主题","扩展","随机"]
    angles=[-90,-30,30,90,150,210]
    R=70
    for i,(ic,lb,a) in enumerate(zip(icons,labels,angles)):
        ang=math.radians(a)
        x=cx+R*math.cos(ang)-23; y=cy+R*math.sin(ang)-23
        d.ellipse([x,y,x+46,y+46], fill=panel, outline=acc, width=1)
        stub_icon(d,x+16,y+16,ic,acc,14)
    d.text((cx-12,cy-6),"Menu",fill=acc,font=F_BODY)
    img.save(out)

def render_chat(theme, out):
    bg,panel,acc,accd,txt,txtd,bd,*_ = THEMES[theme]
    img=Image.new("RGB",(W,H),bg); d=ImageDraw.Draw(img)
    render_topbar(d,theme,show_ai=True)
    # Title left
    stub_icon(d,8,7,"chevron_left" if False else "msg",acc,14) if False else None
    d.text((10,8),"对话",fill=acc,font=F_BODY)
    # Bubbles
    # bot bubble
    d.rounded_rectangle([10,40,170,62],8,fill=panel,outline=bd,width=1)
    d.text((16,46),"你好,在听呢",fill=txt,font=F_BODY)
    # me bubble
    d.rounded_rectangle([130,72,310,94],8,fill=acc)
    d.text((140,78),"帮我设个闹钟 7:30",fill=bg,font=F_BODY)
    # bot thinking dots
    d.rounded_rectangle([10,104,52,124],8,fill=panel,outline=bd,width=1)
    for i,opa in enumerate([1,0.6,0.3]):
        cc=tuple(int(c*opa+(panel[i2] if i2<3 else 0)*(1-opa)) for i2,c in enumerate(txt))
        d.ellipse([18+i*10,111,22+i*10,115],fill=cc)
    # bottom input bar
    d.rectangle([0,H-32,W,H],fill=panel)
    stub_icon(d,8,H-22,"mic",txtd,14)
    d.rounded_rectangle([28,H-26,W-72,H-8],4,outline=bd,width=1)
    d.text((34,H-23),"输入...",fill=txtd,font=F_SMALL)
    d.rounded_rectangle([W-66,H-26,W-8,H-8],4,fill=acc)
    d.text((W-50,H-22),"发送",fill=bg,font=F_SMALL)
    img.save(out)

def render_wifi_ap(theme, out):
    bg,panel,acc,accd,txt,txtd,bd,*_ = THEMES[theme]
    img=Image.new("RGB",(W,H),bg); d=ImageDraw.Draw(img)
    render_topbar(d,theme)
    d.text((10,8),"配网",fill=acc,font=F_BODY)
    # title
    d.text((W/2-60,40),"请用手机连接 Wi-Fi",fill=txt,font=F_BODY)
    # SSID box
    d.rounded_rectangle([60,62,260,86],4,outline=acc,width=2)
    d.text((90,67),"MiraBot-A1B2",fill=acc,font=F_TITLE)
    # QR placeholder
    qx,qy=120,100
    d.rectangle([qx,qy,qx+80,qy+80],fill=(255,255,255))
    # finder squares
    for fx,fy in [(qx+4,qy+4),(qx+60,qy+4),(qx+4,qy+60)]:
        d.rectangle([fx,fy,fx+16,fy+16],outline=(0,0,0),width=2)
        d.rectangle([fx+5,fy+5,fx+11,fy+11],fill=(0,0,0))
    # mosaic
    for px in range(qx+24,qx+60,4):
        for py in range(qy+24,qy+60,4):
            if (px+py)%8==0: d.rectangle([px,py,px+3,py+3],fill=(0,0,0))
    d.text((W/2-72,H-26),"连接后浏览器自动打开",fill=txtd,font=F_SMALL)
    img.save(out)

def render_wifi_pair(theme, out):
    bg,panel,acc,accd,txt,txtd,bd,*_ = THEMES[theme]
    img=Image.new("RGB",(W,H),bg); d=ImageDraw.Draw(img)
    render_topbar(d,theme)
    d.text((10,8),"云配对",fill=acc,font=F_BODY)
    d.text((W/2-72,46),"请在 App 输入云配对码",fill=txt,font=F_BODY)
    # 6-digit big
    code="842 679"
    d.text((W/2-58,80),code,fill=acc,font=F_BIG)
    d.text((W/2-50,H-30),"剩余 04:32 · 重新生成",fill=txtd,font=F_SMALL)
    img.save(out)

def render_ota(theme, out):
    bg,panel,acc,accd,txt,txtd,bd,*_ = THEMES[theme]
    img=Image.new("RGB",(W,H),bg); d=ImageDraw.Draw(img)
    render_topbar(d,theme,show_ai=False)
    d.text((10,8),"系统升级",fill=acc,font=F_BODY)
    d.text((20,40),"v6.1.3 → v6.2.0",fill=txt,font=F_BODY)
    d.text((20,58),"修复对话稳定性,新增 MCP 调用",fill=txtd,font=F_SMALL)
    # progress
    d.rounded_rectangle([20,100,W-20,116],4,fill=panel,outline=bd,width=1)
    d.rounded_rectangle([20,100,W-20-(W-40)*0.4,116],4,fill=acc) if False else \
        d.rounded_rectangle([20,100,20+(W-40)*0.65,116],4,fill=acc)
    d.text((W/2-10,124),"65%",fill=txt,font=F_MONO)
    d.text((W/2-30,142),"正在下载...",fill=txtd,font=F_SMALL)
    # button
    d.rounded_rectangle([W/2-44,H-40,W/2+44,H-18],4,fill=panel,outline=bd,width=1)
    d.text((W/2-22,H-35),"暂停下载",fill=txt,font=F_SMALL)
    img.save(out)

def render_skills(theme, out):
    bg,panel,acc,accd,txt,txtd,bd,*_ = THEMES[theme]
    img=Image.new("RGB",(W,H),bg); d=ImageDraw.Draw(img)
    render_topbar(d,theme)
    d.text((10,8),"技能",fill=acc,font=F_BODY)
    # tabs
    d.rounded_rectangle([10,32,90,52],3,fill=accd,outline=acc,width=1)
    d.text((28,36),"已安装",fill=acc,font=F_SMALL)
    d.rounded_rectangle([100,32,180,52],3,outline=bd,width=1)
    d.text((124,36),"商店",fill=txtd,font=F_SMALL)
    d.rounded_rectangle([W-50,32,W-10,52],3,outline=bd,width=1)
    d.text((W-43,36),"同步",fill=txtd,font=F_SMALL)
    skills=[("🌤  weather_today","查询任意城市天气"),
            ("⏰  alarm","定时与闹钟"),
            ("🍅  pomodoro","番茄钟"),
            ("🦜  parrot","鹦鹉学舌")]
    for i,(n,desc) in enumerate(skills):
        y=60+i*40
        d.rounded_rectangle([10,y,W-10,y+34],4,fill=panel,outline=bd,width=1)
        d.text((18,y+5),n,fill=txt,font=F_BODY)
        d.text((18,y+19),desc,fill=txtd,font=F_SMALL)
        # checkbox
        d.rounded_rectangle([W-30,y+11,W-18,y+23],2,outline=acc,width=1)
        d.line([W-28,y+17,W-25,y+20],fill=acc); d.line([W-25,y+20,W-20,y+13],fill=acc)
    img.save(out)

def render_skill_detail(theme, out):
    bg,panel,acc,accd,txt,txtd,bd,*_ = THEMES[theme]
    img=Image.new("RGB",(W,H),bg); d=ImageDraw.Draw(img)
    render_topbar(d,theme)
    d.text((10,8),"weather_today",fill=acc,font=F_BODY)
    d.text((W-50,8),"v1.0.2",fill=txtd,font=F_SMALL)
    d.text((20,40),"🌤  查询任意城市当日天气",fill=txt,font=F_BODY)
    d.text((20,62),"作者:mira-team",fill=txtd,font=F_SMALL)
    d.text((20,76),"大小:4 KB · 更新:3 天前",fill=txtd,font=F_SMALL)
    d.text((20,98),"需要权限:",fill=txt,font=F_SMALL)
    d.text((30,114),"• 网络访问 (wttr.in)",fill=txtd,font=F_SMALL)
    # checkbox
    d.rounded_rectangle([20,140,30,150],2,outline=acc,width=1)
    d.text((36,140),"我已了解并同意上述权限",fill=txt,font=F_SMALL)
    # buttons
    d.rounded_rectangle([60,180,150,206],4,fill=acc)
    d.text((92,186),"安装",fill=bg,font=F_BODY)
    d.rounded_rectangle([170,180,260,206],4,outline=bd,width=1)
    d.text((202,186),"取消",fill=txt,font=F_BODY)
    img.save(out)

def render_settings(theme, out):
    bg,panel,acc,accd,txt,txtd,bd,*_ = THEMES[theme]
    img=Image.new("RGB",(W,H),bg); d=ImageDraw.Draw(img)
    # top
    d.rectangle([0,0,W,28],fill=panel)
    stub_icon(d,8,7,"chevron_left" if False else "settings",acc,14) if False else None
    d.text((10,8),"系统设置",fill=acc,font=F_BODY)
    # sliders
    for i,(lbl,val,ic) in enumerate([("音量",70,"volume"),("亮度",80,"sun")]):
        y=36+i*22
        d.rounded_rectangle([10,y,W-10,y+18],4,fill=panel,outline=bd,width=1)
        stub_icon(d,16,y+2,ic,txtd,12)
        d.rounded_rectangle([34,y+8,W-50,y+11],2,fill=bd)
        d.rounded_rectangle([34,y+8,34+(W-84)*val/100,y+11],2,fill=acc)
        d.text((W-44,y+4),f"{val}%",fill=txtd,font=F_SMALL)
    # rows
    items=[("Wi-Fi 网络","TP-LINK_EB17","wifi"),
           ("AI 模型","Qwen-7B","cpu"),
           ("技能管理","6 已装","puzzle"),
           ("远端 MCP","关","plug"),
           ("隐私设置","默认安全","shield"),
           ("关于","v6.2 · A1B2","info")]
    for i,(lbl,val,ic) in enumerate(items):
        y=86+i*22
        d.rounded_rectangle([10,y,W-10,y+18],4,fill=panel,outline=bd,width=1)
        stub_icon(d,16,y+2,ic,acc,12)
        d.text((34,y+4),lbl,fill=txt,font=F_BODY)
        d.text((W-90,y+4),val,fill=txtd,font=F_SMALL)
        d.polygon([(W-22,y+6),(W-18,y+9),(W-22,y+12)],outline=txtd,width=1)
    img.save(out)

def render_console(theme, out):
    bg,panel,acc,accd,txt,txtd,bd,*_ = THEMES[theme]
    img=Image.new("RGB",(W,H),bg); d=ImageDraw.Draw(img)
    d.text((12,8),"开发者控制台",fill=acc,font=F_BODY)
    d.rounded_rectangle([W-30,6,W-10,22],8,outline=acc,width=1)
    d.line([W-25,11,W-15,17],fill=acc); d.line([W-25,17,W-15,11],fill=acc)
    tabs=[("体感测试",True),("场景模拟",False),("记忆数据",False)]
    tw=(W-24)/3
    for i,(t,on) in enumerate(tabs):
        x=12+i*tw
        if on:
            d.rounded_rectangle([x,32,x+tw-4,50],3,fill=accd,outline=acc,width=1)
            d.text((x+12,36),t,fill=acc,font=F_SMALL)
        else:
            d.rounded_rectangle([x,32,x+tw-4,50],3,outline=bd,width=1)
            d.text((x+12,36),t,fill=txtd,font=F_SMALL)
    grid=[("Pitch>15° → curious","cpu"),("Pitch<-15° → yawn","cpu"),
          ("Roll>15° → look_R","cpu"),("Roll<-15° → look_L","cpu"),
          ("Shake → dizzy","alert"),("Tap Z>1.5g → wink","plug"),
          ("Gyro Z → naughty","refresh"),("Idle 30s → breath","clock")]
    for i,(lab,ic) in enumerate(grid):
        col,row=i%2,i//2
        x=12+col*((W-24)/2); y=58+row*32
        d.rounded_rectangle([x,y,x+(W-24)/2-4,y+28],4,fill=panel,outline=bd,width=1)
        stub_icon(d,x+6,y+8,ic,acc,12)
        d.text((x+24,y+9),lab,fill=txt,font=F_SMALL)
    img.save(out)

def render_boot(theme, out):
    bg,panel,acc,accd,txt,txtd,bd,*_ = THEMES[theme]
    img=Image.new("RGB",(W,H),bg); d=ImageDraw.Draw(img)
    d.text((W/2-30,80),"MiraBot",fill=acc,font=F_TITLE)
    # progress strip
    d.rectangle([W/2-100,120,W/2+100,124],fill=panel,outline=bd)
    d.rectangle([W/2-100,120,W/2-30,124],fill=acc)
    d.text((W/2-46,140),"SYSTEM BOOTING...",fill=txtd,font=F_SMALL)
    img.save(out)

PAGES=[("home",render_home),("menu",render_menu),("chat",render_chat),
       ("wifi_ap",render_wifi_ap),("wifi_pair",render_wifi_pair),
       ("ota",render_ota),("skills",render_skills),("skill_detail",render_skill_detail),
       ("settings",render_settings),("console",render_console),("boot",render_boot)]

mr=A("..","mock_renders") if False else A("mock_renders") if False else os.path.join(os.path.dirname(__file__) if False else ROOT, "mock_renders")
mr = A("mock_renders") if False else os.path.join(ROOT,"mock_renders")
os.makedirs(mr,exist_ok=True)
# Render all pages × all themes
count=0
for tn in THEMES.keys():
    for pname,fn in PAGES:
        fn(tn, os.path.join(mr,f"{pname}_{tn}.png"))
        count+=1
print(f"[render] {count} mock PNGs")
print("DONE")
