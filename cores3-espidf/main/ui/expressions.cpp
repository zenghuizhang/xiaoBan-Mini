/*
 * v3.10 表情引擎 — scale=1.6, 对齐 Face.tsx v5.0
 * 眼睛: lv_draw_rect 椭圆 + 柔光
 * 嘴巴: lv_draw_arc 原生弧线 (v3.10 重构)
 * 眩晕: lv_draw_arc 螺旋线
 */
#include "expressions.h"
#include "theme_v3.h"
#include "audio_feedback.h"
#include <esp_log.h>
#include <esp_random.h>
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#include <M5Unified.h>
#include <math.h>

// IMU 眼神方向: 设备倾斜时瞳孔朝倾斜方向偏移
static volatile int s_tilt_pupil_x = 0;  // -20 ~ +20
static volatile int s_tilt_pupil_y = 0;  // -10 ~ +10

static const char *TAG = "FACE";

static Expression current_expr = EXPR_IDLE;
static TimerHandle_t blink_timer = NULL;
static TimerHandle_t carousel_timer = NULL;
static volatile bool blinking = false;

// ========== 情绪状态机 (竞品对齐: Emotion State Machine) ==========
// energy: 0-100, 长时间无交互自然衰减, 用户交互时回升
// mood:   -100~100, 受事件影响 (触摸+, 摇晃-, 对话+)
static int s_energy = 70;   // 初始精力
static int s_mood = 0;      // 初始心情
static uint32_t s_last_interaction_ms = 0;  // 上次交互时间

static lv_obj_t *face_container = NULL;
static lv_obj_t *tear_left = NULL, *tear_right = NULL;
static lv_anim_t tear_anim_l, tear_anim_r;

static volatile bool pending_carousel = false;
static volatile uint32_t next_random_expr = EXPR_IDLE;
static volatile bool pending_blink = false;
static volatile bool pending_blink_state = false;

// 动画状态
static int32_t anim_dizzy_rot = 0;      // 眩晕旋转角度 (0~3600)
static int32_t anim_wink_phase = 0;
static int32_t anim_pupil_x = 0;
static int32_t anim_pupil_y = 0;
static int32_t anim_breath_val = 0;
static lv_anim_t face_anim;
static lv_anim_t pupil_anim;
static lv_anim_t breath_anim;
static lv_anim_t dizzy_decel_anim;
static int _anim_tick = 0;
static bool face_drawing_enabled = true;

static void _face_draw_cb(lv_event_t *e);

// ========== 绘制辅助 ==========
static lv_color_t _fg(void) { return theme_fg(); }
static lv_color_t _bg(void) { return theme_bg(); }

static int _eye_radius(void)
{
    if (theme_v3_get_current() == THEME_COCOA) return DEV_EYE_R;
    return EYE_R_TECH;
}

// v3.10: 柔光用多层半透明叠加模拟 (lv_draw_blur 在 ESP32 SW 渲染器上不可用)
static void _draw_glow(lv_layer_t *layer, int cx, int cy, int w, int h, int r, lv_color_t color)
{
    for (int i = 2; i >= 0; i--) {
        lv_draw_rect_dsc_t dsc;
        lv_draw_rect_dsc_init(&dsc);
        dsc.bg_color = color;
        dsc.bg_opa = (lv_opa_t)(12 + i * 8);  // 12, 20, 28 → 柔和渐变
        dsc.radius = r + 4 + i * 3;
        dsc.border_width = 0;
        int pad = 3 + i * 2;
        lv_area_t a;
        a.x1 = cx - w/2 - pad; a.y1 = cy - h/2 - pad;
        a.x2 = cx + w/2 + pad; a.y2 = cy + h/2 + pad;
        lv_draw_rect(layer, &dsc, &a);
    }
}

// v3.10: 弧线嘴巴 (lv_draw_arc 原生绘制)
// 0°=右, 90°=下, 180°=左, 270°=上
// 微笑: center 在嘴巴上方, 弧线通过底部 (角度 ~30°-150°)
// 沮丧: center 在嘴巴下方, 弧线通过顶部 (角度 ~210°-330°)

// v7.6: 眉毛 (竞品对齐: 弧线眉毛角度随表情变化)
// angle > 0: 眉头向下眉尾上扬 (惊讶/好奇)
// angle < 0: 眉头上扬眉尾下压 (生气/困惑)
// angle = 0: 水平 (平静)
static void _draw_eyebrow(lv_layer_t *layer, int cx, int cy, int width,
                           int angle_deg, lv_color_t color, lv_opa_t opa)
{
    if (angle_deg == 0) return;  // 水平时不画 (太细看不清)
    lv_draw_arc_dsc_t dsc;
    lv_draw_arc_dsc_init(&dsc);
    dsc.color = color;
    dsc.opa = opa;
    dsc.width = 2;
    dsc.rounded = 1;
    // 眉毛弧度: 角度越大弧越弯
    int arc_span = 40 + abs(angle_deg);
    dsc.start_angle = 270 - arc_span / 2;
    dsc.end_angle = 270 + arc_span / 2;
    dsc.center.x = cx;
    dsc.center.y = cy + abs(angle_deg) / 3;  // 弯曲时中心下移
    dsc.radius = width / 2 + abs(angle_deg) / 4;
    lv_draw_arc(layer, &dsc);
}
static void _draw_mouth_arc(lv_layer_t *layer, int cx, int cy, int radius,
                             int start_angle, int end_angle, int width,
                             lv_color_t color, lv_opa_t opa)
{
    lv_draw_arc_dsc_t dsc;
    lv_draw_arc_dsc_init(&dsc);
    dsc.color = color;
    dsc.opa = opa;
    dsc.width = width;
    dsc.rounded = 1;
    dsc.start_angle = start_angle;
    dsc.end_angle = end_angle;
    dsc.center.x = cx;
    dsc.center.y = cy;
    dsc.radius = radius;
    lv_draw_arc(layer, &dsc);
}

// v3.10: 填充圆嘴 (粗弧线模拟实心圆)
static void _draw_mouth_circle(lv_layer_t *layer, int cx, int cy, int radius,
                                lv_color_t color, lv_opa_t opa)
{
    lv_draw_arc_dsc_t dsc;
    lv_draw_arc_dsc_init(&dsc);
    dsc.color = color;
    dsc.opa = opa;
    dsc.width = radius;   // 粗线 = 实心圆
    dsc.rounded = 1;
    dsc.start_angle = 0;
    dsc.end_angle = 360;
    dsc.center.x = cx;
    dsc.center.y = cy;
    dsc.radius = radius;
    lv_draw_arc(layer, &dsc);
}

