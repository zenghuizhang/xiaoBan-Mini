#pragma once

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 弹簧动画参数
 * 基于真实物理的动画系统，这就是 LVGL 吊打 Arduino 的地方！
 */
typedef struct {
    float stiffness;    // 刚度 k = 300 (Face.tsx 默认值)
    float damping;      // 阻尼 c = 20 (Face.tsx 默认值)
    float mass;         // 质量 m = 1
} SpringParams;

// ========== 标准弹簧预设 ==========

// Face.tsx 默认值 - 机器人标准动画
#define SPRING_DEFAULT  ((SpringParams){300, 20, 1})

// 柔软 - 呼吸效果，很治愈
#define SPRING_SOFT     ((SpringParams){180, 12, 1})

// 干脆 - 触摸反应，干脆利落
#define SPRING_SNAPPY    ((SpringParams){500, 25, 1})

// 弹性 - 弹跳效果
#define SPRING_BOUNCY    ((SpringParams){300, 8, 1})

// 困倦 - 慢动作
#define SPRING_SLEEPY    ((SpringParams){100, 30, 1})

/**
 * 创建一个属性动画（任意 int32_t 属性）
 */
void anim_spring(
    lv_obj_t *obj,
    int32_t *prop,
    int32_t start_val,
    int32_t end_val,
    SpringParams params,
    lv_anim_exec_xcb_t exec_cb,
    lv_anim_ready_cb_t ready_cb
);

/**
 * Y 位移动画（表情上下浮动）
 */
void anim_spring_y(lv_obj_t *obj, int32_t end_y, SpringParams params);

/**
 * 透明度动画（明暗呼吸）
 */
void anim_spring_opa(lv_obj_t *obj, int32_t end_opa, SpringParams params);

/**
 * 缩放动画（眨眼效果）
 */
void anim_spring_scale(lv_obj_t *obj, int32_t end_scale, SpringParams params);

/**
 * 旋转动画（眩晕效果）
 */
void anim_spring_rotate(lv_obj_t *obj, int32_t end_deg, SpringParams params);

/**
 * 呼吸发光效果 - 眼睛明暗变化
 */
void anim_start_breathing(lv_obj_t *obj);

/**
 * 停止呼吸动画
 */
void anim_stop_breathing(lv_obj_t *obj);

#ifdef __cplusplus
}
#endif
