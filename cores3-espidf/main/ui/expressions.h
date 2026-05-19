#pragma once

#include <lvgl.h>
#include "theme_v3.h"

#ifdef __cplusplus
extern "C" {
#endif

// 严格对齐 Face.tsx 的 8 种状态
typedef enum {
    EXPR_IDLE,        // 发呆 - 睁眼
    EXPR_HAPPY,       // 开心 - 上半圆眼睛 + 下半圆嘴巴
    EXPR_TALKING,     // 说话中
    EXPR_MENU,        // 菜单状态
    EXPR_DIZZY,       // 眩晕
    EXPR_CRYING,      // 哭泣
    EXPR_NAUGHTY,     // 调皮 - 眨一只眼
    EXPR_WINK         // 眨眼笑 - 左眼挤眼+右眼弯月+嘴巴歪笑
} Expression;

/**
 * 表情坐标定义（严格对齐 Face.tsx）
 * Face.tsx: const baseScale = 1.4
 * 以下尺寸均为原始值，绘制时乘以 scale 因子
 */
#define FACE_BASE_SCALE 1.4f    // Face.tsx baseScale
#define FACE_EYE_GAP    56      // gap-14 = 56px (对齐 Face.tsx)

// 眼睛尺寸 - Face.tsx 原始值（绘制时 × scale）
#define EYE_W_IDLE      32
#define EYE_H_IDLE      40
#define EYE_W_HAPPY     36
#define EYE_H_HAPPY     12
#define EYE_R_IDLE      16      // borderRadius 不缩放 (Face.tsx 特性)

// 眼嘴 Y 位置
#define EYE_Y           105
#define MOUTH_X         160
#define MOUTH_Y         172

/**
 * 初始化表情引擎
 */
void expressions_init(void);

/**
 * 设置表情（带动画过渡）
 */
void expression_set(Expression expr, bool animate);

/**
 * 获取当前表情
 */
Expression expression_get_current(void);

/**
 * 启动随机表情轮播定时器
 */
void expression_start_carousel(void);

/**
 * 主题切换后刷新表情
 */
void expression_refresh_theme(void);

/**
 * 处理待决的表情轮播和眨眼（在主循环调用，避免定时器堆栈溢出）
 */
void expression_process_pending(void);

/**
 * 启用/禁用面部绘制（Menu 浮层打开时需禁用）
 */
void expression_set_drawing_enabled(bool enabled);

#ifdef __cplusplus
}
#endif