// v3.10: 螺旋线眼睛 (眩晕效果)
static void _draw_spiral_eye(lv_layer_t *layer, int cx, int cy, int anim_rot,
                              lv_color_t color)
{
    float base = anim_rot * M_PI / 1800.0f;
    lv_draw_arc_dsc_t dsc;
    for (int i = 0; i < 3; i++) {
        lv_draw_arc_dsc_init(&dsc);
        dsc.color = color;
        dsc.width = 3;
        dsc.rounded = 1;
        int r = 8 + i * 7;
        dsc.start_angle = (int)((base + i * 1.2f) * 180.0f / M_PI) % 360;
        dsc.end_angle = dsc.start_angle + 300;
        dsc.center.x = cx;
        dsc.center.y = cy;
        dsc.radius = r;
        lv_draw_arc(layer, &dsc);
    }
}

// ========== 核心绘制 ==========
static void _face_draw_cb(lv_event_t *e)
{
    lv_layer_t *layer = lv_event_get_layer(e);
    if (!layer || !face_drawing_enabled) return;

    Expression expr = current_expr;
    lv_color_t fg = _fg(), bg = _bg();
    int r_style = _eye_radius();
    float s = FACE_SCALE;  // v5.0: 1.6

    int ey = EYE_Y, my = MOUTH_Y;

    // --- 眼睛参数 (Face.tsx v5.0) ---
    int base_lw = EYE_W_BASE, base_lh = EYE_H_BASE;
    int base_rw = EYE_W_BASE, base_rh = EYE_H_BASE;
    int ly_off = 0, ry_off = 0;

    float breath_scale = 1.0f, breath_opa = 1.0f;

    // v5.0 瞳孔微动: 从动画中读取 x/y 偏移 + IMU 倾斜方向
    int pupil_x = anim_pupil_x + s_tilt_pupil_x;

    // 呼吸值 → scale/opacity (5级呼吸: deep_sleep 5% / light_rest 8% / idle 10% / alert 15% / excited 18%)
    float breath_factor = anim_breath_val / 255.0f;
    float breath_amp = (expr == EXPR_DEEP_SLEEP) ? 0.05f : (expr == EXPR_LIGHT_REST) ? 0.08f
                     : (expr == EXPR_ALERT) ? 0.15f : (expr == EXPR_EXCITED) ? 0.18f : 0.10f;
    // cubic-eased breathing: 更自然, 吸气快->呼气慢
    float raw = sinf(breath_factor * 2.0f * M_PI);
    float eased = raw * raw * raw;  // cubic: 平滑缓动
    float breath_mod = 1.0f + breath_amp * eased;

    switch (expr) {
    case EXPR_IDLE:
        if (blinking) { base_lh = 1; base_rh = 1; }
        ly_off = ry_off = -15;  // PRD: 眼睛上移，视觉居中
        break;
    case EXPR_DEEP_SLEEP:
        base_lh = base_rh = 16; ly_off = ry_off = 5;
        breath_scale = 0.92f; breath_opa = 0.3f;
        break;
    case EXPR_LIGHT_REST:
        base_lh = base_rh = (blinking ? 1 : 32);
        breath_scale = 0.96f; breath_opa = 0.5f;
        break;
    case EXPR_ALERT:
        base_lh = base_rh = (blinking ? 1 : 44);
        base_lw = base_rw = 34;
        ly_off = ry_off = -2;
        breath_scale = 1.10f; breath_opa = 0.8f;
        break;
    case EXPR_HAPPY:
        base_lw = base_rw = 36; base_lh = base_rh = 12;
        ly_off = ry_off = -10;
        break;
    case EXPR_TALKING:
        if (blinking) { base_lh = 1; base_rh = 1; }
        break;
    case EXPR_MENU:
        base_lw = base_rw = 20; base_lh = base_rh = 20;
        ly_off = ry_off = -20;
        break;
    case EXPR_DIZZY:
        // v3.10: 眩晕 — 跳过矩形眼, 用 pupil 圆点 + 螺旋弧线
        base_lw = base_rw = 10; base_lh = base_rh = 10;
        break;
    case EXPR_CRYING:
        base_lw = base_rw = 32; base_lh = base_rh = 8;
        ly_off = ry_off = 5;
        break;
    case EXPR_NAUGHTY:
        base_lh = 6;  // 左眼半闭
        base_rh = EYE_H_BASE;
        break;
    case EXPR_WINK:
        // v6.2: 左眼闭合, 右眼睁大挑眉 (单次 1.5s)
        if (anim_wink_phase) {
            base_lw = 36; base_lh = 1; ly_off = 14;  // 左眼闭合
            base_rw = 38; base_rh = 54; ry_off = -16; // 右眼睁大
        }
        break;
    case EXPR_BREATH:
        if (blinking) { base_lh = 1; base_rh = 1; }
        breath_scale = 0.97f; breath_opa = 0.7f;
        break;
    case EXPR_LOOK_LEFT:   // v6.1: 倾斜左→眼睛看右
        if (blinking) { base_lh = 1; base_rh = 1; }
        pupil_x = 20;  // 向右看
        break;
    case EXPR_LOOK_RIGHT:  // v6.1: 倾斜右→眼睛看左
        if (blinking) { base_lh = 1; base_rh = 1; }
        pupil_x = -20;  // 向左看
        break;
    case EXPR_LOOK_AROUND:
        if (blinking) { base_lh = 1; base_rh = 1; }
        pupil_x = anim_pupil_x * 3;
        break;
    case EXPR_YAWN:
        base_lh = base_rh = 20; ly_off = ry_off = 8;
        break;
    case EXPR_CURIOUS:
        // v6.1: 左眼36x32,x:-10,y:-10,rot:10  右眼40x36,x:-10,y:-15,rot:10
        base_lw = 32; base_lh = 36; ly_off = -10;  // 左眼
        base_rw = 36; base_rh = 40; ry_off = -15;  // 右眼更大
        pupil_x = -16;  // 看左上
        break;
    case EXPR_ANGRY:
        base_lh = base_rh = 20; base_lw = base_rw = 30;
        ly_off = ry_off = 8;
        break;
    case EXPR_EXCITED:
        if (blinking) { base_lh = 1; base_rh = 1; }
        base_lh = base_rh = 45; base_lw = base_rw = 36;
        break;
    case EXPR_SAD:
        base_lh = base_rh = 16; ly_off = ry_off = 13;
        break;
    case EXPR_CELEBRATE:
        if (blinking) { base_lh = 1; base_rh = 1; }
        ly_off = ry_off = (anim_wink_phase ? -10 : 0);
        break;
    case EXPR_MORNING:
        if (blinking) { base_lh = 1; base_rh = 1; }
        break;
    // v6.0 新表情
    case EXPR_THINKING:
        base_lw = base_rw = 28; base_lh = 32; base_rh = 24;
        ly_off = -5; ry_off = 0;
        break;
    case EXPR_SURPRISED:
        base_lw = base_rw = 40; base_lh = base_rh = 45;
        ly_off = ry_off = -10;
        break;
    case EXPR_SLEEP_WAKE:
        base_lw = base_rw = 32; base_lh = base_rh = 10;
        break;
    case EXPR_LOST:
        // v6.1: height 20, width 32, borderRadius 16 16 4 4, y:10, rotate: -15/15
        base_lw = base_rw = 32; base_lh = base_rh = 20;
        ly_off = 16; ry_off = 16;
        break;
    // v7.6 不对称表情
    case EXPR_CONFUSED:
        // 左眼大右眼小, 微微斜视
        base_lw = 34; base_lh = 38; ly_off = -8;   // 左眼大
        base_rw = 26; base_rh = 28; ry_off = -4;   // 右眼小
        pupil_x = 8;  // 微微右看
        break;
    case EXPR_SUSPICIOUS:
        // 双眼半眯, 微微左看 (怀疑地看)
        base_lw = base_rw = 36; base_lh = base_rh = 18;
        ly_off = ry_off = 4;
        pupil_x = -12;  // 左看
        break;
    }

    // 呼吸调制: 眼睛 scale
    float bm = breath_mod;

    int lw = (int)(base_lw * s * breath_scale * bm);
    int lh = (int)(base_lh * s * breath_scale * bm);
    int rw = (int)(base_rw * s * breath_scale * bm);
    int rh = (int)(base_rh * s * breath_scale * bm);
    ly_off = (int)(ly_off * s * bm); ry_off = (int)(ry_off * s * bm);

    int lr = r_style, rr = r_style;
    if (expr == EXPR_MENU) lr = rr = 10;
    if (expr == EXPR_DIZZY) lr = rr = 8;
    if (expr == EXPR_CRYING) lr = rr = 4;
    if (expr == EXPR_ANGRY) lr = rr = 8;

    // 动态眼距 + 眩晕旋转偏移
    int gap = (int)(FACE_EYE_GAP * s);
    int total = lw + gap + rw;
    int lx = (320 - total) / 2 + lw / 2 + pupil_x;
    int rx = lx + lw / 2 + gap + rw / 2 + pupil_x;

    // v3.10: 眩晕"原地转圈" — 眼睛位置固定, 螺旋弧线旋转


    // 发光 (白天模式不发光, PRD 规范)
    if (!theme_get_colors()->is_light) {
        _draw_glow(layer, lx, ey + ly_off, lw, lh, lr, fg);
        _draw_glow(layer, rx, ey + ry_off, rw, rh, rr, fg);
    }

    // --- 瞳孔缩放因子 (竞品对齐: 好奇放大, 警觉缩小) ---
    float pupil_scale = 1.0f;
    switch (expr) {
    case EXPR_CURIOUS: case EXPR_SURPRISED: pupil_scale = 1.3f; break;  // 瞳孔放大
    case EXPR_EXCITED: case EXPR_HAPPY:     pupil_scale = 1.15f; break;
    case EXPR_ANGRY: case EXPR_ALERT:       pupil_scale = 0.7f; break;   // 瞳孔缩小
    case EXPR_SAD: case EXPR_LOST:          pupil_scale = 0.85f; break;
    case EXPR_DEEP_SLEEP: case EXPR_LIGHT_REST: pupil_scale = 0.6f; break;
    default: break;
    }

    // --- 左眼 ---
    if (expr == EXPR_DIZZY) {
        // v3.10: 眩晕原地转圈 — 小圆点瞳孔 + 螺旋弧线 (螺旋在 draw 末尾绘制)
        _draw_mouth_circle(layer, lx, ey + ly_off, (int)(5*s), fg, 160);
    } else {
        lv_draw_rect_dsc_t eye_dsc;
        lv_draw_rect_dsc_init(&eye_dsc);
        eye_dsc.bg_color = fg;
        eye_dsc.bg_opa = (expr == EXPR_MENU) ? 76 : (int)(breath_opa * 255);
        eye_dsc.radius = lr;
        eye_dsc.border_width = 0;

        lv_area_t la;
        la.x1 = lx - lw/2; la.y1 = ey + ly_off - lh/2;
        la.x2 = lx + lw/2; la.y2 = ey + ly_off + lh/2;

        bool l_mask = (expr == EXPR_HAPPY);
        if (l_mask) {
            lv_draw_rect(layer, &eye_dsc, &la);
            lv_draw_rect_dsc_t mdsc;
            lv_draw_rect_dsc_init(&mdsc);
            mdsc.bg_color = bg; mdsc.bg_opa = LV_OPA_COVER;
            mdsc.radius = 0; mdsc.border_width = 0;
            lv_area_t ma;
            int pad = (int)(4 * s);
            ma.x1 = la.x1 - pad; ma.y1 = ey + ly_off;
            ma.x2 = la.x2 + pad; ma.y2 = la.y2 + pad;
            lv_draw_rect(layer, &mdsc, &ma);
        } else {
            lv_draw_rect(layer, &eye_dsc, &la);
            // 瞳孔: 眼睛内部的深色圆点, 大小随表情变化
            if (lh > (int)(8*s) && expr != EXPR_MENU) {
                int pr = (int)(lw * 0.25f * pupil_scale);
                if (pr > lw/3) pr = lw/3;
                if (pr < 3) pr = 3;
                lv_draw_rect_dsc_t pdsc;
                lv_draw_rect_dsc_init(&pdsc);
                pdsc.bg_color = bg; pdsc.bg_opa = (int)(breath_opa * 200);
                pdsc.radius = pr; pdsc.border_width = 0;
                lv_area_t pa;
                pa.x1 = lx - pr; pa.y1 = ey + ly_off - pr;
                pa.x2 = lx + pr; pa.y2 = ey + ly_off + pr;
                lv_draw_rect(layer, &pdsc, &pa);
            }
        }
    }

    // --- 右眼 ---
    if (expr == EXPR_DIZZY) {
        _draw_mouth_circle(layer, rx, ey + ry_off, (int)(5*s), fg, 160);
    } else {
        lv_draw_rect_dsc_t eye_dsc;
        lv_draw_rect_dsc_init(&eye_dsc);
        eye_dsc.bg_color = fg;
        eye_dsc.bg_opa = (expr == EXPR_MENU) ? 76 : (int)(breath_opa * 255);
        eye_dsc.radius = rr;
        eye_dsc.border_width = 0;

        lv_area_t ra;
        ra.x1 = rx - rw/2; ra.y1 = ey + ry_off - rh/2;
        ra.x2 = rx + rw/2; ra.y2 = ey + ry_off + rh/2;

        bool r_mask = (expr == EXPR_HAPPY);
        if (r_mask) {
            lv_draw_rect(layer, &eye_dsc, &ra);
            lv_draw_rect_dsc_t mdsc;
            lv_draw_rect_dsc_init(&mdsc);
            mdsc.bg_color = bg; mdsc.bg_opa = LV_OPA_COVER;
            mdsc.radius = 0; mdsc.border_width = 0;
            lv_area_t ma;
            int pad = (int)(4 * s);
            ma.x1 = ra.x1 - pad; ma.y1 = ey + ry_off;
            ma.x2 = ra.x2 + pad; ma.y2 = ra.y2 + pad;
            lv_draw_rect(layer, &mdsc, &ma);
        } else {
            lv_draw_rect(layer, &eye_dsc, &ra);
            // 瞳孔: 右眼
            if (rh > (int)(8*s) && expr != EXPR_MENU) {
                int pr = (int)(rw * 0.25f * pupil_scale);
                if (pr > rw/3) pr = rw/3;
                if (pr < 3) pr = 3;
                lv_draw_rect_dsc_t pdsc;
                lv_draw_rect_dsc_init(&pdsc);
                pdsc.bg_color = bg; pdsc.bg_opa = (int)(breath_opa * 200);
                pdsc.radius = pr; pdsc.border_width = 0;
                lv_area_t pa;
                pa.x1 = rx - pr; pa.y1 = ey + ry_off - pr;
                pa.x2 = rx + pr; pa.y2 = ey + ry_off + pr;
                lv_draw_rect(layer, &pdsc, &pa);
            }
        }
    }

    // --- 眉毛 (竞品对齐: 角度随表情变化) ---
    {
        int brow_angle_l = 0, brow_angle_r = 0;
        switch (expr) {
        case EXPR_SURPRISED: brow_angle_l = brow_angle_r = 25; break;   // 双眉上扬
        case EXPR_ANGRY:     brow_angle_l = brow_angle_r = -20; break;  // 双眉下压
        case EXPR_CURIOUS:   brow_angle_l = 10; brow_angle_r = 18; break; // 右眉更高
        case EXPR_SAD:       brow_angle_l = brow_angle_r = -12; break;  // 微微下压
        case EXPR_THINKING:  brow_angle_l = 5; brow_angle_r = -8; break; // 一高一低
        case EXPR_NAUGHTY:   brow_angle_l = -5; brow_angle_r = 12; break; // 调皮挑眉
        case EXPR_CONFUSED:  brow_angle_l = -10; brow_angle_r = 15; break; // 反向
        case EXPR_SUSPICIOUS: brow_angle_l = brow_angle_r = -5; break;   // 微微下压
        case EXPR_EXCITED:   brow_angle_l = brow_angle_r = 15; break;
        case EXPR_LOST:      brow_angle_l = brow_angle_r = -8; break;
        default: break;
        }
        if (brow_angle_l != 0 || brow_angle_r != 0) {
            int brow_y = ey - (int)(30 * s);  // 眉毛在眼睛上方
            int brow_w = (int)(20 * s);       // 眉毛宽度
            lv_opa_t brow_opa = (int)(breath_opa * 180);
            _draw_eyebrow(layer, lx, brow_y + ly_off, brow_w, brow_angle_l, fg, brow_opa);
            _draw_eyebrow(layer, rx, brow_y + ry_off, brow_w, brow_angle_r, fg, brow_opa);
        }
    }

    // --- 嘴巴 (v3.10: lv_draw_arc 原生弧线) ---
    lv_color_t mc = fg;
    lv_opa_t m_opa = (expr == EXPR_MENU) ? 76 : LV_OPA_COVER;

    switch (expr) {
    case EXPR_IDLE:
    case EXPR_BREATH:
    case EXPR_MORNING:
    case EXPR_LOOK_LEFT:
    case EXPR_LOOK_RIGHT:
    case EXPR_LOOK_AROUND:
    case EXPR_CURIOUS: {
        // 细微笑弧: 半径~19px, 窄角度 70°-110°, 线宽 3px
        int r = (int)(12 * s);
        _draw_mouth_arc(layer, 160, my - r, r, 70, 110, (int)(2*s+0.5f), mc, m_opa);
        break;
    }
    case EXPR_DEEP_SLEEP: {
        int r = (int)(14 * s);
        _draw_mouth_arc(layer, 160, my - r, r, 78, 102, 1, mc, 60);
        break;
    }
    case EXPR_LIGHT_REST:
    case EXPR_ALERT: {
        int r = (int)(12 * s);
        _draw_mouth_arc(layer, 160, my - r, r, 68, 112, (int)(2*s+0.5f), mc, m_opa);
        break;
    }
    case EXPR_HAPPY:
    case EXPR_EXCITED:
    case EXPR_CELEBRATE: {
        // 大笑弧: 半径~35px, 宽角度 20°-160°, 粗线 13px
        int r = (int)(22 * s);
        int w = (int)(8 * s);
        _draw_mouth_arc(layer, 160, my - r, r, 20, 160, w, mc, m_opa);
        // 弧线柔光
        _draw_mouth_arc(layer, 160, my - r, r, 20, 160, w + 6, mc, 30);
        break;
    }
    case EXPR_TALKING: {
        // 说话竖椭圆: lv_draw_rect 圆角矩形更适合椭圆
        int hw = (int)(10 * s), hh = (int)(12 * s);
        lv_draw_rect_dsc_t dsc;
        lv_draw_rect_dsc_init(&dsc);
        dsc.bg_color = mc; dsc.bg_opa = m_opa;
        dsc.radius = hw; dsc.border_width = 0;
        lv_area_t a;
        a.x1 = 160 - hw; a.y1 = my - hh; a.x2 = 160 + hw; a.y2 = my + hh;
        lv_draw_rect(layer, &dsc, &a);
        break;
    }
    case EXPR_DIZZY: {
        // 小圆嘴 (粗弧线实心圆)
        int r = (int)(8 * s);
        _draw_mouth_circle(layer, 160, my + (int)(10*s), r, mc, m_opa);
        break;
    }
    case EXPR_CRYING:
    case EXPR_SAD:
    case EXPR_LOST: {
        // 倒弧 frown: 中心在嘴巴下方, 弧线通过顶部
        int r = (int)(18 * s);
        int w = (int)(4 * s);
        int cy_off = (expr == EXPR_CRYING) ? (int)(12*s) : (expr == EXPR_SAD) ? (int)(10*s) : (int)(8*s);
        _draw_mouth_arc(layer, 160, my + cy_off + r, r, 210, 330, w, mc, m_opa);
        break;
    }
    case EXPR_NAUGHTY: {
        // 歪嘴笑: 偏移中心, 弧线偏右
        int r = (int)(24 * s);
        _draw_mouth_arc(layer, 160 + (int)(6*s), my - r + (int)(5*s), r, 30, 140, (int)(6*s), mc, m_opa);
        break;
    }
    case EXPR_WINK: {
        int r = (int)(24 * s);
        int span = anim_wink_phase ? 130 : 70;
        int cy = anim_wink_phase ? my - r - (int)(10*s) : my - r;
        int cx = 160 + (anim_wink_phase ? (int)(10*s) : (int)(4*s));
        _draw_mouth_arc(layer, cx, cy, r, 30, 30 + span, (int)(6*s), mc, m_opa);
        break;
    }
    case EXPR_YAWN: {
        // 大圆嘴 (打哈欠)
        int r = (int)(14 * s);
        _draw_mouth_circle(layer, 160, my + (int)(10*s), r, mc, m_opa);
        break;
    }
    case EXPR_ANGRY: {
        // 直线嘴: 大半径极平弧 (近乎直线)
        int r = (int)(40 * s);
        _draw_mouth_arc(layer, 160, my + (int)(15*s) - r, r, 85, 95, (int)(2*s), mc, m_opa);
        break;
    }
    case EXPR_THINKING: {
        // 小圆嘴偏移
        int r = (int)(6 * s);
        _draw_mouth_circle(layer, 150, my + (int)(5*s), r, mc, m_opa);
        break;
    }
    case EXPR_SURPRISED: {
        // 惊讶 O 型嘴
        int r = (int)(12 * s);
        _draw_mouth_circle(layer, 160, my + (int)(15*s), r, mc, m_opa);
        break;
    }
    case EXPR_SLEEP_WAKE: {
        int r = (int)(14 * s);
        _draw_mouth_arc(layer, 160, my - r, r, 78, 102, 1, mc, m_opa);
        break;
    }
    case EXPR_MENU: {
        int r = (int)(18 * s);
        _draw_mouth_arc(layer, 160, my - (int)(20*s) - r, r, 70, 110, (int)(2*s), mc, m_opa);
        break;
    }
    case EXPR_CONFUSED: {
        // 歪嘴微笑 (不对称)
        int r = (int)(16 * s);
        _draw_mouth_arc(layer, 160 + (int)(4*s), my - r + (int)(3*s), r, 50, 120, (int)(3*s), mc, m_opa);
        break;
    }
    case EXPR_SUSPICIOUS: {
        // 紧抿嘴 (直线微弯)
        int r = (int)(50 * s);
        _draw_mouth_arc(layer, 160, my + (int)(8*s) - r, r, 82, 98, (int)(2*s), mc, m_opa);
        break;
    }
    }

    // v3.10: 眩晕螺旋眼 (原地绘制, 替代矩形眼睛)
    if (expr == EXPR_DIZZY && anim_dizzy_rot != 0) {
        _draw_spiral_eye(layer, lx, ey + ly_off, anim_dizzy_rot, fg);
        _draw_spiral_eye(layer, rx, ey + ry_off, anim_dizzy_rot + 600, fg);
    }
}

