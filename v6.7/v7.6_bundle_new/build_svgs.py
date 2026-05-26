#!/usr/bin/env python3
"""Render all v7.6 mocks as 320x240 SVG, then upscale-rasterize to 640x480 PNG.

Each output:
  /home/mira/.session/117238480403/v7.6_bundle/svg/<name>.svg   — vector source
  /home/mira/.session/117238480403/v7.6_bundle/svg/<name>.png   — 640x480 raster

This script writes pure SVG strings (no external icon lib). For lucide-style
icons we inline simplified path approximations sufficient for design preview.
"""
import os, base64, io
from xml.sax.saxutils import escape

SVG_DIR = '/home/mira/.session/117238480403/v7.6_bundle/svg'
os.makedirs(SVG_DIR, exist_ok=True)

# ---- theme ------------------------------------------------------------------
TECH = dict(
    bg='#000000', panel='#0a1e28', accent='#22D3EE', accent_hi='#33C5FF',
    text='#b4ebff', border='#14506e', danger='#F43F5E',
)
LAVENDER = dict(
    bg='#FAF5FF', panel='#FFFFFF', accent='#9333EA', accent_hi='#A855F7',
    text='#4C1D95', border='#E9D5FF', danger='#EF4444',
)
CHILD = dict(
    bg='#FFF9E6', panel='#FFFFFF', accent='#FF7F50', accent_hi='#FFAA78',
    text='#c85a28', border='#FFC8A0', danger='#F43F5E',
)
COCOA = dict(
    bg='#2D1B0E', panel='#3F2B20', accent='#FB7185', accent_hi='#FDA4AF',
    text='#FEF3C7', border='#573D2C', danger='#EF4444',
)
THEMES = {'tech': TECH, 'lavender': LAVENDER, 'child': CHILD, 'cocoa': COCOA}

# ---- svg helpers ------------------------------------------------------------
W, H = 320, 240

def svg_open(bg, extra_defs=''):
    return (f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {W} {H}" '
            f'width="640" height="480" font-family="Noto Sans CJK SC, DejaVu Sans, sans-serif" '
            f'shape-rendering="geometricPrecision">'
            f'<defs>{extra_defs}</defs>'
            f'<rect width="{W}" height="{H}" fill="{bg}"/>')

def svg_close():
    return '</svg>'

def rect(x, y, w, h, fill='none', stroke='none', sw=1, rx=0, ry=None, opacity=1):
    ry = rx if ry is None else ry
    return (f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="{rx}" ry="{ry}" '
            f'fill="{fill}" stroke="{stroke}" stroke-width="{sw}" opacity="{opacity}"/>')

def circle(cx, cy, r, fill='none', stroke='none', sw=1, opacity=1):
    return (f'<circle cx="{cx}" cy="{cy}" r="{r}" fill="{fill}" stroke="{stroke}" '
            f'stroke-width="{sw}" opacity="{opacity}"/>')

def text(x, y, s, fill='#fff', size=10, weight='normal', anchor='start', opacity=1, ls=0):
    return (f'<text x="{x}" y="{y}" fill="{fill}" font-size="{size}" font-weight="{weight}" '
            f'text-anchor="{anchor}" opacity="{opacity}" letter-spacing="{ls}">{escape(s)}</text>')

def line(x1, y1, x2, y2, stroke, sw=1, opacity=1, cap='round'):
    return (f'<line x1="{x1}" y1="{y1}" x2="{x2}" y2="{y2}" stroke="{stroke}" '
            f'stroke-width="{sw}" stroke-linecap="{cap}" opacity="{opacity}"/>')

def path(d, fill='none', stroke='none', sw=1, opacity=1):
    return (f'<path d="{d}" fill="{fill}" stroke="{stroke}" stroke-width="{sw}" '
            f'stroke-linecap="round" stroke-linejoin="round" opacity="{opacity}"/>')

# ---- lucide-style icon shims (stroke 1.2 in 320 viewBox ≈ 2.4 in 24 grid) ----
def icon_msg(cx, cy, c, s=8):
    """MessageSquare 16px box centered at cx,cy."""
    x, y = cx - s, cy - s
    return path(f"M{x} {y+1.5} q0 -1.5 1.5 -1.5 h{2*s-3} q1.5 0 1.5 1.5 v{2*s-5} q0 1.5 -1.5 1.5 h{2*s-7} l-3 3 v-3 h-2 q-1.5 0 -1.5 -1.5 z",
                stroke=c, sw=1.2)

def icon_sparkles(cx, cy, c, s=8):
    return (path(f"M{cx} {cy-s} l1.5 4 4 1.5 -4 1.5 -1.5 4 -1.5 -4 -4 -1.5 4 -1.5 z", fill=c) +
            path(f"M{cx+s-2} {cy+s-3} l1 2 2 1 -2 1 -1 2 -1 -2 -2 -1 2 -1 z", fill=c))

def icon_palette(cx, cy, c, s=8):
    return (path(f"M{cx} {cy-s} a{s} {s} 0 1 0 {s-1} {s+2} q-{s-1} 0 -{s-1} -{s+2} a{s} {s} 0 0 0 0 0 z", stroke=c, sw=1.2) +
            circle(cx-3, cy-3, 1.2, fill=c) + circle(cx+3, cy-3, 1.2, fill=c) +
            circle(cx+3.5, cy+1, 1.2, fill=c))

