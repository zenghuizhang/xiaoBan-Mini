// page_menu.c — 6-sector radial overlay (MI-04)
// Sectors clockwise from 12 o'clock: chat, skills, scenario, console, settings, wifi
#include "xb_pages.h"
#include "../core/xb_theme.h"
#include "../core/xb_face.h"
#include "../widgets/xb_widgets.h"
#include <math.h>

extern const lv_image_dsc_t ic_message_circle, ic_layers, ic_sparkles,
                            ic_terminal, ic_settings, ic_wifi;

typedef struct {
    const lv_image_dsc_t* icon;
    const char* label;
    xb_page_id_t target;
} sector_t;

static const sector_t SECTORS[6] = {
    { &ic_message_circle, "Chat",     XB_PAGE_CHAT },
    { &ic_layers,         "Skills",   XB_PAGE_SKILLS },
    { &ic_sparkles,       "Scene",    XB_PAGE_HOME },     // scenario alias
    { &ic_terminal,       "Console",  XB_PAGE_CONSOLE },
    { &ic_settings,       "Settings", XB_PAGE_SETTINGS },
    { &ic_wifi,           "WiFi",     XB_PAGE_WIFI_AP },
};

static void on_sector(lv_event_t* e) {
    sector_t* s = (sector_t*)lv_event_get_user_data(e);
    xb_router_goto(s->target);
}

static void on_close(lv_event_t* e) {
    (void)e;
    xb_router_back();
}

lv_obj_t* page_menu_create(lv_obj_t* parent) {
    const theme_t* th = xb_theme_get();
    lv_obj_t* p = lv_obj_create(parent);
    lv_obj_set_size(p, 320, 240);
    lv_obj_center(p);
    lv_obj_set_style_bg_color(p, th->bg, 0);
    lv_obj_set_style_bg_opa(p, LV_OPA_70, 0);
    lv_obj_set_style_border_width(p, 0, 0);
    lv_obj_set_style_pad_all(p, 0, 0);
    lv_obj_remove_flag(p, LV_OBJ_FLAG_SCROLLABLE);

    xb_face_set(FACE_MENU);

    // 6 sector buttons in a circle, radius 78
    const int cx = 160, cy = 130, r = 78;
    for (int i = 0; i < 6; ++i) {
        float angle = -90.0f + i * 60.0f;          // start at top
        float rad = angle * 3.14159265f / 180.0f;
        int x = cx + (int)(cosf(rad) * r) - 28;
        int y = cy + (int)(sinf(rad) * r) - 28;

        lv_obj_t* btn = xb_card(p);
        lv_obj_set_size(btn, 56, 56);
        lv_obj_set_style_radius(btn, 28, 0);
        lv_obj_set_style_bg_opa(btn, LV_OPA_60, 0);
        lv_obj_set_pos(btn, x, y);
        lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(btn, on_sector, LV_EVENT_CLICKED, (void*)&SECTORS[i]);

        lv_obj_t* ic = lv_image_create(btn);
        lv_image_set_src(ic, SECTORS[i].icon);
        lv_obj_set_style_image_recolor(ic, th->accent, 0);
        lv_obj_set_style_image_recolor_opa(ic, LV_OPA_COVER, 0);
        lv_obj_align(ic, LV_ALIGN_CENTER, 0, -6);

        lv_obj_t* lab = lv_label_create(btn);
        lv_label_set_text(lab, SECTORS[i].label);
        lv_obj_set_style_text_color(lab, th->text, 0);
        lv_obj_align(lab, LV_ALIGN_BOTTOM_MID, 0, -2);
    }

    // central close button
    lv_obj_t* close_btn = lv_button_create(p);
    lv_obj_set_size(close_btn, 40, 40);
    lv_obj_set_style_radius(close_btn, 20, 0);
    lv_obj_set_style_bg_color(close_btn, th->panel, 0);
    lv_obj_align(close_btn, LV_ALIGN_CENTER, 0, 10);
    lv_obj_add_event_cb(close_btn, on_close, LV_EVENT_CLICKED, NULL);
    lv_obj_t* x = lv_label_create(close_btn);
    lv_label_set_text(x, LV_SYMBOL_CLOSE);
    lv_obj_set_style_text_color(x, th->accent, 0);
    lv_obj_center(x);

    return p;
}
