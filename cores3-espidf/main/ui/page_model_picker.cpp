// page_model_picker.cpp — 6 AI model rows (v6.7)
#include "page_model_picker.h"
#include "xb_widgets.h"
#include "theme_v3.h"
#include "expressions.h"
#include "font_zh_14.h"
#include <esp_log.h>
#include <string.h>

static const char* TAG = "MODEL";

typedef struct { const char* id; const char* name; const char* hint; } model_t;

static const model_t MODELS[6] = {
    { "auto",   "Auto",        "Smart routing"          },
    { "gpt-4o", "GPT-4o",      "Multimodal, med-latency" },
    { "claude", "Claude 3.5",  "Long context, docs"     },
    { "doubao", "Doubao Pro",  "Chinese, fastest"       },
    { "qwen",   "Qwen-Max",    "Tool-use, Tongyi"       },
    { "phi",    "Phi-3-mini",  "Local, offline"         },
};

static const lv_font_t* _F(void) {
    return (const lv_font_t*)&font_zh_14;
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
    ESP_LOGI(TAG, "XB_EVT_MODEL_CHANGED: %s", id);
    // Walk up to root and delete
    lv_obj_t* t = (lv_obj_t*)lv_event_get_target(e);
    while (t) {
        lv_obj_t* p = lv_obj_get_parent(t);
        if (!lv_obj_get_parent(p)) { lv_obj_delete(t); break; }
        t = p;
    }
    expression_set_drawing_enabled(true);
}

lv_obj_t* page_model_picker_create(lv_obj_t* parent) {
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
    lv_obj_t* tb = xb_topbar_create(root, "Model", true);
    lv_obj_set_style_text_font(lv_obj_get_child(tb, 1), _F(), 0);
    lv_obj_add_flag(tb, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(tb, back_cb, LV_EVENT_CLICKED, NULL);

    for (int i = 0; i < 6; ++i) {
        const model_t* m = &MODELS[i];
        lv_obj_t* row = xb_card(root);
        lv_obj_set_size(row, 296, 28);
        lv_obj_set_pos(row, 12, 50 + i * 32);
        lv_obj_set_style_radius(row, 14, 0);
        lv_obj_set_style_bg_color(row, th->panel, 0);
        lv_obj_set_style_bg_opa(row, LV_OPA_20, 0);
        lv_obj_set_style_border_color(row, th->border, 0);
        lv_obj_set_style_border_width(row, 1, 0);
        lv_obj_add_event_cb(row, pick_cb, LV_EVENT_CLICKED, (void*)m->id);

        lv_obj_t* nm = lv_label_create(row);
        lv_label_set_text(nm, m->name);
        lv_obj_set_style_text_color(nm, fg, 0);
        lv_obj_set_style_text_font(nm, _F(), 0);
        lv_obj_align(nm, LV_ALIGN_LEFT_MID, 8, 0);

        lv_obj_t* hi = lv_label_create(row);
        lv_label_set_text(hi, m->hint);
        lv_obj_set_style_text_color(hi, th->text_dim, 0);
        lv_obj_align(hi, LV_ALIGN_RIGHT_MID, -8, 0);
    }

    expression_set_drawing_enabled(false);
    return root;
}
