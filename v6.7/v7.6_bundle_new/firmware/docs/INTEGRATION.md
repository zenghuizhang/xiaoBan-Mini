# claw_xb_ui v7.6 — Integration Guide

## 1. Drop-in placement

```
<your-esp-idf-project>/
├── CMakeLists.txt
├── main/
│   ├── CMakeLists.txt          # add xb_ui to REQUIRES
│   ├── main.c                  # (or use src/main.c from this bundle)
│   └── bsp_corS3.c             # display + touch init -> lvgl_port_*
└── components/
    └── xb_ui/                  # <-- this entire `firmware/` folder
        ├── CMakeLists.txt
        ├── idf_component.yml
        ├── src/...
        └── assets/themes/theme_tokens.h
```

`main/CMakeLists.txt` only needs:

```cmake
idf_component_register(SRCS "main.c" "bsp_corS3.c"
                       INCLUDE_DIRS "."
                       REQUIRES xb_ui)
```

## 2. Toolchain

| Tool                 | Version              |
|----------------------|----------------------|
| ESP-IDF              | **5.5.4 LTS**        |
| LVGL                 | **9.5.0**            |
| esp_lvgl_port        | ≥ 2.4.0              |
| Target               | `esp32s3` (CoreS3)   |
| Flash / PSRAM        | 16 MB / 8 MB (QSPI)  |

```bash
idf.py set-target esp32s3
idf.py menuconfig   # enable PSRAM Octal mode, LVGL with custom tick
idf.py build flash monitor
```

## 3. Boot sequence

`main.c` orders subsystems strictly:

1. `nvs_flash_init()` — required by `xb_theme` and `xb_persona`
2. `xb_event_init()`
3. `xb_theme_init()` — reads NVS `xb_theme/active`, defaults to `"tech"`
4. `xb_persona_init()` — reads NVS `xb_persona/active`, defaults to `"lyra"`
5. `xb_memory_init()` — opens `/littlefs/memory.db` if `XB_HAS_SQLITE`, else RAM fallback
6. `bsp_init()` — your board layer brings up LVGL on the active screen
7. `xb_router_init(lv_screen_active())` — boots into `PAGE_BOOT`, animates to `PAGE_HOME`

## 4. Event hookup

The UI consumes — and does **not** produce — these events from device drivers:

| Event                  | Producer                | Notes |
|------------------------|-------------------------|-------|
| `XB_EVT_WIFI_STATE`    | network stack           | payload `int` 0/1/2 |
| `XB_EVT_BATTERY`       | AXP2101 driver          | payload `int %` |
| `XB_EVT_PLUG`          | AXP2101 driver          | payload `bool` |
| `XB_EVT_IMU`           | MPU6886 driver          | payload `imu_event_t` |
| `XB_EVT_CHAT_DELTA/DONE/ERROR` | LLM client      | streaming tokens |

The console page synthesizes all of these locally for off-device demo.

## 5. NVS namespaces

Both ≤ 15 chars (ESP-IDF requirement).

| Namespace    | Key      | Type   | Default   |
|--------------|----------|--------|-----------|
| `xb_theme`   | `active` | str    | `"tech"`  |
| `xb_persona` | `active` | str    | `"lyra"`  |

## 6. Card opacity rule (v7.6)

`xb_card_panel_opa()` returns:

| Theme    | is_light | Card alpha | Rationale                                    |
|----------|----------|------------|----------------------------------------------|
| tech     | 0        | `0x4D` 30% | Dark — let face glow show through            |
| lavender | 1        | `0xFF`     | Light — maximize legibility                  |
| child    | 1        | `0xFF`     | Light                                        |
| cocoa    | 0        | `0x4D` 30% | Dark                                         |

All cards/buttons/bubbles obey this — no per-page branching needed.

## 7. Face color rule

The face engine paints eyes/mouth/halo with `theme->accent_hi` — **NOT** `accent`.
This matches the React prototype's `face.color` and gives the eyes their characteristic
highlight saturation. The glow halo uses the same color at 0xB3 opacity, 20px wide.

## 8. Production checklist

- [ ] Wire `bsp_init()` for ST7789 + FT6336U
- [ ] Add `esp_sqlite` to `idf_component.yml` deps (optional, for persistent memory)
- [ ] Generate icon registry: `tools/svg_to_lvgl.py assets/icons/ → src/assets/xb_icons.c`
- [ ] Replace `xb_icons_stub.c` with the generated registry
- [ ] Replace `LV_SYMBOL_*` fallbacks at call sites with `xb_icon_get("name")`
- [ ] Hook the LLM client into `XB_EVT_CHAT_*`
- [ ] Run `python tools/coverage_diff.py` against the React prototype
