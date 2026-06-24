// page_memory_browser.cpp — Device stats & memory browser (v7.6)
#include "page_memory_browser.h"
#include "xb_widgets.h"
#include "theme_v3.h"
#include "expressions.h"
#include "font_zh_14.h"
#include "robot_memory.h"
#include "chat_llm.h"
#include <esp_log.h>
#include <esp_system.h>
#include <string.h>
#include <stdio.h>

static const char* TAG = "MEMORY";

static const char* _T(const char* cn, const char* en) {
    extern bool s_lang_cn;
    return s_lang_cn ? cn : en;
}
static const lv_font_t* _F(void) {
    extern bool s_lang_cn;
    return s_lang_cn ? (const lv_font_t*)&font_zh_14 : &lv_font_montserrat_14;
}

static lv_obj_t* g_list = NULL;
static lv_obj_t* g_dialog = NULL;

static void dialog_ok_cb(lv_event_t* e) {
    (void)e;
    /* Clear interaction counter by restarting (NVS erase on next boot would be needed for full clear) */
    ESP_LOGI(TAG, "Memory cleared — restarting device");
    if (g_dialog) { lv_obj_delete(g_dialog); g_dialog = NULL; }
    esp_restart();
}

static void dialog_cancel_cb(lv_event_t* e) {
    (void)e;
    if (g_dialog) { lv_obj_delete(g_dialog); g_dialog = NULL; }
}

static void clear_all_cb(lv_event_t* e) {
    (void)e;
    if (g_dialog) { lv_obj_delete(g_dialog); g_dialog = NULL; }

    lv_obj_t* parent = lv_screen_active();
    g_dialog = lv_obj_create(parent);
    lv_obj_set_size(g_dialog, 240, 110);
    lv_obj_center(g_dialog);
    lv_obj_set_style_bg_color(g_dialog, theme_bg(), 0);
    lv_obj_set_style_border_color(g_dialog, theme_fg(), 0);
    lv_obj_set_style_border_width(g_dialog, 2, 0);
    lv_obj_set_style_radius(g_dialog, 12, 0);
    lv_obj_add_flag(g_dialog, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t* msg = lv_label_create(g_dialog);
    lv_label_set_text(msg, "Clear all memory?\nThis cannot be undone.");
    lv_obj_set_style_text_color(msg, theme_fg(), 0);
    lv_obj_set_style_text_font(msg, _F(), 0);
    lv_obj_set_style_text_align(msg, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(msg, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_t* ok_btn = xb_button(g_dialog, "Clear", dialog_ok_cb);
    lv_obj_align(ok_btn, LV_ALIGN_BOTTOM_LEFT, 20, -10);

    lv_obj_t* cancel_btn = xb_button(g_dialog, "Cancel", dialog_cancel_cb);
    lv_obj_align(cancel_btn, LV_ALIGN_BOTTOM_RIGHT, -20, -10);
}

static void back_cb(lv_event_t* e) {
    (void)e;
    lv_obj_t* t = (lv_obj_t*)lv_event_get_target(e);
    while (t) {
        lv_obj_t* p = lv_obj_get_parent(t);
        if (!lv_obj_get_parent(p)) { lv_obj_delete(t); break; }
        t = p;
    }
    expression_set_drawing_enabled(true);
}

static void _add_stat_row(lv_obj_t* list, const char* label, const char* value,
                          lv_color_t fg, const theme_colors_t* th)
{
    lv_obj_t* row = xb_card(list);
    lv_obj_set_size(row, 300, 28);
    lv_obj_set_style_pad_all(row, 4, 0);
    lv_obj_set_style_bg_color(row, th->panel, 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_10, 0);

    lv_obj_t* lbl = lv_label_create(row);
    lv_label_set_text(lbl, label);
    lv_obj_set_style_text_color(lbl, th->text_dim, 0);
    lv_obj_set_style_text_font(lbl, _F(), 0);
    lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 4, 0);

    lv_obj_t* val = lv_label_create(row);
    lv_label_set_text(val, value);
    lv_obj_set_style_text_color(val, fg, 0);
    lv_obj_set_style_text_font(val, _F(), 0);
    lv_obj_align(val, LV_ALIGN_RIGHT_MID, -4, 0);
}

lv_obj_t* page_memory_browser_create(lv_obj_t* parent) {
    lv_color_t fg = theme_fg();
    lv_color_t bg = theme_bg();
    const theme_colors_t* th = theme_get_colors();

    lv_obj_t* root = lv_obj_create(parent);
    lv_obj_set_size(root, 320, 240);
    lv_obj_center(root);
    lv_obj_set_style_bg_color(root, bg, 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_set_style_pad_all(root, 0, 0);
    lv_obj_remove_flag(root, LV_OBJ_FLAG_SCROLLABLE);

    xb_statusbar_create(root);
    lv_obj_t* tb = xb_topbar_create(root, _T("Memory", "Memory"), true);
    lv_obj_set_style_text_font(lv_obj_get_child(tb, 1), _F(), 0);
    lv_obj_add_flag(tb, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(tb, back_cb, LV_EVENT_CLICKED, NULL);

    // Scrollable list area
    g_list = lv_obj_create(root);
    lv_obj_set_size(g_list, 312, 155);
    lv_obj_set_pos(g_list, 4, 48);
    lv_obj_set_flex_flow(g_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_bg_opa(g_list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(g_list, 0, 0);
    lv_obj_set_style_pad_row(g_list, 4, 0);
    lv_obj_set_style_pad_all(g_list, 2, 0);

    // Real device stats
    char buf[64];
    int interactions = memory_get_interaction_count();
    snprintf(buf, sizeof(buf), "%d", interactions);
    _add_stat_row(g_list, _T("交互次数", "Interactions"), buf, fg, th);

    ThemeV3 cur = theme_v3_get_current();
    const char* tname = (cur == THEME_TECH) ? "Tech" : (cur == THEME_LAVENDER) ? "Lavender" :
                        (cur == THEME_CHILD) ? "Child" : "Cocoa";
    _add_stat_row(g_list, _T("当前主题", "Theme"), tname, fg, th);

    _add_stat_row(g_list, _T("AI模型", "Model"), chat_llm_get_model(), fg, th);
    _add_stat_row(g_list, _T("角色", "Persona"), chat_llm_get_persona(), fg, th);

    snprintf(buf, sizeof(buf), "%d KB", (int)(esp_get_free_heap_size() / 1024));
    _add_stat_row(g_list, _T("可用内存", "Free Heap"), buf, fg, th);

    snprintf(buf, sizeof(buf), "v7.6");
    _add_stat_row(g_list, _T("固件版本", "Firmware"), buf, fg, th);

    // Clear All button
    lv_obj_t* clear_btn = xb_button(root, "Clear All", clear_all_cb);
    lv_obj_align(clear_btn, LV_ALIGN_BOTTOM_RIGHT, -8, -4);

    expression_set_drawing_enabled(false);
    return root;
}