// ========== 泪水 ==========
static void _create_tear_objects(void)
{
    if (tear_left) return;
    float s = FACE_SCALE;
    lv_obj_t *screen = lv_screen_active();
    lv_color_t tc = (theme_v3_get_current() == THEME_TECH) ? lv_color_hex(0x22D3EE)
                  : (theme_v3_get_current() == THEME_COCOA) ? lv_color_hex(0x22C55E)
                  : lv_color_hex(0x60A5FA);
    int tw = (int)(10 * s + 0.5f), th = (int)(20 * s + 0.5f);
    tear_left = lv_obj_create(screen);
    lv_obj_set_size(tear_left, tw, th);
    lv_obj_set_style_radius(tear_left, tw/2, 0);
    lv_obj_set_style_bg_color(tear_left, tc, 0);
    lv_obj_set_style_bg_opa(tear_left, LV_OPA_60, 0);
    lv_obj_set_style_border_width(tear_left, 0, 0);
    lv_obj_add_flag(tear_left, LV_OBJ_FLAG_HIDDEN);
    tear_right = lv_obj_create(screen);
    lv_obj_set_size(tear_right, tw, th);
    lv_obj_set_style_radius(tear_right, tw/2, 0);
    lv_obj_set_style_bg_color(tear_right, tc, 0);
    lv_obj_set_style_bg_opa(tear_right, LV_OPA_60, 0);
    lv_obj_set_style_border_width(tear_right, 0, 0);
    lv_obj_add_flag(tear_right, LV_OBJ_FLAG_HIDDEN);
}

