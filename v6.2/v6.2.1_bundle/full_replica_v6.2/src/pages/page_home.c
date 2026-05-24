// page_home.c — face fills the screen with statusbar pinned to top.
// Long-press anywhere -> open menu overlay.
#include "xb_pages.h"
#include "../core/xb_theme.h"
#include "../core/xb_face.h"
#include "../core/xb_event.h"
#include "../widgets/xb_widgets.h"

static void on_long_press(lv_event_t* e) {
    (void)e;
    xb_router_goto(XB_PAGE_MENU);
}

static void on_click(lv_event_t* e) {
    (void)e;
    xb_router_goto(XB_PAGE_CHAT);
}

lv_obj_t* page_home_create(lv_obj_t* parent) {
    const theme_t* th = xb_theme_get();
    lv_obj_t* p = lv_obj_create(parent);
    lv_obj_set_size(p, 320, 240);
    lv_obj_center(p);
    lv_obj_set_style_bg_color(p, th->bg, 0);
    lv_obj_set_style_border_width(p, 0, 0);
    lv_obj_set_style_pad_all(p, 0, 0);
    lv_obj_remove_flag(p, LV_OBJ_FLAG_SCROLLABLE);

    xb_statusbar_create(p);

    lv_obj_t* face_zone = lv_obj_create(p);
    lv_obj_set_size(face_zone, 320, 218);
    lv_obj_align(face_zone, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(face_zone, th->bg, 0);
    lv_obj_set_style_border_width(face_zone, 0, 0);
    lv_obj_set_style_pad_all(face_zone, 0, 0);
    lv_obj_remove_flag(face_zone, LV_OBJ_FLAG_SCROLLABLE);

    xb_face_create(face_zone);
    xb_face_set(FACE_BREATH);

    lv_obj_add_flag(face_zone, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(face_zone, on_long_press, LV_EVENT_LONG_PRESSED, NULL);
    lv_obj_add_event_cb(face_zone, on_click, LV_EVENT_CLICKED, NULL);

    return p;
}
