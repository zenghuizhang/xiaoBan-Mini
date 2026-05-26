// page_model_picker.c — 6 model rows.
#include "xb_pages.h"
#include "../core/xb_theme.h"
#include "../core/xb_event.h"
#include "../widgets/xb_widgets.h"
#include <string.h>

typedef struct { const char* id; const char* name; const char* hint; } model_t;

static const model_t MODELS[6] = {
    { "auto",    "Auto",      "智能路由 · 推荐"     },
    { "gpt-4o",  "GPT-4o",    "多模态 · 中等延迟"   },
    { "claude",  "Claude 3.5","长上下文 · 文档强项" },
    { "doubao",  "Doubao",    "中文优 · 国内最快"   },
    { "qwen",    "Qwen-Max",  "通义 · 工具调用"     },
    { "phi",     "Phi-3-mini","本地 · 离线可用"     },
};

static char g_active[16] = "auto";

static void pick_cb(lv_event_t* e) {
    const char* id = (const char*)lv_event_get_user_data(e);
    strncpy(g_active, id, sizeof(g_active) - 1);
    xb_event_post(XB_EVT_MODEL_CHANGED, (void*)g_active);
    xb_toast(id, 0);
    xb_router_back();
}
static void back_cb(lv_event_t* e) { (void)e; xb_router_back(); }

lv_obj_t* page_model_picker_create(lv_obj_t* parent) {
    const theme_t* th = xb_theme_get();
    lv_obj_t* root = lv_obj_create(parent);
    lv_obj_set_size(root, 320, 240);
    lv_obj_center(root);
    lv_obj_set_style_bg_opa(root, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_set_style_pad_all(root, 0, 0);
    lv_obj_remove_flag(root, LV_OBJ_FLAG_SCROLLABLE);

    xb_topbar(root, "选择模型", back_cb);

    for (int i = 0; i < 6; ++i) {
        const model_t* m = &MODELS[i];
        lv_obj_t* row = lv_btn_create(root);
        lv_obj_set_size(row, 296, 28);
        lv_obj_set_pos(row, 12, 32 + i * 32);
        lv_obj_set_style_radius(row, 14, 0);
        lv_obj_set_style_bg_color(row, th->panel, 0);
        lv_obj_set_style_bg_opa(row, xb_card_panel_opa(), 0);
        lv_obj_set_style_border_color(row,
            strcmp(g_active, m->id) == 0 ? th->accent : th->border, 0);
        lv_obj_set_style_border_width(row, 1, 0);
        lv_obj_add_event_cb(row, pick_cb, LV_EVENT_CLICKED, (void*)m->id);

        lv_obj_t* nm = lv_label_create(row);
        lv_label_set_text(nm, m->name);
        lv_obj_set_style_text_color(nm, th->text, 0);
        lv_obj_align(nm, LV_ALIGN_LEFT_MID, 8, 0);

        lv_obj_t* hi = lv_label_create(row);
        lv_label_set_text(hi, m->hint);
        lv_obj_set_style_text_color(hi, th->text_dim, 0);
        lv_obj_align(hi, LV_ALIGN_RIGHT_MID, -8, 0);
    }
    return root;
}
