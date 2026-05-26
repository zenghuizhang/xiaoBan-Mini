// page_settings.c — 5 Chinese groups, Developer default-visible (v7.6).
//   1. 通用     · 主题 / 语言 / 时区
//   2. 网络     · WiFi / MCP 服务
//   3. 模型     · 默认模型 / 上下文长度
//   4. 关于     · 版本 / 设备 ID
//   5. 开发者   · [测试] 控制台 / 重启 / 恢复出厂
#include "xb_pages.h"
#include "../core/xb_theme.h"
#include "../widgets/xb_widgets.h"

typedef struct { const char* zh; page_id_t go; } row_t;

typedef struct {
    const char* title;
    const char* badge;        // NULL or "测试"
    int         n_rows;
    const row_t rows[4];
} group_t;

static const group_t GROUPS[5] = {
    { "通用", NULL, 1, { {"主题", PAGE_THEME_PICKER} } },
    { "网络", NULL, 1, { {"WiFi 配网", PAGE_WIFI_AP} } },
    { "模型", NULL, 1, { {"默认模型", PAGE_MODEL_PICKER} } },
    { "关于", NULL, 1, { {"技能（即将上线）", PAGE_SKILLS_EMPTY} } },
    { "开发者", "测试", 1, { {"控制台", PAGE_CONSOLE} } },
};

static void row_cb(lv_event_t* e) {
    page_id_t t = (page_id_t)(intptr_t)lv_event_get_user_data(e);
    xb_router_go(t);
}
static void back_cb(lv_event_t* e) { (void)e; xb_router_back(); }

lv_obj_t* page_settings_create(lv_obj_t* parent) {
    const theme_t* th = xb_theme_get();
    lv_obj_t* root = lv_obj_create(parent);
    lv_obj_set_size(root, 320, 240);
    lv_obj_center(root);
    lv_obj_set_style_bg_opa(root, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_set_style_pad_all(root, 0, 0);

    xb_topbar(root, "设置", back_cb);
    xb_statusbar(root);
    xb_statusbar_set_mode(XB_STATUSBAR_ALWAYS);

    lv_obj_t* list = lv_obj_create(root);
    lv_obj_set_size(list, 312, 196);
    lv_obj_set_pos(list, 4, 32);
    lv_obj_set_layout(list, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(list, 0, 0);
    lv_obj_set_style_pad_row(list, 4, 0);

    for (int g = 0; g < 5; ++g) {
        const group_t* gr = &GROUPS[g];
        lv_obj_t* hdr = lv_label_create(list);
        if (gr->badge) {
            char buf[32];
            snprintf(buf, sizeof(buf), "%s  [%s]", gr->title, gr->badge);
            lv_label_set_text(hdr, buf);
        } else {
            lv_label_set_text(hdr, gr->title);
        }
        lv_obj_set_style_text_color(hdr, th->accent_hi, 0);

        for (int r = 0; r < gr->n_rows; ++r) {
            lv_obj_t* btn = lv_btn_create(list);
            lv_obj_set_size(btn, 280, 26);
            lv_obj_set_style_radius(btn, 13, 0);
            lv_obj_set_style_bg_color(btn, th->panel, 0);
            lv_obj_set_style_bg_opa(btn, xb_card_panel_opa(), 0);
            lv_obj_set_style_border_color(btn, th->border, 0);
            lv_obj_set_style_border_width(btn, 1, 0);
            lv_obj_add_event_cb(btn, row_cb, LV_EVENT_CLICKED,
                                (void*)(intptr_t)gr->rows[r].go);

            lv_obj_t* l = lv_label_create(btn);
            lv_label_set_text(l, gr->rows[r].zh);
            lv_obj_set_style_text_color(l, th->text, 0);
            lv_obj_align(l, LV_ALIGN_LEFT_MID, 8, 0);

            lv_obj_t* chev = lv_label_create(btn);
            lv_label_set_text(chev, LV_SYMBOL_RIGHT);
            lv_obj_set_style_text_color(chev, th->text_dim, 0);
            lv_obj_align(chev, LV_ALIGN_RIGHT_MID, -8, 0);
        }
    }
    return root;
}
