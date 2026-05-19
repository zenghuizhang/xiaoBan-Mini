/*
 * V3.10 — 纯 LVGL 原生绘制
 *
 * 眼睛: lv_draw_rect 画竖椭圆
 * 嘴巴: lv_draw_arc 画弧线
 * 发光: lv_draw_blur 柔光
 * 眩晕: lv_draw_arc 螺旋
 *
 * 全部在面部容器 lv_obj_t 的 LV_EVENT_DRAW_POST 中绘制
 */

#include "expressions.h"
#include "theme_v3.h"
#include <esp_log.h>
#include <esp_random.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/timers.h>

static const char *TAG = "EXPRESSIONS";

static Expression current_expr = EXPR_IDLE;
static TimerHandle_t blink_timer = NULL;
static TimerHandle_t carousel_timer = NULL;
static volatile bool blinking = false;

// 面部容器: 独立 lv_obj, 在 StatusBar 下方, BottomBar 上方
// DRAW_POST 在此容器上 → 渲染层在 BottomBar 之下 (z-order)
// 移除 CLICKABLE → 触屏事件穿透到 screen
static lv_obj_t *face_container = NULL;

// 泪水对象 (crying 用独立 lv_obj)
static lv_obj_t *tear_left = NULL;
static lv_obj_t *tear_right = NULL;
static lv_anim_t tear_anim_l;
static lv_anim_t tear_anim_r;

// 轮播 + 眨眼标记
static volatile bool pending_carousel = false;
static volatile uint32_t next_random_expr = EXPR_IDLE;
static volatile bool pending_blink = false;
static volatile bool pending_blink_state = false;

// 动画参数
static int32_t anim_dizzy_rot = 0;     // 眩晕旋转角度
static int32_t anim_wink_phase = 0;    // 眨眼相位 (0=睁, 1=眯)
static lv_anim_t face_anim;

// 面部绘制开关（Menu 打开时关闭，避免绘制在浮层上方）
static bool face_drawing_enabled = true;

// ========== 前向声明 ==========
static void _face_draw_cb(lv_event_t *e);

static lv_color_t _eye_color(bool is_left)
{
    // Face.tsx: child 所有面部元素统一用 orange-400，成人用 cyan-400
    return (theme_v3_get_current() == THEME_ADULT) ? ADULT_FG : CHILD_FG;
}

static lv_color_t _mouth_color(void)
{
    return (theme_v3_get_current() == THEME_ADULT) ? ADULT_FG : CHILD_FG;
}

// Face.tsx: const baseScale = 1.4 (两套主题同一缩放)
static float _face_scale(void)
{
    return FACE_BASE_SCALE;
}

// 眼睛圆角: 两套主题统一使用 Face.tsx 精确值
static int _eye_radius(int base_r, int w, int h)
{
    return base_r;
}

static void _draw_glow(lv_layer_t *layer, int cx, int cy, int w, int h, int r, lv_color_t color)
{
    // 画一个比原图大的半透明椭圆作为柔光底层
    lv_draw_rect_dsc_t dsc;
    lv_draw_rect_dsc_init(&dsc);
    dsc.bg_color = color;
    dsc.bg_opa = 40;
    dsc.radius = r + 6;
    dsc.border_width = 0;

    lv_area_t a;
    a.x1 = cx - w/2 - 6;
    a.y1 = cy - h/2 - 6;
    a.x2 = cx + w/2 + 6;
    a.y2 = cy + h/2 + 6;
    lv_draw_rect(layer, &dsc, &a);
}

