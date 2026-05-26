// page_settings.cpp — 5组11项设置页, 对齐 full_replica page_settings.c
#include "page_settings.h"
#include "xb_widgets.h"
#include "theme_v3.h"
#include "expressions.h"
#include "wifi_config.h"
#include "page_ota.h"
#include "page_console.h"
#include "font_zh_14.h"
#include <M5Unified.h>
#include <esp_log.h>

static const char* TAG = "SETTINGS";
// GROUPS now uses _L() at display time; GRP_CN/GRP_EN arrays are above

typedef struct { const char* label_cn; const char* label_en; const char* value; int kind; } item_t;
// kind: 0=nav, 1=toggle, 2=slider

// Group data (双语)
static item_t G_GENERAL[] = {
    {"主题",   "Theme",       "Tech",    0},
    {"语言",   "Language",    "CN",      0},
    {"亮度",   "Brightness",  "70%",     2},
};
static item_t G_AUDIO[] = {
    {"音量",   "Volume",      "60%",     2},
    {"语音",   "Voice",       "Lyra",    0},
};
static item_t G_NETWORK[] = {
    {"Wi-Fi",  "Wi-Fi",       "",        0},
    {"升级",   "Update",      "v6.2",    0},
};
static item_t G_PRIVACY[] = {
    {"数据分析","Analytics",  "Off",     1},
    {"麦克风", "Mic mute",    "Off",     1},
};
static item_t G_SYSTEM[] = {
    {"控制台", "Console",     "Dev",     0},
    {"关于",   "About",       "v6.7",    0},
    {"恢复出厂","Factory reset","",      0},
};
static item_t* GROUP_DATA[] = {G_GENERAL, G_AUDIO, G_NETWORK, G_PRIVACY, G_SYSTEM};
static int GROUP_LEN[] = {3, 2, 2, 2, 3};

// 组名双语
static const char* GRP_CN[] = {"通用", "音频", "网络", "隐私", "系统"};
static const char* GRP_EN[] = {"General", "Audio", "Network", "Privacy", "System"};

static const char* _T(const char* cn, const char* en) {
    extern bool s_lang_cn;
    return s_lang_cn ? cn : en;
}
static const lv_font_t* _F(void) {
    extern bool s_lang_cn;
    return s_lang_cn ? (const lv_font_t*)&font_zh_14 : &lv_font_montserrat_14;
}

static lv_obj_t* s_page = NULL;
static lv_obj_t* s_br_label = NULL;
static lv_obj_t* s_vol_label = NULL;

// Forward
static void _settings_rebuild(void);
static void _page_del_cb(lv_event_t* e) { s_page = NULL; }

// ========== Callbacks ==========
static void on_back(lv_event_t* e) {
    (void)e;
    if (s_page) { lv_obj_delete(s_page); s_page = NULL; }
    expression_set_drawing_enabled(true);
}

static void on_item(lv_event_t* e) {
    const item_t* it = (const item_t*)lv_event_get_user_data(e);
    if (!it) return;

    // Theme
    if (strcmp(it->label_en, "Theme") == 0) {
        ThemeV3 c = theme_v3_get_current();
        theme_v3_switch(c == THEME_TECH ? THEME_CHILD : c == THEME_CHILD ? THEME_COCOA : THEME_TECH);
        expression_refresh_theme();
        lv_obj_delete(s_page); s_page = NULL;
        page_settings_create(lv_screen_active());
        return;
    }
    // Language
    if (strcmp(it->label_en, "Language") == 0) {
        extern bool s_lang_cn;
        s_lang_cn = !s_lang_cn;
        lv_obj_delete(s_page); s_page = NULL;
        page_settings_create(lv_screen_active());
        return;
    }
    // Wi-Fi
    if (strcmp(it->label_en, "Wi-Fi") == 0) {
        if (s_page) { lv_obj_delete(s_page); s_page = NULL; }
        if (wifi_get_state() == WIFI_CONNECTED) {
            esp_wifi_disconnect();
            vTaskDelay(pdMS_TO_TICKS(200));
            wifi_config_t empty = {0};
            esp_wifi_set_config(WIFI_IF_STA, &empty);
        }
        wifi_start_ap_config();
        return;
    }
    // Update → OTA
    if (strcmp(it->label_en, "Update") == 0) {
        if (s_page) { lv_obj_delete(s_page); s_page = NULL; }
        page_ota_create(lv_screen_active());
        return;
    }
    // Console
    if (strcmp(it->label_en, "Console") == 0) {
        if (s_page) { lv_obj_delete(s_page); s_page = NULL; }
        page_console_create(lv_screen_active());
        return;
    }
    // Factory reset
    if (strcmp(it->label_en, "Factory reset") == 0) {
        esp_restart();
        return;
    }
}

