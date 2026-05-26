// AUTO-GENERATED theme_tokens.h — DO NOT EDIT BY HAND
// v7.6 — regenerated from React prototype "陪伴机器人 UI 原型 v7.6"
//   Active themes : Tech (default) / Lavender / Child / Cocoa
//   Dropped vs v6.3 round2 : Warm / Dev  (replaced by Lavender / Cocoa)
//   Legacy carry  : Orange / Blue / Pink / Green / Purple / Yellow / Mono
//                   (all unknown names fall back to TECH for OTA back-compat)
//
// Token notes
//   - Tech / Cocoa are dark themes  -> cards use panel @ 30% alpha
//   - Lavender / Child are light themes -> cards use opaque panel
//   - accent_hi  = brighter highlight, used by face eyes/mouth, RGB strip, glow halos.
//                  In React it is the actual face color (NOT accent).
#pragma once
#include <string.h>
#include "lvgl.h"

typedef struct {
    lv_color_t bg, panel, accent, accent_dim, text, text_dim, border, danger, success;
    lv_color_t accent_hi;   // brighter highlight used by face / RGB strip / glow
    uint8_t    is_light;    // 0 = dark theme, 1 = light theme  (drives card alpha logic)
} theme_t;

// ---- TECH (default, dark) ---------------------------------------------------
static const theme_t THEME_TECH = {
    lv_color_make(  0,  0,  0),    // bg         = #000000
    lv_color_make( 10, 30, 40),    // panel      = #0a1e28
    lv_color_make( 34,211,238),    // accent     = #22D3EE  (cyan-400)
    lv_color_make( 15, 90,120),    // accent_dim
    lv_color_make(180,235,255),    // text       = #b4ebff
    lv_color_make(110,170,200),    // text_dim
    lv_color_make( 20, 80,110),    // border     = #14506e
    lv_color_make(244, 63, 94),    // danger     = #F43F5E
    lv_color_make( 34,197, 94),    // success
    lv_color_make( 51,197,255),    // accent_hi  = #33C5FF
    0,                              // is_light
};

// ---- LAVENDER (light, soft purple) ------------------------------------------
static const theme_t THEME_LAVENDER = {
    lv_color_make(250,245,255),    // bg         = #FAF5FF
    lv_color_make(255,255,255),    // panel      = #FFFFFF
    lv_color_make(147, 51,234),    // accent     = #9333EA  (purple-600)
    lv_color_make(168, 85,247),    // accent_dim ~ accent_hi sibling
    lv_color_make( 76, 29,149),    // text       = #4C1D95
    lv_color_make(124, 58,237),    // text_dim
    lv_color_make(233,213,255),    // border     = #E9D5FF
    lv_color_make(239, 68, 68),    // danger     = #EF4444
    lv_color_make( 34,197, 94),    // success
    lv_color_make(168, 85,247),    // accent_hi  = #A855F7
    1,                              // is_light
};

// ---- CHILD (light, warm coral) ----------------------------------------------
static const theme_t THEME_CHILD = {
    lv_color_make(255,249,230),    // bg         = #FFF9E6
    lv_color_make(255,255,255),    // panel      = #FFFFFF
    lv_color_make(255,127, 80),    // accent     = #FF7F50  (coral)
    lv_color_make(255,170,120),    // accent_dim
    lv_color_make(200, 90, 40),    // text       = #c85a28
    lv_color_make(210,140, 90),    // text_dim
    lv_color_make(255,200,160),    // border     = #FFC8A0
    lv_color_make(244, 63, 94),    // danger     = #F43F5E
    lv_color_make( 34,197, 94),    // success
    lv_color_make(255,170,120),    // accent_hi  = #FFAA78
    1,                              // is_light
};

// ---- COCOA (dark, strawberry cocoa) -----------------------------------------
static const theme_t THEME_COCOA = {
    lv_color_make( 45, 27, 14),    // bg         = #2D1B0E
    lv_color_make( 63, 43, 32),    // panel      = #3F2B20
    lv_color_make(251,113,133),    // accent     = #FB7185  (rose-400)
    lv_color_make(253,164,175),    // accent_dim
    lv_color_make(254,243,199),    // text       = #FEF3C7
    lv_color_make(217,193,160),    // text_dim
    lv_color_make( 87, 61, 44),    // border     = #573D2C
    lv_color_make(239, 68, 68),    // danger     = #EF4444
    lv_color_make( 34,197, 94),    // success
    lv_color_make(253,164,175),    // accent_hi  = #FDA4AF
    0,                              // is_light
};

// ---- lookup ----------------------------------------------------------------
// Accepts only the 4 active themes. Any legacy or unknown name silently falls
// back to TECH so old NVS values from previous firmware still boot cleanly.
static inline const theme_t* xb_theme_lookup(const char* name) {
    if(!name) return &THEME_TECH;
    if(strcmp(name,"tech")    ==0) return &THEME_TECH;
    if(strcmp(name,"lavender")==0) return &THEME_LAVENDER;
    if(strcmp(name,"child")   ==0) return &THEME_CHILD;
    if(strcmp(name,"cocoa")   ==0) return &THEME_COCOA;
    return &THEME_TECH;
}

// Card opacity helper: in dark themes cards are panel @ 30% to keep face
// visible; in light themes cards use opaque panel for legibility.
static inline lv_opa_t xb_card_panel_opa(const theme_t* t) {
    return t->is_light ? LV_OPA_COVER : (lv_opa_t)(0x4D); // 0x4D ≈ 30%
}