// ========== 核心绘制回调 ==========
static void _face_draw_cb(lv_event_t *e)
{
    lv_layer_t *layer = lv_event_get_layer(e);
    if (!layer) return;

    // Menu 打开时跳过面部绘制，避免覆盖在浮层上方
    if (!face_drawing_enabled) return;

    Expression expr = current_expr;
    bool is_adult = (theme_v3_get_current() == THEME_ADULT);

    // Face.tsx: const baseScale = 1.4; const scale = isChild ? baseScale * 1.1 : baseScale
    float s = _face_scale();

    // --- Y 坐标 (容器 y=18, 所以减去 18) ---
    int ey = EYE_Y - 18;
    int mx = MOUTH_X;
    int my = MOUTH_Y - 18;

    // --- 眼睛参数 (Face.tsx 原始值，下面乘以 scale) ---
    int base_lw = EYE_W_IDLE, base_lh = EYE_H_IDLE;
    int base_rw = EYE_W_IDLE, base_rh = EYE_H_IDLE;
    int base_lr = EYE_R_IDLE, base_rr = EYE_R_IDLE;
    int ly_off = 0, ry_off = 0;

    switch (expr) {
    case EXPR_IDLE:
    case EXPR_TALKING:
        if (blinking) { base_lh = 4; base_rh = 4; }
        break;
    case EXPR_HAPPY:
        base_lw = base_rw = EYE_W_HAPPY;
        base_lh = base_rh = EYE_H_HAPPY;
        ly_off = ry_off = -10;
        break;
    case EXPR_NAUGHTY:
        base_lh = 2;  // 左眼眯缝 (极细, 调皮效果)
        break;
    case EXPR_WINK:
        // 左眼: 眨眼脉冲 (anim_wink_phase: 0→1→0 = 睁→眯→睁)
        base_lw = anim_wink_phase ? 34 : 32;
        base_lh = anim_wink_phase ? 1 : 4;
        ly_off = anim_wink_phase ? 2 : 0;
        // 右眼弯月: 同步脉冲 (Face.tsx: height [12,16,12], y [-10,-15,-10])
        base_rw = anim_wink_phase ? 38 : 36;
        base_rh = anim_wink_phase ? 14 : 12;
        ry_off = anim_wink_phase ? -13 : -10;
        break;
    case EXPR_DIZZY:
        base_lw = base_rw = 32; base_lh = base_rh = 32;
        base_lr = base_rr = 8;
        break;
    case EXPR_MENU:
        base_lw = base_rw = 20; base_lh = base_rh = 20;
        base_lr = base_rr = 10;
        ly_off = ry_off = -20;
        break;
    case EXPR_CRYING:
        base_lw = base_rw = 32; base_lh = base_rh = 8;
        base_lr = base_rr = 4;
        ly_off = ry_off = 5;
        break;
    }

    // 应用 Face.tsx baseScale=1.4 缩放
    int lw = (int)(base_lw * s), lh = (int)(base_lh * s);
    int rw = (int)(base_rw * s), rh = (int)(base_rh * s);
    ly_off = (int)(ly_off * s); ry_off = (int)(ry_off * s);

    // 半径: 成人精确值（不缩放），儿童 50% 完全圆角
    int lr = _eye_radius(base_lr, lw, lh);
    int rr = _eye_radius(base_rr, rw, rh);

    // 动态眼距: gap-14=56px * scale (居中计算)
    int gap = (int)(FACE_EYE_GAP * s);
    int total = lw + gap + rw;
    int lx = (320 - total) / 2 + lw / 2;
    int rx = lx + lw / 2 + gap + rw / 2;

    lv_color_t lc = _eye_color(true), rc = _eye_color(false);

    // --- 发光底层 ---
    _draw_glow(layer, lx, ey + ly_off, lw, lh, lr, lc);
    _draw_glow(layer, rx, ey + ry_off, rw, rh, rr, rc);

    // --- 左眼 ---
    lv_draw_rect_dsc_t eye_dsc;
    lv_draw_rect_dsc_init(&eye_dsc);
    eye_dsc.bg_color = lc;
    eye_dsc.bg_opa = (expr == EXPR_MENU) ? 76 : LV_OPA_COVER;
    eye_dsc.radius = lr;
    eye_dsc.border_width = 0;

    lv_area_t la;
    la.x1 = lx - lw/2; la.y1 = ey + ly_off - lh/2;
    la.x2 = lx + lw/2; la.y2 = ey + ly_off + lh/2;

    // 开心眼/弯月眼: 画完整椭圆，用背景色遮掉下半部分
    bool needs_mask = (expr == EXPR_HAPPY) || (expr == EXPR_WINK && ly_off < 0);
    if (needs_mask) {
        lv_draw_rect(layer, &eye_dsc, &la);
        lv_draw_rect_dsc_t mask_dsc;
        lv_draw_rect_dsc_init(&mask_dsc);
        mask_dsc.bg_color = (is_adult) ? ADULT_BG : CHILD_BG;
        mask_dsc.bg_opa = LV_OPA_COVER;
        mask_dsc.radius = 0;
        mask_dsc.border_width = 0;
        lv_area_t ma;
        int pad = (int)(4 * s);
        ma.x1 = la.x1 - pad; ma.y1 = ey + ly_off;
        ma.x2 = la.x2 + pad; ma.y2 = la.y2 + pad;
        lv_draw_rect(layer, &mask_dsc, &ma);
    } else {
        lv_draw_rect(layer, &eye_dsc, &la);
    }

    // --- 右眼 ---
    eye_dsc.bg_color = rc;
    eye_dsc.radius = rr;

    lv_area_t ra;
    ra.x1 = rx - rw/2; ra.y1 = ey + ry_off - rh/2;
    ra.x2 = rx + rw/2; ra.y2 = ey + ry_off + rh/2;

    bool r_needs_mask = (expr == EXPR_HAPPY) || (expr == EXPR_WINK);
    if (r_needs_mask) {
        lv_draw_rect(layer, &eye_dsc, &ra);
        lv_draw_rect_dsc_t mask_dsc;
        lv_draw_rect_dsc_init(&mask_dsc);
        mask_dsc.bg_color = (is_adult) ? ADULT_BG : CHILD_BG;
        mask_dsc.bg_opa = LV_OPA_COVER;
        mask_dsc.radius = 0;
        mask_dsc.border_width = 0;
        lv_area_t ma;
        int pad = (int)(4 * s);
        ma.x1 = ra.x1 - pad; ma.y1 = ey + ry_off;
        ma.x2 = ra.x2 + pad; ma.y2 = ra.y2 + pad;
        lv_draw_rect(layer, &mask_dsc, &ma);
    } else {
        lv_draw_rect(layer, &eye_dsc, &ra);
    }

    // --- 嘴巴 (lv_draw_rect 填充, 对齐 Face.tsx borderRadius) ---
    lv_color_t mc = _mouth_color();
    lv_opa_t m_opa = (expr == EXPR_MENU) ? 76 : LV_OPA_COVER;
    lv_color_t bg_c = (is_adult) ? ADULT_BG : CHILD_BG;

    // 辅助: 画半圆嘴巴 (下半圆 smile 或上半圆 frown)
    // mask_top=true → 遮上半部，留下半圆 (底线圆角, happy/naughty/wink)
    // mask_top=false → 遮下半部，留上半圆 (顶线圆角, crying)
    auto _draw_mouth_half = [&](int cw, int ch, int cr, int cy_off, int cx_off, bool mask_top) {
        int hw = (int)(cw * s) / 2;
        int hh = (int)(ch * s);
        int r = (int)(cr * s);
        int y0 = my + (int)(cy_off * s);
        int x0 = mx + (int)(cx_off * s);

        lv_draw_rect_dsc_t dsc;
        lv_draw_rect_dsc_init(&dsc);
        dsc.bg_color = mc;
        dsc.bg_opa = m_opa;
        dsc.radius = r;
        dsc.border_width = 0;
        lv_area_t a;
        a.x1 = x0 - hw; a.y1 = y0;
        a.x2 = x0 + hw; a.y2 = y0 + hh;
        lv_draw_rect(layer, &dsc, &a);

        lv_draw_rect_dsc_t mask;
        lv_draw_rect_dsc_init(&mask);
        mask.bg_color = bg_c;
        mask.bg_opa = LV_OPA_COVER;
        mask.radius = 0;
        mask.border_width = 0;
        lv_area_t ma;
        ma.x1 = a.x1 - 4; ma.x2 = a.x2 + 4;
        if (mask_top) { ma.y1 = y0 - 4; ma.y2 = y0; }
        else          { ma.y1 = y0 + hh / 2; ma.y2 = y0 + hh + 4; }
        lv_draw_rect(layer, &mask, &ma);
    };

    switch (expr) {
    case EXPR_IDLE: {
        // Face.tsx: 24×4, borderRadius 2px → 扁平药丸
        int hw = (int)(12 * s);
        int hh = (int)(2 * s + 0.5f);
        int r = (int)(2 * s); if (r < 1) r = 1;
        lv_draw_rect_dsc_t dsc;
        lv_draw_rect_dsc_init(&dsc);
        dsc.bg_color = mc; dsc.bg_opa = m_opa;
        dsc.radius = r; dsc.border_width = 0;
        lv_area_t a;
        a.x1 = mx - hw; a.y1 = my - hh;
        a.x2 = mx + hw; a.y2 = my + hh;
        lv_draw_rect(layer, &dsc, &a);
        break;
    }
    case EXPR_HAPPY:
        // Face.tsx: 40×20, borderRadius 0 0 20px 20px → 下半圆微笑
        _draw_mouth_half(40, 20, 20, -5, 0, true);
        _draw_glow(layer, mx, my - (int)(5 * s),
                   (int)(40 * s), (int)(20 * s), (int)(20 * s), mc);
        break;
    case EXPR_TALKING: {
        // Face.tsx: 20×24, borderRadius 12px → 竖椭圆
        int hw = (int)(10 * s);
        int hh = (int)(12 * s);
        int r = (int)(is_adult ? 12 : hw) * s; if (r > hw) r = hw; if (r > hh) r = hh;
        lv_draw_rect_dsc_t dsc;
        lv_draw_rect_dsc_init(&dsc);
        dsc.bg_color = mc; dsc.bg_opa = m_opa;
        dsc.radius = r; dsc.border_width = 0;
        lv_area_t a;
        a.x1 = mx - hw; a.y1 = my - hh;
        a.x2 = mx + hw; a.y2 = my + hh;
        lv_draw_rect(layer, &dsc, &a);
        break;
    }
    case EXPR_DIZZY: {
        // Face.tsx: 16×16, borderRadius 50% → 正圆
        int r = (int)(8 * s);
        lv_draw_rect_dsc_t dsc;
        lv_draw_rect_dsc_init(&dsc);
        dsc.bg_color = mc; dsc.bg_opa = m_opa;
        dsc.radius = r; dsc.border_width = 0;
        lv_area_t a;
        a.x1 = mx - r; a.y1 = my + (int)(10 * s) - r;
        a.x2 = mx + r; a.y2 = my + (int)(10 * s) + r;
        lv_draw_rect(layer, &dsc, &a);
        break;
    }
    case EXPR_CRYING:
        // Face.tsx: 30×12, borderRadius 12px 12px 0 0 → 上半圆 (哭嘴, 倒弧)
        _draw_mouth_half(30, 12, 12, 15, 0, false);
        break;
    case EXPR_NAUGHTY:
        // Face.tsx: 36×16, borderRadius 0 0 16px 16px, rotate -10°
        // 下半圆歪笑 (x 偏移模拟旋转)
        _draw_mouth_half(36, 16, 16, -5, 4, true);
        break;
    case EXPR_WINK:
        // Face.tsx: 40×20, borderRadius 0 0 20px 20px, rotate 12°
        _draw_mouth_half(40, 20, 20, -8, 5, true);
        break;
    case EXPR_MENU: {
        // Face.tsx: 10×4, borderRadius 2px → 微型药丸
        int hw = (int)(5 * s);
        int hh = (int)(2 * s + 0.5f);
        int r = (int)(2 * s); if (r < 1) r = 1;
        lv_draw_rect_dsc_t dsc;
        lv_draw_rect_dsc_init(&dsc);
        dsc.bg_color = mc; dsc.bg_opa = m_opa;
        dsc.radius = r; dsc.border_width = 0;
        lv_area_t a;
        a.x1 = mx - hw; a.y1 = my - (int)(20 * s) - hh;
        a.x2 = mx + hw; a.y2 = my - (int)(20 * s) + hh;
        lv_draw_rect(layer, &dsc, &a);
        break;
    }
    }
}

