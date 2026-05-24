#pragma once

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

// v5.0 三个主题: Tech (cyan), Child (coral), Dev (green)
typedef enum {
    THEME_TECH,
    THEME_CHILD,
    THEME_DEV
} ThemeV3;

// v5.0: scale = 1.6 (从 v3.8 的 1.4 升级)
#define FACE_SCALE  1.6f

// Tech 主题: cyan-400 #22D3EE
// v6.2: Tech cyan #33C5FF (从 #22D3EE 升级)
#define TECH_FG            lv_color_hex(0x33C5FF)
#define TECH_BG            lv_color_hex(0x000000)

// v6.2: Child coral #FF9E7D (从 #FF7F50 升级)
#define CHILD_FG           lv_color_hex(0xFF9E7D)
#define CHILD_BG           lv_color_hex(0xFFF9E6)

// Dev 主题: green-500 #22C55E (新增)
#define DEV_FG             lv_color_hex(0x22C55E)
#define DEV_BG             lv_color_hex(0x000000)
#define DEV_RADIUS         4       // dev 主题方角风格

void theme_v3_init(ThemeV3 default_theme);
void theme_v3_switch(ThemeV3 theme);
ThemeV3 theme_v3_get_current(void);

// 便捷宏: 当前主题的前景色/背景色
lv_color_t theme_fg(void);
lv_color_t theme_bg(void);

#ifdef __cplusplus
}
#endif