static void _br_slider_cb(lv_event_t* e) {
    int v = lv_slider_get_value((lv_obj_t*)lv_event_get_target(e));
    M5.Display.setBrightness(v);
    if (s_br_label) lv_label_set_text_fmt(s_br_label, "%d%%", v * 100 / 255);
}

static void _vol_slider_cb(lv_event_t* e) {
    int v = lv_slider_get_value((lv_obj_t*)lv_event_get_target(e));
    M5.Speaker.setVolume(v);
    if (s_vol_label) lv_label_set_text_fmt(s_vol_label, "%d%%", v * 100 / 255);
}

// ========== Row builder ==========
static void make_row(lv_obj_t* parent, const item_t* it) {
    lv_color_t fg = theme_fg();
    lv_obj_t* row = xb_card(parent);
    lv_obj_set_size(row, 296, 32);
    lv_obj_set_style_pad_all(row, 4, 0);
    if (it->kind == 0) {
        lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(row, on_item, LV_EVENT_CLICKED, (void*)it);
    }

    lv_obj_t* l = lv_label_create(row);
    lv_label_set_text(l, _T(it->label_cn, it->label_en));
    lv_obj_set_style_text_color(l, fg, 0);
    lv_obj_set_style_text_font(l, _F(), 0);
    lv_obj_align(l, LV_ALIGN_LEFT_MID, 6, 0);

    if (it->kind == 1) {  // Toggle
        lv_obj_t* sw = lv_switch_create(row);
        lv_obj_set_size(sw, 36, 20);
        lv_obj_align(sw, LV_ALIGN_RIGHT_MID, -4, 0);
        lv_obj_set_style_bg_color(sw, fg, LV_PART_INDICATOR | LV_STATE_CHECKED);
    } else if (it->kind == 2) {  // Slider
        lv_obj_t* sl = lv_slider_create(row);
        lv_obj_set_size(sl, 80, 6);
        lv_obj_align(sl, LV_ALIGN_RIGHT_MID, -4, 0);
        lv_slider_set_range(sl, 10, 255);
        lv_obj_set_style_bg_color(sl, fg, LV_PART_INDICATOR);
        lv_obj_set_style_bg_color(sl, fg, LV_PART_KNOB);
        lv_obj_set_style_bg_color(sl, fg, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(sl, LV_OPA_20, LV_PART_MAIN);
        if (strcmp(it->label_en, "Brightness") == 0) {
            lv_slider_set_value(sl, M5.Display.getBrightness(), LV_ANIM_OFF);
            lv_obj_add_event_cb(sl, _br_slider_cb, LV_EVENT_VALUE_CHANGED, NULL);
            s_br_label = lv_label_create(row);
            lv_label_set_text_fmt(s_br_label, "%d%%", M5.Display.getBrightness() * 100 / 255);
            lv_obj_set_style_text_color(s_br_label, fg, 0);
            lv_obj_set_style_text_opa(s_br_label, LV_OPA_60, 0);
            lv_obj_align_to(s_br_label, sl, LV_ALIGN_OUT_LEFT_MID, -44, 0);
        } else if (strcmp(it->label_en, "Volume") == 0) {
            lv_slider_set_value(sl, M5.Speaker.getVolume(), LV_ANIM_OFF);
            lv_obj_add_event_cb(sl, _vol_slider_cb, LV_EVENT_VALUE_CHANGED, NULL);
            s_vol_label = lv_label_create(row);
            lv_label_set_text_fmt(s_vol_label, "%d%%", M5.Speaker.getVolume() * 100 / 255);
            lv_obj_set_style_text_color(s_vol_label, fg, 0);
            lv_obj_set_style_text_opa(s_vol_label, LV_OPA_60, 0);
            lv_obj_align_to(s_vol_label, sl, LV_ALIGN_OUT_LEFT_MID, -44, 0);
        }
    } else {  // Nav
        lv_obj_t* v = lv_label_create(row);
        lv_label_set_text(v, it->value && it->value[0] ? it->value : ">");
        lv_obj_set_style_text_color(v, fg, 0);
        lv_obj_set_style_text_font(v, _F(), 0);
        lv_obj_set_style_text_opa(v, LV_OPA_60, 0);
        lv_obj_align(v, LV_ALIGN_RIGHT_MID, -4, 0);
    }
}

// ========== Page create ==========
lv_obj_t* page_settings_create(lv_obj_t* parent) {
    if (s_page) return s_page;
    lv_color_t bg = theme_bg();
    lv_color_t fg = theme_fg();

    s_page = lv_obj_create(parent);
    lv_obj_set_size(s_page, 320, 240);
    lv_obj_center(s_page);
    lv_obj_set_style_bg_color(s_page, bg, 0);
    lv_obj_set_style_border_width(s_page, 0, 0);
    lv_obj_set_style_pad_all(s_page, 0, 0);
    lv_obj_remove_flag(s_page, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(s_page, _page_del_cb, LV_EVENT_DELETE, NULL);

    xb_statusbar_create(s_page);
    lv_obj_t* tb = xb_topbar_create(s_page, _T("设置", "Settings"), true);
    lv_obj_add_flag(tb, LV_OBJ_FLAG_CLICKABLE);
    // Apply Chinese font to topbar title label (2nd child)
    if (lv_obj_get_child_cnt(tb) >= 2) {
        lv_obj_set_style_text_font(lv_obj_get_child(tb, 1), _F(), 0);
    }
    lv_obj_add_event_cb(tb, on_back, LV_EVENT_CLICKED, NULL);

    // Update dynamic values
    ThemeV3 cur = theme_v3_get_current();
    G_GENERAL[0].value = cur == THEME_TECH ? "Tech" : cur == THEME_CHILD ? "Child" : "Dev";
    extern bool s_lang_cn;
    G_GENERAL[1].value = s_lang_cn ? "CN" : "EN";

    // Scrollable list
    lv_obj_t* list = lv_obj_create(s_page);
    lv_obj_set_size(list, 320, 190);
    lv_obj_align(list, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(list, 0, 0);
    lv_obj_set_style_pad_all(list, 8, 0);
    lv_obj_set_style_pad_gap(list, 4, 0);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_add_flag(list, LV_OBJ_FLAG_SCROLLABLE);

    for (int g = 0; g < 5; g++) {
        lv_obj_t* h = lv_label_create(list);
        lv_label_set_text(h, _T(GRP_CN[g], GRP_EN[g]));
        lv_obj_set_style_text_color(h, fg, 0);
        lv_obj_set_style_text_font(h, _F(), 0);
        lv_obj_set_style_pad_top(h, 4, 0);
        for (int k = 0; k < GROUP_LEN[g]; k++) {
            make_row(list, &GROUP_DATA[g][k]);
        }
    }

    expression_set_drawing_enabled(false);
    ESP_LOGI(TAG, "Settings page created (5 groups)");
    return s_page;
}
