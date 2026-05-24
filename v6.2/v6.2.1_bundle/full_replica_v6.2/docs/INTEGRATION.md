# Integration guide

## 1. Drop into ESP-IDF project

```
my_project/
└── components/
    └── xb_ui/                 ← copy this entire pack here
        ├── CMakeLists.txt
        ├── src/
        └── assets/
```

Then in your top-level `main/CMakeLists.txt`:

```cmake
idf_component_register(SRCS "app_main.c" REQUIRES xb_ui)
```

In your `app_main.c`:

```c
#include "esp_lvgl_port.h"
extern void xb_app_start(void);

void app_main(void) {
    // 1) Bring up display + lvgl_port via your board package (M5 CoreS3 BSP, etc.)
    bsp_display_start();
    // 2) Start the UI
    xb_app_start();
}
```

## 2. Replace icon stubs with real images

The icons under `assets/icons/*.svg` are the source of truth. Convert them
to LVGL C arrays once:

```bash
# 1. Rasterize to PNG (any size — we recommend 16x16 or 24x24)
for f in assets/icons/*.svg; do
    rsvg-convert -h 16 "$f" -o "${f%.svg}.png"
done

# 2. Convert PNG to LVGL C array (LVGL 9 online tool or lv_img_conv)
lv_img_conv assets/icons/*.png \
    --output-format c \
    --color-format ARGB8888 \
    --output-dir src/assets/generated/
```

Then **delete** `src/assets/xb_icons_stub.c` and add the generated `.c` files
to `CMakeLists.txt`. Symbol names in the generated files must match what the
page code expects (`ic_chevron_left`, `ic_battery`, etc.). If they don't,
rename via `--name-prefix=ic_`.

## 3. Wire system events

Every page already subscribes; just call `xb_event_post` from your
real handlers:

```c
// Wi-Fi state machine
on_wifi_disconnected: xb_event_post(XB_EVT_WIFI_STATE, (void*)0);
on_wifi_connecting:   xb_event_post(XB_EVT_WIFI_STATE, (void*)1);
on_wifi_connected:    xb_event_post(XB_EVT_WIFI_STATE, (void*)2);

// Battery monitor (every 30s)
xb_event_post(XB_EVT_BATTERY, (void*)(intptr_t)pct);

// OTA
xb_event_post(XB_EVT_OTA_PROGRESS, (void*)(intptr_t)progress);

// Chat streaming (per token)
xb_event_post(XB_EVT_CHAT_DELTA, (void*)token_str);
xb_event_post(XB_EVT_CHAT_DONE, NULL);
```

On the real device replace `xb_event.c` with a thin wrapper around
`claw_event_router` so events propagate to other components too.

## 4. Face engine

`xb_face.c` is a minimal LVGL renderer with eye + mouth shapes. On hardware
delete it and forward `xb_face_set` to `claw_emote` — the rest of the code
already routes through this single setter.

## 5. AVG animations

`assets/anims/*.json` describes 10 named timelines (MI-01 dot-loading,
MI-03 toast, MI-04 menu burst, etc.). The widget primitives (`xb_dot_loading`,
`xb_toast`, etc.) implement them inline with `lv_anim`. If you swap to
`lottie` or `claw_anim_player`, point the players at these JSONs.

## 6. Theme switch

```c
xb_theme_set("child");   // broadcasts XB_EVT_THEME_CHANGED automatically
```

Subscribe to `XB_EVT_THEME_CHANGED` in any custom code and re-read styles.
The 10 built-in keys are: `tech child dev orange blue pink green purple yellow mono`.

## 7. Build flags

- LVGL config must enable: `LV_USE_TABVIEW`, `LV_USE_BAR`, `LV_USE_SLIDER`,
  `LV_USE_SWITCH`, `LV_USE_TEXTAREA`, `LV_USE_FLEX`, `LV_USE_IMAGE`,
  `LV_FONT_MONTSERRAT_28`.
- `CONFIG_LV_COLOR_DEPTH=16` for the panel; ARGB8888 stubs auto-convert.
