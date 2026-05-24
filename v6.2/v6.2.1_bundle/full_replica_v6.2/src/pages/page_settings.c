// page_settings.c — full 11-item settings, organized in 5 groups (per UX v6.2).
//   General : theme, language, brightness
//   Audio   : volume, voice
//   Network : wifi, ota
//   Privacy : analytics, mic_mute
//   System  : about, factory_reset
#include "xb_pages.h"
#include "../core/xb_theme.h"
#include "../widgets/xb_widgets.h"

extern const lv_image_dsc_t ic_palette, ic_globe, ic_sun, ic_volume,
                            ic_mic, ic_wifi, ic_download, ic_eye_off,
                            ic_mic_off, ic_info, ic_alert_triangle;

typedef struct {
    const lv_image_dsc_t* icon;
    const char* label;
    const char* value;
    int kind;          // 0 nav  1 toggle  2 slider
} item_t;

static const char* GROUPS[] = { "General", "Audio", "Network", "Privacy", "System" };

static const item_t G_GENERAL[] = {
    { &ic_palette, "Theme",      "Tech",     0 },
    { &ic_globe,   "Language",   "English",  0 },
    { &ic_sun,     "Brightness", "70%",      2 },
};
static const item_t G_AUDIO[] = {
    { &ic_volume,  "Volume",     "60%",      2 },
    { &ic_mic,     "Voice",      "Lyra",     0 },
};
static const item_t G_NETWORK[] = {
    { &ic_wifi,     "Wi-Fi",     "home-5G",  0 },
    { &ic_download, "Update",    "v6.2.0",   0 },
};
static const item_t G_PRIVACY[] = {
    { &ic_eye_off, "Analytics",  "Off",      1 },
    { &ic_mic_off, "Mic mute",   "Off",      1 },
};
static const item_t G_SYSTEM[] = {
    { &ic_info,           "About",        "v6.2.0", 0 },
    { &ic_alert_triangle, "Factory reset","",       0 },
};

static const item_t* GROUP_DATA[] = { G_GENERAL, G_AUDIO, G_NETWORK, G_PRIVACY, G_SYSTEM };
static const int GROUP_LEN[] = { 3, 2, 2, 2, 2 };

static void on_back(lv_event_t* e) { (void)e; xb_router_back(); }

static void on_item(lv_event_t* e) {
    const item_t* it = (const item_t*)lv_event_get_user_data(e);
    if (!it) return;
    if (it->icon == &ic_wifi)     xb_router_goto(XB_PAGE_WIFI_AP);
    if (it->icon == &ic_download) xb_router_goto(XB_PAGE_OTA);
}

static void make_row(lv_obj_t* parent, const item_t* it) {
    const theme_t* th = xb_theme_get();
    lv_obj_t* row = xb_card(parent);
    lv_obj_set_size(row, 296, 32);
    lv_obj_set_style_pad_all(row, 4, 0);
    lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(row, on_item, LV_EVENT_CLICKED, (void*)it);

    lv_obj_t* i = lv_image_create(row);
    lv_image_set_src(i, it->icon);
    lv_obj_set_style_image_recolor(i, th->accent, 0);
    lv_obj_set_style_image_recolor_opa(i, LV_OPA_COVER, 0);
    lv_obj_align(i, LV_ALIGN_LEFT_MID, 0, 0);

    lv_obj_t* l = lv_label_create(row);
    lv_label_set_text(l, it->label);
    lv_obj_set_style_text_color(l, th->text, 0);
    lv_obj_align(l, LV_ALIGN_LEFT_MID, 28, 0);

    if (it->kind == 1) {
        lv_obj_t* sw = lv_switch_create(row);
        lv_obj_set_size(sw, 36, 20);
        lv_obj_align(sw, LV_ALIGN_RIGHT_MID, -4, 0);
    } else if (it->kind == 2) {
        lv_obj_t* sl = lv_slider_create(row);
        lv_obj_set_size(sl, 90, 6);
        lv_obj_align(sl, LV_ALIGN_RIGHT_MID, -4, 0);
        lv_slider_set_value(sl, 70, LV_ANIM_OFF);
        lv_obj_set_style_bg_color(sl, th->panel, 0);
        lv_obj_set_style_bg_color(sl, th->accent, LV_PART_INDICATOR);
        lv_obj_set_style_bg_color(sl, th->accent, LV_PART_KNOB);
    } else {
        lv_obj_t* v = lv_label_create(row);
        lv_label_set_text(v, it->value);
        lv_obj_set_style_text_color(v, th->text_dim, 0);
        lv_obj_align(v, LV_ALIGN_RIGHT_MID, -4, 0);
    }
}

lv_obj_t* page_settings_create(lv_obj_t* parent) {
    const theme_t* th = xb_theme_get();
    lv_obj_t* p = lv_obj_create(parent);
    lv_obj_set_size(p, 320, 240);
    lv_obj_center(p);
    lv_obj_set_style_bg_color(p, th->bg, 0);
    lv_obj_set_style_border_width(p, 0, 0);
    lv_obj_set_style_pad_all(p, 0, 0);
    lv_obj_remove_flag(p, LV_OBJ_FLAG_SCROLLABLE);

    xb_statusbar_create(p);
    lv_obj_t* tb = xb_topbar_create(p, "Settings", true);
    lv_obj_add_flag(tb, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(tb, on_back, LV_EVENT_CLICKED, NULL);

    // scrollable list
    lv_obj_t* list = lv_obj_create(p);
    lv_obj_set_size(list, 320, 190);
    lv_obj_align(list, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(list, 0, 0);
    lv_obj_set_style_pad_all(list, 8, 0);
    lv_obj_set_style_pad_gap(list, 4, 0);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);

    for (int g = 0; g < 5; ++g) {
        lv_obj_t* h = lv_label_create(list);
        lv_label_set_text(h, GROUPS[g]);
        lv_obj_set_style_text_color(h, th->accent, 0);
        for (int k = 0; k < GROUP_LEN[g]; ++k) {
            make_row(list, &GROUP_DATA[g][k]);
        }
    }
    return p;
}
