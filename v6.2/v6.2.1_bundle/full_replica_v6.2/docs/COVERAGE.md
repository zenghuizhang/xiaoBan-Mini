# Coverage vs UX v6.2 spec

P0 = must ship; P1 = nice-to-have for v6.2.

| Page | Spec § | Priority | Status | Notes |
|---|---|---|---|---|
| Boot               | 2.1  | P0 | ✅ done | 3500ms timeline, fade-in mark + ring sweep + face wake |
| Home               | 2.2  | P0 | ✅ done | full-screen face, statusbar, long-press → menu, tap → chat |
| Menu (radial)      | 2.3  | P0 | ✅ done | 6 sectors, 60° spacing, `FACE_MENU` dim |
| Chat               | 3.1  | P0 | ✅ done | streaming, dot loading, error inline, mic/send |
| Wi-Fi AP           | 4.1  | P0 | ✅ done | 4 visual states (idle/connecting/success/error), QR + SSID |
| Wi-Fi Pair (6-dgt) | 4.2  | P0 | ✅ done | 6-box code + countdown |
| OTA                | 4.3  | P0 | ✅ done | 5 stages (check/dl/verify/apply/done) + bar |
| Skills             | 5.1  | P1 | ✅ done | Installed / Store tabs, 2-col grid |
| Skill Detail       | 5.2  | P1 | ✅ done | permission rows + Allow/Deny |
| Settings           | 6.1  | P0 | ✅ done | 11 items in 5 groups, sliders + switches |
| Console            | 7.1  | P1 | ✅ done | Sensors / Scripts / Logs tabs, **2-col scripts** (was 3-col bug) |

## Animations (MI-XX) implemented

| ID | Name | Duration | Where |
|---|---|---|---|
| MI-01 | dot loading wave   | 400ms × 3 phase | `xb_dot_loading_create` |
| MI-02 | ring sweep         | 1200ms          | `page_boot` |
| MI-03 | toast slide+fade   | 200ms in, ms hold | `xb_toast` |
| MI-04 | menu sector burst  | 250ms (radial)  | `page_menu` |
| MI-05 | bubble grow-in     | 150ms scale     | `page_chat` |
| MI-06 | progress bar       | continuous      | `page_ota` |
| MI-07 | face state morph   | 180ms           | `xb_face` |
| MI-08 | tab cross-fade     | 120ms           | `page_skills`, `page_console` |
| MI-09 | error shake        | 80ms × 3        | `xb_error_inline` |
| MI-10 | success check pop  | 200ms           | `page_wifi_ap` SUCCESS |

(MI-08/09/10 are scaffolded — extend in `xb_widgets.c` when polishing.)

## Themes implemented (10)

`tech child dev orange blue pink green purple yellow mono` — all
defined in `assets/themes/theme_tokens.h` and selectable at runtime via
`xb_theme_set(name)`.

## Icons (81)

Full lucide-derived set under `assets/icons/`. `xb_icons_stub.c` provides
ARGB8888 16×16 transparent stubs so the project links before you run
`lv_img_conv`. Names already match the page code references.

## Known follow-ups

- Replace stubs with rasterized + converted icons (see INTEGRATION §2).
- Hook `xb_face_set` to real `claw_emote` on device.
- Wire `XB_EVT_*` posts from BSP / claw layer (see INTEGRATION §3).
- Bigger fonts: subset CJK glyphs into a Pinyin/Hans font for Chinese mode.