def icon_settings(cx, cy, c, s=8):
    # simplified gear: octagon ring + center hole
    pts = []
    import math
    for i in range(8):
        a = math.pi/8 + i*math.pi/4
        r1, r2 = s, s-2
        pts.append((cx + r1*math.cos(a), cy + r1*math.sin(a)))
        a2 = a + math.pi/8
        pts.append((cx + r2*math.cos(a2), cy + r2*math.sin(a2)))
    d = "M " + " L ".join(f"{x:.1f} {y:.1f}" for x,y in pts) + " Z"
    return path(d, stroke=c, sw=1.2) + circle(cx, cy, 2, stroke=c, sw=1.2)

def icon_user(cx, cy, c, s=8):
    return (rect(cx-s+1, cy-s+1, 2*s-2, 2*s-2, stroke=c, sw=1.2, rx=2) +
            circle(cx, cy-1.5, 2.5, stroke=c, sw=1.2) +
            path(f"M{cx-4} {cy+s-2} q4 -3 8 0", stroke=c, sw=1.2))

def icon_brain(cx, cy, c, s=8):
    return (path(f"M{cx-s} {cy} q0 -{s} {s/1.5} -{s} q1 0 2 1 q1 -1 2 -1 q{s/1.5} 0 {s/1.5} {s} q0 {s-2} -{s/1.5} {s} q-1 0 -2 -1 q-1 1 -2 1 q-{s/1.5} 0 -{s/1.5} -{s} z", stroke=c, sw=1.2) +
            line(cx, cy-s+1, cx, cy+s-1, c, 1))

def icon_close(cx, cy, c, s=6):
    return (line(cx-s, cy-s, cx+s, cy+s, c, 1.5) +
            line(cx-s, cy+s, cx+s, cy-s, c, 1.5))

def icon_chevron_left(cx, cy, c, s=6):
    return path(f"M{cx+s/2} {cy-s} l-{s} {s} l{s} {s}", stroke=c, sw=1.5)

def icon_chevron_right(cx, cy, c, s=4):
    return path(f"M{cx-s/2} {cy-s} l{s} {s} l-{s} {s}", stroke=c, sw=1.2)

def icon_battery(cx, cy, c, fill_color=None, level=0.85, danger=False):
    # 16-wide battery, body 13x7, tip 2x4
    bx, by = cx-7, cy-3
    fc = fill_color or c
    color = c
    out = rect(bx, by, 12, 6, stroke=color, sw=1.2, rx=1)
    out += rect(bx+12, by+1.5, 1.5, 3, fill=color)
    if not danger:
        out += rect(bx+1, by+1, 10*level, 4, fill=fc, rx=0.5)
    else:
        # warning triangle inside
        out += path(f"M{cx} {by+1.2} l3 4 h-6 z", stroke=color, sw=1, fill='none')
        out += line(cx, by+2.5, cx, by+3.8, color, 0.9)
        out += circle(cx, by+4.4, 0.4, fill=color)
    return out

def icon_wifi(cx, cy, c, offline=False, danger='#F43F5E'):
    arc1 = path(f"M{cx-7} {cy-1} q7 -7 14 0", stroke=c, sw=1.2, opacity=0.4 if offline else 0.8)
    arc2 = path(f"M{cx-4} {cy+1.5} q4 -4 8 0", stroke=c, sw=1.2, opacity=0.4 if offline else 0.8)
    dot = circle(cx, cy+3.5, 0.9, fill=c, opacity=0.4 if offline else 0.8)
    out = arc1 + arc2 + dot
    if offline:
        out += line(cx-8, cy-3, cx+8, cy+5, danger, 1.5)
    return out

def icon_plug(cx, cy, c):
    return (rect(cx-3, cy-4, 6, 8, stroke=c, sw=1.2, rx=1) +
            line(cx-2, cy-6, cx-2, cy-4, c, 1.2) +
            line(cx+2, cy-6, cx+2, cy-4, c, 1.2) +
            line(cx, cy+4, cx, cy+6, c, 1.2))

def icon_qrcode(cx, cy, size=80):
    # render a simple qr-like grid (deterministic pattern)
    import random
    rnd = random.Random(7)
    cell = size / 21
    parts = [rect(cx-size/2-2, cy-size/2-2, size+4, size+4, fill='#fff')]
    for r in range(21):
        for c in range(21):
            # finder squares 7x7 at corners
            inside_finder = ((r<7 and c<7) or (r<7 and c>=14) or (r>=14 and c<7))
            if inside_finder:
                if (r in (0,6) or c in (0,6) or (r>=2 and r<=4 and c>=2 and c<=4)
                    or (r in (0,6) and (r<7 and c>=14) and False)):
                    pass
                # draw filled border + center
                if (r in (0,6) or c in (0,6)) or (2<=r<=4 and 2<=c<=4):
                    parts.append(rect(cx-size/2 + (c if (r<7 and c<7) else c-14)*cell + (size-7*cell if c>=14 else 0),
                                      cy-size/2 + (r if r<7 else r-14)*cell + (size-7*cell if r>=14 else 0),
                                      cell, cell, fill='#000'))
            else:
                if rnd.random() < 0.45:
                    parts.append(rect(cx-size/2 + c*cell, cy-size/2 + r*cell, cell, cell, fill='#000'))
    return ''.join(parts)

