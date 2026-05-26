// page_menu.c — 6-sector radial menu.
// Angles (deg, 0 = right): -90 top, -30, 30, 90 bottom, 150, 210
// Order: 对话 / 模型 / 主题 / 设置 / 人格 / 记忆
//        (Theme replaces Skills in v7.6, Palette icon at the 4 o'clock sector)
#include "xb_pages.h"
#include "../core/xb_face.h"
#include "../core/xb_theme.h"
#include "../widgets/xb_widgets.h"
#include <math.h>

typedef struct {
    const char* label_zh;
    const char* icon;       // LV_SYMBOL_* fallback
    int         angle_deg;
    page_id_t   target;
} sector_t;

static const sector_t SECTORS[6] = {
    { "对话", LV_SYMBOL_BELL,   -90, PAGE_CHAT          },
    { "模型", LV_SYMBOL_LIST,   -30, PAGE_MODEL_PICKER  },
    { "主题", LV_SYMBOL_EDIT,    30, PAGE_THEME_PICKER  },
    { "设置", LV_SYMBOL_SETTINGS,90, PAGE_SETTINGS      },
    { "人格", LV_SYMBOL_USER,   150, PAGE_PERSONA_GRID  },
    { "记忆", LV_SYMBOL_SAVE,   210, PAGE_MEMORY_BROWSER},
};

static void sector_cb(lv_event_t* e) {
    page_id_t t = (page_id_t)(intptr_t)lv_event_get_user_data(e);
    xb_router_go(t);
}

static void back_cb(lv_event_t* e) { (void)e; xb_router_go(PAGE_HOME); }

lv_obj_t* page_menu_create(lv_obj_t* parent) {
    const theme_t* th = xb_theme_get();
    lv_obj_t* root = lv_obj_create(parent);
    lv_obj_set_size(root, 320, 240);
    lv_obj_center(root);
    lv_obj_set_style_bg_opa(root, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_set_style_pad_all(root, 0, 0);
    lv_obj_remove_flag(root, LV_OBJ_FLAG_SCROLLABLE);

    xb_topbar(root, "菜单", back_cb);
    xb_face_create(root);
    xb_face_set(FACE_MENU);

    const int cx = 160, cy = 130, R = 78;
    for (int i = 0; i < 6; ++i) {
        const sector_t* s = &SECTORS[i];
        float rad = s->angle_deg * 3.14159265f / 180.0f;
        int x = cx + (int)(cosf(rad) * R) - 24;
        int y = cy + (int)(sinf(rad) * R) - 24;

        lv_obj_t* btn = lv_btn_create(root);
        lv_obj_set_size(btn, 48, 48);
        lv_obj_set_pos(btn, x, y);
        lv_obj_set_style_radius(btn, 24, 0);
        lv_obj_set_style_bg_color(btn, th->panel, 0);
        lv_obj_set_style_bg_opa(btn, xb_card_panel_opa(), 0);
        lv_obj_set_style_border_color(btn, th->accent, 0);
        lv_obj_set_style_border_width(btn, 1, 0);
        lv_obj_add_event_cb(btn, sector_cb, LV_EVENT_CLICKED, (void*)(intptr_t)s->target);

        lv_obj_t* ic = lv_label_create(btn);
        lv_label_set_text(ic, s->icon);
        lv_obj_set_style_text_color(ic, th->accent_hi, 0);
        lv_obj_align(ic, LV_ALIGN_TOP_MID, 0, 4);

        lv_obj_t* lb = lv_label_create(btn);
        lv_label_set_text(lb, s->label_zh);
        lv_obj_set_style_text_color(lb, th->text, 0);
        lv_obj_align(lb, LV_ALIGN_BOTTOM_MID, 0, -4);
    }
    return root;
}
