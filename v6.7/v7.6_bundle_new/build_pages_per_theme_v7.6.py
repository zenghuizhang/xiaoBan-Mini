#!/usr/bin/env python3
# build_pages_per_theme_v7.6.py
# Generate every key page in all 4 themes (Tech/Lavender/Child/Cocoa) so other
# agents can replicate non-Tech themes accurately. Total: 12 pages * 4 themes
# = 48 SVGs in svg_per_theme/.
#
# Canvas: 320x240 (M5Stack CoreS3 native).
# All geometry mirrors the React prototype + theme_tokens.h.
import os, math, pathlib

OUT = pathlib.Path(__file__).resolve().parent / "svg_per_theme"
OUT.mkdir(exist_ok=True)

# ---- token table (must match assets/themes/theme_tokens.h) -----------------
T = {
    "tech": dict(
        bg="#000000", panel="#0A1E28", accent="#22D3EE", accent_hi="#33C5FF",
        accent_dim="#0F5A78", text="#B4EBFF", text_dim="#6EAAC8",
        border="#14506E", danger="#F43F5E", success="#22C55E",
        is_light=False, card_opa=0x4D),
    "lavender": dict(
        bg="#FAF5FF", panel="#FFFFFF", accent="#9333EA", accent_hi="#A855F7",
        accent_dim="#A855F7", text="#4C1D95", text_dim="#7C3AED",
        border="#E9D5FF", danger="#EF4444", success="#22C55E",
        is_light=True, card_opa=0xFF),
    "child": dict(
        bg="#FFF9E6", panel="#FFFFFF", accent="#FF7F50", accent_hi="#FFAA78",
        accent_dim="#FFAA78", text="#C85A28", text_dim="#D28C5A",
        border="#FFC8A0", danger="#F43F5E", success="#22C55E",
        is_light=True, card_opa=0xFF),
    "cocoa": dict(
        bg="#2D1B0E", panel="#3F2B20", accent="#FB7185", accent_hi="#FDA4AF",
        accent_dim="#FDA4AF", text="#FEF3C7", text_dim="#D9C1A0",
        border="#573D2C", danger="#EF4444", success="#22C55E",
        is_light=False, card_opa=0x4D),
}

# rgba helper
def rgba(hexc, opa_byte):
    h = hexc.lstrip("#")
    r, g, b = int(h[0:2], 16), int(h[2:4], 16), int(h[4:6], 16)
    return f"rgba({r},{g},{b},{opa_byte/255:.3f})"

# panel fill given theme = panel @ card_opa
def panel_fill(t):
    return rgba(t["panel"], t["card_opa"])

W, H = 320, 240
FONT = ('font-family="PingFang SC, Noto Sans CJK SC, Inter, sans-serif"')

# ---- common header/svg wrapper ----
def header(name, t):
    return f'''<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {W} {H}"
  width="{W*2}" height="{H*2}" style="background:{t['bg']}">
  <defs>
    <filter id="glow"><feGaussianBlur stdDeviation="3"/></filter>
  </defs>
  <rect width="{W}" height="{H}" fill="{t['bg']}"/>
  <!-- RGB strip -->
  <rect x="0" y="0" width="{W}" height="2" fill="{t['accent_hi']}" opacity="0.85"/>
'''

def footer(): return "</svg>\n"

