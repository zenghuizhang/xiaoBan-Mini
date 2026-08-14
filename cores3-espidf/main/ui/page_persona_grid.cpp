// page_persona_grid.cpp — 3x2 grid: Lyra / Echo / Nova / Sage / Pico / Doc (v6.7)
#include "page_persona_grid.h"
#include "xb_widgets.h"
#include "theme_v3.h"
#include "expressions.h"
#include "font_zh_14.h"
#include "chat_llm.h"
#include <esp_log.h>
#include <string.h>

static const char* TAG = "PERSONA";

static const char* _T(const char* cn, const char* en) {
    extern bool s_lang_cn; return s_lang_cn ? cn : en;
}

// v7.6 人格: 中文 tagline + emoji (对齐 PRD v3.0 §4.2)
typedef struct { const char* id; const char* name; const char* tagline_cn; const char* tagline_en; const char* emoji; } persona_t;

static const persona_t PERSONAS[6] = {
    { "lyra", "Lyra", "温柔诗人",   "Gentle poet",     "🎵" },
    { "echo", "Echo", "话痨复读机", "Echo repeater",   "🪞" },
    { "nova", "Nova", "极客科普",   "Geek explainer",  "🌟" },
    { "sage", "Sage", "冷静顾问",   "Calm advisor",    "🦉" },
    { "pico", "Pico", "童趣小鸡",   "Playful chick",   "🐣" },
    { "doc",  "Doc",  "严谨医师",   "Strict doctor",   "🩺" },
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
    const char* id = (const char*)lv_event_get_user_data(e);
    chat_llm_set_persona(id);
    ESP_LOGI(TAG, "Persona changed: %s", id);
    // Walk up to root and delete
    lv_obj_t* t = (lv_obj_t*)lv_event_get_target(e);
    while (t) {
        lv_obj_t* p = lv_obj_get_parent(t);
        if (!lv_obj_get_parent(p)) { lv_obj_delete(t); break; }
        t = p;
    }
    expression_set_drawing_enabled(true);
}

lv_obj_t* page_persona_grid_create(lv_obj_t* parent) {
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
    lv_obj_t* tb = xb_topbar_create(root, _T("角色", "Persona"), true);
    lv_obj_set_style_text_font(lv_obj_get_child(tb, 1), _F(), 0);
    lv_obj_add_flag(tb, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(tb, back_cb, LV_EVENT_CLICKED, NULL);

    for (int i = 0; i < 6; ++i) {
        const persona_t* p = &PERSONAS[i];
        int col = i % 3, row = i / 3;
        lv_obj_t* tile = xb_card(root);
        lv_obj_set_size(tile, 92, 84);
        lv_obj_set_pos(tile, 18 + col * 98, 50 + row * 92);
        lv_obj_set_style_radius(tile, 14, 0);
        lv_obj_set_style_bg_color(tile, th->panel, 0);
        lv_obj_set_style_bg_opa(tile, LV_OPA_20, 0);
        lv_obj_set_style_border_color(tile, th->border, 0);
        lv_obj_set_style_border_width(tile, 2, 0);
        lv_obj_add_event_cb(tile, pick_cb, LV_EVENT_CLICKED, (void*)p->id);

        // Emoji (top)
        lv_obj_t* em = lv_label_create(tile);
        lv_label_set_text(em, p->emoji);
        lv_obj_set_style_text_font(em, &lv_font_montserrat_14, 0);
        lv_obj_align(em, LV_ALIGN_TOP_MID, 0, 4);

        // Name
        lv_obj_t* nm = lv_label_create(tile);
        lv_label_set_text(nm, p->name);
        lv_obj_set_style_text_color(nm, th->accent_hi, 0);
        lv_obj_set_style_text_font(nm, _F(), 0);
        lv_obj_align(nm, LV_ALIGN_TOP_MID, 0, 22);

        // Tagline (bottom)
        lv_obj_t* ds = lv_label_create(tile);
        lv_label_set_text(ds, _T(p->tagline_cn, p->tagline_en));
        lv_obj_set_style_text_color(ds, th->text_dim, 0);
        lv_obj_set_style_text_font(ds, _F(), 0);
        lv_label_set_long_mode(ds, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(ds, 84);
        lv_obj_align(ds, LV_ALIGN_BOTTOM_MID, 0, -4);
    }

    expression_set_drawing_enabled(false);
    return root;
}