# ---- common bits -----------------------------------------------------------
def status_bar(t, mode='always', battery=88, charging=True, offline=False, lowbat=False):
    """22-px high status bar at the top, 4 slots right-aligned."""
    if mode == 'idle':
        return ''
    parts = [rect(0, 0, W, 22, fill=t['bg'], opacity=0.5)]
    x = W - 8
    # battery
    bw = 16
    parts.append(text(x-bw-3, 14, f"{battery}%", fill=t['danger'] if lowbat else t['text'],
                      size=8, anchor='end'))
    parts.append(icon_battery(x-8, 11, t['danger'] if lowbat else t['text'],
                              fill_color=t['accent'] if not lowbat else None,
                              danger=lowbat, level=battery/100))
    x = x - bw - 24
    # wifi
    parts.append(icon_wifi(x, 11, t['text'], offline=offline, danger=t['danger']))
    x -= 18
    # ai dot
    parts.append(circle(x, 11, 4, fill=t['accent'], opacity=0.85))
    x -= 14
    # plug
    if charging:
        parts.append(icon_plug(x, 11, t['accent']))
    return ''.join(parts)

def subpage_header(t, title, right_text=None):
    out = [rect(0, 22, W, 28, fill=t['bg'])]  # bg under bar
    out.append(line(0, 50, W, 50, t['border'], 1))
    out.append(icon_chevron_left(14, 36, t['accent']))
    out.append(text(26, 41, title, fill=t['text'], size=12, weight='700'))
    if right_text:
        out.append(rect(W-58, 28, 50, 18, fill=t['panel'], stroke=t['border'], sw=1, rx=2))
        out.append(text(W-33, 41, right_text, fill=t['text'], size=9, anchor='middle'))
    return ''.join(out)

# ---- face (idle) ------------------------------------------------------------
def face(t, state='idle'):
    """Render a face at center of (W,H) area (excluding any header)."""
    cx, cy = W // 2, 130
    out = []
    glow_filter = ''
    if state == 'idle':
        ew, eh = 32, 64  # scale 1.6, base 32x40 -> still 32x64
        out.append(rect(cx-44-ew/2, cy-eh/2, ew, eh, fill=t['accent_hi'], rx=16, opacity=0.95))
        out.append(rect(cx+44-ew/2, cy-eh/2, ew, eh, fill=t['accent_hi'], rx=16, opacity=0.95))
        out.append(rect(cx-12, cy+30, 24, 4, fill=t['accent_hi'], rx=2, opacity=0.95))
    elif state == 'lost':
        # half-closed sad eyes + downward mouth arc
        out.append(rect(cx-44-16, cy-2, 32, 24, fill=t['accent_hi'], rx=10, opacity=0.95))
        out.append(rect(cx+44-16, cy-2, 32, 24, fill=t['accent_hi'], rx=10, opacity=0.95))
        out.append(path(f"M{cx-12} {cy+34} q12 8 24 0", stroke=t['accent_hi'], sw=4))
    elif state == 'boot':
        # mid-yawn: half eyes + open mouth
        out.append(rect(cx-44-16, cy-12, 32, 24, fill=t['accent_hi'], rx=10))
        out.append(rect(cx+44-16, cy-12, 32, 24, fill=t['accent_hi'], rx=10))
        out.append(rect(cx-14, cy+18, 28, 28, fill=t['accent_hi'], rx=14))
    elif state == 'menu':
        # peeking eyes (smaller, dimmed) — used in menu overlay
        out.append(rect(cx-44-10, cy-22, 20, 20, fill=t['accent_hi'], rx=10, opacity=0.25))
        out.append(rect(cx+44-10, cy-22, 20, 20, fill=t['accent_hi'], rx=10, opacity=0.25))
    return ''.join(out)

def rgb_strip(t, color=None, opacity=0.6):
    color = color or t['accent_hi']
    return rect(0, H-3, W, 3, fill=color, opacity=opacity)

# ---- pages ------------------------------------------------------------------

def page_boot(t):
    s = [svg_open(t['bg'])]
    s.append(face(t, 'boot'))
    s.append(svg_close())
    return ''.join(s)

def page_idle_clean(t):
    s = [svg_open(t['bg'])]
    s.append(face(t, 'idle'))
    s.append(rgb_strip(t, opacity=0.6))
    s.append(svg_close())
    return ''.join(s)

def page_idle_peek(t):
    s = [svg_open(t['bg'])]
    s.append(face(t, 'idle'))
    s.append(status_bar(t, 'always', battery=88, charging=True))
    s.append(rgb_strip(t))
    s.append(svg_close())
    return ''.join(s)

def page_critical_lowbat(t):
    s = [svg_open(t['bg'])]
    s.append(face(t, 'lost'))
    s.append(status_bar(t, 'always', battery=12, charging=False, lowbat=True))
    s.append(rgb_strip(t, color=t['danger'], opacity=0.7))
    s.append(svg_close())
    return ''.join(s)