# ---- face primitive ----
def face(t, variant="idle", cx=160, cy=130):
    """Draw 2 eyes + mouth using accent_hi."""
    c = t["accent_hi"]
    GEOM = {
        "idle":    (32,32,48, 0, 24, 4,28,2),
        "happy":   (32,20,48,-4, 40,12,28,6),
        "talking": (32,32,48, 0, 28,16,28,8),
        "alert":   (36,36,52, 0, 28, 4,28,2),
        "thinking":(28,32,48,-2, 20, 4,32,2),
        "menu":    (28,28,52, 0, 20, 4,28,2),
        "wink":    (32,32,48, 0, 28, 8,28,4),
        "celebrate":(28,28,48,-4, 40,16,28,8),
        "boot_open":(32,64,48,0, 24, 4,28,2),
        "yawn":    (32,16,48, 0, 28,36,28,8),
        "deep_sleep":(32,2,48,0, 24, 2,28,1),
        "lost":    (28,16,48, 8, 28, 8,32,6),
        "crying":  (28,20,48, 8, 20, 8,32,6),
    }
    ew, eh, gap, edy, mw, mh, mdy, mr = GEOM.get(variant, GEOM["idle"])
    glow = f'filter="url(#glow)" opacity="0.7"'
    parts = []
    # eyes
    for sgn in (-1, 1):
        x = cx + (gap/2)*sgn - (ew if sgn < 0 else 0)
        y = cy + edy
        parts.append(f'<rect x="{x:.0f}" y="{y:.0f}" width="{ew}" height="{eh}" '
                     f'rx="{eh/2:.0f}" fill="{c}"/>')
        parts.append(f'<rect x="{x-4:.0f}" y="{y-4:.0f}" width="{ew+8}" height="{eh+8}" '
                     f'rx="{(eh+8)/2:.0f}" fill="{c}" {glow}/>')
    # mouth
    mx = cx - mw/2
    my = cy + mdy
    parts.append(f'<rect x="{mx:.0f}" y="{my:.0f}" width="{mw}" height="{mh}" '
                 f'rx="{mr}" fill="{c}"/>')
    parts.append(f'<rect x="{mx-3:.0f}" y="{my-3:.0f}" width="{mw+6}" height="{mh+6}" '
                 f'rx="{mr+3}" fill="{c}" {glow}/>')
    return "\n".join(parts)

# ---- topbar ----
def topbar(t, title):
    return f'''
  <text x="160" y="14" text-anchor="middle" {FONT} font-size="11"
        fill="{t['text']}">{title}</text>
  <text x="10" y="14" {FONT} font-size="11" fill="{t['accent']}">&#x2039;</text>
'''

# ---- statusbar (idle/full) ----
def statusbar(t, mode="full", danger=False):
    if mode == "idle":
        return f'<circle cx="6" cy="8" r="2.5" fill="{t["accent_hi"]}"/>'
    col = t["danger"] if danger else t["text_dim"]
    return f'''
  <text x="6" y="11" {FONT} font-size="9" fill="{col}">📶</text>
  <text x="300" y="11" {FONT} font-size="9" text-anchor="end" fill="{col}">🔋 87%</text>
  <text x="155" y="11" {FONT} font-size="9" text-anchor="middle" fill="{t['accent_hi']}">●</text>
'''

# ---- card helper ----
def card(t, x, y, w, h):
    return (f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="14"'
            f' fill="{panel_fill(t)}" stroke="{t["border"]}" stroke-width="1"/>')

# ============================================================================
# Page builders
# ============================================================================

def p_boot(t):
    s  = header("boot", t)
    s += face(t, "boot_open")
    s += f'<text x="160" y="220" text-anchor="middle" {FONT} font-size="9" fill="{t["text_dim"]}">v7.6 · 揉眼启动…</text>'
    return s + footer()

def p_home_idle(t):
    s  = header("home_idle", t)
    s += statusbar(t, "idle")
    s += face(t, "idle")
    return s + footer()

def p_home_peek(t):
    s  = header("home_peek", t)
    s += statusbar(t, "full")
    s += face(t, "idle")
    return s + footer()

def p_home_critical(t):
    s  = header("home_critical", t)
    s += statusbar(t, "full", danger=True)
    s += face(t, "crying")
    s += f'<text x="160" y="218" text-anchor="middle" {FONT} font-size="10" fill="{t["danger"]}">⚠ 电量不足 7%</text>'
    return s + footer()

