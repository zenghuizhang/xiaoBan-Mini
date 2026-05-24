// page_console.cpp — Developer console, 3 tabs (对齐 full_replica page_console.c)
//   Sensors : 5 sensor readout rows
//   Scripts : 2-col grid of demo scripts (not 3-col — replica fix)
//   Logs    : ring buffer textarea
#include "page_console.h"
#include "xb_widgets.h"
#include "theme_v3.h"
#include "expressions.h"
#include <stdio.h>

static void on_back(lv_event_t* e) {
    (void)e;
    lv_obj_t* t = (lv_obj_t*)lv_event_get_target(e);
    while (t) {
        lv_obj_t* p = lv_obj_get_parent(t);
        if (!lv_obj_get_parent(p)) { lv_obj_delete(t); break; }
        t = p;
    }
    expression_set_drawing_enabled(true);
}

static void make_sensor_row(lv_obj_t* parent, const char* icon, const char* k, const char* v) {
    lv_color_t fg = theme_fg();
    lv_obj_t* row = xb_card(parent);
    lv_obj_set_size(row, 296, 26);
    lv_obj_set_style_pad_all(row, 4, 0);

    lv_obj_t* ic = lv_label_create(row);
    lv_label_set_text(ic, icon);
    lv_obj_set_style_text_color(ic, fg, 0);
    lv_obj_align(ic, LV_ALIGN_LEFT_MID, 4, 0);

    lv_obj_t* lk = lv_label_create(row);
    lv_label_set_text(lk, k);
    lv_obj_set_style_text_color(lk, fg, 0);
    lv_obj_align(lk, LV_ALIGN_LEFT_MID, 28, 0);

    lv_obj_t* lv = lv_label_create(row);
    lv_label_set_text(lv, v);
    lv_obj_set_style_text_color(lv, fg, 0);
    lv_obj_align(lv, LV_ALIGN_RIGHT_MID, -4, 0);
}

static void make_script(lv_obj_t* parent, const char* icon, const char* name) {
    lv_color_t fg = theme_fg();
    lv_obj_t* card = xb_card(parent);
    lv_obj_set_size(card, 148, 56);

    lv_obj_t* ic = lv_label_create(card);
    lv_label_set_text(ic, icon);
    lv_obj_set_style_text_color(ic, fg, 0);
    lv_obj_align(ic, LV_ALIGN_LEFT_MID, 4, 0);

    lv_obj_t* l = lv_label_create(card);
    lv_label_set_long_mode(l, LV_LABEL_LONG_DOT);
    lv_obj_set_width(l, 100);
    lv_label_set_text(l, name);
    lv_obj_set_style_text_color(l, fg, 0);
    lv_obj_align(l, LV_ALIGN_LEFT_MID, 36, 0);
}

lv_obj_t* page_console_create(lv_obj_t* parent) {
    lv_color_t fg = theme_fg();
    lv_color_t bg = theme_bg();

    lv_obj_t* p = lv_obj_create(parent);
    lv_obj_set_size(p, 320, 240);
    lv_obj_center(p);
    lv_obj_set_style_bg_color(p, bg, 0);
    lv_obj_set_style_border_width(p, 0, 0);
    lv_obj_set_style_pad_all(p, 0, 0);
    lv_obj_remove_flag(p, LV_OBJ_FLAG_SCROLLABLE);

    xb_statusbar_create(p);
    lv_obj_t* tb = xb_topbar_create(p, "Console", true);
    lv_obj_add_flag(tb, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(tb, on_back, LV_EVENT_CLICKED, NULL);

    // Tabview
    lv_obj_t* tv = lv_tabview_create(p);
    lv_tabview_set_tab_bar_position(tv, LV_DIR_TOP);
    lv_tabview_set_tab_bar_size(tv, 24);
    lv_obj_set_size(tv, 320, 190);
    lv_obj_align(tv, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_set_style_bg_color(tv, bg, 0);

    lv_obj_t* t1 = lv_tabview_add_tab(tv, "Sensors");
    lv_obj_t* t2 = lv_tabview_add_tab(tv, "Scripts");
    lv_obj_t* t3 = lv_tabview_add_tab(tv, "Logs");

    // --- Sensors tab ---
    lv_obj_t* col = lv_obj_create(t1);
    lv_obj_set_size(col, 320, 160);
    lv_obj_set_style_bg_opa(col, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(col, 0, 0);
    lv_obj_set_style_pad_all(col, 6, 0);
    lv_obj_set_style_pad_gap(col, 4, 0);
    lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
    make_sensor_row(col, "(+)", "IMU yaw",    "12.4 deg");
    make_sensor_row(col, "(T)", "Temp",       "26.1 C");
    make_sensor_row(col, "(*)", "Lux",        "320");
    make_sensor_row(col, "(#)", "CPU",        "37%");
    make_sensor_row(col, "(~)", "Heap free",  "412 KB");

    // --- Scripts tab (2-col grid) ---
    lv_obj_t* grid = lv_obj_create(t2);
    lv_obj_set_size(grid, 320, 160);
    lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(grid, 0, 0);
    lv_obj_set_style_pad_all(grid, 6, 0);
    lv_obj_set_style_pad_gap(grid, 6, 0);
    lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW_WRAP);
    make_script(grid, "(>)", "blink_demo");
    make_script(grid, "(o)", "ble_scan");
    make_script(grid, "(!)", "speed_test");
    make_script(grid, "($)", "shell_repl");
    make_script(grid, "(+)", "imu_calib");
    make_script(grid, "(~)", "stress");

    // --- Logs tab ---
    lv_obj_t* logs = lv_textarea_create(t3);
    lv_obj_set_size(logs, 320, 160);
    lv_obj_set_style_bg_color(logs, fg, 0);
    lv_obj_set_style_bg_opa(logs, LV_OPA_10, 0);
    lv_obj_set_style_text_color(logs, fg, 0);
    lv_obj_set_style_border_width(logs, 0, 0);
    lv_textarea_set_text(logs,
        "[boot] xb v6.2.0\n"
        "[wifi] connected home-5G rssi=-52\n"
        "[ai]   model=qwen-2.5-1.5b loaded\n"
        "[mcp]  3 servers active\n"
        "[ota]  channel=stable\n");
    lv_obj_add_state(logs, LV_STATE_DISABLED);

    expression_set_drawing_enabled(false);
    return p;
}