def page_menu(t, hover='chat'):
    s = [svg_open(t['bg'])]
    # background face (dimmed)
    s.append(rect(0, 0, W, H, fill=t['bg']))
    s.append(face(t, 'menu'))
    # backdrop
    s.append(rect(0, 0, W, H, fill=t['bg'], opacity=0.8))
    s.append(status_bar(t, 'idle'))  # no
    cx, cy = W//2, H//2
    R = 78/2  # remember 320 viewBox is 1:1 with css px in v6.3
    # actually radius in code is 78 in 320 viewBox; keep
    R = 78
    sectors = [
        ('chat',     -90, 'msg'),
        ('model',    -30, 'spk'),
        ('theme',     30, 'pal'),
        ('settings',  90, 'set'),
        ('persona',  150, 'usr'),
        ('memory',   210, 'brn'),
    ]
    import math
    for sid, ang, ic in sectors:
        rad = math.radians(ang)
        x = cx + math.cos(rad) * R
        y = cy + math.sin(rad) * R
        is_hover = (sid == hover)
        size = 32 if not is_hover else 35  # 56→28 in viewBox, scaled
        fill = t['panel'] if is_hover else t['panel']
        opa = 1 if is_hover else 0.8
        border_col = t['accent'] if is_hover else t['border']
        s.append(circle(x, y, size/2, fill=fill, stroke=border_col, sw=1.2, opacity=opa))
        if is_hover:
            # outer glow ring
            s.append(circle(x, y, size/2+3, fill='none', stroke=t['accent'], sw=1, opacity=0.5))
            s.append(circle(x, y, size/2+6, fill='none', stroke=t['accent'], sw=0.5, opacity=0.3))
        ic_col = t['accent_hi'] if is_hover else t['accent']
        if ic == 'msg':  s.append(icon_msg(x, y, ic_col, s=6))
        if ic == 'spk':  s.append(icon_sparkles(x, y, ic_col, s=6))
        if ic == 'pal':  s.append(icon_palette(x, y, ic_col, s=6))
        if ic == 'set':  s.append(icon_settings(x, y, ic_col, s=6))
        if ic == 'usr':  s.append(icon_user(x, y, ic_col, s=6))
        if ic == 'brn':  s.append(icon_brain(x, y, ic_col, s=6))
    # center close
    s.append(circle(cx, cy, 12, fill=t['panel'], stroke=t['border'], sw=1, opacity=0.7))
    s.append(icon_close(cx, cy, t['accent'], s=4))
    # hover label at top center
    if hover:
        labels = dict(chat='对话', model='模型', theme='主题',
                      settings='设置', persona='人格', memory='记忆')
        s.append(text(cx, cy-3, labels[hover], fill=t['text'], size=11,
                      weight='700', anchor='middle', ls=2))
    s.append(svg_close())
    return ''.join(s)

def page_model_picker(t):
    s = [svg_open(t['bg'])]
    s.append(status_bar(t, 'always'))
    s.append(subpage_header(t, '大模型选择'))
    models = [
        ('ⓘ Auto (智能路由)',          '',           True),
        ('💎 GPT-4o',                  '云端 · 专业版', False),
        ('🎭 Claude 3.5 Sonnet',       '云端 · 专业版', False),
        ('🚀 Doubao Pro',              '云端 · 标准版', False),
        ('🦾 Qwen-2.5 1.5B',           '本地 · 极速版', False),
        ('📱 Phi-3 mini',              '本地 · 极速版', False),
    ]
    y = 56
    for label, sub, sel in models:
        # card
        card_op = 0.3 if t==TECH or t==COCOA else 1
        s.append(rect(8, y, W-16, 18, fill=t['panel'], stroke=t['border'], sw=0.6, rx=2, opacity=card_op))
        # radio
        if sel:
            s.append(circle(16, y+9, 3.5, fill=t['accent']))
            s.append(circle(16, y+9, 1.5, fill=t['bg']))
        else:
            s.append(circle(16, y+9, 3.5, stroke=t['border'], sw=1))
        s.append(text(24, y+8, label, fill=t['text'], size=8))
        if sub:
            s.append(text(24, y+15, sub, fill=t['text'], size=6, opacity=0.6))
        if sel:
            s.append(text(W-12, y+11, '✓', fill=t['accent'], size=9, anchor='end'))
        y += 20
    # footer
    s.append(rect(0, H-18, W, 18, fill=t['panel'], opacity=0.3 if t in (TECH,COCOA) else 1))
    s.append(line(0, H-18, W, H-18, t['border'], 0.6))
    s.append(text(W/2, H-7, '切换会立即生效，正在进行的对话不打断', fill=t['text'],
                  size=6, opacity=0.6, anchor='middle'))
    s.append(svg_close())
    return ''.join(s)

