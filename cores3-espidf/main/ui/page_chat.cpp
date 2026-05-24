// page_chat.cpp — Chat/Dialogue page for xiaoBan-Mini v6.2.1 (S2)
#include "page_chat.h"
#include "xb_widgets.h"
#include "theme_v3.h"
#include "expressions.h"
#include <string.h>
#include <stdlib.h>

typedef struct {
    lv_obj_t* page;
    lv_obj_t* list;
    lv_obj_t* dots;
} chat_ctx_t;

static chat_ctx_t* s_ctx = NULL;

// ── bubble helper ──────────────────────────────────────────────────────────
// Returns the row object so callers can scroll-to-view.
static lv_obj_t* _add_bubble(lv_obj_t* list, const char* text, bool is_user)
{
    lv_color_t fg = theme_fg();

    // Row: flex row for left/right alignment
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

    // Bubble card
    lv_obj_t* b = lv_obj_create(row);
    lv_obj_set_width(b, LV_SIZE_CONTENT);
    lv_obj_set_style_max_width(b, 220, 0);
    lv_obj_set_height(b, LV_SIZE_CONTENT);
    lv_obj_set_style_radius(b, 10, 0);
    lv_obj_set_style_bg_color(b, fg, 0);
    lv_obj_set_style_bg_opa(b, is_user ? LV_OPA_COVER : LV_OPA_20, 0);
    lv_obj_set_style_border_color(b, fg, 0);
    lv_obj_set_style_border_width(b, is_user ? 0 : 1, 0);
    lv_obj_set_style_border_opa(b, LV_OPA_30, 0);
    lv_obj_set_style_pad_all(b, 8, 0);
    lv_obj_remove_flag(b, LV_OBJ_FLAG_SCROLLABLE);

    // Label
    lv_obj_t* l = lv_label_create(b);
    lv_label_set_long_mode(l, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(l, 200);
    lv_label_set_text(l, text);
    lv_obj_set_style_text_color(l, is_user ? theme_bg() : fg, 0);

    return row;
}

// ── callbacks ──────────────────────────────────────────────────────────────

static void _on_back(lv_event_t* e)
{
    (void)e;
    if (s_ctx && s_ctx->page) {
        lv_obj_delete(s_ctx->page);
    }
}

static void _on_delete(lv_event_t* e)
{
    chat_ctx_t* ctx = (chat_ctx_t*)lv_event_get_user_data(e);
    if (ctx) { lv_free(ctx); s_ctx = NULL; }
}

static void _on_test_send(lv_event_t* e)
{
    (void)e;
    page_chat_add_message("Hello, what time is it?", true);
    page_chat_set_thinking(true);
}

// ── public API ─────────────────────────────────────────────────────────────

lv_obj_t* page_chat_create(lv_obj_t* parent)
{
    lv_color_t fg = theme_fg();
    lv_color_t bg = theme_bg();

    chat_ctx_t* ctx = (chat_ctx_t*)lv_malloc(sizeof(chat_ctx_t));
    memset(ctx, 0, sizeof(*ctx));
    s_ctx = ctx;

    // Page root
    lv_obj_t* p = lv_obj_create(parent);
    lv_obj_set_size(p, 320, 240);
    lv_obj_center(p);
    lv_obj_set_style_bg_color(p, bg, 0);
    lv_obj_set_style_border_width(p, 0, 0);
    lv_obj_set_style_pad_all(p, 0, 0);
    lv_obj_remove_flag(p, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(p, _on_delete, LV_EVENT_DELETE, ctx);
    ctx->page = p;

    // Statusbar + Topbar (22 + 28 = 50 px from top)
    xb_statusbar_create(p);
    lv_obj_t* tb = xb_topbar_create(p, "Chat", true);
    lv_obj_add_flag(tb, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(tb, _on_back, LV_EVENT_CLICKED, NULL);

    // Message list: 320 x 150, pinned below topbar area
    lv_obj_t* list = lv_obj_create(p);
    lv_obj_set_size(list, 320, 150);
    lv_obj_align(list, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(list, 0, 0);
    lv_obj_set_style_pad_all(list, 8, 0);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(list, 6, 0);
    ctx->list = list;

    // Greeting bubble
    _add_bubble(list, "Hi! Tap mic or type to start.", false);

    // Input bar — 40 px pinned bottom
    lv_obj_t* bar = lv_obj_create(p);
    lv_obj_set_size(bar, 320, 40);
    lv_obj_align(bar, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(bar, fg, 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_10, 0);
    lv_obj_set_style_border_width(bar, 0, 0);

    // Placeholder hint (no keyboard on device)
    lv_obj_t* hint = lv_label_create(bar);
    lv_label_set_text(hint, "Tap to speak...");
    lv_obj_set_style_text_color(hint, fg, 0);
    lv_obj_set_style_text_opa(hint, LV_OPA_50, 0);
    lv_obj_center(hint);

    // Dev send button (circular, accent fill)
    lv_obj_t* btn = lv_obj_create(bar);
    lv_obj_set_size(btn, 32, 32);
    lv_obj_set_style_radius(btn, 16, 0);
    lv_obj_set_style_bg_color(btn, fg, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_align(btn, LV_ALIGN_RIGHT_MID, -4, 0);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(btn, _on_test_send, LV_EVENT_CLICKED, NULL);
    lv_obj_t* arrow = lv_label_create(btn);
    lv_label_set_text(arrow, ">");
    lv_obj_set_style_text_color(arrow, bg, 0);
    lv_obj_center(arrow);

    return p;
}

void page_chat_add_message(const char* text, bool is_user)
{
    if (!s_ctx || !s_ctx->list || !text) return;

    // Auto-hide dots when a bot reply arrives
    if (!is_user && s_ctx->dots) {
        lv_obj_delete(s_ctx->dots);
        s_ctx->dots = NULL;
    }

    lv_obj_t* row = _add_bubble(s_ctx->list, text, is_user);
    expression_set(is_user ? EXPR_IDLE : EXPR_TALKING, true);
    lv_obj_scroll_to_view(row, LV_ANIM_OFF);
}

void page_chat_set_thinking(bool thinking)
{
    if (!s_ctx || !s_ctx->list) return;

    if (thinking) {
        if (!s_ctx->dots) {
            s_ctx->dots = xb_dot_loading_create(s_ctx->list);
            lv_obj_scroll_to_view(s_ctx->dots, LV_ANIM_OFF);
        }
        expression_set(EXPR_THINKING, true);
    } else {
        if (s_ctx->dots) {
            lv_obj_delete(s_ctx->dots);
            s_ctx->dots = NULL;
        }
        expression_set(EXPR_IDLE, true);
    }
}
