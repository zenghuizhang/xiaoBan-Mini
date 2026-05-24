// page_skill_detail.c — permission consent before install/run.
#include "xb_pages.h"
#include "../core/xb_theme.h"
#include "../widgets/xb_widgets.h"

extern const lv_image_dsc_t ic_mic, ic_cloud, ic_shield, ic_clock, ic_thermometer;

static void on_back(lv_event_t* e) { (void)e; xb_router_back(); }
static void on_allow(lv_event_t* e) { (void)e; xb_router_back(); }
static void on_deny(lv_event_t* e)  { (void)e; xb_router_back(); }

static void add_perm(lv_obj_t* list, const lv_image_dsc_t* ic, const char* text) {
    const theme_t* th = xb_theme_get();
    lv_obj_t* row = xb_card(list);
    lv_obj_set_size(row, 296, 28);
    lv_obj_set_style_pad_all(row, 4, 0);
    lv_obj_t* i = lv_image_create(row);
    lv_image_set_src(i, ic);
    lv_obj_set_style_image_recolor(i, th->accent, 0);
    lv_obj_set_style_image_recolor_opa(i, LV_OPA_COVER, 0);
    lv_obj_align(i, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_t* l = lv_label_create(row);
    lv_label_set_text(l, text);
    lv_obj_set_style_text_color(l, th->text, 0);
    lv_obj_align(l, LV_ALIGN_LEFT_MID, 28, 0);
}

lv_obj_t* page_skill_detail_create(lv_obj_t* parent, const char* skill_id) {
    (void)skill_id;
    const theme_t* th = xb_theme_get();
    lv_obj_t* p = lv_obj_create(parent);
    lv_obj_set_size(p, 320, 240);
    lv_obj_center(p);
    lv_obj_set_style_bg_color(p, th->bg, 0);
    lv_obj_set_style_border_width(p, 0, 0);
    lv_obj_set_style_pad_all(p, 0, 0);
    lv_obj_remove_flag(p, LV_OBJ_FLAG_SCROLLABLE);

    xb_statusbar_create(p);
    lv_obj_t* tb = xb_topbar_create(p, "Weather", true);
    lv_obj_add_flag(tb, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(tb, on_back, LV_EVENT_CLICKED, NULL);

    lv_obj_t* desc = lv_label_create(p);
    lv_label_set_text(desc, "Provides current and 7-day forecasts.");
    lv_obj_set_style_text_color(desc, th->text_dim, 0);
    lv_obj_align(desc, LV_ALIGN_TOP_LEFT, 12, 56);
    lv_obj_set_width(desc, 296);
    lv_label_set_long_mode(desc, LV_LABEL_LONG_WRAP);

    lv_obj_t* list = lv_obj_create(p);
    lv_obj_set_size(list, 320, 90);
    lv_obj_align(list, LV_ALIGN_TOP_MID, 0, 90);
    lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(list, 0, 0);
    lv_obj_set_style_pad_all(list, 8, 0);
    lv_obj_set_style_pad_gap(list, 4, 0);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);

    add_perm(list, &ic_cloud, "Network access");
    add_perm(list, &ic_shield, "Location (city)");

    // bottom buttons
    lv_obj_t* row = lv_obj_create(p);
    lv_obj_set_size(row, 320, 40);
    lv_obj_align(row, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(row, th->panel, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* dn = xb_button(row, "Deny",  on_deny);  (void)dn;
    lv_obj_t* al = xb_button(row, "Allow", on_allow); (void)al;
    return p;
}