def page_persona(t):
    s = [svg_open(t['bg'])]
    s.append(status_bar(t, 'always'))
    s.append(subpage_header(t, '人格配置'))
    items = [
        ('🎵','Lyra','温柔诗人',   True),
        ('🪞','Echo','话痨复读机', False),
        ('🌟','Nova','极客科普',   False),
        ('🦉','Sage','冷静顾问',   False),
        ('🐣','Pico','童趣小鸡',   False),
        ('🩺','Doc', '严谨医师',   False),
    ]
    card_w, card_h = 96, 70
    gap = 8
    grid_w = card_w*3 + gap*2
    x0 = (W - grid_w)//2
    y0 = 58
    card_op = 0.3 if t in (TECH,COCOA) else 1
    for idx, (emoji, name, desc, sel) in enumerate(items):
        col, row = idx % 3, idx // 3
        x = x0 + col*(card_w+gap)
        y = y0 + row*(card_h+gap)
        border_col = t['accent'] if sel else t['border']
        sw = 1.6 if sel else 0.8
        s.append(rect(x, y, card_w, card_h, fill=t['panel'], stroke=border_col,
                      sw=sw, rx=3, opacity=card_op))
        s.append(text(x+card_w/2, y+24, emoji, size=18, anchor='middle'))
        s.append(text(x+card_w/2, y+42, name, fill=t['accent'] if sel else t['text'],
                      size=10, weight='700', anchor='middle'))
        s.append(text(x+card_w/2, y+56, f'「{desc}」', fill=t['text'],
                      size=7, opacity=0.7, anchor='middle'))
    s.append(svg_close())
    return ''.join(s)

def page_memory(t, modal=False):
    s = [svg_open(t['bg'])]
    s.append(status_bar(t, 'always'))
    s.append(subpage_header(t, '长程记忆', right_text='清空'))
    items = [
        ('msg','chat · 「用户喜欢喝美式咖啡」', '2 小时前'),
        ('set','settings · 「主题=tech」',     '3 天前'),
        ('pkg','skills · 「pomodoro 已安装」', '1 周前'),
        ('act','usage · 「用户偏好早晨活跃」', '2 周前'),
        ('gl', 'net · 「家庭 Wi-Fi 自动连接」', '1 个月前'),
        ('msg','chat · 「不爱吃香菜」',         '1 个月前'),
    ]
    y = 56
    h = 28
    for ic, body, time in items:
        s.append(line(8, y+h, W-8, y+h, t['border'], 0.5))
        # icon
        cx, cy = 16, y+h/2
        col = t['accent']
        if ic == 'msg': s.append(icon_msg(cx, cy, col, s=5))
        if ic == 'set': s.append(icon_settings(cx, cy, col, s=5))
        if ic == 'pkg': s.append(rect(cx-5, cy-4, 10, 8, stroke=col, sw=1, rx=1) +
                                  line(cx-5, cy, cx+5, cy, col, 1))
        if ic == 'act': s.append(path(f"M{cx-5} {cy} h2 l1 -3 l2 6 l1 -3 h4", stroke=col, sw=1.2))
        if ic == 'gl':  s.append(circle(cx, cy, 4.5, stroke=col, sw=1) +
                                  path(f"M{cx-4.5} {cy} h9 M{cx} {cy-4.5} q3 4.5 0 9 q-3 -4.5 0 -9", stroke=col, sw=0.8))
        s.append(text(28, y+11, body, fill=t['text'], size=8))
        s.append(text(28, y+22, time, fill=t['text'], size=6, opacity=0.6))
        # trash
        s.append(path(f"M{W-18} {cy-3} h6 M{W-19} {cy-3} v6 a1 1 0 0 0 1 1 h4 a1 1 0 0 0 1 -1 v-6",
                      stroke=t['danger'], sw=1))
        y += h
    if modal:
        # full overlay
        s.append(rect(0, 0, W, H, fill='#000', opacity=0.4))
        my = H - 78
        s.append(rect(0, my, W, 78, fill=t['panel'], stroke=t['border'], sw=1, rx=4))
        s.append(text(12, my+18, '清空所有长程记忆?', fill=t['text'], size=11, weight='700'))
        s.append(text(12, my+33, '这只清除记忆里的对话偏好/学习数据，', fill=t['text'], size=7, opacity=0.7))
        s.append(text(12, my+42, '不影响设置和 Wi-Fi 状态', fill=t['text'], size=7, opacity=0.7))
        # buttons
        bw = (W - 24 - 8)/2
        s.append(rect(12, my+50, bw, 20, fill='none', stroke=t['accent'], sw=1, rx=3))
        s.append(text(12+bw/2, my+63, '取消', fill=t['accent'], size=9, weight='700', anchor='middle'))
        s.append(rect(12+bw+8, my+50, bw, 20, fill=t['danger'], rx=3))
        s.append(text(12+bw+8+bw/2, my+63, '全部清空', fill='#fff', size=9, weight='700', anchor='middle'))
    s.append(svg_close())
    return ''.join(s)

