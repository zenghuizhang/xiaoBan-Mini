# xiaobao v6.2 — Full Replica Pack

Drop-in source + assets to fully reproduce the UX v6.2 desktop-companion-robot
UI on M5Stack CoreS3 (ESP32-S3, 320×240) using **LVGL 9.5 + esp_lvgl_port**.

This pack exists because the previous on-device implementation diverged from
the spec: missing icons, solid-bright cards, 3-column grids that truncated
text. Here the **theme tokens, widget primitives, page implementations, and
asset references all match the design source 1:1**.

## What's in the box

| Folder | Purpose |
|---|---|
| `src/core/`     | theme runtime, event bus, face engine bridge |
| `src/widgets/`  | `xb_card`, `xb_button`, `xb_statusbar`, `xb_topbar`, `xb_dot_loading`, `xb_toast`, `xb_error_inline` |
| `src/pages/`    | 11 pages: boot, home, menu, chat, wifi_ap, wifi_pair, ota, skills, skill_detail, settings, console + a tiny stack-based router |
| `src/assets/`   | icon `lv_image_dsc_t` stubs (replace with `lv_img_conv` output before shipping) |
| `assets/icons/` | 81 lucide-style SVG sources |
| `assets/themes/theme_tokens.h` | 10 theme structs (tech, child, dev, orange, blue, pink, green, purple, yellow, mono) |
| `assets/anims/` | 10 AVG animation timelines (MI-01..MI-10) as JSON |
| `assets/fonts/` | font subset manifest |
| `mock_renders/` | 110 hi-fi PNG mocks (10 themes × 11 pages) for visual reference |
| `docs/`         | `INTEGRATION.md`, `COVERAGE.md` |

## Quick map

- **Card pattern** (everywhere): bg=panel @ opa30, 1px border, radius 4, pad 6
- **Statusbar** (22px) right-aligned slots: battery / wifi / ai_dot / plug
- **Topbar** (28px) below statusbar, accent title, optional back chevron
- **Console grid is 2-column** (148px cards), fixing the prior 3-col truncation
- All icons go through `image_recolor` so any theme just works

## How pages talk to the system

Every page subscribes to `xb_event_*` and receives:

| Event | Used by |
|---|---|
| `XB_EVT_THEME_CHANGED` | (everywhere — pages re-read `xb_theme_get`) |
| `XB_EVT_WIFI_STATE`    | statusbar, page_wifi_ap |
| `XB_EVT_BATTERY`       | statusbar |
| `XB_EVT_AI_STATE`      | statusbar, face |
| `XB_EVT_OTA_PROGRESS`  | page_ota |
| `XB_EVT_CHAT_DELTA/DONE/ERROR` | page_chat |
| `XB_EVT_FACE_REQUEST`  | xb_face |

See [INTEGRATION.md](docs/INTEGRATION.md) for hookup; coverage matrix in
[COVERAGE.md](docs/COVERAGE.md).
