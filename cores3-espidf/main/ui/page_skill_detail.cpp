// page_skill_detail.cpp — Skill detail with permissions & Allow/Deny
#include "page_skill_detail.h"
#include "xb_widgets.h"
#include "theme_v3.h"
#include "expressions.h"
#include <string.h>
#include <stdio.h>

typedef struct {
    const char* name;
    const char* desc;
    const char* perms[4];
    int perm_count;
} skill_detail_t;

static const skill_detail_t DETAILS[] = {
    {"Voice",
     "Speech synthesis & recognition engine. "
     "Supports 40+ languages and custom wake words.",
     {"Microphone access", "Audio output", "Storage"}, 3},
    {"Weather",
     "Current conditions and 7-day forecasts "
     "with severe weather alerts.",
     {"Network access", "Location (city)"}, 2},
    {"Timer",
     "Countdown timer and alarm clock with "
     "snooze, repeat, and custom labels.",
     {"System clock", "Notifications"}, 2},
    {"Translate",
     "Real-time multi-language translation "
     "supporting 100+ language pairs.",
     {"Network access", "Microphone access", "Text processing"}, 3},
    {"Parrot",
     "Record short speech clips and play them "
     "back — fun for kids and pets.",
     {"Microphone access", "Audio output", "Storage"}, 3},
};

static lv_obj_t* g_page = NULL;

static void on_back(lv_event_t* e) {
    (void)e;
    if (g_page) { lv_obj_delete(g_page); g_page = NULL; }
    expression_set_drawing_enabled(true);
}

static void on_action(lv_event_t* e) {
    (void)e;
    if (g_page) { lv_obj_delete(g_page); g_page = NULL; }
    expression_set_drawing_enabled(true);
}

static void add_perm_row(lv_obj_t* list, const char* text) {
    lv_color_t fg = theme_fg();
    lv_obj_t* row = xb_card(list);
    lv_obj_set_size(row, 296, 26);
    lv_obj_set_style_pad_all(row, 4, 0);

    lv_obj_t* dot = lv_obj_create(row);
    lv_obj_set_size(dot, 6, 6);
    lv_obj_set_style_radius(dot, 3, 0);
    lv_obj_set_style_bg_color(dot, fg, 0);
    lv_obj_set_style_bg_opa(dot, LV_OPA_60, 0);
    lv_obj_set_style_border_width(dot, 0, 0);
    lv_obj_align(dot, LV_ALIGN_LEFT_MID, 6, 0);

    lv_obj_t* label = lv_label_create(row);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, fg, 0);
    lv_obj_align(label, LV_ALIGN_LEFT_MID, 22, 0);
}

lv_obj_t* page_skill_detail_create(lv_obj_t* parent, const char* skill_name) {
    if (g_page) return g_page;
    lv_color_t bg = theme_bg();
    lv_color_t fg = theme_fg();

    const skill_detail_t* detail = &DETAILS[0];
    int n = sizeof(DETAILS) / sizeof(DETAILS[0]);
    for (int i = 0; i < n; i++) {
        if (strcmp(DETAILS[i].name, skill_name) == 0) {
            detail = &DETAILS[i]; break;
        }
    }

    g_page = lv_obj_create(parent);
    lv_obj_set_size(g_page, 320, 240);
    lv_obj_center(g_page);
    lv_obj_set_style_bg_color(g_page, bg, 0);
    lv_obj_set_style_border_width(g_page, 0, 0);
    lv_obj_set_style_pad_all(g_page, 0, 0);
    lv_obj_remove_flag(g_page, LV_OBJ_FLAG_SCROLLABLE);

    xb_statusbar_create(g_page);

    char title[32];
    snprintf(title, sizeof(title), "%s", detail->name);
    lv_obj_t* tb = xb_topbar_create(g_page, title, true);
    lv_obj_add_flag(tb, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(tb, on_back, LV_EVENT_CLICKED, NULL);

    lv_obj_t* desc = lv_label_create(g_page);
    lv_label_set_text(desc, detail->desc);
    lv_obj_set_style_text_color(desc, fg, 0);
    lv_obj_set_style_text_opa(desc, LV_OPA_70, 0);
    lv_obj_align(desc, LV_ALIGN_TOP_LEFT, 12, 56);
    lv_obj_set_width(desc, 296);
    lv_label_set_long_mode(desc, LV_LABEL_LONG_WRAP);

    lv_obj_t* perm_hdr = lv_label_create(g_page);
    lv_label_set_text(perm_hdr, "Permissions Required");
    lv_obj_set_style_text_color(perm_hdr, fg, 0);
    lv_obj_set_style_text_opa(perm_hdr, LV_OPA_60, 0);
    lv_obj_set_style_text_font(perm_hdr, &lv_font_montserrat_14, 0);
    lv_obj_align(perm_hdr, LV_ALIGN_TOP_LEFT, 12, 110);

    lv_obj_t* list = lv_obj_create(g_page);
    lv_obj_set_size(list, 320, 80);
    lv_obj_align(list, LV_ALIGN_TOP_MID, 0, 125);
    lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(list, 0, 0);
    lv_obj_set_style_pad_all(list, 6, 0);
    lv_obj_set_style_pad_gap(list, 2, 0);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);

    for (int i = 0; i < detail->perm_count; i++) {
        add_perm_row(list, detail->perms[i]);
    }

    lv_obj_t* btn_row = lv_obj_create(g_page);
    lv_obj_set_size(btn_row, 320, 40);
    lv_obj_align(btn_row, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(btn_row, fg, 0);
    lv_obj_set_style_bg_opa(btn_row, LV_OPA_10, 0);
    lv_obj_set_style_border_width(btn_row, 0, 0);
    lv_obj_set_flex_flow(btn_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_row, LV_FLEX_ALIGN_SPACE_EVENLY,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* deny_btn  = xb_button(btn_row, "Deny",  on_action);
    lv_obj_t* allow_btn = xb_button(btn_row, "Allow", on_action);
    (void)deny_btn; (void)allow_btn;

    expression_set_drawing_enabled(false);
    return g_page;
}
