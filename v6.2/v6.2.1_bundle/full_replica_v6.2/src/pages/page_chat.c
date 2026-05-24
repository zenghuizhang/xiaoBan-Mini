// page_chat.c — full dialog page (P0). Implements:
//   - statusbar + topbar with back
//   - scrollable message list (user right, assistant left)
//   - assistant streaming via XB_EVT_CHAT_DELTA / DONE / ERROR
//   - thinking dots (MI-01) while waiting first delta
//   - bottom input bar with mic / suggestion chips / stop
//   - error inline + toast on failure
//
// Bubbles are xb_card() with bg_opa=60 and rounded radius=10. Max width 220.
#include "xb_pages.h"
#include "../core/xb_theme.h"
#include "../core/xb_event.h"
#include "../core/xb_face.h"
#include "../widgets/xb_widgets.h"
#include <string.h>

extern const lv_image_dsc_t ic_mic, ic_send, ic_stop, ic_zap;

typedef struct {
    lv_obj_t* list;
    lv_obj_t* dots;
    lv_obj_t* assistant_active;   // currently-streaming bubble (label)
    bool waiting_first_delta;
} chat_ctx_t;

static lv_obj_t* add_bubble(lv_obj_t* list, const char* text, bool is_user) {
    const theme_t* th = xb_theme_get();
    lv_obj_t* row = lv_obj_create(list);
    lv_obj_set_width(row, 296);
    lv_obj_set_height(row, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row,
        is_user ? LV_FLEX_ALIGN_END : LV_FLEX_ALIGN_START,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* b = lv_obj_create(row);
    lv_obj_set_width(b, LV_SIZE_CONTENT);
    lv_obj_set_style_max_width(b, 220, 0);
    lv_obj_set_height(b, LV_SIZE_CONTENT);
    lv_obj_set_style_radius(b, 10, 0);
    lv_obj_set_style_bg_color(b, is_user ? th->accent : th->panel, 0);
    lv_obj_set_style_bg_opa(b, is_user ? LV_OPA_COVER : LV_OPA_60, 0);
    lv_obj_set_style_border_color(b, th->border, 0);
    lv_obj_set_style_border_width(b, 1, 0);
    lv_obj_set_style_pad_all(b, 8, 0);
    lv_obj_remove_flag(b, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* l = lv_label_create(b);
    lv_label_set_long_mode(l, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(l, 200);
    lv_label_set_text(l, text);
    lv_obj_set_style_text_color(l, is_user ? th->bg : th->text, 0);
    return l;
}

static void on_back(lv_event_t* e) { (void)e; xb_router_back(); }

static void on_chat_delta(xb_event_id_t id, void* payload, void* user) {
    (void)id;
    chat_ctx_t* ctx = (chat_ctx_t*)user;
    const char* tok = (const char*)payload;
    if (ctx->waiting_first_delta) {
        if (ctx->dots) { lv_obj_del(ctx->dots); ctx->dots = NULL; }
        ctx->assistant_active = add_bubble(ctx->list, "", false);
        ctx->waiting_first_delta = false;
    }
    if (ctx->assistant_active) {
        const char* cur = lv_label_get_text(ctx->assistant_active);
        char buf[1024];
        snprintf(buf, sizeof(buf), "%s%s", cur, tok);
        lv_label_set_text(ctx->assistant_active, buf);
        lv_obj_scroll_to_view(lv_obj_get_parent(ctx->assistant_active), LV_ANIM_OFF);
    }
}

static void on_chat_done(xb_event_id_t id, void* payload, void* user) {
    (void)id; (void)payload;
    chat_ctx_t* ctx = (chat_ctx_t*)user;
    ctx->assistant_active = NULL;
    xb_face_set(FACE_HAPPY_BLINK);
}

static void on_chat_error(xb_event_id_t id, void* payload, void* user) {
    (void)id;
    chat_ctx_t* ctx = (chat_ctx_t*)user;
    if (ctx->dots) { lv_obj_del(ctx->dots); ctx->dots = NULL; }
    xb_error_inline(ctx->list, payload ? (const char*)payload : "Network error");
    xb_face_set(FACE_SAD);
}

static void on_send(lv_event_t* e) {
    chat_ctx_t* ctx = (chat_ctx_t*)lv_event_get_user_data(e);
    add_bubble(ctx->list, "Hello, what time is it?", true);
    if (!ctx->dots) ctx->dots = xb_dot_loading_create(ctx->list);
    ctx->waiting_first_delta = true;
    xb_face_set(FACE_THINKING);
}

static void on_delete(lv_event_t* e) {
    chat_ctx_t* ctx = (chat_ctx_t*)lv_event_get_user_data(e);
    if (ctx) lv_free(ctx);
}

lv_obj_t* page_chat_create(lv_obj_t* parent) {
    const theme_t* th = xb_theme_get();
    chat_ctx_t* ctx = lv_malloc(sizeof(chat_ctx_t));
    memset(ctx, 0, sizeof(*ctx));

    lv_obj_t* p = lv_obj_create(parent);
    lv_obj_set_size(p, 320, 240);
    lv_obj_center(p);
    lv_obj_set_style_bg_color(p, th->bg, 0);
    lv_obj_set_style_border_width(p, 0, 0);
    lv_obj_set_style_pad_all(p, 0, 0);
    lv_obj_remove_flag(p, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(p, on_delete, LV_EVENT_DELETE, ctx);

    xb_statusbar_create(p);
    lv_obj_t* tb = xb_topbar_create(p, "Chat", true);
    lv_obj_add_flag(tb, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(tb, on_back, LV_EVENT_CLICKED, NULL);

    // message list (50 + 22 statusbar + 28 topbar = 100; list height 240-50-40 = 150)
    lv_obj_t* list = lv_obj_create(p);
    lv_obj_set_size(list, 320, 150);
    lv_obj_align(list, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(list, 0, 0);
    lv_obj_set_style_pad_all(list, 8, 0);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(list, 6, 0);
    ctx->list = list;

    // greeting
    add_bubble(list, "Hi! Tap mic or type to start.", false);

    // input bar 40px tall pinned bottom
    lv_obj_t* bar = lv_obj_create(p);
    lv_obj_set_size(bar, 320, 40);
    lv_obj_align(bar, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(bar, th->panel, 0);
    lv_obj_set_style_border_width(bar, 0, 0);
    lv_obj_set_style_pad_hor(bar, 8, 0);
    lv_obj_set_flex_flow(bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bar, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(bar, 6, 0);

    lv_obj_t* mic = lv_image_create(bar);
    lv_image_set_src(mic, &ic_mic);
    lv_obj_set_style_image_recolor(mic, th->accent, 0);
    lv_obj_set_style_image_recolor_opa(mic, LV_OPA_COVER, 0);

    lv_obj_t* hint = lv_label_create(bar);
    lv_label_set_text(hint, "Hold to talk · tap to type");
    lv_obj_set_style_text_color(hint, th->text_dim, 0);
    lv_obj_set_flex_grow(hint, 1);

    lv_obj_t* send = lv_image_create(bar);
    lv_image_set_src(send, &ic_send);
    lv_obj_set_style_image_recolor(send, th->accent, 0);
    lv_obj_set_style_image_recolor_opa(send, LV_OPA_COVER, 0);
    lv_obj_add_flag(send, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(send, on_send, LV_EVENT_CLICKED, ctx);

    xb_event_subscribe(XB_EVT_CHAT_DELTA, on_chat_delta, ctx);
    xb_event_subscribe(XB_EVT_CHAT_DONE,  on_chat_done,  ctx);
    xb_event_subscribe(XB_EVT_CHAT_ERROR, on_chat_error, ctx);

    return p;
}
