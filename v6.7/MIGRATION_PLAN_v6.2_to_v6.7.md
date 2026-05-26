# MIGRATION PLAN: v6.2 to v6.7 (v7.6 Prototype)

> Generated: 2026-05-25
> Baseline: v6.2.1 (React prototype + ESP-IDF firmware full_replica)
> Target: v6.7 / v7.6 bundle (React prototype v7.6 + ESP-IDF 5.5.4 firmware)
> Scope: Comprehensive change analysis covering UI components, firmware source, themes, icons, settings, and interaction patterns

---

## Executive Summary

v7.6 is a **major architectural redesign** of the information architecture, theme system, and developer experience. The key philosophical shift is from "6 terminal-semantic sectors" to "user expression + AI personality": Skills is demoted from a primary Menu sector to a Settings sub-item (3% usage rate), Theme is promoted to a primary Menu sector (1.2x/day usage), and 2 new personality-driven pages (PersonaGrid, MemoryBrowser) are introduced. Two themes (Warm, Dev) are completely replaced with more broadly appealing alternatives (Lavender, Cocoa). The entire Settings UI is localized from English to Chinese, and the Developer console goes from hidden (5-tap) to default-visible with a warning badge.

---

## 1. New Pages/Components (v6.7, Not in v6.2)

### 1.1 New Firmware Pages

| File | Purpose | Notes |
|---|---|---|
| `src/pages/page_model_picker.c` | 6-model selection (Auto, GPT-4o, Claude 3.5, Doubao Pro, Qwen-Max, Phi-3-mini) | Menu 2 o'clock sector; emits `XB_EVT_MODEL_CHANGED` |
| `src/pages/page_theme_picker.c` | 2x2 theme grid (Tech/Lavender/Child/Cocoa) | Menu 4 o'clock sector (replaces Skills); immediate switch, no confirmation |
| `src/pages/page_persona_grid.c` | 3x2 persona selection grid (Lyra/Echo/Nova/Sage/Pico/Doc) | Menu 8 o'clock sector; persists to NVS `xb_persona/active` |
| `src/pages/page_memory_browser.c` | Scrollable memory list + purge modal | Menu 10 o'clock sector; SQLite-backed via `xb_memory_store` |
| `src/pages/page_skills_empty.c` | Placeholder page "即将上线" | Skills demoted from Menu to Settings sub-item; real content planned for v7.7 |
| `src/pages/page_chat.c` | Chat bubble + dot loader with `XB_EVT_CHAT_DELTA/DONE/ERROR` | Menu 12 o'clock sector; new chat page (was placeholder in v6.2) |

### 1.2 New Simulation Subsystem (`src/sim/`)

| File | Purpose | v6.2 Equivalent |
|---|---|---|
| `xb_persona.h/c` | 6-persona registry with NVS persistence | None (personas did not exist) |
| `xb_memory_store.h/c` | SQLite-backed memory store (200-entry LRU) | None (memory existed only as stub in ScenarioOverlay) |
| `xb_imu_sim.h/c` | 6 IMU event triggers (前倾/后仰/左倾/右倾/摇晃/旋转) | None (IMU was simulated inline in ScenarioOverlay) |
| `xb_scenario_sim.h/c` | 6 scenario triggers with face/RGB/bubble mapping | None (scenarios were handled via URL state params) |

### 1.3 New Core Capabilities

| Feature | Description | Files |
|---|---|---|
| `xb_card_panel_opa()` | Card opacity helper: dark themes 30% transparent, light themes opaque | `xb_theme.h`, `theme_tokens.h` |
| `xb_dialog_bubble()` | Top-center bubble widget with text/suggest types | `xb_widgets.h/c` |
| `xb_rgb_strip()` | 1.5px high strip with scene-mapped colors | `xb_widgets.h/c` |
| `xb_toast()` | Transient banner (auto-hides after 2200ms) | `xb_widgets.h/c` |
| `xb_modal()` | Centered modal with title/body/OK/Cancel | `xb_widgets.h/c` |
| 4-mode StatusBar | Added `XB_STATUSBAR_ALWAYS` for all sub-pages | `xb_widgets.h/c` |

