// v7.6 4-theme palette — 严格对齐 theme_tokens.h (RGB565 grid-snapped)
#include "theme_v3.h"
#include <esp_log.h>

static const char* TAG = "THEME";
static ThemeV3 cur = THEME_TECH;

static const theme_colors_t P[4] = {
    // TECH dark cyan — bg=#000000 accent=#22D3EE accent_hi=#33C5FF
    {lv_color_hex(0x000000),lv_color_hex(0x0A1E28),lv_color_hex(0x22D3EE),lv_color_hex(0x0F5A78),
     lv_color_hex(0xB4EBFF),lv_color_hex(0x6EAAC8),lv_color_hex(0x14506E),lv_color_hex(0xF43F5E),
     lv_color_hex(0x22C55E),lv_color_hex(0x33C5FF),0},
    // LAVENDER light purple — bg=#FAF5FF accent=#9333EA accent_hi=#A855F7
    {lv_color_hex(0xFAF5FF),lv_color_hex(0xFFFFFF),lv_color_hex(0x9333EA),lv_color_hex(0xA855F7),
     lv_color_hex(0x4C1D95),lv_color_hex(0x7C3AED),lv_color_hex(0xE9D5FF),lv_color_hex(0xEF4444),
     lv_color_hex(0x22C55E),lv_color_hex(0xA855F7),1},
    // CHILD light coral — bg=#FFF9E6 accent=#FF7F50 accent_hi=#FFAA78
    {lv_color_hex(0xFFF9E6),lv_color_hex(0xFFFFFF),lv_color_hex(0xFF7F50),lv_color_hex(0xFFAA78),
     lv_color_hex(0xC85A28),lv_color_hex(0xD28C5A),lv_color_hex(0xFFC8A0),lv_color_hex(0xF43F5E),
     lv_color_hex(0x22C55E),lv_color_hex(0xFFAA78),1},
    // COCOA dark rose — bg=#2D1B0E accent=#FB7185 accent_hi=#FDA4AF
    {lv_color_hex(0x2D1B0E),lv_color_hex(0x3F2B20),lv_color_hex(0xFB7185),lv_color_hex(0xFDA4AF),
     lv_color_hex(0xFEF3C7),lv_color_hex(0xD9C1A0),lv_color_hex(0x573D2C),lv_color_hex(0xEF4444),
     lv_color_hex(0x22C55E),lv_color_hex(0xFDA4AF),0},
};

void theme_v3_init(ThemeV3 t){cur=t;lv_obj_set_style_bg_color(lv_screen_active(),P[cur].bg,0);}
void theme_v3_switch(ThemeV3 t){if(t==cur)return;cur=t;lv_obj_set_style_bg_color(lv_screen_active(),P[cur].bg,0);}
ThemeV3 theme_v3_get_current(void){return cur;}
lv_color_t theme_fg(void){return P[cur].accent_hi;}
lv_color_t theme_bg(void){return P[cur].bg;}
const theme_colors_t* theme_get_colors(void){return &P[cur];}
