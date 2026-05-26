// page_memory_browser.cpp — scrollable list + "Clear All" with confirmation (v6.7)
#include "page_memory_browser.h"
#include "xb_widgets.h"
#include "theme_v3.h"
#include "expressions.h"
#include "font_zh_14.h"
#include <esp_log.h>
#include <string.h>

static const char* TAG = "MEMORY";

// Demo memory entries
typedef struct { const char* ts; const char* text; } mem_t;
static const mem_t DEMO_MEMORIES[5] = {
    { "10:32", "User asked about weather in Tokyo" },
    { "10:18", "Set reminder for team meeting at 3pm" },
    { "09:55", "Discussed project timeline updates" },
    { "09:30", "User prefers short, concise answers" },
    { "09:12", "Greeting: Good morning start of day" },
};

static const lv_font_t* _F(void) {
    return (const lv_font_t*)&font_zh_14;
}
static const lv_font_t* _F_small(void) {
    return &lv_font_montserrat_14;
}

static lv_obj_t* g_list = NULL;
static lv_obj_t* g_dialog = NULL;

static void dialog_ok_cb(lv_event_t* e) {
    (void)e;
    ESP_LOGI(TAG, "Memory cleared");
    if (g_dialog) { lv_obj_delete(g_dialog); g_dialog = NULL; }
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
    lv_obj_t* tb = xb_topbar_create(root, "Memory", true);
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

    // Render demo entries
    for (int i = 0; i < 5; ++i) {
        lv_obj_t* row = xb_card(g_list);
        lv_obj_set_size(row, 300, 36);
        lv_obj_set_style_pad_all(row, 4, 0);
        lv_obj_set_style_bg_color(row, th->panel, 0);
        lv_obj_set_style_bg_opa(row, LV_OPA_10, 0);

        lv_obj_t* ts = lv_label_create(row);
        lv_label_set_text(ts, DEMO_MEMORIES[i].ts);
        lv_obj_set_style_text_color(ts, th->text_dim, 0);
        lv_obj_set_style_text_font(ts, _F_small(), 0);
        lv_obj_align(ts, LV_ALIGN_TOP_LEFT, 0, 2);

        lv_obj_t* tx = lv_label_create(row);
        lv_label_set_text(tx, DEMO_MEMORIES[i].text);
        lv_obj_set_style_text_color(tx, fg, 0);
        lv_obj_set_style_text_font(tx, _F(), 0);
        lv_obj_align(tx, LV_ALIGN_BOTTOM_LEFT, 0, -2);
    }

    // Clear All button
    lv_obj_t* clear_btn = xb_button(root, "Clear All", clear_all_cb);
    lv_obj_align(clear_btn, LV_ALIGN_BOTTOM_RIGHT, -8, -4);

    expression_set_drawing_enabled(false);
    return root;
}