// ========== 泪水 ==========
static void _create_tear_objects(void)
{
    if (tear_left) return;
    lv_obj_t *screen = lv_screen_active();
    lv_color_t tc = (theme_v3_get_current() == THEME_ADULT)
        ? lv_color_hex(0x22D3EE) : lv_color_hex(0x60A5FA);

    // Face.tsx: w-2.5 h-5 = 10x20px，乘以 scale
    float s = _face_scale();
    int tw = (int)(10 * s + 0.5f);
    int th = (int)(20 * s + 0.5f);

    tear_left = lv_obj_create(screen);
    lv_obj_set_size(tear_left, tw, th);
    lv_obj_set_style_radius(tear_left, tw / 2, 0);
    lv_obj_set_style_bg_color(tear_left, tc, 0);
    lv_obj_set_style_bg_opa(tear_left, LV_OPA_60, 0);
    lv_obj_set_style_border_width(tear_left, 0, 0);
    lv_obj_add_flag(tear_left, LV_OBJ_FLAG_HIDDEN);

    tear_right = lv_obj_create(screen);
    lv_obj_set_size(tear_right, tw, th);
    lv_obj_set_style_radius(tear_right, tw / 2, 0);
    lv_obj_set_style_bg_color(tear_right, tc, 0);
    lv_obj_set_style_bg_opa(tear_right, LV_OPA_60, 0);
    lv_obj_set_style_border_width(tear_right, 0, 0);
    lv_obj_add_flag(tear_right, LV_OBJ_FLAG_HIDDEN);
}

