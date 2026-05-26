// page_theme_picker.c — 2x2 grid: tech / lavender / child / cocoa.
#include "xb_pages.h"
#include "../core/xb_theme.h"
#include "../widgets/xb_widgets.h"
#include <string.h>

typedef struct { const char* id; const char* name_zh; } theme_choice_t;

static const theme_choice_t THEMES[4] = {
    { "tech",     "极客 · 暗" },
    { "lavender", "薰衣草 · 亮" },
    { "child",    "童趣 · 亮" },
    { "cocoa",    "可可 · 暗" },
};

static void pick_cb(lv_event_t* e) {
    const char* id = (const char*)lv_event_get_user_data(e);
    xb_theme_set(id);
    xb_toast(id, 0);
    xb_router_back();
}
static void back_cb(lv_event_t* e) { (void)e; xb_router_back(); }

lv_obj_t* page_theme_picker_create(lv_obj_t* parent) {
    const theme_t* th = xb_theme_get();
    lv_obj_t* root = lv_obj_create(parent);
    lv_obj_set_size(root, 320, 240);
    lv_obj_center(root);
    lv_obj_set_style_bg_opa(root, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_set_style_pad_all(root, 0, 0);
    lv_obj_remove_flag(root, LV_OBJ_FLAG_SCROLLABLE);

    xb_topbar(root, "选择主题", back_cb);

    const char* active = xb_theme_name();
    for (int i = 0; i < 4; ++i) {
        const theme_choice_t* t = &THEMES[i];
        int col = i % 2, row = i / 2;
        lv_obj_t* tile = lv_btn_create(root);
        lv_obj_set_size(tile, 130, 78);
        lv_obj_set_pos(tile, 20 + col * 150, 38 + row * 90);
        lv_obj_set_style_radius(tile, 16, 0);
        lv_obj_set_style_bg_color(tile, th->panel, 0);
        lv_obj_set_style_bg_opa(tile, xb_card_panel_opa(), 0);
        lv_obj_set_style_border_color(tile,
            strcmp(active, t->id) == 0 ? th->accent_hi : th->border, 0);
        lv_obj_set_style_border_width(tile, 2, 0);
        lv_obj_add_event_cb(tile, pick_cb, LV_EVENT_CLICKED, (void*)t->id);

        lv_obj_t* l = lv_label_create(tile);
        lv_label_set_text(l, t->name_zh);
        lv_obj_set_style_text_color(l, th->text, 0);
        lv_obj_center(l);
    }
    return root;
}
