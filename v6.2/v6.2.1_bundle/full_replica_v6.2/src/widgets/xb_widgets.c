// xb_widgets.c
#include "xb_widgets.h"
#include "../core/xb_theme.h"

extern const lv_image_dsc_t ic_chevron_left, ic_battery, ic_wifi, ic_ai_dot, ic_plug;

lv_obj_t* xb_card(lv_obj_t* parent) {
    const theme_t* th = xb_theme_get();
    lv_obj_t* o = lv_obj_create(parent);
    lv_obj_set_style_radius(o, 4, 0);
    lv_obj_set_style_bg_color(o, th->panel, 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_30, 0);
    lv_obj_set_style_border_color(o, th->border, 0);
    lv_obj_set_style_border_width(o, 1, 0);
    lv_obj_set_style_pad_all(o, 6, 0);
    lv_obj_remove_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    return o;
}

lv_obj_t* xb_button(lv_obj_t* parent, const char* text, lv_event_cb_t cb) {
    const theme_t* th = xb_theme_get();
    lv_obj_t* b = lv_button_create(parent);
    lv_obj_set_style_bg_color(b, th->accent, 0);
    lv_obj_set_style_radius(b, 4, 0);
    lv_obj_set_style_pad_hor(b, 12, 0);
    lv_obj_t* l = lv_label_create(b); lv_label_set_text(l, text);
    lv_obj_set_style_text_color(l, th->bg, 0);
    if (cb) lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, NULL);
    return b;
}

lv_obj_t* xb_statusbar_create(lv_obj_t* parent) {
    const theme_t* th = xb_theme_get();
    lv_obj_t* bar = lv_obj_create(parent);
    lv_obj_set_size(bar, 320, 22);
    lv_obj_align(bar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(bar, th->panel, 0);
    lv_obj_set_style_border_width(bar, 0, 0);
    lv_obj_set_style_pad_hor(bar, 6, 0);
    lv_obj_set_flex_flow(bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bar, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    // 4 slots from right: battery, wifi, ai, mcp
    const lv_image_dsc_t* slots[] = { &ic_plug, &ic_ai_dot, &ic_wifi, &ic_battery };
    for (int i = 0; i < 4; ++i) {
        lv_obj_t* ic = lv_image_create(bar);
        lv_image_set_src(ic, slots[i]);
        lv_obj_set_style_image_recolor(ic, th->text, 0);
        lv_obj_set_style_image_recolor_opa(ic, LV_OPA_COVER, 0);
        lv_obj_set_style_pad_left(ic, 4, 0);
    }
    return bar;
}

lv_obj_t* xb_topbar_create(lv_obj_t* parent, const char* title, bool back) {
    const theme_t* th = xb_theme_get();
    lv_obj_t* tb = lv_obj_create(parent);
    lv_obj_set_size(tb, 320, 28);
    lv_obj_align(tb, LV_ALIGN_TOP_MID, 0, 22);  // below statusbar
    lv_obj_set_style_bg_color(tb, th->panel, 0);
    lv_obj_set_style_border_width(tb, 0, 0);
    if (back) {
        lv_obj_t* b = lv_image_create(tb);
        lv_image_set_src(b, &ic_chevron_left);
        lv_obj_set_style_image_recolor(b, th->accent, 0);
        lv_obj_set_style_image_recolor_opa(b, LV_OPA_COVER, 0);
        lv_obj_align(b, LV_ALIGN_LEFT_MID, 6, 0);
    }
    lv_obj_t* t = lv_label_create(tb);
    lv_label_set_text(t, title);
    lv_obj_set_style_text_color(t, th->accent, 0);
    lv_obj_align(t, LV_ALIGN_CENTER, 0, 0);
    return tb;
}

// dot loading (MI-01) ------------------------------------------------------
typedef struct { lv_obj_t* dots[3]; } dot_ctx_t;
static void dot_anim_cb(void* var, int32_t v) {
    lv_obj_set_style_opa((lv_obj_t*)var, v, 0);
}
lv_obj_t* xb_dot_loading_create(lv_obj_t* parent) {
    const theme_t* th = xb_theme_get();
    lv_obj_t* row = xb_card(parent);
    lv_obj_set_size(row, 42, 20);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_gap(row, 4, 0);
    for (int i = 0; i < 3; ++i) {
        lv_obj_t* d = lv_obj_create(row);
        lv_obj_set_size(d, 6, 6);
        lv_obj_set_style_radius(d, 3, 0);
        lv_obj_set_style_bg_color(d, th->text, 0);
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

// toast (MI-03)
lv_obj_t* xb_toast(lv_obj_t* parent, const char* text, uint32_t ms) {
    const theme_t* th = xb_theme_get();
    lv_obj_t* o = xb_card(parent);
    lv_obj_set_height(o, 32);
    lv_obj_set_style_radius(o, 16, 0);
    lv_obj_align(o, LV_ALIGN_BOTTOM_MID, 0, -60);
    lv_obj_t* l = lv_label_create(o);
    lv_label_set_text(l, text);
    lv_obj_set_style_text_color(l, th->text, 0);
    lv_obj_center(l);
    lv_anim_t a; lv_anim_init(&a);
    lv_anim_set_var(&a, o);
    lv_anim_set_values(&a, LV_OPA_COVER, LV_OPA_TRANSP);
    lv_anim_set_duration(&a, 200);
    lv_anim_set_delay(&a, ms);
    lv_anim_set_exec_cb(&a, dot_anim_cb);
    lv_anim_start(&a);
    return o;
}

lv_obj_t* xb_error_inline(lv_obj_t* parent, const char* text) {
    const theme_t* th = xb_theme_get();
    lv_obj_t* o = xb_card(parent);
    lv_obj_set_style_bg_color(o, th->danger, 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_30, 0);
    lv_obj_set_style_border_color(o, th->danger, 0);
    lv_obj_t* l = lv_label_create(o); lv_label_set_text(l, text);
    lv_obj_set_style_text_color(l, th->danger, 0);
    return o;
}