def p_menu(t):
    s  = header("menu", t)
    s += topbar(t, "菜单")
    s += face(t, "menu")
    # 6 sectors at -90,-30,30,90,150,210, r=78, btn 48x48
    LABELS = ["对话","模型","主题","设置","人格","记忆"]
    ICONS  = ["💬","🧠","🎨","⚙","👤","📖"]
    for i, deg in enumerate([-90,-30,30,90,150,210]):
        rad = math.radians(deg)
        cx = 160 + math.cos(rad)*78
        cy = 130 + math.sin(rad)*78
        s += f'<circle cx="{cx:.0f}" cy="{cy:.0f}" r="22" fill="{panel_fill(t)}" stroke="{t["accent"]}" stroke-width="1"/>'
        s += f'<text x="{cx:.0f}" y="{cy-1:.0f}" text-anchor="middle" {FONT} font-size="12" fill="{t["accent_hi"]}">{ICONS[i]}</text>'
        s += f'<text x="{cx:.0f}" y="{cy+12:.0f}" text-anchor="middle" {FONT} font-size="8" fill="{t["text"]}">{LABELS[i]}</text>'
    return s + footer()

def p_chat(t):
    s  = header("chat", t)
    s += topbar(t, "对话")
    s += statusbar(t, "full")
    s += card(t, 12, 32, 296, 140)
    s += f'<text x="20" y="56" {FONT} font-size="11" fill="{t["text"]}">张赠辉你好,今天想聊点什么?</text>'
    s += f'<text x="20" y="76" {FONT} font-size="11" fill="{t["text_dim"]}">我可以陪你写代码 / 闲聊 / 总结…</text>'
    # dot loader
    for i in range(3):
        s += f'<circle cx="{20+i*14}" cy="160" r="3.5" fill="{t["accent_hi"]}" opacity="{0.4+i*0.2}"/>'
    # send button
    s += f'<rect x="100" y="200" width="120" height="30" rx="15" fill="{panel_fill(t)}" stroke="{t["accent"]}"/>'
    s += f'<text x="160" y="220" text-anchor="middle" {FONT} font-size="11" fill="{t["text"]}">说点什么</text>'
    return s + footer()

def p_model_picker(t):
    s  = header("model", t)
    s += topbar(t, "选择模型")
    MODELS = [("Auto","智能路由 · 推荐"),("GPT-4o","多模态"),
              ("Claude 3.5","长上下文"),("Doubao","中文优"),
              ("Qwen-Max","通义"),("Phi-3-mini","本地离线")]
    for i,(n,h) in enumerate(MODELS):
        y = 32 + i*32
        border = t["accent"] if i==0 else t["border"]
        s += f'<rect x="12" y="{y}" width="296" height="26" rx="13" fill="{panel_fill(t)}" stroke="{border}"/>'
        s += f'<text x="22" y="{y+17}" {FONT} font-size="11" fill="{t["text"]}">{n}</text>'
        s += f'<text x="298" y="{y+17}" text-anchor="end" {FONT} font-size="9" fill="{t["text_dim"]}">{h}</text>'
    return s + footer()

def p_theme_picker(t, active_id):
    s  = header("theme", t)
    s += topbar(t, "选择主题")
    NAMES = [("tech","极客 · 暗"),("lavender","薰衣草 · 亮"),
             ("child","童趣 · 亮"),("cocoa","可可 · 暗")]
    for i,(tid, nm) in enumerate(NAMES):
        col = i%2; row = i//2
        x = 20 + col*150; y = 38 + row*90
        sw_bg = T[tid]["bg"]
        sw_accent = T[tid]["accent_hi"]
        border = t["accent_hi"] if tid == active_id else t["border"]
        s += f'<rect x="{x}" y="{y}" width="130" height="78" rx="14" fill="{sw_bg}" stroke="{border}" stroke-width="2"/>'
        # mini face preview
        for sgn in (-1, 1):
            s += f'<rect x="{x+50+sgn*16-(8 if sgn<0 else 0)}" y="{y+30}" width="8" height="8" rx="4" fill="{sw_accent}"/>'
        s += f'<rect x="{x+58}" y="{y+44}" width="14" height="3" rx="1.5" fill="{sw_accent}"/>'
        text_col = t["text"] if t["is_light"] else t["text"]
        s += f'<text x="{x+65}" y="{y+72}" text-anchor="middle" {FONT} font-size="10" fill="{text_col}">{nm}</text>'
    return s + footer()