static void _tear_y_cb(void *obj, int32_t val)
{
    lv_obj_set_y((lv_obj_t *)obj, val);
    if (val > EYE_Y + (int)(25 * _face_scale())) lv_obj_set_style_opa((lv_obj_t *)obj, LV_OPA_0, 0);
    else lv_obj_set_style_opa((lv_obj_t *)obj, LV_OPA_60, 0);
}

static void _start_tears(void)
{
    _create_tear_objects();

    // 动态计算哭泣时眼睛位置 (32x8 按 scale 缩放)
    float s = _face_scale();
    int lw = (int)(32 * s), rw = (int)(32 * s);
    int gap = (int)(FACE_EYE_GAP * s);
    int total = lw + gap + rw;
    int lx = (320 - total) / 2 + lw / 2;
    int rx = lx + lw / 2 + gap + rw / 2;

    int tw = (int)(10 * s + 0.5f) / 2;  // half tear width
    int sy = EYE_Y + (int)(4 * s);
    lv_obj_set_pos(tear_left, lx - tw, sy);
    lv_obj_clear_flag(tear_left, LV_OBJ_FLAG_HIDDEN);

    int drop = (int)(30 * s);
    lv_anim_init(&tear_anim_l);
    lv_anim_set_var(&tear_anim_l, tear_left);
    lv_anim_set_exec_cb(&tear_anim_l, _tear_y_cb);
    lv_anim_set_values(&tear_anim_l, sy, sy + drop);
    lv_anim_set_time(&tear_anim_l, 1500);
    lv_anim_set_repeat_count(&tear_anim_l, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_path_cb(&tear_anim_l, lv_anim_path_ease_in);
    lv_anim_start(&tear_anim_l);

    lv_obj_set_pos(tear_right, rx - tw, sy);
    lv_obj_clear_flag(tear_right, LV_OBJ_FLAG_HIDDEN);

    lv_anim_init(&tear_anim_r);
    lv_anim_set_var(&tear_anim_r, tear_right);
    lv_anim_set_exec_cb(&tear_anim_r, _tear_y_cb);
    lv_anim_set_values(&tear_anim_r, sy, sy + drop);
    lv_anim_set_time(&tear_anim_r, 1500);
    lv_anim_set_delay(&tear_anim_r, 750);
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

// ========== 动画驱动 ==========
static void _face_anim_cb(void *obj, int32_t v)
{
    anim_dizzy_rot = v;
    anim_wink_phase = v;
    lv_obj_invalidate(face_container);
}

static void _start_expression_anims(void)
{
    // 停止旧动画
    lv_anim_delete(face_container, _face_anim_cb);

    switch (current_expr) {
    case EXPR_DIZZY:
        // 旋转动画 (驱动重绘偏移)
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
        // 脉冲: 反复刷新重绘
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
        // 眨眼脉冲: 1.8s 周期 (Face.tsx wink keyframes)
        lv_anim_init(&face_anim);
        lv_anim_set_var(&face_anim, face_container);
        lv_anim_set_exec_cb(&face_anim, _face_anim_cb);
        lv_anim_set_values(&face_anim, 0, 1);
        lv_anim_set_time(&face_anim, 900);
        lv_anim_set_playback_time(&face_anim, 900);
        lv_anim_set_repeat_count(&face_anim, LV_ANIM_REPEAT_INFINITE);
        lv_anim_set_path_cb(&face_anim, lv_anim_path_ease_in);
        lv_anim_start(&face_anim);
        break;
    case EXPR_CRYING:
        _start_tears();
        break;
    default:
        _stop_tears();
        break;
    }
}

// ========== 渲染 ==========
static void _render_expression(Expression expr)
{
    current_expr = expr;
    lv_obj_invalidate(face_container);  // 触发重绘
    _start_expression_anims();

    ESP_LOGI(TAG, "表情: %d", expr);
}

// ========== 定时器回调 (只设标志!) ==========
static void _blink_callback(TimerHandle_t timer)
{
    if (blinking) {
        pending_blink_state = false;
        pending_blink = true;
        blinking = false;
        uint32_t next = 3000 + (esp_random() % 2000);
        xTimerChangePeriod(blink_timer, pdMS_TO_TICKS(next), 0);
    } else {
        pending_blink_state = true;
        pending_blink = true;
        blinking = true;
        xTimerChangePeriod(blink_timer, pdMS_TO_TICKS(150), 0);
    }
    xTimerStart(blink_timer, 0);
}

static void _carousel_callback(TimerHandle_t timer)
{
    next_random_expr = esp_random() % 5;
    pending_carousel = true;
    uint32_t next = 4000 + (esp_random() % 3000);
    xTimerChangePeriod(carousel_timer, pdMS_TO_TICKS(next), 0);
}

// ========== 公开 API ==========
void expressions_init(void)
{
    ESP_LOGI(TAG, "V3.10 原生绘制引擎");
    ESP_LOGI(TAG, "  眼睛: lv_draw_rect 竖椭圆");
    ESP_LOGI(TAG, "  嘴巴: lv_draw_arc 弧线");
    ESP_LOGI(TAG, "  发光: lv_draw_rect 柔光底层");
    ESP_LOGI(TAG, "  眩晕: lv_draw_arc 螺旋");
    ESP_LOGI(TAG, "  8种表情 + 眨眼 + 轮播");

    // 面部容器: 独立 obj, z-order 低于 StatusBar/BottomBar
    // 移除 CLICKABLE + SCROLLABLE → 触屏事件穿透
    face_container = lv_obj_create(lv_screen_active());
    lv_obj_set_size(face_container, 320, 192);
    lv_obj_set_pos(face_container, 0, 18);  // StatusBar 下方
    lv_obj_set_style_bg_opa(face_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(face_container, 0, 0);
    lv_obj_set_style_pad_all(face_container, 0, 0);
    lv_obj_remove_flag(face_container, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(face_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(face_container, _face_draw_cb, LV_EVENT_DRAW_POST, NULL);

    blink_timer = xTimerCreate("blink", pdMS_TO_TICKS(4000), pdTRUE, NULL, _blink_callback);
    carousel_timer = xTimerCreate("carousel", pdMS_TO_TICKS(5000), pdTRUE, NULL, _carousel_callback);

    _render_expression(EXPR_IDLE);

    uint32_t first = 3000 + (esp_random() % 2000);
    xTimerChangePeriod(blink_timer, pdMS_TO_TICKS(first), 0);
    xTimerStart(blink_timer, 0);
}

void expression_set(Expression expr, bool animate)
{
    if (expr == current_expr) return;
    _stop_tears();
    _render_expression(expr);
}

Expression expression_get_current(void) { return current_expr; }

void expression_start_carousel(void)
{
    uint32_t delay = 4000 + (esp_random() % 3000);
    xTimerChangePeriod(carousel_timer, pdMS_TO_TICKS(delay), 0);
    xTimerStart(carousel_timer, 0);
}

void expression_refresh_theme(void)
{
    lv_obj_invalidate(face_container);
}

void expression_process_pending(void)
{
    // V3.9 fix: 眨眼在主循环处理
    if (pending_blink) {
        pending_blink = false;
        blinking = pending_blink_state;
        lv_obj_invalidate(face_container);
    }

    if (pending_carousel) {
        pending_carousel = false;
        Expression next[] = {EXPR_IDLE, EXPR_HAPPY, EXPR_NAUGHTY, EXPR_WINK, EXPR_TALKING};
        expression_set(next[next_random_expr], true);
    }
}

void expression_set_drawing_enabled(bool enabled)
{
    if (face_drawing_enabled == enabled) return;
    face_drawing_enabled = enabled;
    lv_obj_invalidate(face_container);
}
