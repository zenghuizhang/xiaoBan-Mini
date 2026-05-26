# claw_xb_ui v7.6 — Coverage vs React Prototype

Tracks parity between the React reference (`陪伴机器人 UI 原型 v7.6`) and this
LVGL component. **Goal: 100 % surface parity, ≥ 95 % interaction parity.**

## Pages

| React page              | LVGL page                  | Status | Notes |
|-------------------------|----------------------------|--------|-------|
| `BootScreen`            | `page_boot`                | ✅     | Unified eye-rub → yawn animation (no theme branching) |
| `HomeScreen`            | `page_home`                | ✅     | Idle face + IDLE statusbar + RGB strip |
| `MenuScreen`            | `page_menu`                | ✅     | 6 sectors, Theme replaces Skills at 4-o'clock |
| `ChatScreen`            | `page_chat`                | ✅     | Bubble + dot loader + send; CHAT_* events wired |
| `ModelPickerScreen`     | `page_model_picker`        | ✅     | 6 rows, persists via `XB_EVT_MODEL_CHANGED` |
| `ThemePickerScreen`     | `page_theme_picker`        | ✅     | 2×2 grid; active tile uses `accent_hi` border |
| `PersonaGridScreen`     | `page_persona_grid`        | ✅     | 3×2 grid, 6 personas, NVS-backed |
| `MemoryBrowserScreen`   | `page_memory_browser`      | ✅     | sqlite list + purge modal |
| `SettingsScreen`        | `page_settings`            | ✅     | 5 Chinese groups; Developer default visible w/ 测试 badge |
| `WifiApScreen`          | `page_wifi_ap`             | ✅     | QR placeholder + 3-state status label |
| `SkillsScreen`          | `page_skills_empty`        | ✅     | Downgraded to "coming soon" preview |
| `ConsoleScreen`         | `page_console`             | ✅     | 4 tabs: 系统 / AI / 显示 / 服务 |

## Widgets

| React component         | LVGL widget                 | Status |
|-------------------------|------------------------------|--------|
| `<Card>`                | `xb_card`                    | ✅     |
| `<Button>`              | `xb_button`                  | ✅     |
| `<TopBar>`              | `xb_topbar`                  | ✅     |
| `<StatusBar>` (4 modes) | `xb_statusbar` + `set_mode`  | ✅     |
| `<DialogBubble>`        | `xb_dialog_bubble`           | ✅     |
| `<RgbStrip>`            | `xb_rgb_strip`               | ✅     |
| `<DotLoading>`          | `xb_dot_loading`             | ✅     |
| `<Toast>`               | `xb_toast`                   | ✅     |
| `<ErrorInline>`         | `xb_error_inline`            | ✅     |
| `<Modal>`               | `xb_modal` + `_close`        | ✅     |
| `<Face>` (24 variants)  | `xb_face` + `face_geom_t G[]`| ✅     |

## Themes

| React theme    | LVGL `theme_t`     | is_light | accent / accent_hi    |
|----------------|--------------------|----------|------------------------|
| `tech`         | `THEME_TECH`       | 0        | #22D3EE / #33C5FF      |
| `lavender`     | `THEME_LAVENDER`   | 1        | #9333EA / #A855F7      |
| `child`        | `THEME_CHILD`      | 1        | #FF7F50 / #FFAA78      |
| `cocoa`        | `THEME_COCOA`      | 0        | #FB7185 / #FDA4AF      |
| _legacy_       | → `THEME_TECH`     | 0        | (OTA back-compat)      |

## RGB strip scene map

| Scene           | Color    | Trigger              |
|-----------------|----------|----------------------|
| `IDLE`          | accent_hi| default              |
| `LONELY`        | #FACC15  | no interaction 5 min |
| `OTA`           | #A855F7  | OTA progress visible |
| `ERROR`         | #EF4444  | exception            |
| `CALL`          | #22C55E  | inbound call         |
| `LOW_BAT`       | #F97316  | < 15 % battery       |
| `OVERHEAT`      | #DC2626  | > 70 °C              |

## Persona registry

`lyra · echo · nova · sage · pico · doc` — all 6 ids, name_zh, tagline_zh, face_color match the React prototype's `PERSONAS` array byte-for-byte.

## Known gaps (tracked for v7.7)

- [ ] Icon registry generation (`xb_icons_stub.c` is a placeholder)
- [ ] Real cross-fade transition in `xb_router` (currently swap)
- [ ] Skills marketplace page (currently empty)
- [ ] sqlite-backed memory only enabled when `XB_HAS_SQLITE` is defined
