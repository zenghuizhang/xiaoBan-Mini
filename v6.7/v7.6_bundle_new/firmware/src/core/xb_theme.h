// xb_theme.h — runtime theme switch.
// v7.6: 4 themes (tech/lavender/child/cocoa); legacy names (warm/dev/...) silently fall back to tech.
#pragma once
#include "lvgl.h"
#include "../../assets/themes/theme_tokens.h"

void           xb_theme_init(void);                // load NVS "xb_theme/active", default "tech"
const theme_t* xb_theme_get(void);                 // current
const char*    xb_theme_name(void);                // "tech" | "lavender" | "child" | "cocoa"
void           xb_theme_set(const char* name);     // persists + posts XB_EVT_THEME_CHANGED
bool           xb_theme_is_light(void);            // shorthand for xb_theme_get()->is_light

// Card opacity rule per v7.6: dark themes use panel @ 30% so face shows through;
// light themes use opaque panel for legibility.
lv_opa_t       xb_card_panel_opa(void);            // 0xFF (light) or 0x4D (dark)