def page_settings(t):
    s = [svg_open(t['bg'])]
    # full panel background
    s.append(rect(0, 0, W, H, fill=t['bg'], opacity=0.95))
    s.append(status_bar(t, 'always'))
    # custom header (no chevron line)
    s.append(rect(8, 28, 22, 18, fill=t['panel'], rx=9, opacity=0.7))
    s.append(icon_chevron_left(19, 37, t['text']))
    s.append(text(W/2, 41, '系统设置', fill=t['text'], size=10, weight='700',
                  anchor='middle', ls=3))
    # groups
    groups = [
        ('通用', [
            ('user','账号绑定','User01', False, False),
            ('gl',  '语言设置','简体中文', False, False),
            ('clk', '睡眠定时','30分钟', False, False),
        ], None),
        ('显示与声音', [], None),  # sliders below
        ('拓展功能', [
            ('wifi','Wi-Fi网络','未连接', False, False),
            ('pkg', '技能插件','3 个已安装', False, False),
        ], None),
        ('系统', [
            ('dl',  '系统更新','有新版本', True, False),
            ('warn','清除偏好数据','', True, False),
            ('info','关于系统','v7.2', False, False),
        ], None),
        ('开发者选项', [
            ('term','控制台','', False, False),
            ('warn','详细日志','', False, True),
        ], '测试'),
    ]
    card_op = 0.3 if t in (TECH, COCOA) else 1
    y = 54
    for title, items, badge in groups:
        s.append(text(10, y+6, title, fill=t['text'], size=7, opacity=0.7, weight='700'))
        if badge:
            s.append(rect(10 + len(title)*5 + 4, y+1, 16, 7, fill=t['accent'], rx=1))
            s.append(text(10 + len(title)*5 + 12, y+6, badge, fill='#000', size=5,
                          weight='700', anchor='middle'))
        y += 9
        if title == '显示与声音':
            # 2 sliders
            s.append(rect(8, y, W-16, 24, fill=t['panel'], stroke=t['border'], sw=0.6, rx=2, opacity=card_op))
            for i, (ic, val) in enumerate([('sun', 80), ('vol', 70)]):
                yy = y + 4 + i*11
                if ic == 'sun':
                    s.append(circle(14, yy+3, 2.5, stroke=t['text'], sw=1, opacity=0.7))
                    for ang in range(0, 360, 45):
                        import math
                        a = math.radians(ang)
                        s.append(line(14+math.cos(a)*5, yy+3+math.sin(a)*5,
                                      14+math.cos(a)*7, yy+3+math.sin(a)*7, t['text'], 0.6, opacity=0.7))
                else:
                    s.append(path(f"M10 {yy+1} v4 h2 l3 2 v-8 l-3 2 z", fill=t['text'], opacity=0.7))
                    s.append(path(f"M16 {yy} q2 3 0 6", stroke=t['text'], sw=0.8, opacity=0.7))
                s.append(rect(22, yy+2.4, W-30, 1.2, fill=t['text'], opacity=0.2, rx=0.6))
                s.append(rect(22, yy+2.4, (W-30)*(val/100), 1.2, fill=t['accent'], rx=0.6))
            y += 26
            continue
        # generic action list
        if items:
            s.append(rect(8, y, W-16, len(items)*15, fill=t['panel'], stroke=t['border'],
                          sw=0.6, rx=2, opacity=card_op))
            for ii, (ic, label, val, alert, toggle) in enumerate(items):
                yy = y + ii*15
                # icon
                col = t['danger'] if alert else t['text']
                cx0 = 14
                if ic in ('user',): icon_user(cx0, yy+7, col, s=4)
                # use small box icon
                s.append(rect(cx0-3, yy+4, 6, 6, stroke=col, sw=0.7, rx=1, opacity=0.7))
                s.append(text(24, yy+9, label, fill=col, size=8))
                if val:
                    s.append(text(W-20, yy+9, val, fill=col, size=7, anchor='end', opacity=0.7))
                if toggle:
                    s.append(rect(W-26, yy+5, 14, 7, fill=t['accent'], rx=3.5))
                    s.append(circle(W-15, yy+8.5, 2.5, fill=t['bg']))
                else:
                    s.append(icon_chevron_right(W-12, yy+9, col, s=2.5))
                if ii < len(items)-1:
                    s.append(line(8, yy+15, W-8, yy+15, t['border'], 0.3))
            y += len(items)*15 + 4
    s.append(svg_close())
    return ''.join(s)

def page_theme_picker(t):
    s = [svg_open(t['bg'])]
    s.append(status_bar(t, 'always'))
    s.append(subpage_header(t, '主题风格'))
    cards = [
        ('科技青',    'Tech',     '#22D3EE', True),
        ('柔紫',      'Lavender', '#9333EA', False),
        ('温暖儿童',  'Child',    '#FF7F50', False),
        ('草莓可可',  'Cocoa',    '#FB7185', False),
    ]
    cw, ch, gap = 148, 56, 8
    x0 = (W - cw*2 - gap)//2
    y0 = 58
    card_op = 0.3 if t in (TECH, COCOA) else 1
    for idx, (name, desc, dot, sel) in enumerate(cards):
        col, row = idx % 2, idx // 2
        x = x0 + col*(cw+gap)
        y = y0 + row*(ch+gap)
        border_col = t['accent'] if sel else t['border']
        sw = 1.8 if sel else 0.8
        s.append(rect(x, y, cw, ch, fill=t['panel'], stroke=border_col, sw=sw, rx=3, opacity=card_op))
        # dot
        dot_r = 5 if sel else 4
        s.append(circle(x + cw/2 - 22, y + ch/2 - 4, dot_r, fill=dot))
        # soft glow
        s.append(circle(x + cw/2 - 22, y + ch/2 - 4, dot_r+1.5, fill='none', stroke=dot, sw=0.4, opacity=0.5))
        s.append(text(x + cw/2 - 12, y + ch/2 - 1, name,
                      fill=t['accent'] if sel else t['text'], size=10, weight='700'))
        s.append(text(x + cw/2, y + ch/2 + 10, desc, fill=t['text'],
                      size=7, opacity=0.6, anchor='middle'))
    s.append(svg_close())
    return ''.join(s)

