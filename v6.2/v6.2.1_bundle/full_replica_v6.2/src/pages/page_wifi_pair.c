// page_wifi_pair.c — 6-digit pairing code variant.
// User reads code from screen and types into companion app.
#include "xb_pages.h"
#include "../core/xb_theme.h"
#include "../widgets/xb_widgets.h"

static void on_back(lv_event_t* e) { (void)e; xb_router_back(); }

lv_obj_t* page_wifi_pair_create(lv_obj_t* parent) {
    const theme_t* th = xb_theme_get();
    lv_obj_t* p = lv_obj_create(parent);
    lv_obj_set_size(p, 320, 240);
    lv_obj_center(p);
    lv_obj_set_style_bg_color(p, th->bg, 0);
    lv_obj_set_style_border_width(p, 0, 0);
    lv_obj_set_style_pad_all(p, 0, 0);
    lv_obj_remove_flag(p, LV_OBJ_FLAG_SCROLLABLE);

    xb_statusbar_create(p);
    lv_obj_t* tb = xb_topbar_create(p, "Pair", true);
    lv_obj_add_flag(tb, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(tb, on_back, LV_EVENT_CLICKED, NULL);

    lv_obj_t* hint = lv_label_create(p);
    lv_label_set_text(hint, "Enter this code in the app:");
    lv_obj_set_style_text_color(hint, th->text_dim, 0);
    lv_obj_align(hint, LV_ALIGN_TOP_MID, 0, 70);

    // 6 digit boxes
    lv_obj_t* row = lv_obj_create(p);
    lv_obj_set_size(row, 300, 56);
    lv_obj_align(row, LV_ALIGN_CENTER, 0, 10);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    const char* digits = "73K9X2";
    for (int i = 0; i < 6; ++i) {
        lv_obj_t* box = xb_card(row);
        lv_obj_set_size(box, 38, 50);
        lv_obj_t* d = lv_label_create(box);
        char s[2] = { digits[i], 0 };
        lv_label_set_text(d, s);
        lv_obj_set_style_text_color(d, th->accent, 0);
        lv_obj_set_style_text_font(d, &lv_font_montserrat_28, 0);
        lv_obj_center(d);
    }

    lv_obj_t* tip = lv_label_create(p);
    lv_label_set_text(tip, "Code refreshes in 5:00");
    lv_obj_set_style_text_color(tip, th->text_dim, 0);
    lv_obj_align(tip, LV_ALIGN_BOTTOM_MID, 0, -20);
    return p;
}