---

## 2. Removed Pages/Components

### 2.1 Removed Firmware Pages

| File (v6.2) | Reason for Removal |
|---|---|
| `page_ota.c` | OTA functionality folded into Console and StatusBar critical mode; no standalone page needed |
| `page_wifi_pair.c` | WiFi pairing code variant removed; only AP QR method retained |
| `page_skills.c` | Full skills page with Installed/Store tabs removed; replaced by `page_skills_empty.c` placeholder |
| `page_skill_detail.c` | Skill detail drill-down removed as Skills is demoted to placeholder |

### 2.2 Removed React Components (from prototype)

| Component | Reason | Replacement |
|---|---|---|
| `ScenarioOverlay.tsx` | 3-tab developer console (IMU/Scenarios/Memory) | Split into Console 4-tab system (Chinese) + dedicated MemoryBrowser page |
| `BootAnimation.tsx` (theme branches) | 3 different boot animations (tech/dev flash line, child bounce) | Unified eye-rub animation (no theme branch) in `page_boot.c` |

### 2.3 Removed/Replaced Themes

| Theme (v6.2) | Status | Replacement |
|---|---|---|
| **Warm** | Removed entirely | **Lavender** (柔紫, light/purple, #9333EA accent) |
| **Dev** | Removed entirely | **Cocoa** (草莓可可, dark/strawberry-pink, #FB7185 accent) |
| Orange, Blue, Pink, Green, Purple, Yellow, Mono | Legacy fallback only | All unknown names fall back to TECH silently |

### 2.4 Removed Features/Patterns

| Feature (v6.2) | Reason |
|---|---|
| 5-tap Developer unlock | Replaced by default-visible Developer section with "测试" badge |
| Theme-branch Boot animation | Unified animation strengthens "life-form" persona |
| English Settings labels | Full Chinese localization (5 groups) |
| `theme === 'tech' ? 30% : 100%` card opacity | Replaced by `isLightTheme()` / `xb_card_panel_opa()` helper; correctly handles Cocoa as dark |
| `face color = accent` | Changed to `face color = accent_hi` (brighter highlight) |
| `xb_theme_lookup()` rejecting unknown names | Now silently falls back to TECH (OTA back-compat) |

---

## 3. Modified Pages/Components (Changed Layout/Behavior)

### 3.1 Menu Overlay (`page_menu.c`)

| Aspect | v6.2 | v7.6 |
|---|---|---|
| Sector count | 6 (same) | 6 (same) |
| Sector order (clockwise from 12 o'clock) | Chat / Skills / Scene / Console / Settings / WiFi | **对话 / 模型 / 主题 / 设置 / 人格 / 记忆** |
| 12 o'clock (-90 deg) | Chat (MessageCircle) | 对话 (Bell/LV_SYMBOL_BELL) |
| 2 o'clock (-30 deg) | Skills (Layers) | 模型 (List/LV_SYMBOL_LIST) |
| 4 o'clock (30 deg) | Scene (Sparkles) | **主题 (Edit/LV_SYMBOL_EDIT)** -- was Skills |
| 6 o'clock (90 deg) | Console (Terminal) | 设置 (Settings/LV_SYMBOL_SETTINGS) |
| 8 o'clock (150 deg) | Settings (Settings) | **人格 (User/LV_SYMBOL_USER)** -- new |
| 10 o'clock (210 deg) | WiFi (WiFi) | **记忆 (Save/LV_SYMBOL_SAVE)** -- new |
| Labels | English | Chinese |
| Face shown | FACE_MENU | FACE_MENU |

### 3.2 Settings Page (`page_settings.c`)

| Aspect | v6.2 | v7.6 |
|---|---|---|
| Language | English | Chinese |
| Groups count | 5 | 5 |
| Group 1 | **General**: Theme, Language, Brightness (nav+slider) | **通用**: 主题 (nav to ThemePicker) |
| Group 2 | **Audio**: Volume (slider), Voice (nav) | **网络**: WiFi 配网 (nav to WiFi AP) |
| Group 3 | **Network**: Wi-Fi, Update (both nav) | **模型**: 默认模型 (nav to ModelPicker) |
| Group 4 | **Privacy**: Analytics (toggle), Mic mute (toggle) | **关于**: 技能（即将上线）(nav to Skills Empty placeholder) |
| Group 5 | **System**: About (hidden 5-tap), Factory reset (alert) | **开发者 [测试]**: 控制台 (nav to Console), badge=accent |
| Developer access | Hidden behind 5-tap on About | Default visible with "测试" badge |
| Item widgets | Icon + label + value/slider/toggle | Icon + label + chevron-right |
| StatusBar | Inherited from home | Always visible (XB_STATUSBAR_ALWAYS) |

### 3.3 Console Page (`page_console.c`)

| Aspect | v6.2 | v7.6 |
|---|---|---|
| Tab count | 3 (Sensors, Scripts, Logs) | 4 (系统, AI, 显示, 服务) |
| Tab language | English | Chinese |
| System tab | Sensor readouts: IMU yaw, Temp, Lux, CPU, Heap | **体感测试**: 6 Chinese buttons (前倾/后仰/左倾/右倾/摇晃/旋转) + **情景测试**: 6 scenario buttons |
| AI tab | Did not exist | AI state toggles (空闲/思考/说话) + bubble inject |
| Display tab | Did not exist | Theme cycle, StatusBar mode cycle, Face variant cycle |
| Service tab | Did not exist as separate; Scripts tab had 6 scripts | WiFi state cycle, Battery -20%, MCP toggle, OTA +20% |
| Scripts/Logs | 6 script cards (2-col) + log textarea | Removed; replaced by AI/Display/Service tabs |
| Button events | Single click only | `LV_EVENT_PRESSED/RELEASED/PRESS_LOST` (touch-aware) |

### 3.4 Boot Animation (`page_boot.c`)

| Aspect | v6.2 | v7.6 |
|---|---|---|
| Style | Theme-branch: Tech/Dev = scan-line + blinking blocks, Child = bouncing emoji | **Unified**: eye_rub -> yawn -> fully_open (all themes) |
| Duration | 3500ms | 2400ms (400ms per step, 4 steps + goto HOME) |
| Visual elements | Brand mark image + loading ring + version tag | face_engine geometric animation only |
| Face variants | Sleep/wake | DEEP_SLEEP -> BOOT_MID -> BOOT_OPEN -> IDLE |
| bg behavior | Full-screen cover with own bg | Uses current theme.bg |

### 3.5 Theme System (`theme_tokens.h` + `xb_theme.h`)

| Aspect | v6.2 | v7.6 |
|---|---|---|
| Primary themes | 4 (Tech, Warm, Child, Dev) | **4 (Tech, Lavender, Child, Cocoa)** |
| Extended themes | 6 (Orange, Blue, Pink, Green, Purple, Yellow) + 1 Mono = 10 total | Legacy carry only (fall back to TECH) |
| `theme_t` struct | 9 fields: bg, panel, accent, accent_dim, text, text_dim, border, danger, success | 11 fields: **+accent_hi, +is_light** |
| Face color | `accent` field | **`accent_hi` field** (brighter highlight) |
| Dark/light detection | None (all themes treated as generic) | **`is_light` field**: 0=dark (Tech/Cocoa), 1=light (Lavender/Child) |
| Card opacity | Hardcoded check: `theme == 'tech' ? 30% : 100%` | **`xb_card_panel_opa()`**: `is_light ? 0xFF : 0x4D` |
| Unknown name handling | Error (crash risk) | **Silent fallback to TECH** |
| NVS namespace | Not specified | `xb_theme` (<= 15 chars) |

### 3.6 Event Bus (`xb_event.h`)

| Aspect | v6.2 | v7.6 |
|---|---|---|
| Event count | 11 events (THEME_CHANGED through CHAT_ERROR) | 21 events |
| New events | -- | FACE_REQUEST, SCENE_REQUEST, BUBBLE, RGB_SCENE, STATUSBAR_MODE, STATUSBAR_PEEK, PERSONA_CHANGED, MODEL_CHANGED, MEMORY_CHANGED, IMU, PLUG |
| Unsubscribe | Not supported | `xb_event_unsubscribe()` added |
| Max subscribers | Unlimited | MAX_SUBS=12 |

### 3.7 Widget Toolkit (`xb_widgets.h`)

| Aspect | v6.2 | v7.6 |
|---|---|---|
| Widgets | statusbar, topbar, dot_loading, toast, error_inline, button, card | + dialog_bubble, rgb_strip, **modal**, statusbar with 4 modes |
| StatusBar modes | idle / transient / critical (3) | + **always** (4 total) |
| Card API | `xb_card(parent)` | `xb_card(parent, w, h)` with auto `xb_card_panel_opa()` |
| Topbar | `xb_topbar_create(parent, title, bool back)` | `xb_topbar(parent, title, back_cb)` |

### 3.8 Face Engine (`xb_face.h`)

| Aspect | v6.2 | v7.6 |
|---|---|---|
| Variant count | 26 + FACE_MAX (=27) | 24 + FACE_VARIANT_MAX (=24) |
| New variants | -- | FACE_BOOT_MID, FACE_BOOT_OPEN |
| Removed variants | FACE_HAPPY_BLINK, FACE_SAD, FACE_LOOK_AROUND, FACE_LIGHT_REST | (merged or removed) |
| Face color | accent | **accent_hi** (brighter, with 20px halo @ 0xB3 opacity) |
| Redraw on theme | Not supported | `xb_face_redraw()` added |
| Blink | Not described | 150ms blink, random 3-5s interval |

---

## 4. New Firmware Source Files (v7.6 that Didn't Exist in v6.2)

### Completely New Files

| File | Size (est.) | Description |
|---|---|---|
| `src/sim/xb_persona.h` | ~20 lines | 6-persona type definitions and API (init/set/active/lookup) |
| `src/sim/xb_persona.c` | ~80 lines | 6 persona definitions (Lyra/Echo/Nova/Sage/Pico/Doc), NVS persistence |
| `src/sim/xb_memory_store.h` | ~20 lines | Memory entry struct + API (init/count/list/add/purge) |
| `src/sim/xb_memory_store.c` | ~120 lines | SQLite-backed store with RAM fallback, 200-entry LRU |
| `src/sim/xb_imu_sim.h` | ~12 lines | 6-axis IMU event enum (TILT_FWD/BACK/LEFT/RIGHT/SHAKE/ROTATE) |
| `src/sim/xb_imu_sim.c` | ~30 lines | IMU event fire + Chinese name lookup, posts XB_EVT_IMU |
| `src/sim/xb_scenario_sim.h` | ~20 lines | 6-scenario enum + scene_def_t struct with face/RGB/bubble mapping |
| `src/sim/xb_scenario_sim.c` | ~60 lines | Scenario fire function, drives face + RGB strip + bubble |
| `src/pages/page_model_picker.c` | ~65 lines | 6-row model selection list (Auto/GPT-4o/Claude/Doubao/Qwen/Phi) |
| `src/pages/page_theme_picker.c` | ~55 lines | 2x2 theme picker grid with accent_hi border for active |
| `src/pages/page_persona_grid.c` | ~55 lines | 3x2 persona grid with name/tagline display |
| `src/pages/page_memory_browser.c` | ~75 lines | Scrollable memory list + "清空" purge button + modal |
| `src/pages/page_skills_empty.c` | ~20 lines | Placeholder page: "技能（即将上线）" |
| `firmware/docs/INTEGRATION.md` | -- | Integration guide for bsp+NVS+card opacity |
| `firmware/docs/COVERAGE.md` | -- | React<->LVGL coverage table |

### Significantly Refactored Files

| File | Changes |
|---|---|
| `src/main.c` | New startup sequence: nvs/event/theme/persona/memory/bsp/router init |
| `src/pages/xb_pages.h` | Page enum expanded from 12 to 12 entries (new pages replace ota/wifi_pair/skills/detail) |
| `src/pages/xb_router.c` | 8-layer page stack with LV_LAYOUT switching |

---

## 5. Theme Changes

### 5.1 Theme Set Comparison

| # | v6.2 (3 primary + 7 extended = 10) | v7.6 (4 active + legacy fallback) |
|---|---|---|
| 1 | **Tech** (dark, cyan #33C5FF) | **Tech** (dark, cyan #22D3EE accent / #33C5FF accent_hi) |
| 2 | ~~Warm~~ (light, warm) | **Lavender** (light, purple #9333EA / #A855F7) |
| 3 | **Child** (light, coral #FF7F50) | **Child** (light, coral #FF7F50 / #FFAA78) |
| 4 | ~~Dev~~ (dark, green #22C55E) | **Cocoa** (dark, strawberry #FB7185 / #FDA4AF) |
| 5-10 | Orange, Blue, Pink, Green, Purple, Yellow, Mono | Legacy only; `xb_theme_lookup()` maps all unknown to TECH |

### 5.2 Token Table Comparison

**v6.2 Tokens (Tech):**
```
bg=#000000, panel=#0A1E28, accent=#33C5FF, text=#B4EBFF, border=#14506E, danger=#F43F5E, success=#22C55E
```

**v7.6 Tokens (Tech):**
```
bg=#000000, panel=#0A1E28, accent=#22D3EE, accent_hi=#33C5FF, text=#B4EBFF, border=#14506E, danger=#F43F5E
```

Key differences in v7.6 tokens:
- **accent** changed from `#33C5FF` to `#22D3EE` (slightly dimmer for non-face elements)
- **accent_hi** = old accent `#33C5FF` (now exclusively for face/RGB strip/glow)
- **is_light** field added to theme_t struct
- Lavender/Cocoa accent colors are entirely new (no v6.2 equivalent)
- Danger colors now differ per theme (Tech/Child use #F43F5E, Lavender/Cocoa use #EF4444)

### 5.3 Design Rationale (from PRD v3.0)

- Warm replaced by Lavender: 28% user preference vs 61% for Lavender (internal 80-person test)
- Dev replaced by Cocoa: 12% vs 47% for Cocoa
- Cocoa fills dark-theme need alongside Tech with more sophisticated premium brown
- Both new themes target female user segment (primary growth vector)

---

## 6. SVG Icons

### 6.1 Count Comparison

| Category | v6.2 | v7.6 |
|---|---|---|
| Functional icons (assets/icons/) | 81 | 81 (identical set) |
| Design mockups (svg/) | 0 | 20 SVGs (+ 20 PNGs) |

### 6.2 Mockup Files (New in v7.6)

| # | File | Description |
|---|---|---|
| M01 | p01_boot.svg | Boot eye-rub animation freeze |
| M02 | p02_home_idle_clean.svg | Full-screen idle face, no statusbar |
| M03 | p02_home_peek.svg | Idle face + transient statusbar |
| M04 | p02_home_critical_lowbat.svg | Lost face + 12% red battery statusbar |
| M05 | p03_menu.svg | 6-sector radial menu, 12 o'clock hover |
| M06 | p06_model_picker.svg | 6 model rows |
| M07 | p07_persona_grid.svg | 3x2 persona cards, Lyra selected |
| M08 | p08_memory_browser.svg | 6 memory rows |
| M09 | p08_memory_purge_modal.svg | Memory browser + purge modal |
| M10 | p09_settings.svg | 5 Chinese groups, Developer with test badge |
| M11 | p10_theme_picker_4themes.svg | 2x2 theme comparison |
| M12 | p11_console_ai.svg | AI engine 2x2 cards |
| M13 | p11_console_system.svg | 6 IMU test buttons |
| M14 | p11_console_service.svg | 6 scenario buttons |
| Aux | p11_console_display.svg | Display debug 2x2 cards |
| Aux | p_wifi_ap.svg | WiFi AP QR config |
| Aux | p_theme_preview_{tech,lavender,child,cocoa}.svg | 4 standalone theme thumbnails |

### 6.3 New Lucide Icon References (used in new pages)

New lucide icons referenced in v7.6 but not present in v6.2 component code:
- `Sparkles` (模型/model selector)
- `Brain` (记忆/memory)
- `UserSquare` (人格/persona)
- `Briefcase` (技能/skills)
- `Activity` (使用统计/usage)
- `Terminal` (运行日志/run logs)
- `BatteryWarning` (低电/low battery)

---

## 7. Settings Page Changes (Detailed)

### 7.1 Structure Comparison

**v6.2 Structure (English):**
```
1. General
   - Theme (nav)        → palette icon, value "Tech"
   - Language (nav)     → globe icon, value "English"
   - Brightness (slider) → sun icon, 70%
2. Audio
   - Volume (slider)    → volume icon, 60%
   - Voice (nav)        → mic icon, value "Lyra"
3. Network
   - Wi-Fi (nav)        → wifi icon, value "home-5G"
   - Update (nav)       → download icon, value "v6.2.0"
4. Privacy
   - Analytics (toggle) → eye_off icon, Off
   - Mic mute (toggle)  → mic_off icon, Off
5. System
   - About (nav)        → info icon, 5-tap to unlock Developer
   - Factory reset (alert)→ alert_triangle icon
```

**v7.6 Structure (Chinese):**
```
1. 通用
   - 主题 (nav)         → PAGE_THEME_PICKER
2. 网络
   - WiFi 配网 (nav)    → PAGE_WIFI_AP
3. 模型
   - 默认模型 (nav)     → PAGE_MODEL_PICKER
4. 关于
   - 技能（即将上线）   → PAGE_SKILLS_EMPTY
5. 开发者 [测试]        ← badge=accent, always visible
   - 控制台 (nav)       → PAGE_CONSOLE
```

### 7.2 Items Added
- 主题 (ThemePicker) -- was in General group, now its own sub-page
- WiFi 配网 -- was under Network group
- 默认模型 (ModelPicker) -- entirely new
- 技能（即将上线） -- placeholder for future Skills store
- 控制台 -- was hidden behind 5-tap, now default visible

### 7.3 Items Removed
- Language selector (removed)
- Brightness slider (removed from Settings, now in Console Display tab)
- Volume slider (removed from Settings, now in Console Display tab)
- Voice/Lyra selector (removed; replaced by PersonaGrid in Menu)
- Analytics toggle (removed)
- Mic mute toggle (removed)
- Factory reset (removed from Settings)
- 5-tap About unlock mechanism (completely removed)

---

## 8. New Interaction Patterns

### 8.1 Boot Animation (Unified)
- **v6.2**: Theme-branch: Tech/Dev show scan-line + "SYSTEM BOOTING..." + blinking blocks; Child shows bouncing emoji + "HELLO!"
- **v7.6**: All themes share the same eye-rub animation:
  - t=0ms: DEEP_SLEEP (eyes fully closed)
  - t=400ms: BOOT_MID (eyes half-open height=16, mouth yawn 28x36)
  - t=1400ms: BOOT_OPEN (eyes fully open height=64)
  - t=2400ms: IDLE -> goto HOME
  - bg = current theme.bg throughout

### 8.2 Radial Menu (Reorganized)
- Sector 4 o'clock was **Skills (Package icon)** in v6.2; now **Theme (Palette icon)**
- Sector 6 o'clock was **Console** in v6.2; now **Settings**
- Sector 8 o'clock was **Settings** in v6.2; now **Persona** (new)
- Sector 10 o'clock was **WiFi** in v6.2; now **Memory** (new)
- Console and WiFi removed from Menu (accessible via Settings now)

### 8.3 StatusBar Always Mode
- **v6.2**: Sub-pages inherit home's statusbar mode (idle=hidden, critical=shown)
- **v7.6**: All non-home sub-pages have statusbar permanently visible (XB_STATUSBAR_ALWAYS)
- Ensures battery/WiFi awareness on Settings, Console, Model, Persona, Memory, Theme, Skills pages

### 8.4 DialogBubble
- **v6.2**: Basic bubble in RobotUI.tsx with text/suggest types mapped via URL state
- **v7.6**: Dedicated `xb_dialog_bubble()` widget with:
  - `XB_BUBBLE_TEXT` -- plain text bubble at 85% width, top-aligned
  - `XB_BUBBLE_SUGGEST` -- text + check/cross action buttons
  - Glow: `box-shadow ${accent}33 0 0 12`
  - Backdrop: `bg=${panel}CC + border ${border}80`

### 8.5 RGB Strip Scene Mapping
- **v6.2**: Single color per theme (tech=cyan, dev=green, child=coral), same opacity curve
- **v7.6**: Scene-driven color/period mapping:
  - idle/breath: `accent_hi`, 4s pulse
  - lonely_3: `#FACC15` (yellow), 4s slow
  - reward: `accent_hi`, 0.5s fast
  - angry: `danger`, 1.2s medium
  - ota: `#A855F7` (purple), 1.5s
  - error: `danger`, 0.5s urgent

### 8.6 Card Opacity Rule (Dark/Light)
- **v6.2**: `theme === 'tech' ? panel@30% : panel@100%` (hardcoded to Tech only)
- **v7.6**: `is_light ? 0xFF : 0x4D` (correctly handles both Tech AND Cocoa as dark)
- This was a critical round-3 bug fix (B1 in DESIGNER_AI_PROMPT_round3.md)

### 8.7 Console Touch Events
- **v6.2**: Buttons use `LV_EVENT_CLICKED` only
- **v7.6**: Buttons use `LV_EVENT_PRESSED` + `LV_EVENT_RELEASED` + `LV_EVENT_PRESS_LOST`
- Covers both mouse (desktop preview) and touch (real CoreS3 device)

### 8.8 Developer Experience
- **v6.2**: 5-tap on About page to unlock Developer options (hidden "easter egg")
- **v7.6**: Developer section always visible with `[测试]` badge (`bg=accent text=bg`)
- Console tabbar fully Chinese (系统/AI/显示/服务)
- Console.System: 6 Chinese IMU buttons instead of 3 English buttons
- Console.Service: 6 Chinese emoji scenario buttons instead of 3 big cards + 6 English buttons

### 8.9 Theme/Picker Switching
- **v6.2**: Theme cycling in Menu via icon click -> next in rotation
- **v7.6**: Dedicated 2x2 ThemePicker page with:
  - Name + desc for each theme
  - Colored dot indicators (#22D3EE/#9333EA/#FF7F50/#FB7185)
  - Active theme = accent_hi border
  - Immediate switch (no confirmation modal)

---

## 9. Firmware File Inventory Comparison

### 9.1 v6.2 Files (17 files)
```
src/main.c
src/core/xb_event.h, xb_event.c
src/core/xb_theme.h, xb_theme.c
src/core/xb_face.h, xb_face.c
src/widgets/xb_widgets.h, xb_widgets.c
src/pages/xb_pages.h, xb_router.c
src/pages/page_boot.c, page_home.c, page_menu.c, page_chat.c
src/pages/page_wifi_ap.c, page_wifi_pair.c, page_ota.c
src/pages/page_skills.c, page_skill_detail.c
src/pages/page_settings.c, page_console.c
src/assets/xb_icons_stub.c
assets/themes/theme_tokens.h
```

### 9.2 v7.6 Files (24 files)
```
src/main.c
src/core/xb_event.h, xb_event.c          (+unsubscribe, +10 events)
src/core/xb_theme.h, xb_theme.c          (+is_light, +acc_hi, +card_opa, +silent fallback)
src/core/xb_face.h, xb_face.c            (+24 variants, +accent_hi, +BOOT_MID/OPEN, +redraw)
src/widgets/xb_widgets.h, xb_widgets.c   (+dialog_bubble, +rgb_strip, +modal, +4-mode statusbar)
src/sim/xb_persona.h, xb_persona.c       [NEW]
src/sim/xb_memory_store.h, xb_memory_store.c  [NEW]
src/sim/xb_imu_sim.h, xb_imu_sim.c       [NEW]
src/sim/xb_scenario_sim.h, xb_scenario_sim.c  [NEW]
src/pages/xb_pages.h, xb_router.c        (+new page enums, +8-layer stack)
src/pages/page_boot.c                    [REFACTORED: unified animation]
src/pages/page_home.c
src/pages/page_menu.c                    [REFACTORED: Chinese sectors, new targets]
src/pages/page_chat.c                    [REFACTORED: bubble + delta/done/error events]
src/pages/page_model_picker.c            [NEW]
src/pages/page_theme_picker.c            [NEW]
src/pages/page_persona_grid.c            [NEW]
src/pages/page_memory_browser.c          [NEW]
src/pages/page_settings.c                [REFACTORED: Chinese, dev visible]
src/pages/page_wifi_ap.c
src/pages/page_skills_empty.c            [NEW -- placeholder]
src/pages/page_console.c                 [REFACTORED: 4 Chinese tabs]
src/assets/xb_icons_stub.c
assets/themes/theme_tokens.h             [REFACTORED: new 4-token system]
```

---

## 10. Migration Impact Summary

### High-Impact Breaking Changes
1. **Theme system**: Warm/Dev NVS values must be migrated to Lavender/Cocoa or fall back to Tech
2. **Settings structure**: All navigation paths to old settings items must be updated
3. **Menu routing**: 4 of 6 sectors target different pages
4. **Event bus**: New events required; unsubscribe support needed for page teardown
5. **NVS namespaces**: Must use `xb_theme`, `xb_persona` (both <= 15 chars)

### Medium-Impact Changes
6. **Face engine**: Variant count changed (27 -> 24); BOOT_MID/BOOT_OPEN added
7. **Card opacity**: Universal helper replaces hardcoded Tech check
8. **StatusBar**: Always mode required for all sub-pages
9. **Widget API**: New signatures for xb_card(), xb_topbar()

### Low-Impact / Additive
10. **New pages**: ModelPicker, ThemePicker, PersonaGrid, MemoryBrowser -- additive, no migration needed
11. **Firmware**: 4 new sim modules, all additive
12. **Build config**: ESP-IDF 5.5.4 + LVGL 9.5.0 + esp-claw claw_core@v1.0 (unchanged from v6.3)

### Migration Hardening Checklist (from MANIFEST.md section 7)
- [ ] 4 themes toggle without restart; card opacity correct (Cocoa transparent, Lavender opaque)
- [ ] Settings "开发者选项" default-visible with "测试" badge
- [ ] Console.System 6 Chinese buttons trigger IMU -> face expression change
- [ ] Console.Service 6 scenarios complete within 30s auto-restore to idle
- [ ] OTA statusbar critical + RGB purple 1.5s pulse
- [ ] Old theme NVS values ("warm", "dev") boot without crash, auto-fallback to Tech
- [ ] `xb_theme` / `xb_persona` NVS namespace length <= 15
- [ ] Face color = accent_hi (not accent)
- [ ] Zero 5-tap unlock code remaining in entire codebase
- [ ] Zero Warm/Dev theme references remaining
