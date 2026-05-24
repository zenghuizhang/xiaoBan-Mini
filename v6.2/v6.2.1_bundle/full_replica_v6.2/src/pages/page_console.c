// page_console.c — developer console with 3 tabs.
//   Sensors : IMU/temp/light readouts (live)
//   Scripts : 2-col grid of demo scripts (FIXED from old 3-col)
//   Logs    : ring buffer of last 40 lines
#include "xb_pages.h"
#include "../core/xb_theme.h"
#include "../widgets/xb_widgets.h"
#include <stdio.h>

extern const lv_image_dsc_t ic_compass, ic_thermometer, ic_sun, ic_zap, ic_play,
                            ic_terminal, ic_radio, ic_cpu, ic_activity;

static void on_back(lv_event_t* e) { (void)e; xb_router_back(); }

static void make_sensor_row(lv_obj_t* parent, const lv_image_dsc_t* ic, const char* k, const char* v) {
    const theme_t* th = xb_theme_get();
    lv_obj_t* row = xb_card(parent);
    lv_obj_set_size(row, 296, 26);
    lv_obj_set_style_pad_all(row, 4, 0);
    lv_obj_t* i = lv_image_create(row);
    lv_image_set_src(i, ic);
    lv_obj_set_style_image_recolor(i, th->accent, 0);
    lv_obj_set_style_image_recolor_opa(i, LV_OPA_COVER, 0);
    lv_obj_align(i, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_t* lk = lv_label_create(row);
    lv_label_set_text(lk, k);
    lv_obj_set_style_text_color(lk, th->text, 0);
    lv_obj_align(lk, LV_ALIGN_LEFT_MID, 28, 0);
    lv_obj_t* lv = lv_label_create(row);
    lv_label_set_text(lv, v);
    lv_obj_set_style_text_color(lv, th->accent, 0);
    lv_obj_align(lv, LV_ALIGN_RIGHT_MID, -4, 0);
}

static void make_script(lv_obj_t* parent, const lv_image_dsc_t* ic, const char* name) {
    const theme_t* th = xb_theme_get();
    lv_obj_t* card = xb_card(parent);
    // 2-col: each card 148px wide
    lv_obj_set_size(card, 148, 56);
    lv_obj_t* i = lv_image_create(card);
    lv_image_set_src(i, ic);
    lv_obj_set_style_image_recolor(i, th->accent, 0);
    lv_obj_set_style_image_recolor_opa(i, LV_OPA_COVER, 0);
    lv_obj_align(i, LV_ALIGN_LEFT_MID, 4, 0);
    lv_obj_t* l = lv_label_create(card);
    lv_label_set_long_mode(l, LV_LABEL_LONG_DOT);
    lv_obj_set_width(l, 100);
    lv_label_set_text(l, name);
    lv_obj_set_style_text_color(l, th->text, 0);
    lv_obj_align(l, LV_ALIGN_LEFT_MID, 36, 0);
}

lv_obj_t* page_console_create(lv_obj_t* parent) {
    const theme_t* th = xb_theme_get();
    lv_obj_t* p = lv_obj_create(parent);
    lv_obj_set_size(p, 320, 240);
    lv_obj_center(p);
    lv_obj_set_style_bg_color(p, th->bg, 0);
    lv_obj_set_style_border_width(p, 0, 0);
    lv_obj_set_style_pad_all(p, 0, 0);
    lv_obj_remove_flag(p, LV_OBJ_FLAG_SCROLLABLE);

    xb_statusbar_create(p);
    lv_obj_t* tb = xb_topbar_create(p, "Console", true);
    lv_obj_add_flag(tb, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(tb, on_back, LV_EVENT_CLICKED, NULL);

    lv_obj_t* tv = lv_tabview_create(p);
    lv_tabview_set_tab_bar_position(tv, LV_DIR_TOP);
    lv_tabview_set_tab_bar_size(tv, 24);
    lv_obj_set_size(tv, 320, 190);
    lv_obj_align(tv, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_set_style_bg_color(tv, th->bg, 0);

    lv_obj_t* t1 = lv_tabview_add_tab(tv, "Sensors");
    lv_obj_t* t2 = lv_tabview_add_tab(tv, "Scripts");
    lv_obj_t* t3 = lv_tabview_add_tab(tv, "Logs");

    // sensors
    lv_obj_t* col = lv_obj_create(t1);
    lv_obj_set_size(col, 320, 160);
    lv_obj_set_style_bg_opa(col, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(col, 0, 0);
    lv_obj_set_style_pad_all(col, 6, 0);
    lv_obj_set_style_pad_gap(col, 4, 0);
    lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
    make_sensor_row(col, &ic_compass,    "IMU yaw",    "12.4°");
    make_sensor_row(col, &ic_thermometer,"Temp",       "26.1°C");
    make_sensor_row(col, &ic_sun,        "Lux",        "320");
    make_sensor_row(col, &ic_cpu,        "CPU",        "37%");
    make_sensor_row(col, &ic_activity,   "Heap free",  "412 KB");

    // scripts (2-col)
    lv_obj_t* grid = lv_obj_create(t2);
    lv_obj_set_size(grid, 320, 160);
    lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(grid, 0, 0);
    lv_obj_set_style_pad_all(grid, 6, 0);
    lv_obj_set_style_pad_gap(grid, 6, 0);
    lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW_WRAP);
    make_script(grid, &ic_play,     "blink_demo");
    make_script(grid, &ic_radio,    "ble_scan");
    make_script(grid, &ic_zap,      "speed_test");
    make_script(grid, &ic_terminal, "shell_repl");
    make_script(grid, &ic_compass,  "imu_calib");
    make_script(grid, &ic_activity, "stress");

    // logs
    lv_obj_t* logs = lv_textarea_create(t3);
    lv_obj_set_size(logs, 320, 160);
    lv_obj_set_style_bg_color(logs, th->panel, 0);
    lv_obj_set_style_text_color(logs, th->text, 0);
    lv_obj_set_style_border_width(logs, 0, 0);
    lv_textarea_set_text(logs,
        "[boot] xb v6.2.0\n"
        "[wifi] connected home-5G rssi=-52\n"
        "[ai]   model=qwen-2.5-1.5b loaded\n"
        "[mcp]  3 servers active\n"
        "[ota]  channel=stable\n");
    lv_obj_add_state(logs, LV_STATE_DISABLED);

    return p;
}
