// page_memory_browser.c — scrollable list + purge modal.
#include "xb_pages.h"
#include "../core/xb_theme.h"
#include "../core/xb_event.h"
#include "../sim/xb_memory_store.h"
#include "../widgets/xb_widgets.h"

static lv_obj_t* g_list = NULL;

static void render_list(void) {
    if (!g_list) return;
    lv_obj_clean(g_list);
    const theme_t* th = xb_theme_get();
    mem_entry_t buf[16];
    int n = xb_memory_list(buf, 16, 0);
    if (n == 0) {
        lv_obj_t* l = lv_label_create(g_list);
        lv_label_set_text(l, "暂无记忆。");
        lv_obj_set_style_text_color(l, th->text_dim, 0);
        lv_obj_center(l);
        return;
    }
    for (int i = 0; i < n; ++i) {
        lv_obj_t* row = xb_card(g_list, 280, 36);
        lv_obj_set_style_pad_all(row, 6, 0);
        lv_obj_t* t = lv_label_create(row);
        lv_label_set_text(t, buf[i].title);
        lv_obj_set_style_text_color(t, th->text, 0);
        lv_obj_align(t, LV_ALIGN_TOP_LEFT, 0, 0);
        lv_obj_t* s = lv_label_create(row);
        lv_label_set_text(s, buf[i].snippet);
        lv_obj_set_style_text_color(s, th->text_dim, 0);
        lv_obj_align(s, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    }
}

static void on_mem_changed(xb_event_id_t id, void* p, void* u) {
    (void)id; (void)p; (void)u;
    render_list();
}

static void purge_ok_cb(lv_event_t* e) { (void)e; xb_memory_purge(); }

static void purge_cb(lv_event_t* e) {
    (void)e;
    xb_modal("清空记忆", "此操作不可恢复，是否继续？", purge_ok_cb, NULL);
}
static void back_cb(lv_event_t* e) { (void)e; xb_router_back(); }

lv_obj_t* page_memory_browser_create(lv_obj_t* parent) {
    lv_obj_t* root = lv_obj_create(parent);
    lv_obj_set_size(root, 320, 240);
    lv_obj_center(root);
    lv_obj_set_style_bg_opa(root, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_set_style_pad_all(root, 0, 0);

    xb_topbar(root, "记忆", back_cb);

    g_list = lv_obj_create(root);
    lv_obj_set_size(g_list, 312, 168);
    lv_obj_set_pos(g_list, 4, 32);
    lv_obj_set_layout(g_list, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(g_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_bg_opa(g_list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(g_list, 0, 0);
    lv_obj_set_style_pad_row(g_list, 4, 0);

    lv_obj_t* purge = xb_button(root, "清空", purge_cb);
    lv_obj_align(purge, LV_ALIGN_BOTTOM_RIGHT, -8, -4);

    xb_event_subscribe(XB_EVT_MEMORY_CHANGED, on_mem_changed, NULL);
    render_list();
    return root;
}