static void _tear_y_cb(void *obj, int32_t val)
{
    lv_obj_set_y((lv_obj_t *)obj, val);
    if (val > EYE_Y + (int)(25 * FACE_SCALE))
        lv_obj_set_style_opa((lv_obj_t *)obj, LV_OPA_0, 0);
    else lv_obj_set_style_opa((lv_obj_t *)obj, LV_OPA_60, 0);
}

static void _start_tears(void)
{
    _create_tear_objects();
    float s = FACE_SCALE;
    int lw = (int)(32 * s), rw = (int)(32 * s);
    int gap = (int)(FACE_EYE_GAP * s);
    int lx = (320 - lw - gap - rw) / 2 + lw / 2;
    int rx = lx + lw / 2 + gap + rw / 2;
    int tw = (int)(5 * s), sy = EYE_Y + (int)(4 * s), drop = (int)(30 * s);
    lv_obj_set_pos(tear_left, lx - tw, sy);
    lv_obj_clear_flag(tear_left, LV_OBJ_FLAG_HIDDEN);
    lv_anim_init(&tear_anim_l);
    lv_anim_set_var(&tear_anim_l, tear_left);
    lv_anim_set_exec_cb(&tear_anim_l, _tear_y_cb);
    lv_anim_set_values(&tear_anim_l, sy, sy + drop);
    lv_anim_set_time(&tear_anim_l, 1200);
    lv_anim_set_repeat_count(&tear_anim_l, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_path_cb(&tear_anim_l, lv_anim_path_ease_in);
    lv_anim_start(&tear_anim_l);
    lv_obj_set_pos(tear_right, rx - tw, sy);
    lv_obj_clear_flag(tear_right, LV_OBJ_FLAG_HIDDEN);
    lv_anim_init(&tear_anim_r);
    lv_anim_set_var(&tear_anim_r, tear_right);
    lv_anim_set_exec_cb(&tear_anim_r, _tear_y_cb);
    lv_anim_set_values(&tear_anim_r, sy, sy + drop);
    lv_anim_set_time(&tear_anim_r, 1200);
    lv_anim_set_delay(&tear_anim_r, 600);
    lv_anim_set_repeat_count(&tear_anim_r, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_path_cb(&tear_anim_r, lv_anim_path_ease_in);
    lv_anim_start(&tear_anim_r);
}

static void _stop_tears(void)
{
    if (tear_left) {
        lv_anim_delete(tear_left, _tear_y_cb);
        lv_obj_add_flag(tear_left, LV_OBJ_FLAG_HIDDEN);
    }
    if (tear_right) {
        lv_anim_delete(tear_right, _tear_y_cb);
        lv_obj_add_flag(tear_right, LV_OBJ_FLAG_HIDDEN);
    }
}

// ========== 动画 ==========
static void _face_anim_cb(void *obj, int32_t v)
{
    anim_dizzy_rot = v;
    anim_wink_phase = v;
    lv_obj_invalidate(face_container);
}

// 竞品对齐: 主动探索 — 高能量时偶尔大范围扫视
static int s_scan_amplitude = 5;  // 当前扫视幅度 (5=微移, 15=扫视)

static void _pupil_anim_cb(void *obj, int32_t v)
{
    float rad = v * M_PI / 180.0f;

    // 高能量 + idle 时, 偶尔切换到大范围扫视
    if (current_expr == EXPR_IDLE && s_energy > 50) {
        // 每个动画周期(8s)有 20% 概率进入扫视模式
        if (v > 340 && v < 350 && (esp_random() % 100) < 20) {
            s_scan_amplitude = 12 + (esp_random() % 8);  // 12-20px
        }
        // 扫视周期结束后恢复
        if (v < 20) s_scan_amplitude = 5;
    } else {
        s_scan_amplitude = 5;
    }

    anim_pupil_x = (int32_t)(sinf(rad * 3.7f) * (float)s_scan_amplitude);
    anim_pupil_y = (int32_t)(cosf(rad * 2.3f) * 3.0f);
    if (++_anim_tick % 6 == 0) lv_obj_invalidate(face_container);
}

static void _breath_anim_cb(void *obj, int32_t v)
{
    anim_breath_val = v;
    if (++_anim_tick % 6 == 0) lv_obj_invalidate(face_container);
}

// 眩晕减速: 从全速逐渐降到 0
static void _dizzy_decel_cb(void *obj, int32_t v)
{
    anim_dizzy_rot = v;
    lv_obj_invalidate(face_container);
}

static void _start_expression_anims(void)
{
    lv_anim_delete(face_container, _face_anim_cb);
    lv_anim_delete(face_container, _pupil_anim_cb);
    lv_anim_delete(face_container, _breath_anim_cb);
    lv_anim_delete(face_container, _dizzy_decel_cb);

    // 如果不是 dizzy, 且之前是 dizzy, 做减速动画
    if (current_expr != EXPR_DIZZY && anim_dizzy_rot > 0) {
        lv_anim_init(&dizzy_decel_anim);
        lv_anim_set_var(&dizzy_decel_anim, face_container);
        lv_anim_set_exec_cb(&dizzy_decel_anim, _dizzy_decel_cb);
        lv_anim_set_values(&dizzy_decel_anim, anim_dizzy_rot % 3600, 0);
        lv_anim_set_time(&dizzy_decel_anim, 800);
        lv_anim_set_path_cb(&dizzy_decel_anim, lv_anim_path_ease_out);
        lv_anim_start(&dizzy_decel_anim);
    } else if (current_expr != EXPR_DIZZY) {
        anim_dizzy_rot = 0;
    }

    // 瞳孔微动: 大部分状态都启用
    bool pupil_active = true;
    bool breath_active = true;
    int pupil_dur = 8000;  // idle: 8s 周期
    int breath_dur = 4000; // idle: 4s 周期

    switch (current_expr) {
    case EXPR_DIZZY:
        pupil_active = false; breath_active = false;
        lv_anim_init(&face_anim);
        lv_anim_set_var(&face_anim, face_container);
        lv_anim_set_exec_cb(&face_anim, _face_anim_cb);
        lv_anim_set_values(&face_anim, 0, 3600);
        lv_anim_set_time(&face_anim, 1000);
        lv_anim_set_repeat_count(&face_anim, LV_ANIM_REPEAT_INFINITE);
        lv_anim_set_path_cb(&face_anim, lv_anim_path_linear);
        lv_anim_start(&face_anim);
        break;
    case EXPR_TALKING:
        pupil_dur = 4000; breath_dur = 2000;
        lv_anim_init(&face_anim);
        lv_anim_set_var(&face_anim, face_container);
        lv_anim_set_exec_cb(&face_anim, _face_anim_cb);
        lv_anim_set_values(&face_anim, 0, 1);
        lv_anim_set_time(&face_anim, 100);
        lv_anim_set_playback_time(&face_anim, 100);
        lv_anim_set_repeat_count(&face_anim, LV_ANIM_REPEAT_INFINITE);
        lv_anim_start(&face_anim);
        break;
    case EXPR_WINK:
        pupil_active = false;
        lv_anim_init(&face_anim);
        lv_anim_set_var(&face_anim, face_container);
        lv_anim_set_exec_cb(&face_anim, _face_anim_cb);
        lv_anim_set_values(&face_anim, 0, 1);
        lv_anim_set_time(&face_anim, 375);
        lv_anim_set_playback_time(&face_anim, 375);
        lv_anim_set_repeat_count(&face_anim, 1);
        lv_anim_set_path_cb(&face_anim, lv_anim_path_ease_in_out);
        lv_anim_start(&face_anim);
        break;
    case EXPR_CRYING:
        pupil_dur = 4000; breath_dur = 3000;
        _start_tears();
        break;
    case EXPR_CELEBRATE:
        pupil_dur = 1000; breath_dur = 500;
        lv_anim_init(&face_anim);
        lv_anim_set_var(&face_anim, face_container);
        lv_anim_set_exec_cb(&face_anim, _face_anim_cb);
        lv_anim_set_values(&face_anim, 0, 1);
        lv_anim_set_time(&face_anim, 500);
        lv_anim_set_playback_time(&face_anim, 500);
        lv_anim_set_repeat_count(&face_anim, LV_ANIM_REPEAT_INFINITE);
        lv_anim_set_path_cb(&face_anim, lv_anim_path_ease_in_out);
        lv_anim_start(&face_anim);
        break;
    case EXPR_ALERT:
        pupil_dur = 2500; breath_dur = 2500;
        break;
    case EXPR_EXCITED:
        pupil_dur = 1500; breath_dur = 1500;
        break;
    case EXPR_DEEP_SLEEP:
        pupil_dur = 10000; breath_dur = 8000;
        break;
    case EXPR_LIGHT_REST:
        pupil_dur = 8000; breath_dur = 6000;
        break;
    case EXPR_LOOK_AROUND:
        pupil_dur = 3000;  // faster eye movement
        break;
    case EXPR_SLEEP_WAKE:
        pupil_active = false; breath_active = false;
        break;
    case EXPR_THINKING:
        pupil_dur = 6000; breath_dur = 3000;
        break;
    case EXPR_BREATH:
    case EXPR_IDLE:
        pupil_dur = 8000; breath_dur = 4000;
        break;
    default:
        _stop_tears();
        break;
    }

    // 瞳孔微动 (v5.0 pupilVariants: 自然漂移轨迹)
    if (pupil_active) {
        lv_anim_init(&pupil_anim);
        lv_anim_set_var(&pupil_anim, face_container);
        lv_anim_set_exec_cb(&pupil_anim, _pupil_anim_cb);
        lv_anim_set_values(&pupil_anim, 0, 360);
        lv_anim_set_time(&pupil_anim, pupil_dur);
        lv_anim_set_repeat_count(&pupil_anim, LV_ANIM_REPEAT_INFINITE);
        lv_anim_set_path_cb(&pupil_anim, lv_anim_path_linear);
        lv_anim_start(&pupil_anim);
    }

    // 呼吸脉冲 (v5.0 breathing: scale + opacity 同步)
    if (breath_active) {
        lv_anim_init(&breath_anim);
        lv_anim_set_var(&breath_anim, face_container);
        lv_anim_set_exec_cb(&breath_anim, _breath_anim_cb);
        lv_anim_set_values(&breath_anim, 0, 255);
        lv_anim_set_time(&breath_anim, breath_dur);
        lv_anim_set_playback_time(&breath_anim, breath_dur);
        lv_anim_set_repeat_count(&breath_anim, LV_ANIM_REPEAT_INFINITE);
        lv_anim_start(&breath_anim);
    }
}

static void _render_expression(Expression expr)
{
    current_expr = expr;
    lv_obj_invalidate(face_container);
    _start_expression_anims();
}

// ========== 定时器 ==========
// 竞品对齐: 不规则眨眼 (偶尔连续眨两次, 偶尔长时间不眨)
static int s_blink_count = 0;  // 当前连续眨眼次数

static void _blink_callback(TimerHandle_t timer)
{
    if (blinking) {
        // 眼睛正在闭合 → 打开
        pending_blink_state = false; pending_blink = true; blinking = false;
        s_blink_count++;

        // 连续眨两次: 15% 概率
        if (s_blink_count < 2 && (esp_random() % 100) < 15) {
            // 短暂间隔后再次眨眼
            xTimerChangePeriod(blink_timer, pdMS_TO_TICKS(80), 0);
        } else {
            s_blink_count = 0;
            // 正常间隔: 2-7s (偶尔长时间不眨)
            uint32_t r = esp_random() % 100;
            uint32_t next;
            if (r < 10)      next = 6000 + (esp_random() % 2000);  // 10% 长不眨 6-8s
            else if (r < 25) next = 4500 + (esp_random() % 1500);  // 15% 较长 4.5-6s
            else              next = 2000 + (esp_random() % 2000);  // 75% 正常 2-4s
            xTimerChangePeriod(blink_timer, pdMS_TO_TICKS(next), 0);
        }
    } else {
        // 眼睛睁开 → 闭合
        pending_blink_state = true; pending_blink = true; blinking = true;
        xTimerChangePeriod(blink_timer, pdMS_TO_TICKS(120), 0);  // 闭合时长 120ms
    }
    xTimerStart(blink_timer, 0);
}

// 情绪驱动的表情选择 (替代纯随机)
static Expression _emotion_pick_expression(void)
{
    // 根据 energy 和 mood 选择表情池
    // energy: 精力高→活跃表情, 精力低→疲倦表情
    // mood:   心情好→积极表情, 心情差→消极表情
    static const Expression pool_high_energy[] = {
        EXPR_HAPPY, EXPR_EXCITED, EXPR_CELEBRATE, EXPR_CURIOUS,
        EXPR_WINK, EXPR_NAUGHTY, EXPR_SURPRISED, EXPR_LOOK_LEFT, EXPR_LOOK_RIGHT
    };
    static const Expression pool_mid_energy[] = {
        EXPR_IDLE, EXPR_THINKING, EXPR_WINK, EXPR_LOOK_LEFT,
        EXPR_LOOK_RIGHT, EXPR_CURIOUS, EXPR_CONFUSED, EXPR_SUSPICIOUS, EXPR_IDLE, EXPR_IDLE
    };
    static const Expression pool_low_energy[] = {
        EXPR_YAWN, EXPR_SAD, EXPR_DEEP_SLEEP, EXPR_LIGHT_REST,
        EXPR_LOST, EXPR_IDLE, EXPR_IDLE, EXPR_IDLE  // 大量 idle
    };
    static const Expression pool_good_mood[] = {
        EXPR_HAPPY, EXPR_CELEBRATE, EXPR_EXCITED, EXPR_WINK, EXPR_NAUGHTY
    };
    static const Expression pool_bad_mood[] = {
        EXPR_ANGRY, EXPR_SAD, EXPR_LOST, EXPR_CRYING
    };

    const Expression *pool;
    int pool_size;

    // 心情极端时优先用心情池
    if (s_mood > 50) {
        pool = pool_good_mood; pool_size = sizeof(pool_good_mood)/sizeof(pool_good_mood[0]);
    } else if (s_mood < -30) {
        pool = pool_bad_mood; pool_size = sizeof(pool_bad_mood)/sizeof(pool_bad_mood[0]);
    } else if (s_energy > 70) {
        pool = pool_high_energy; pool_size = sizeof(pool_high_energy)/sizeof(pool_high_energy[0]);
    } else if (s_energy > 35) {
        pool = pool_mid_energy; pool_size = sizeof(pool_mid_energy)/sizeof(pool_mid_energy[0]);
    } else {
        pool = pool_low_energy; pool_size = sizeof(pool_low_energy)/sizeof(pool_low_energy[0]);
    }

    return pool[esp_random() % pool_size];
}

static void _carousel_callback(TimerHandle_t timer)
{
    // 情绪衰减: 每次轮播 energy -2
    if (s_energy > 10) s_energy -= 2;
    // 心情自然回零
    if (s_mood > 0) s_mood--;
    else if (s_mood < 0) s_mood++;

    // 根据情绪选表情
    next_random_expr = (uint32_t)_emotion_pick_expression();
    pending_carousel = true;

    // 低能量时轮播更慢 (5-9s), 高能量时更快 (3-6s)
    int base = (s_energy > 60) ? 3000 : (s_energy > 30) ? 4000 : 5000;
    int range = (s_energy > 60) ? 3000 : (s_energy > 30) ? 3000 : 4000;
    uint32_t next = base + (esp_random() % range);
    xTimerChangePeriod(carousel_timer, pdMS_TO_TICKS(next), 0);
}

// ========== 公开 API ==========
void expressions_init(void)
{
    face_container = lv_obj_create(lv_screen_active());
    lv_obj_set_size(face_container, 320, 192);
    lv_obj_set_pos(face_container, 0, 0);  // 留底部 48px, 避免覆盖全屏
    lv_obj_set_style_bg_opa(face_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(face_container, 0, 0);
    lv_obj_set_style_pad_all(face_container, 0, 0);
    lv_obj_remove_flag(face_container, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(face_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(face_container, _face_draw_cb, LV_EVENT_DRAW_POST, NULL);

    blink_timer = xTimerCreate("blink", pdMS_TO_TICKS(4000), pdTRUE, NULL, _blink_callback);
    carousel_timer = xTimerCreate("carousel", pdMS_TO_TICKS(4000), pdTRUE, NULL, _carousel_callback);
    _render_expression(EXPR_IDLE);
    uint32_t first = 3000 + (esp_random() % 2000);
    xTimerChangePeriod(blink_timer, pdMS_TO_TICKS(first), 0);
    xTimerStart(blink_timer, 0);
    ESP_LOGI(TAG, "v5.0 表情引擎 (scale=1.6) 就绪");
}

void expression_set(Expression expr, bool animate)
{
    if (expr == current_expr) return;
    _stop_tears();
    _render_expression(expr);

    // 声音反馈 (竞品对齐: 表情切换时的音效)
    switch (expr) {
    case EXPR_HAPPY: case EXPR_CELEBRATE: case EXPR_EXCITED:
        audio_play(AUDIO_EXPR_HAPPY); break;
    case EXPR_SURPRISED: case EXPR_CURIOUS:
        audio_play(AUDIO_EXPR_SURPRISED); break;
    case EXPR_SAD: case EXPR_CRYING: case EXPR_LOST:
        audio_play(AUDIO_EXPR_SAD); break;
    case EXPR_DEEP_SLEEP: case EXPR_LIGHT_REST: case EXPR_YAWN:
        audio_play(AUDIO_EXPR_SLEEP); break;
    case EXPR_DIZZY:
        audio_play(AUDIO_EXPR_DIZZY); break;
    default: break;
    }
}

Expression expression_get_current(void) { return current_expr; }

void expression_start_carousel(void)
{
    uint32_t delay = 5000 + (esp_random() % 3000);
    xTimerChangePeriod(carousel_timer, pdMS_TO_TICKS(delay), 0);
    xTimerStart(carousel_timer, 0);
}

void expression_refresh_theme(void) { lv_obj_invalidate(face_container); }

void expression_process_pending(void)
{
    if (pending_blink) {
        pending_blink = false; blinking = pending_blink_state;
        lv_obj_invalidate(face_container);
    }
    if (pending_carousel) {
        pending_carousel = false;
        expression_set((Expression)next_random_expr, true);
    }
}

void expression_set_drawing_enabled(bool enabled)
{
    if (face_drawing_enabled == enabled) return;
    face_drawing_enabled = enabled;
    lv_obj_invalidate(face_container);
}

// ========== 情绪系统公开 API ==========
void expression_notify_touch(void)
{
    s_energy = (s_energy + 15 > 100) ? 100 : s_energy + 15;
    s_mood = (s_mood + 10 > 100) ? 100 : s_mood + 10;
    s_last_interaction_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
}

void expression_notify_chat(void)
{
    s_energy = (s_energy + 20 > 100) ? 100 : s_energy + 20;
    s_mood = (s_mood + 15 > 100) ? 100 : s_mood + 15;
    s_last_interaction_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
}

void expression_notify_shake(void)
{
    s_mood = (s_mood - 20 < -100) ? -100 : s_mood - 20;
    s_last_interaction_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
}

void expression_notify_good_event(void)
{
    s_mood = (s_mood + 25 > 100) ? 100 : s_mood + 25;
    s_energy = (s_energy + 10 > 100) ? 100 : s_energy + 10;
}

int expression_get_energy(void) { return s_energy; }
int expression_get_mood(void) { return s_mood; }

// IMU 眼神方向: 从主循环调用, 传入 pitch/roll 角度
void expression_update_tilt(float pitch_deg, float roll_deg)
{
    // pitch (前倾/后仰) → 瞳孔 Y 偏移 (前倾看下方, 后仰看上方)
    // roll  (左倾/右倾) → 瞳孔 X 偏移 (左倾看左, 右倾看右)
    // 限制范围, 小角度不响应 (死区 ±5°)
    int px = 0, py = 0;
    if (fabsf(roll_deg) > 5.0f) {
        px = (int)(roll_deg * 0.8f);  // 1° → 0.8px
        if (px > 18) px = 18;
        if (px < -18) px = -18;
    }
    if (fabsf(pitch_deg) > 5.0f) {
        py = (int)(pitch_deg * 0.5f);
        if (py > 10) py = 10;
        if (py < -10) py = -10;
    }
    s_tilt_pupil_x = px;
    s_tilt_pupil_y = py;
    // 持续移动时每帧 invalidate
    if (px != 0 || py != 0) lv_obj_invalidate(face_container);
}
