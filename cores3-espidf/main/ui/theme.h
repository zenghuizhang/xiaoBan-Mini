#pragma once

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    THEME_DARK,
    THEME_LIGHT
} ThemeMode;

// ========== 黑夜模式（青色发光） ==========
// 4层发光 - 从内到外透明度递减
#define GLOW_LAYERS 4

// 发光层级偏移量（像素）
#define GLOW_OFFSET_1  2    // 最内层
#define GLOW_OFFSET_2  4
#define GLOW_OFFSET_3  6
#define GLOW_OFFSET_4  8    // 最外层

// 发光层级透明度
#define GLOW_OPA_1     LV_OPA_100   // 核心 100%
#define GLOW_OPA_2     LV_OPA_50    // 第一层 50%
#define GLOW_OPA_3     LV_OPA_20    // 第二层 20%
#define GLOW_OPA_4     LV_OPA_10    // 最外层 10%

// 颜色定义
#define COLOR_CORE        lv_color_hex(0xFFFFFF)   // 核心纯白
#define COLOR_GLOW_1      lv_color_hex(0x22D3EE)   // 亮青色
#define COLOR_GLOW_2      lv_color_hex(0x06B6D4)   // 中青色
#define COLOR_GLOW_3      lv_color_hex(0x0891B2)   // 暗青色
#define COLOR_GLOW_4      lv_color_hex(0x155E75)   // 最深青色

#define COLOR_DARK_BG     lv_color_hex(0x000000)
#define COLOR_LIGHT_BG    lv_color_hex(0xF0FDFA)
#define COLOR_LIGHT_FG    lv_color_hex(0x164E63)

/**
 * 初始化主题系统
 */
void theme_init(ThemeMode mode);

/**
 * 切换主题
 */
void theme_switch(ThemeMode mode);

/**
 * 获取当前主题
 */
ThemeMode theme_get_current(void);

/**
 * 绘制一个带 4 层发光效果的圆角矩形
 * 
 * @param parent 父对象
 * @param cx 中心 X
 * @param cy 中心 Y
 * @param w 宽度
 * @param h 高度
 * @param r 圆角半径
 */
lv_obj_t* theme_draw_glowing_pill(lv_obj_t *parent, int cx, int cy, int w, int h, int r);

#ifdef __cplusplus
}
#endif
