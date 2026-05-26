// page_boot.c — unified eye-rub → yawn boot animation (v7.6, theme-agnostic).
// Sequence (ms):  0  show CLOSED  → 600  BOOT_MID (half-open + yawn)
//               → 1600 BOOT_OPEN  → 2400 IDLE → goto HOME
#include "xb_pages.h"
#include "../core/xb_face.h"
#include "../widgets/xb_widgets.h"

static int g_step = 0;
static lv_timer_t* g_timer = NULL;

static void boot_tick(lv_timer_t* t) {
    switch (g_step++) {
        case 0: xb_face_set(FACE_DEEP_SLEEP); break;
        case 1: xb_face_set(FACE_BOOT_MID);   break;
        case 2: xb_face_set(FACE_BOOT_OPEN);  break;
        case 3: xb_face_set(FACE_IDLE);       break;
        default:
            lv_timer_del(t); g_timer = NULL;
            xb_router_go(PAGE_HOME);
            return;
    }
    lv_timer_set_period(t,
        g_step == 1 ? 600 :
        g_step == 2 ? 1000 :
        g_step == 3 ? 800  : 200);
}

lv_obj_t* page_boot_create(lv_obj_t* parent) {
    lv_obj_t* root = lv_obj_create(parent);
    lv_obj_set_size(root, 320, 240);
    lv_obj_center(root);
    lv_obj_set_style_bg_opa(root, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_set_style_pad_all(root, 0, 0);
    lv_obj_remove_flag(root, LV_OBJ_FLAG_SCROLLABLE);

    xb_face_create(root);
    g_step = 0;
    g_timer = lv_timer_create(boot_tick, 400, NULL);
    return root;
}