def page_console_tabs(t, active):
    """Header + tabs (system/ai/display/service). Returns y where content begins."""
    parts = []
    parts.append(status_bar(t, 'always'))
    parts.append(rect(0, 22, W, 28, fill=t['bg']))
    parts.append(line(0, 50, W, 50, t['border'], 1))
    parts.append(icon_chevron_left(14, 36, t['accent']))
    parts.append(text(26, 41, '开发者控制台', fill=t['text'], size=12, weight='700'))
    tabs = [('system','系统状态'), ('ai','AI引擎'),
            ('display','显示调试'), ('service','服务模拟')]
    tw = W / 4
    for i, (tid, tl) in enumerate(tabs):
        tx = i*tw
        is_act = (tid == active)
        col = t['accent'] if is_act else t['text']
        op = 1 if is_act else 0.55
        parts.append(text(tx + tw/2, 64, tl, fill=col, size=9, weight='700',
                          anchor='middle', opacity=op))
        if is_act:
            parts.append(rect(tx+4, 70, tw-8, 2, fill=t['accent'], rx=1))
    parts.append(line(0, 72, W, 72, t['border'], 1))
    return ''.join(parts)

def page_console_ai(t):
    s = [svg_open(t['bg'])]
    s.append(page_console_tabs(t, 'ai'))
    cards = [
        ('term', '运行日志', '查看最近运行日志'),
        ('spk',  '模型路由', '强制指定大模型'),
        ('brn',  '记忆管理', '列表 / 删除 / 清空'),
        ('pkg',  '技能调试', '调用及测试插件'),
    ]
    cw, ch, gap = 144, 38, 6
    x0 = (W - cw*2 - gap)//2
    y0 = 80
    card_op = 0.3 if t in (TECH, COCOA) else 1
    for idx, (ic, title, desc) in enumerate(cards):
        col, row = idx % 2, idx // 2
        x = x0 + col*(cw+gap)
        y = y0 + row*(ch+gap)
        s.append(rect(x, y, cw, ch, fill=t['panel'], stroke=t['border'], sw=0.6, rx=3, opacity=card_op))
        # icon at top-left
        cx, cy = x+10, y+12
        if ic == 'term': s.append(rect(cx-4, cy-3, 8, 6, stroke=t['accent'], sw=1, rx=1) + line(cx-2, cy, cx, cy, t['accent'], 0.8))
        if ic == 'spk':  s.append(icon_sparkles(cx, cy, t['accent'], s=4))
        if ic == 'brn':  s.append(icon_brain(cx, cy, t['accent'], s=4))
        if ic == 'pkg':  s.append(rect(cx-4, cy-3, 8, 6, stroke=t['accent'], sw=1, rx=1))
        s.append(text(x+19, y+12, title, fill=t['text'], size=8, weight='700'))
        s.append(text(x+8, y+26, desc, fill=t['text'], size=6, opacity=0.5))
    s.append(svg_close())
    return ''.join(s)

def page_console_system(t):
    s = [svg_open(t['bg'])]
    s.append(page_console_tabs(t, 'system'))
    # sensor card
    card_op = 0.3 if t in (TECH, COCOA) else 1
    s.append(rect(8, 80, W-16, 28, fill=t['panel'], stroke=t['border'], sw=0.6, rx=3, opacity=card_op))
    s.append(text(14, 90, '传感器状态', fill=t['text'], size=8, weight='700'))
    s.append(text(14, 100, 'IMU: 正常    光照: 420 lx', fill=t['text'], size=6, opacity=0.65))
    # somatosensory title
    s.append(text(8, 122, '体感测试', fill=t['text'], size=8, weight='700'))
    btns = ['前倾','后仰','左倾','右倾','摇晃','旋转']
    bw, bh, gap = 96, 18, 6
    x0 = (W - bw*3 - gap*2)//2
    y0 = 130
    for idx, label in enumerate(btns):
        col, row = idx % 3, idx // 3
        x = x0 + col*(bw+gap)
        y = y0 + row*(bh+gap)
        s.append(rect(x, y, bw, bh, fill=t['panel'], stroke=t['border'], sw=0.8, rx=2, opacity=card_op))
        s.append(text(x + bw/2, y + bh/2 + 3, label, fill=t['text'], size=8, anchor='middle'))
    s.append(svg_close())
    return ''.join(s)

def page_console_service(t):
    s = [svg_open(t['bg'])]
    s.append(page_console_tabs(t, 'service'))
    s.append(text(8, 88, '场景模拟', fill=t['text'], size=8, weight='700'))
    btns = [('🎤','语音唤醒'), ('🤖','OTA模拟'), ('📵','报错状态'),
            ('📱','语音通话'), ('🔋','低电量'),   ('🌡','过热')]
    bw, bh, gap = 96, 22, 6
    x0 = (W - bw*3 - gap*2)//2
    y0 = 96
    card_op = 0.3 if t in (TECH, COCOA) else 1
    for idx, (emoji, label) in enumerate(btns):
        col, row = idx % 3, idx // 3
        x = x0 + col*(bw+gap)
        y = y0 + row*(bh+gap)
        s.append(rect(x, y, bw, bh, fill=t['panel'], stroke=t['border'], sw=0.8, rx=2, opacity=card_op))
        s.append(text(x + bw/2, y + bh/2 + 3, f"{emoji} {label}", fill=t['text'],
                      size=8, anchor='middle'))
    s.append(svg_close())
    return ''.join(s)

