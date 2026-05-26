// page_persona_grid.c — 3x2 grid of 6 personas.
#include "xb_pages.h"
#include "../core/xb_theme.h"
#include "../sim/xb_persona.h"
#include "../widgets/xb_widgets.h"
#include <string.h>

static void pick_cb(lv_event_t* e) {
    const char* id = (const char*)lv_event_get_user_data(e);
    xb_persona_set(id);
    xb_toast(id, 0);
    xb_router_back();
}
static void back_cb(lv_event_t* e) { (void)e; xb_router_back(); }

lv_obj_t* page_persona_grid_create(lv_obj_t* parent) {
    const theme_t* th = xb_theme_get();
    lv_obj_t* root = lv_obj_create(parent);
    lv_obj_set_size(root, 320, 240);
    lv_obj_center(root);
    lv_obj_set_style_bg_opa(root, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_set_style_pad_all(root, 0, 0);
    lv_obj_remove_flag(root, LV_OBJ_FLAG_SCROLLABLE);

    xb_topbar(root, "人格", back_cb);

    const persona_t* active = xb_persona_active();
    for (int i = 0; i < PERSONA_COUNT; ++i) {
        const persona_t* p = &XB_PERSONAS[i];
        int col = i % 3, row = i / 3;
        lv_obj_t* tile = lv_btn_create(root);
        lv_obj_set_size(tile, 92, 84);
        lv_obj_set_pos(tile, 18 + col * 98, 36 + row * 92);
        lv_obj_set_style_radius(tile, 14, 0);
        lv_obj_set_style_bg_color(tile, th->panel, 0);
        lv_obj_set_style_bg_opa(tile, xb_card_panel_opa(), 0);
        lv_obj_set_style_border_color(tile,
            (active && strcmp(active->id, p->id) == 0) ? th->accent_hi : th->border, 0);
        lv_obj_set_style_border_width(tile, 2, 0);
        lv_obj_add_event_cb(tile, pick_cb, LV_EVENT_CLICKED, (void*)p->id);

        lv_obj_t* nm = lv_label_create(tile);
        lv_label_set_text(nm, p->name_zh);
        lv_obj_set_style_text_color(nm, th->accent_hi, 0);
        lv_obj_align(nm, LV_ALIGN_TOP_MID, 0, 6);

        lv_obj_t* tg = lv_label_create(tile);
        lv_label_set_text(tg, p->tagline_zh);
        lv_obj_set_style_text_color(tg, th->text_dim, 0);
        lv_label_set_long_mode(tg, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(tg, 84);
        lv_obj_align(tg, LV_ALIGN_BOTTOM_MID, 0, -4);
    }
    return root;
}