def p_persona_grid(t, active="lyra"):
    s  = header("persona", t)
    s += topbar(t, "人格")
    P = [("lyra","Lyra","温柔陪伴"),("echo","Echo","回声助手"),("nova","Nova","活泼能量"),
         ("sage","Sage","知识导师"),("pico","Pico","童趣小友"),("doc","Doc","工程伙伴")]
    for i,(pid,nm,tg) in enumerate(P):
        col=i%3; row=i//3
        x=18+col*98; y=36+row*92
        border = t["accent_hi"] if pid==active else t["border"]
        s += f'<rect x="{x}" y="{y}" width="92" height="84" rx="14" fill="{panel_fill(t)}" stroke="{border}" stroke-width="2"/>'
        s += f'<text x="{x+46}" y="{y+22}" text-anchor="middle" {FONT} font-size="12" fill="{t["accent_hi"]}">{nm}</text>'
        s += f'<text x="{x+46}" y="{y+50}" text-anchor="middle" {FONT} font-size="9" fill="{t["text_dim"]}">{tg}</text>'
    return s + footer()

def p_memory(t):
    s  = header("memory", t)
    s += topbar(t, "记忆")
    SAMPLES = [("周三 · 项目进度","claw_xb v7.6 已发布给设计 AI"),
               ("周二 · 个人偏好","用户喜欢 Cocoa 主题"),
               ("周一 · 待办","回复王桐的 PRD 评审"),
               ("上周 · 学习","ESP-IDF 5.5.4 LTS 已升级"),
               ("更早 · 闲聊","推荐过《人类简史》")]
    for i,(ti,sn) in enumerate(SAMPLES):
        y = 36 + i*36
        s += card(t, 12, y, 296, 32)
        s += f'<text x="20" y="{y+14}" {FONT} font-size="10" fill="{t["text"]}">{ti}</text>'
        s += f'<text x="20" y="{y+28}" {FONT} font-size="9" fill="{t["text_dim"]}">{sn}</text>'
    s += f'<rect x="240" y="216" width="64" height="20" rx="10" fill="{panel_fill(t)}" stroke="{t["danger"]}"/>'
    s += f'<text x="272" y="229" text-anchor="middle" {FONT} font-size="9" fill="{t["danger"]}">清空</text>'
    return s + footer()

def p_settings(t):
    s  = header("settings", t)
    s += topbar(t, "设置")
    s += statusbar(t, "full")
    GROUPS = [("通用","主题"),("网络","WiFi 配网"),("模型","默认模型"),
              ("关于","版本 v7.6 · 设备 #4F2A"),("开发者 [测试]","控制台")]
    for i,(g,r) in enumerate(GROUPS):
        y = 32 + i*36
        s += f'<text x="12" y="{y+10}" {FONT} font-size="10" fill="{t["accent_hi"]}">{g}</text>'
        s += f'<rect x="12" y="{y+14}" width="296" height="20" rx="10" fill="{panel_fill(t)}" stroke="{t["border"]}"/>'
        s += f'<text x="22" y="{y+28}" {FONT} font-size="10" fill="{t["text"]}">{r}</text>'
        s += f'<text x="298" y="{y+28}" text-anchor="end" {FONT} font-size="10" fill="{t["text_dim"]}">&#x203A;</text>'
    return s + footer()