def page_console_display(t):
    s = [svg_open(t['bg'])]
    s.append(page_console_tabs(t, 'display'))
    cards = [
        ('pal', '主题配置', '切换主题模式'),
        ('smi', '静态表情', '选择静态表情'),
        ('shf', '动态表情', '轮播表情模式'),
        ('bug', '表情调试', '自动遍历 26 帧'),
    ]
    cw, ch, gap = 144, 38, 6
    x0 = (W - cw*2 - gap)//2
    y0 = 80
    card_op = 0.3 if t in (TECH, COCOA) else 1
    for idx, (ic, title, desc) in enumerate(cards):
        col, row = idx % 2, idx // 2
        x = x0 + col*(cw+gap)
        y = y0 + row*(ch+gap)
        s.append(rect(x, y, cw, ch, fill=t['panel'], stroke=t['border'], sw=0.6, rx=3, opacity=card_op))
        cx, cy = x+10, y+12
        if ic == 'pal':  s.append(icon_palette(cx, cy, t['accent'], s=4))
        if ic == 'smi':  s.append(circle(cx, cy, 4, stroke=t['accent'], sw=1) +
                                   circle(cx-1.5, cy-1, 0.6, fill=t['accent']) +
                                   circle(cx+1.5, cy-1, 0.6, fill=t['accent']) +
                                   path(f"M{cx-2} {cy+1} q2 2 4 0", stroke=t['accent'], sw=0.8))
        if ic == 'shf':  s.append(path(f"M{cx-4} {cy-2} h2 l4 4 h2 M{cx-4} {cy+2} h2 l4 -4 h2",
                                        stroke=t['accent'], sw=1))
        if ic == 'bug':  s.append(circle(cx, cy, 3, stroke=t['accent'], sw=1) +
                                   line(cx-5, cy, cx-3, cy, t['accent'], 1) +
                                   line(cx+3, cy, cx+5, cy, t['accent'], 1))
        s.append(text(x+19, y+12, title, fill=t['text'], size=8, weight='700'))
        s.append(text(x+8, y+26, desc, fill=t['text'], size=6, opacity=0.5))
    s.append(svg_close())
    return ''.join(s)

def page_wifi_ap(t):
    s = [svg_open(t['bg'])]
    s.append(status_bar(t, 'always'))
    # back button (top-left floating)
    s.append(rect(6, 28, 18, 18, fill=t['panel'], rx=9, opacity=0.7))
    s.append(icon_chevron_left(15, 37, t['text']))
    cx, cy = W/2, H/2 - 10
    s.append(icon_qrcode(cx, cy, 70))
    s.append(text(cx, cy+50, '扫码配网', fill=t['accent'], size=8, weight='700', anchor='middle', ls=2))
    s.append(text(cx, cy+60, '热点: Robot_AP_1234', fill=t['text'], size=6, anchor='middle', opacity=0.7))
    # button
    s.append(rect(cx-40, cy+70, 80, 16, fill=t['panel'], stroke=t['border'], sw=0.8, rx=8, opacity=0.7))
    s.append(text(cx, cy+81, '模拟接收配置', fill=t['accent'], size=7, anchor='middle'))
    s.append(svg_close())
    return ''.join(s)

# ---- build all --------------------------------------------------------------
MOCKS = [
    ('p01_boot',                          'tech', page_boot),
    ('p02_home_idle_clean',               'tech', page_idle_clean),
    ('p02_home_peek',                     'tech', page_idle_peek),
    ('p02_home_critical_lowbat',          'tech', page_critical_lowbat),
    ('p03_menu',                          'tech', lambda t: page_menu(t, 'chat')),
    ('p06_model_picker',                  'tech', page_model_picker),
    ('p07_persona_grid',                  'tech', page_persona),
    ('p08_memory_browser',                'tech', lambda t: page_memory(t, modal=False)),
    ('p08_memory_purge_modal',            'tech', lambda t: page_memory(t, modal=True)),
    ('p09_settings',                      'tech', page_settings),
    ('p10_theme_picker_4themes',          'tech', page_theme_picker),
    ('p11_console_ai',                    'tech', page_console_ai),
    ('p11_console_system',                'tech', page_console_system),
    ('p11_console_service',               'tech', page_console_service),
    # bonus
    ('p11_console_display',               'tech', page_console_display),
    ('p_wifi_ap',                         'tech', page_wifi_ap),
    # 4-theme comparison thumbnails of idle
    ('p_theme_preview_tech',              'tech',     page_idle_peek),
    ('p_theme_preview_lavender',          'lavender', page_idle_peek),
    ('p_theme_preview_child',             'child',    page_idle_peek),
    ('p_theme_preview_cocoa',             'cocoa',    page_idle_peek),
]

written = []
for name, theme_key, fn in MOCKS:
    t = THEMES[theme_key]
    svg = fn(t)
    p = os.path.join(SVG_DIR, name + '.svg')
    with open(p, 'w') as f:
        f.write(svg)
    written.append(p)

for p in written:
    print(p)
