// v6.7 4-theme palette — RGB565 grid-snapped (R/B×8, G×4)
#include "theme_v3.h"
#include <esp_log.h>

static const char* TAG = "THEME";
static ThemeV3 cur = THEME_TECH;

static const theme_colors_t P[4] = {
    // TECH dark cyan
    {lv_color_hex(0x000000),lv_color_hex(0x081C28),lv_color_hex(0x20D0E8),lv_color_hex(0x085878),
     lv_color_hex(0xB0E8F8),lv_color_hex(0x68A8C8),lv_color_hex(0x105068),lv_color_hex(0xF03C58),
     lv_color_hex(0x20C458),lv_color_hex(0x30C4F8),0},
    // LAVENDER light purple
    {lv_color_hex(0xF8F4F8),lv_color_hex(0xFFFFFF),lv_color_hex(0x9030E8),lv_color_hex(0xA854F0),
     lv_color_hex(0x481C90),lv_color_hex(0x7838E8),lv_color_hex(0xE8D4F8),lv_color_hex(0xE84440),
     lv_color_hex(0x20C458),lv_color_hex(0xA854F0),1},
    // CHILD light coral (warm-tuned for GC9A01)
    {lv_color_hex(0xF8F0D0),lv_color_hex(0xFFFFFF),lv_color_hex(0xF87C50),lv_color_hex(0xF8A878),
     lv_color_hex(0xC85828),lv_color_hex(0xD08C58),lv_color_hex(0xF8C8A0),lv_color_hex(0xF03C58),
     lv_color_hex(0x20C458),lv_color_hex(0xF8A878),1},
    // COCOA dark rose
    {lv_color_hex(0x281808),lv_color_hex(0x382820),lv_color_hex(0xF87080),lv_color_hex(0xF8A4A8),
     lv_color_hex(0xF8F0C0),lv_color_hex(0xD8C0A0),lv_color_hex(0x503C28),lv_color_hex(0xE84440),
     lv_color_hex(0x20C458),lv_color_hex(0xF8A4A8),0},
};

void theme_v3_init(ThemeV3 t){cur=t;lv_obj_set_style_bg_color(lv_screen_active(),P[cur].bg,0);}
void theme_v3_switch(ThemeV3 t){if(t==cur)return;cur=t;lv_obj_set_style_bg_color(lv_screen_active(),P[cur].bg,0);}
ThemeV3 theme_v3_get_current(void){return cur;}
lv_color_t theme_fg(void){return P[cur].accent_hi;}
lv_color_t theme_bg(void){return P[cur].bg;}
const theme_colors_t* theme_get_colors(void){return &P[cur];}
