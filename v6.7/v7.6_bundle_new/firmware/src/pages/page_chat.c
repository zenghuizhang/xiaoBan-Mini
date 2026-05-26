// page_chat.c — minimal chat scaffold.
// v7.6 surfaces: bubble area + dot-loading + send button.
// Hooks XB_EVT_CHAT_DELTA / DONE / ERROR for streaming.
#include "xb_pages.h"
#include "../core/xb_face.h"
#include "../core/xb_theme.h"
#include "../core/xb_event.h"
#include "../widgets/xb_widgets.h"
#include <string.h>

static lv_obj_t* g_bubble = NULL;
static lv_obj_t* g_label  = NULL;
static lv_obj_t* g_loading= NULL;
static char      g_buf[512];
static size_t    g_len = 0;

static void on_delta(xb_event_id_t id, void* p, void* u) {
    (void)id; (void)u;
    const char* tok = (const char*)p;
    if (!tok || !g_label) return;
    size_t n = strlen(tok);
    if (g_len + n < sizeof(g_buf) - 1) {
        memcpy(g_buf + g_len, tok, n);
        g_len += n;
        g_buf[g_len] = 0;
        lv_label_set_text(g_label, g_buf);
    }
}

static void on_done(xb_event_id_t id, void* p, void* u) {
    (void)id; (void)p; (void)u;
    if (g_loading) lv_obj_add_flag(g_loading, LV_OBJ_FLAG_HIDDEN);
    xb_face_set(FACE_HAPPY);
}

static void on_error(xb_event_id_t id, void* p, void* u) {
    (void)id; (void)u;
    if (g_loading) lv_obj_add_flag(g_loading, LV_OBJ_FLAG_HIDDEN);
    xb_face_set(FACE_LOST);
    xb_toast(p ? (const char*)p : "网络错误", 0);
}

static void send_cb(lv_event_t* e) {
    (void)e;
    g_len = 0; g_buf[0] = 0;
    if (g_label) lv_label_set_text(g_label, "");
    if (g_loading) lv_obj_remove_flag(g_loading, LV_OBJ_FLAG_HIDDEN);
    xb_face_set(FACE_TALKING);
    // hook: backend would emit XB_EVT_CHAT_DELTA tokens here.
}

static void back_cb(lv_event_t* e) { (void)e; xb_router_back(); }

lv_obj_t* page_chat_create(lv_obj_t* parent) {
    const theme_t* th = xb_theme_get();
    lv_obj_t* root = lv_obj_create(parent);
    lv_obj_set_size(root, 320, 240);
    lv_obj_center(root);
    lv_obj_set_style_bg_opa(root, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_set_style_pad_all(root, 0, 0);
    lv_obj_remove_flag(root, LV_OBJ_FLAG_SCROLLABLE);

    xb_topbar(root, "对话", back_cb);
    xb_statusbar(root);
    xb_statusbar_set_mode(XB_STATUSBAR_ALWAYS);

    g_bubble = xb_card(root, 296, 140);
    lv_obj_align(g_bubble, LV_ALIGN_TOP_MID, 0, 36);

    g_label = lv_label_create(g_bubble);
    lv_label_set_long_mode(g_label, LV_LABEL_LONG_WRAP);
    lv_label_set_text(g_label, "");
    lv_obj_set_style_text_color(g_label, th->text, 0);
    lv_obj_set_width(g_label, 270);

    g_loading = xb_dot_loading(g_bubble);
    lv_obj_align(g_loading, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_add_flag(g_loading, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t* send = xb_button(root, "说点什么", send_cb);
    lv_obj_align(send, LV_ALIGN_BOTTOM_MID, 0, -8);

    g_len = 0; g_buf[0] = 0;
    xb_event_subscribe(XB_EVT_CHAT_DELTA, on_delta, NULL);
    xb_event_subscribe(XB_EVT_CHAT_DONE,  on_done,  NULL);
    xb_event_subscribe(XB_EVT_CHAT_ERROR, on_error, NULL);
    return root;
}
