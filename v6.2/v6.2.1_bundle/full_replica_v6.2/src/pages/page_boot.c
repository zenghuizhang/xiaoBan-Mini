// page_boot.c — 3500ms boot timeline.
// 0..600ms     fade-in logo (opa 0->cover)
// 600..1800ms  ring sweep (rotate -90..270)
// 1800..3000ms hold + face FACE_SLEEP_WAKE
// 3000..3500ms fade-out -> goto HOME
#include "xb_pages.h"
#include "../core/xb_theme.h"
#include "../core/xb_face.h"
#include "../widgets/xb_widgets.h"

extern const lv_image_dsc_t ic_brand_mark, ic_loading_ring;

static void boot_done_cb(lv_timer_t* t) {
    (void)t;
    xb_router_goto(XB_PAGE_HOME);
}

static void ring_anim_cb(void* var, int32_t v) {
    lv_image_set_rotation((lv_obj_t*)var, v);
}

static void fade_cb(void* var, int32_t v) {
    lv_obj_set_style_opa((lv_obj_t*)var, v, 0);
}

lv_obj_t* page_boot_create(lv_obj_t* parent) {
    const theme_t* th = xb_theme_get();
    lv_obj_t* p = lv_obj_create(parent);
    lv_obj_set_size(p, 320, 240);
    lv_obj_center(p);
    lv_obj_set_style_bg_color(p, th->bg, 0);
    lv_obj_set_style_border_width(p, 0, 0);
    lv_obj_remove_flag(p, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* mark = lv_image_create(p);
    lv_image_set_src(mark, &ic_brand_mark);
    lv_obj_set_style_image_recolor(mark, th->accent, 0);
    lv_obj_set_style_image_recolor_opa(mark, LV_OPA_COVER, 0);
    lv_obj_align(mark, LV_ALIGN_CENTER, 0, -10);
    lv_obj_set_style_opa(mark, LV_OPA_TRANSP, 0);

    lv_anim_t a; lv_anim_init(&a);
    lv_anim_set_var(&a, mark);
    lv_anim_set_values(&a, LV_OPA_TRANSP, LV_OPA_COVER);
    lv_anim_set_duration(&a, 600);
    lv_anim_set_exec_cb(&a, fade_cb);
    lv_anim_start(&a);

    lv_obj_t* ring = lv_image_create(p);
    lv_image_set_src(ring, &ic_loading_ring);
    lv_obj_set_style_image_recolor(ring, th->accent_dim, 0);
    lv_obj_set_style_image_recolor_opa(ring, LV_OPA_COVER, 0);
    lv_obj_align(ring, LV_ALIGN_CENTER, 0, -10);

    lv_anim_t b; lv_anim_init(&b);
    lv_anim_set_var(&b, ring);
    lv_anim_set_values(&b, -900, 2700); // tenths of degree (LVGL convention)
    lv_anim_set_duration(&b, 1200);
    lv_anim_set_delay(&b, 600);
    lv_anim_set_exec_cb(&b, ring_anim_cb);
    lv_anim_start(&b);

    lv_obj_t* tag = lv_label_create(p);
    lv_label_set_text(tag, "xiaobao v6.2");
    lv_obj_set_style_text_color(tag, th->text_dim, 0);
    lv_obj_align(tag, LV_ALIGN_BOTTOM_MID, 0, -28);

    // fade-out at 3000ms
    lv_anim_t c; lv_anim_init(&c);
    lv_anim_set_var(&c, p);
    lv_anim_set_values(&c, LV_OPA_COVER, LV_OPA_TRANSP);
    lv_anim_set_duration(&c, 500);
    lv_anim_set_delay(&c, 3000);
    lv_anim_set_exec_cb(&c, fade_cb);
    lv_anim_start(&c);

    lv_timer_t* t = lv_timer_create(boot_done_cb, 3500, NULL);
    lv_timer_set_repeat_count(t, 1);

    return p;
}
