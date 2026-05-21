#pragma once

#include <lvgl.h>
#include "theme_v3.h"

#ifdef __cplusplus
extern "C" {
#endif

// v5.0 表情状态 (对齐 Face.tsx v5.0 的 20 种状态)
typedef enum {
    EXPR_IDLE,          // 自然呼吸
    EXPR_HAPPY,         // 开心 - 弯月眼+大笑
    EXPR_TALKING,       // 说话
    EXPR_MENU,          // 菜单状态
    EXPR_DIZZY,         // 眩晕
    EXPR_CRYING,        // 哭泣
    EXPR_NAUGHTY,       // 调皮 - 单眼+歪嘴
    EXPR_WINK,          // 眨眼笑 (单次播放)
    EXPR_BREATH,        // 呼吸 (scale/opacity 脉冲)
    EXPR_LOOK_AROUND,   // 张望 (x 轴漂移)
    EXPR_YAWN,          // 哈欠 (半闭眼+大圆嘴)
    EXPR_CURIOUS,       // 好奇 (睁大眼看侧面)
    EXPR_ANGRY,         // 生气 (眯眼+直线嘴)
    EXPR_EXCITED,       // 兴奋 (放大+抖动)
    EXPR_SAD,           // 失落 (下垂眼+下弯嘴)
    EXPR_CELEBRATE,     // 庆祝 (y 轴弹跳)
    EXPR_DEEP_SLEEP,    // 深睡 (极窄眼)
    EXPR_LIGHT_REST,    // 浅休
    EXPR_ALERT,         // 警觉 (睁大)
    EXPR_MORNING,       // 早安
    EXPR_THINKING,      // v6.0: 思考 (侧眼+小嘴)
    EXPR_SURPRISED,     // v6.0: 惊讶 (大眼+圆嘴)
    EXPR_SLEEP_WAKE,    // v6.0: 睡醒 (渐睁眼)
    EXPR_LOST,          // v6.0: 迷茫 (下垂眼+倒嘴)
} Expression;

// v5.0: baseScale = 1.6
#define FACE_EYE_GAP    56      // CSS gap-14 = 56px

// 眼睛基础值 (Face.tsx idle: 32×40 × scale)
#define EYE_W_BASE      32
#define EYE_H_BASE      40

// 眼嘴 Y 坐标 (v5.0: 全屏, 无 StatusBar)
#define EYE_Y           105
#define MOUTH_X         160
#define MOUTH_Y         172

// Dev 主题方角半径
#define EYE_R_TECH      16
#define DEV_EYE_R       4

void expressions_init(void);
void expression_set(Expression expr, bool animate);
Expression expression_get_current(void);
void expression_start_carousel(void);
void expression_refresh_theme(void);
void expression_process_pending(void);
void expression_set_drawing_enabled(bool enabled);

#ifdef __cplusplus
}
#endif
