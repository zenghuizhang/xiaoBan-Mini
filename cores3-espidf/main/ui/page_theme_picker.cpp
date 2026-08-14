// page_theme_picker.cpp — 2x2 grid: Tech / Lavender / Child / Cocoa (v6.7)
#include "page_theme_picker.h"
#include "xb_widgets.h"
#include "theme_v3.h"
#include "expressions.h"
#include "font_zh_14.h"
#include "robot_memory.h"
#include <esp_log.h>
#include <string.h>

static const char* TAG = "THEME";

static const char* _T(const char* cn, const char* en) {
    extern bool s_lang_cn; return s_lang_cn ? cn : en;
}

// v7.6 主题选择器: 中文名 + 色卡 (对齐 PRD §4.1)
typedef struct { ThemeV3 id; const char* name_cn; const char* name_en; lv_color_t swatch; } theme_choice_t;

static const theme_choice_t THEMES[4] = {
    { THEME_TECH,      "科技青",     "Tech",     lv_color_hex(0x33C5FF) },
    { THEME_LAVENDER,  "柔紫",       "Lavender", lv_color_hex(0xA855F7) },
    { THEME_CHILD,     "温暖儿童",   "Child",    lv_color_hex(0xFFAA78) },
    { THEME_COCOA,     "草莓可可",   "Cocoa",    lv_color_hex(0xFDA4AF) },
};

static const lv_font_t* _F(void) {
    extern bool s_lang_cn;
    return s_lang_cn ? (const lv_font_t*)&font_zh_14 : &lv_font_montserrat_14;
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

static void pick_cb(lv_event_t* e) {
    ThemeV3 id = (ThemeV3)(intptr_t)lv_event_get_user_data(e);
    theme_v3_switch(id);
    memory_save_theme((int)id);
    ESP_LOGI(TAG, "Theme switched to %d", (int)id);
    // Walk up to root and delete
    lv_obj_t* t = (lv_obj_t*)lv_event_get_target(e);
    while (t) {
        lv_obj_t* p = lv_obj_get_parent(t);
        if (!lv_obj_get_parent(p)) { lv_obj_delete(t); break; }
        t = p;
    }
    expression_set_drawing_enabled(true);
}

lv_obj_t* page_theme_picker_create(lv_obj_t* parent) {
    lv_color_t fg = theme_fg();
    lv_color_t bg = theme_bg();
    const theme_colors_t* th = theme_get_colors();
    ThemeV3 cur = theme_v3_get_current();

    lv_obj_t* root = lv_obj_create(parent);
    lv_obj_set_size(root, 320, 240);
    lv_obj_center(root);
    lv_obj_set_style_bg_color(root, bg, 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_set_style_pad_all(root, 0, 0);
    lv_obj_remove_flag(root, LV_OBJ_FLAG_SCROLLABLE);

    xb_statusbar_create(root);
    lv_obj_t* tb = xb_topbar_create(root, _T("主题", "Theme"), true);
    lv_obj_set_style_text_font(lv_obj_get_child(tb, 1), _F(), 0);
    lv_obj_add_flag(tb, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(tb, back_cb, LV_EVENT_CLICKED, NULL);

    for (int i = 0; i < 4; ++i) {
        const theme_choice_t* t = &THEMES[i];
        int col = i % 2, row = i / 2;
        lv_obj_t* tile = xb_card(root);
        lv_obj_set_size(tile, 130, 78);
        lv_obj_set_pos(tile, 20 + col * 150, 50 + row * 90);
        lv_obj_set_style_radius(tile, 16, 0);
        lv_obj_set_style_bg_color(tile, th->panel, 0);
        lv_obj_set_style_bg_opa(tile, LV_OPA_20, 0);
        lv_obj_set_style_border_color(tile, cur == t->id ? th->accent_hi : th->border, 0);
        lv_obj_set_style_border_width(tile, 2, 0);
        lv_obj_add_event_cb(tile, pick_cb, LV_EVENT_CLICKED, (void*)(intptr_t)t->id);

        // Color swatch
        lv_obj_t* sw = lv_obj_create(tile);
        lv_obj_set_size(sw, 20, 20);
        lv_obj_set_style_radius(sw, 10, 0);
        lv_obj_set_style_bg_color(sw, t->swatch, 0);
        lv_obj_set_style_border_width(sw, 0, 0);
        lv_obj_align(sw, LV_ALIGN_TOP_LEFT, 8, 8);

        // Chinese name (primary)
        lv_obj_t* l = lv_label_create(tile);
        lv_label_set_text(l, _T(t->name_cn, t->name_en));
        lv_obj_set_style_text_color(l, fg, 0);
        lv_obj_set_style_text_font(l, _F(), 0);
        lv_obj_align(l, LV_ALIGN_CENTER, 0, 0);

        // English name (secondary, dimmed)
        lv_obj_t* en = lv_label_create(tile);
        lv_label_set_text(en, t->name_en);
        lv_obj_set_style_text_color(en, th->text_dim, 0);
        lv_obj_set_style_text_font(en, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_opa(en, LV_OPA_60, 0);
        lv_obj_align(en, LV_ALIGN_BOTTOM_MID, 0, -6);
    }

    expression_set_drawing_enabled(false);
    return root;
}
