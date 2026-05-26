// page_home.c — idle face + statusbar in IDLE mode + RGB strip.
// Tap anywhere → menu.
#include "xb_pages.h"
#include "../core/xb_face.h"
#include "../widgets/xb_widgets.h"

static void tap_cb(lv_event_t* e) { (void)e; xb_router_go(PAGE_MENU); }

lv_obj_t* page_home_create(lv_obj_t* parent) {
    lv_obj_t* root = lv_obj_create(parent);
    lv_obj_set_size(root, 320, 240);
    lv_obj_center(root);
    lv_obj_set_style_bg_opa(root, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_set_style_pad_all(root, 0, 0);
    lv_obj_add_flag(root, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(root, tap_cb, LV_EVENT_CLICKED, NULL);

    xb_rgb_strip(root);
    xb_statusbar(root);
    xb_statusbar_set_mode(XB_STATUSBAR_IDLE);
    xb_face_create(root);
    xb_face_set(FACE_IDLE);
    return root;
}