def p_wifi_ap(t):
    s  = header("wifi_ap", t)
    s += topbar(t, "WiFi 配网")
    s += statusbar(t, "full")
    # QR placeholder
    s += f'<rect x="20" y="40" width="110" height="110" rx="8" fill="#FFFFFF" stroke="{t["accent"]}" stroke-width="2"/>'
    for r in range(7):
        for c in range(7):
            if (r+c)%2==0:
                s += f'<rect x="{26+c*14}" y="{46+r*14}" width="12" height="12" fill="#000"/>'
    s += f'<text x="150" y="60" {FONT} font-size="10" fill="{t["text"]}">1. 手机连接热点</text>'
    s += f'<text x="160" y="74" {FONT} font-size="10" fill="{t["accent_hi"]}">claw_xb_xxxx</text>'
    s += f'<text x="150" y="92" {FONT} font-size="10" fill="{t["text"]}">2. 浏览器自动弹出</text>'
    s += f'<text x="150" y="106" {FONT} font-size="10" fill="{t["text_dim"]}">   配网页面</text>'
    s += f'<text x="150" y="124" {FONT} font-size="10" fill="{t["text"]}">3. 输入家中 WiFi 密码</text>'
    s += f'<text x="160" y="200" text-anchor="middle" {FONT} font-size="11" fill="{t["accent_hi"]}">等待手机扫码…</text>'
    return s + footer()

def p_console_system(t):
    s  = header("console_sys", t)
    s += topbar(t, "控制台 [测试]")
    # tabs
    TABS = ["系统","AI","显示","服务"]
    for i,tn in enumerate(TABS):
        active = (i==0)
        x = 8 + i*78
        col = t["accent_hi"] if active else t["text_dim"]
        s += f'<rect x="{x}" y="30" width="74" height="20" rx="10" fill="{panel_fill(t) if active else "transparent"}" stroke="{col}"/>'
        s += f'<text x="{x+37}" y="44" text-anchor="middle" {FONT} font-size="10" fill="{col}">{tn}</text>'
    s += f'<text x="12" y="68" {FONT} font-size="10" fill="{t["accent_hi"]}">体感测试</text>'
    IMU = ["前倾","后仰","左倾","右倾","摇晃","旋转"]
    for i,nm in enumerate(IMU):
        x = 12+(i%3)*100; y = 76+(i//3)*30
        s += f'<rect x="{x}" y="{y}" width="92" height="24" rx="12" fill="{panel_fill(t)}" stroke="{t["accent"]}"/>'
        s += f'<text x="{x+46}" y="{y+16}" text-anchor="middle" {FONT} font-size="10" fill="{t["text"]}">{nm}</text>'
    s += f'<text x="12" y="146" {FONT} font-size="10" fill="{t["accent_hi"]}">情景测试</text>'
    SCN = ["语音唤醒","OTA 升级","系统报错","来电提醒","低电预警","高温保护"]
    for i,nm in enumerate(SCN):
        x = 12+(i%3)*100; y = 154+(i//3)*30
        s += f'<rect x="{x}" y="{y}" width="92" height="24" rx="12" fill="{panel_fill(t)}" stroke="{t["accent"]}"/>'
        s += f'<text x="{x+46}" y="{y+16}" text-anchor="middle" {FONT} font-size="9" fill="{t["text"]}">{nm}</text>'
    return s + footer()

PAGES = {
    "p01_boot":               p_boot,
    "p02_home_idle":          p_home_idle,
    "p02_home_peek":          p_home_peek,
    "p02_home_critical":      p_home_critical,
    "p03_menu":               p_menu,
    "p04_chat":               p_chat,
    "p05_model_picker":       p_model_picker,
    "p06_theme_picker":       lambda t: p_theme_picker(t, active_id="tech"),
    "p07_persona_grid":       p_persona_grid,
    "p08_memory":             p_memory,
    "p09_settings":           p_settings,
    "p10_wifi_ap":            p_wifi_ap,
    "p11_console_system":     p_console_system,
}

count = 0
for tid, t in T.items():
    tdir = OUT / tid
    tdir.mkdir(exist_ok=True)
    for pname, builder in PAGES.items():
        (tdir / f"{pname}_{tid}.svg").write_text(builder(t))
        count += 1
print(f"Generated {count} per-theme page SVGs into {OUT}/")
print("Themes:", list(T.keys()))
print("Pages :", list(PAGES.keys()))
