// page_skills.cpp — Skills list: Installed | Store tabs, 2-col grid (v7.6)
#include "page_skills.h"
#include "page_skill_detail.h"
#include "xb_widgets.h"
#include "theme_v3.h"
#include "expressions.h"
#include "skill_manager.h"
#include <esp_log.h>
#include <stdlib.h>
#include <string.h>

static const char* TAG = "SKILLS";

static lv_obj_t* g_grid_installed = NULL;
static lv_obj_t* g_grid_store = NULL;
static lv_obj_t* g_page = NULL;

static void on_back(lv_event_t* e) {
    (void)e;
    if (g_page) { lv_obj_delete(g_page); g_page = NULL; }
    expression_set_drawing_enabled(true);
}

static void on_skill_click(lv_event_t* e) {
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    skill_info_t* list = NULL;
    size_t count = 0;
    if (skill_list_installed(&list, &count) != ESP_OK || !list) return;
    if ((size_t)idx >= count) { free(list); return; }

    char name_copy[64];
    strlcpy(name_copy, list[idx].name, sizeof(name_copy));
    free(list);

    if (!g_page) return;
    lv_obj_delete(g_page);
    g_page = NULL;
    page_skill_detail_create(lv_screen_active(), name_copy);
}

static void render_grid(lv_obj_t* grid, bool installed_only) {
    lv_color_t fg = theme_fg();
    lv_obj_clean(grid);

    skill_info_t* list = NULL;
    size_t count = 0;
    if (skill_list_installed(&list, &count) != ESP_OK || !list) {
        ESP_LOGW(TAG, "skill_list_installed failed");
        return;
    }

    for (size_t i = 0; i < count; ++i) {
        if (installed_only && !list[i].installed) continue;

        lv_obj_t* card = xb_card(grid);
        lv_obj_set_size(card, 148, 54);
        lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(card, on_skill_click, LV_EVENT_CLICKED, (void*)(intptr_t)i);

        // Status dot: solid = installed, dim = store
        lv_obj_t* dot = lv_obj_create(card);
        lv_obj_set_size(dot, 8, 8);
        lv_obj_set_style_radius(dot, 4, 0);
        lv_obj_set_style_bg_color(dot, fg, 0);
        lv_obj_set_style_bg_opa(dot, list[i].installed ? LV_OPA_COVER : LV_OPA_30, 0);
        lv_obj_set_style_border_width(dot, 0, 0);
        lv_obj_align(dot, LV_ALIGN_LEFT_MID, 6, 0);

        lv_obj_t* nm = lv_label_create(card);
        lv_label_set_text(nm, list[i].name);
        lv_obj_set_style_text_color(nm, fg, 0);
        lv_obj_align(nm, LV_ALIGN_TOP_LEFT, 22, 4);

        lv_obj_t* ds = lv_label_create(card);
        lv_label_set_text(ds, list[i].summary);
        lv_obj_set_style_text_color(ds, fg, 0);
        lv_obj_set_style_text_opa(ds, LV_OPA_50, 0);
        lv_obj_set_style_text_font(ds, &lv_font_montserrat_14, 0);
        lv_obj_align(ds, LV_ALIGN_TOP_LEFT, 22, 22);
        lv_obj_set_width(ds, 120);
        lv_label_set_long_mode(ds, LV_LABEL_LONG_WRAP);
    }
    free(list);
}

static void on_tab_changed(lv_event_t* e) {
    lv_obj_t* tv = (lv_obj_t*)lv_event_get_target(e);
    uint16_t idx = lv_tabview_get_tab_active(tv);
    render_grid(idx == 0 ? g_grid_installed : g_grid_store, idx == 0);
}

lv_obj_t* page_skills_create(lv_obj_t* parent) {
    if (g_page) return g_page;
    lv_color_t bg = theme_bg();

    g_page = lv_obj_create(parent);
    lv_obj_set_size(g_page, 320, 240);
    lv_obj_center(g_page);
    lv_obj_set_style_bg_color(g_page, bg, 0);
    lv_obj_set_style_border_width(g_page, 0, 0);
    lv_obj_set_style_pad_all(g_page, 0, 0);
    lv_obj_remove_flag(g_page, LV_OBJ_FLAG_SCROLLABLE);

    xb_statusbar_create(g_page);
    lv_obj_t* tb = xb_topbar_create(g_page, "Skills", true);
    lv_obj_add_flag(tb, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(tb, on_back, LV_EVENT_CLICKED, NULL);

    lv_obj_t* tv = lv_tabview_create(g_page);
    lv_tabview_set_tab_bar_position(tv, LV_DIR_TOP);
    lv_tabview_set_tab_bar_size(tv, 24);
    lv_obj_set_size(tv, 320, 188);
    lv_obj_align(tv, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_set_style_bg_color(tv, bg, 0);

    lv_obj_t* t1 = lv_tabview_add_tab(tv, "Installed");
    lv_obj_t* t2 = lv_tabview_add_tab(tv, "Store");

    g_grid_installed = lv_obj_create(t1);
    lv_obj_set_size(g_grid_installed, 320, 158);
    lv_obj_set_style_bg_opa(g_grid_installed, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(g_grid_installed, 0, 0);
    lv_obj_set_style_pad_all(g_grid_installed, 6, 0);
    lv_obj_set_style_pad_gap(g_grid_installed, 6, 0);
    lv_obj_set_flex_flow(g_grid_installed, LV_FLEX_FLOW_ROW_WRAP);

    g_grid_store = lv_obj_create(t2);
    lv_obj_set_size(g_grid_store, 320, 158);
    lv_obj_set_style_bg_opa(g_grid_store, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(g_grid_store, 0, 0);
    lv_obj_set_style_pad_all(g_grid_store, 6, 0);
    lv_obj_set_style_pad_gap(g_grid_store, 6, 0);
    lv_obj_set_flex_flow(g_grid_store, LV_FLEX_FLOW_ROW_WRAP);

    render_grid(g_grid_installed, true);
    render_grid(g_grid_store, false);
    lv_obj_add_event_cb(tv, on_tab_changed, LV_EVENT_VALUE_CHANGED, NULL);

    expression_set_drawing_enabled(false);
    return g_page;
}
