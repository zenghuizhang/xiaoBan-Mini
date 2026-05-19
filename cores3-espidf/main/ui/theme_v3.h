#pragma once

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    THEME_ADULT,
    THEME_CHILD
} ThemeV3;

/*
 *  ✅ V3.6 精致眼睛 - 100% 对齐 Face.tsx
 *  
 *  🎨 LVGL 颜色公式独家破译：0x GG RR BB
 *  
 *  Face.tsx 标准眼睛设计：
 *    1. 眼睛主体：bg-cyan-400 = #22D3EE
 *    2. box-shadow 发光效果（无描边、无高光）
 *    3. 成人眼睛 idle: 32×40, r=16
 */

// 背景 — 对齐 Face.tsx / RobotUI.tsx
#define ADULT_BG            lv_color_hex(0x000000)  // bg-black
#define CHILD_BG            lv_color_hex(0xFFFBEB)  // bg-amber-50

// 前景色 — 对齐 Face.tsx faceColorClass
#define ADULT_FG            lv_color_hex(0x22D3EE)  // bg-cyan-400
#define CHILD_FG            lv_color_hex(0xFB923C)  // bg-orange-400 (所有面部元素同一色)

// 发光效果
#define ADULT_SHADOW        ADULT_FG
#define ADULT_SHADOW_OPA    LV_OPA_40
#define ADULT_SHADOW_W      12

// 儿童阴影
#define CHILD_SHADOW_OPA    LV_OPA_30

// 动画参数
#define SPRING_STIFFNESS    300
#define SPRING_DAMPING      20
#define SPRING_MASS         1

void theme_v3_init(ThemeV3 default_theme);
void theme_v3_switch(ThemeV3 theme);
ThemeV3 theme_v3_get_current(void);
void theme_v3_draw_eye(lv_obj_t *parent, int cx, int cy, bool is_left);
void theme_v3_draw_mouth(lv_obj_t *parent, int cx, int cy);

#ifdef __cplusplus
}
#endif
