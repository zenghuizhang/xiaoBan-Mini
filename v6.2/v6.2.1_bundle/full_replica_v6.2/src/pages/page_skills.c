// page_skills.c — installed | store tabs, 2-col grid of skill cards.
#include "xb_pages.h"
#include "../core/xb_theme.h"
#include "../widgets/xb_widgets.h"

extern const lv_image_dsc_t ic_zap, ic_cloud, ic_book_open, ic_music, ic_clock, ic_thermometer;

typedef struct {
    const lv_image_dsc_t* icon;
    const char* name;
    const char* desc;
    bool installed;
} skill_t;

static const skill_t SKILLS[] = {
    { &ic_clock,       "Time",     "system",   true  },
    { &ic_thermometer, "Weather",  "openw.",   true  },
    { &ic_book_open,   "Story",    "library",  true  },
    { &ic_music,       "Music",    "spotify",  false },
    { &ic_cloud,       "Calendar", "google",   false },
    { &ic_zap,         "Smart",    "homekit",  false },
};

static void on_back(lv_event_t* e) { (void)e; xb_router_back(); }

static void on_skill(lv_event_t* e) {
    (void)e;
    xb_router_goto(XB_PAGE_SKILL_DETAIL);
}

static void render_grid(lv_obj_t* grid, bool installed_only) {
    const theme_t* th = xb_theme_get();
    lv_obj_clean(grid);
    int n = sizeof(SKILLS) / sizeof(SKILLS[0]);
    for (int i = 0; i < n; ++i) {
        if (installed_only && !SKILLS[i].installed) continue;
        lv_obj_t* card = xb_card(grid);
        lv_obj_set_size(card, 148, 64);
        lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(card, on_skill, LV_EVENT_CLICKED, (void*)&SKILLS[i]);

        lv_obj_t* ic = lv_image_create(card);
        lv_image_set_src(ic, SKILLS[i].icon);
        lv_obj_set_style_image_recolor(ic, th->accent, 0);
        lv_obj_set_style_image_recolor_opa(ic, LV_OPA_COVER, 0);
        lv_obj_align(ic, LV_ALIGN_LEFT_MID, 4, 0);

        lv_obj_t* n = lv_label_create(card);
        lv_label_set_text(n, SKILLS[i].name);
        lv_obj_set_style_text_color(n, th->text, 0);
        lv_obj_align(n, LV_ALIGN_TOP_LEFT, 36, 6);

        lv_obj_t* d = lv_label_create(card);
        lv_label_set_text(d, SKILLS[i].desc);
        lv_obj_set_style_text_color(d, th->text_dim, 0);
        lv_obj_align(d, LV_ALIGN_TOP_LEFT, 36, 26);
    }
}

static lv_obj_t* g_grid_installed;
static lv_obj_t* g_grid_store;

static void on_tab_changed(lv_event_t* e) {
    lv_obj_t* tv = lv_event_get_target(e);
    uint16_t idx = lv_tabview_get_tab_active(tv);
    render_grid(idx == 0 ? g_grid_installed : g_grid_store, idx == 0);
}

lv_obj_t* page_skills_create(lv_obj_t* parent) {
    const theme_t* th = xb_theme_get();
    lv_obj_t* p = lv_obj_create(parent);
    lv_obj_set_size(p, 320, 240);
    lv_obj_center(p);
    lv_obj_set_style_bg_color(p, th->bg, 0);
    lv_obj_set_style_border_width(p, 0, 0);
    lv_obj_set_style_pad_all(p, 0, 0);
    lv_obj_remove_flag(p, LV_OBJ_FLAG_SCROLLABLE);

    xb_statusbar_create(p);
    lv_obj_t* tb = xb_topbar_create(p, "Skills", true);
    lv_obj_add_flag(tb, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(tb, on_back, LV_EVENT_CLICKED, NULL);

    lv_obj_t* tv = lv_tabview_create(p);
    lv_tabview_set_tab_bar_position(tv, LV_DIR_TOP);
    lv_tabview_set_tab_bar_size(tv, 24);
    lv_obj_set_size(tv, 320, 190);
    lv_obj_align(tv, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_set_style_bg_color(tv, th->bg, 0);

    lv_obj_t* t1 = lv_tabview_add_tab(tv, "Installed");
    lv_obj_t* t2 = lv_tabview_add_tab(tv, "Store");

    g_grid_installed = lv_obj_create(t1);
    lv_obj_set_size(g_grid_installed, 320, 160);
    lv_obj_set_style_bg_opa(g_grid_installed, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(g_grid_installed, 0, 0);
    lv_obj_set_style_pad_all(g_grid_installed, 6, 0);
    lv_obj_set_style_pad_gap(g_grid_installed, 6, 0);
    lv_obj_set_flex_flow(g_grid_installed, LV_FLEX_FLOW_ROW_WRAP);

    g_grid_store = lv_obj_create(t2);
    lv_obj_set_size(g_grid_store, 320, 160);
    lv_obj_set_style_bg_opa(g_grid_store, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(g_grid_store, 0, 0);
    lv_obj_set_style_pad_all(g_grid_store, 6, 0);
    lv_obj_set_style_pad_gap(g_grid_store, 6, 0);
    lv_obj_set_flex_flow(g_grid_store, LV_FLEX_FLOW_ROW_WRAP);

    render_grid(g_grid_installed, true);
    render_grid(g_grid_store, false);
    lv_obj_add_event_cb(tv, on_tab_changed, LV_EVENT_VALUE_CHANGED, NULL);

    return p;
}
