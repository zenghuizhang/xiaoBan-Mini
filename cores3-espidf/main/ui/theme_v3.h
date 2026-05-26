#pragma once
#include <lvgl.h>
#ifdef __cplusplus
extern "C" {
#endif

// v6.7 四个主题: Tech (cyan), Lavender (purple), Child (coral), Cocoa (rose)
typedef enum {
    THEME_TECH,
    THEME_LAVENDER,
    THEME_CHILD,
    THEME_COCOA
} ThemeV3;

#define FACE_SCALE  1.6f

// backwards-compat macros
#define TECH_FG  lv_color_hex(0x33C5FF)
#define TECH_BG  lv_color_hex(0x000000)
#define CHILD_FG lv_color_hex(0xFFAA78)
#define CHILD_BG lv_color_hex(0xFFF9E6)

typedef struct {
    lv_color_t bg, panel, accent, accent_dim, text, text_dim, border, danger, success;
    lv_color_t accent_hi;  // face/RGB strip/glow 高亮色
    uint8_t is_light;      // 0=dark, 1=light (卡片透明度)
} theme_colors_t;

void theme_v3_init(ThemeV3 t);
void theme_v3_switch(ThemeV3 t);
ThemeV3 theme_v3_get_current(void);
lv_color_t theme_fg(void);   // accent_hi
lv_color_t theme_bg(void);
const theme_colors_t* theme_get_colors(void);

#ifdef __cplusplus
}
#endif
