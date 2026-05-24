// xb_widgets.c — 对齐 full_replica_v6.2 widgets, 适配 theme_v3
#include "xb_widgets.h"
#include "theme_v3.h"

// ========== xb_card — 半透明面板 ==========
lv_obj_t* xb_card(lv_obj_t* parent) {
    lv_color_t fg = theme_fg();
    lv_obj_t* o = lv_obj_create(parent);
    lv_obj_set_style_radius(o, 4, 0);
    lv_obj_set_style_bg_color(o, fg, 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_10, 0);
    lv_obj_set_style_border_color(o, fg, 0);
    lv_obj_set_style_border_width(o, 1, 0);
    lv_obj_set_style_border_opa(o, LV_OPA_30, 0);
    lv_obj_set_style_pad_all(o, 6, 0);
    lv_obj_remove_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    return o;
}

// ========== xb_button — accent 背景按钮 ==========
lv_obj_t* xb_button(lv_obj_t* parent, const char* text, lv_event_cb_t cb) {
    lv_color_t fg = theme_fg();
    lv_obj_t* b = lv_button_create(parent);
    lv_obj_set_style_bg_color(b, fg, 0);
    lv_obj_set_style_radius(b, 4, 0);
    lv_obj_set_style_shadow_width(b, 0, 0);
    lv_obj_set_style_pad_hor(b, 12, 0);
    lv_obj_t* l = lv_label_create(b);
    lv_label_set_text(l, text);
    lv_obj_set_style_text_color(l, theme_bg(), 0);
    lv_obj_center(l);
    if (cb) lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, NULL);
    return b;
}

// ========== xb_statusbar — 22px 全局状态条, 4个图标槽 ==========
lv_obj_t* xb_statusbar_create(lv_obj_t* parent) {
    lv_color_t fg = theme_fg();
    lv_obj_t* bar = lv_obj_create(parent);
    lv_obj_set_size(bar, 320, 22);
    lv_obj_align(bar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(bar, fg, 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_10, 0);
    lv_obj_set_style_border_width(bar, 0, 0);
    lv_obj_set_style_pad_hor(bar, 6, 0);
    lv_obj_set_flex_flow(bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bar, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    // 4 slots: plug, ai, wifi, battery (简化为文字标签)
    static const char* slots[] = {"*", "@", "W", "B"};
    for (int i = 0; i < 4; i++) {
        lv_obj_t* lb = lv_label_create(bar);
        lv_label_set_text(lb, slots[i]);
        lv_obj_set_style_text_color(lb, fg, 0);
        lv_obj_set_style_text_opa(lb, LV_OPA_50, 0);
        lv_obj_set_style_pad_left(lb, 4, 0);
    }
    return bar;
}

// ========== xb_topbar — 28px 顶栏, title + 可选 back ==========
lv_obj_t* xb_topbar_create(lv_obj_t* parent, const char* title, bool back) {
    lv_color_t fg = theme_fg();
    lv_obj_t* tb = lv_obj_create(parent);
    lv_obj_set_size(tb, 320, 28);
    lv_obj_align(tb, LV_ALIGN_TOP_MID, 0, 22);
    lv_obj_set_style_bg_color(tb, fg, 0);
    lv_obj_set_style_bg_opa(tb, LV_OPA_10, 0);
    lv_obj_set_style_border_width(tb, 0, 0);
    if (back) {
        lv_obj_t* b = lv_label_create(tb);
        lv_label_set_text(b, "<");
        lv_obj_set_style_text_color(b, fg, 0);
        lv_obj_align(b, LV_ALIGN_LEFT_MID, 6, 0);
    }
    lv_obj_t* t = lv_label_create(tb);
    lv_label_set_text(t, title);
    lv_obj_set_style_text_color(t, fg, 0);
    lv_obj_align(t, LV_ALIGN_CENTER, 0, 0);
    return tb;
}

// ========== xb_dot_loading — 3点跳动动画 ==========
typedef struct { lv_obj_t* dots[3]; } dot_ctx_t;
static void dot_anim_cb(void* var, int32_t v) {
    lv_obj_set_style_opa((lv_obj_t*)var, v, 0);
}
lv_obj_t* xb_dot_loading_create(lv_obj_t* parent) {
    lv_color_t fg = theme_fg();
    lv_obj_t* row = xb_card(parent);
    lv_obj_set_size(row, 42, 20);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_gap(row, 4, 0);
    for (int i = 0; i < 3; i++) {
        lv_obj_t* d = lv_obj_create(row);
        lv_obj_set_size(d, 6, 6);
        lv_obj_set_style_radius(d, 3, 0);
        lv_obj_set_style_bg_color(d, fg, 0);
        lv_obj_set_style_border_width(d, 0, 0);
        lv_anim_t a; lv_anim_init(&a);
        lv_anim_set_var(&a, d);
        lv_anim_set_values(&a, LV_OPA_30, LV_OPA_COVER);
        lv_anim_set_duration(&a, 400);
        lv_anim_set_playback_duration(&a, 400);
        lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
        lv_anim_set_delay(&a, i * 200);
        lv_anim_set_exec_cb(&a, dot_anim_cb);
        lv_anim_start(&a);
    }
    return row;
}

// ========== xb_error_inline — 红色错误卡片 ==========
lv_obj_t* xb_error_inline(lv_obj_t* parent, const char* text) {
    lv_obj_t* o = xb_card(parent);
    lv_obj_set_style_bg_color(o, lv_color_hex(0xEF4444), 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_20, 0);
    lv_obj_set_style_border_color(o, lv_color_hex(0xEF4444), 0);
    lv_obj_t* l = lv_label_create(o);
    lv_label_set_text(l, text);
    lv_obj_set_style_text_color(l, lv_color_hex(0xFCA5A5), 0);
    return o;
}
