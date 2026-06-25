// page_chat.cpp — Chat/Dialogue page for xiaoBan-Mini v7.6
#include "page_chat.h"
#include "xb_widgets.h"
#include "theme_v3.h"
#include "expressions.h"
#include "chat_llm.h"
#include "font_zh_14.h"
#include <string.h>
#include <stdlib.h>
#include <esp_log.h>

static const char* TAG = "CHAT";

static const char* _T(const char* cn, const char* en) {
    extern bool s_lang_cn; return s_lang_cn ? cn : en;
}
static const lv_font_t* _F(void) {
    extern bool s_lang_cn;
    return s_lang_cn ? (const lv_font_t*)&font_zh_14 : &lv_font_montserrat_14;
}

typedef struct {
    lv_obj_t* page;
    lv_obj_t* list;
    lv_obj_t* dots;
    lv_timer_t* poll_timer;
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
    lv_obj_set_style_text_font(l, _F(), 0);

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
    if (ctx) {
        if (ctx->poll_timer) { lv_timer_delete(ctx->poll_timer); ctx->poll_timer = NULL; }
        lv_free(ctx);
        s_ctx = NULL;
    }
}

/* LVGL timer: poll for LLM response (runs in main loop, safe for LVGL) */
static void _poll_response_cb(lv_timer_t* t)
{
    if (!s_ctx) return;
    char buf[512];
    if (chat_llm_poll_response(buf, sizeof(buf))) {
        page_chat_set_thinking(false);
        page_chat_add_message(buf, false);
        expression_notify_chat();  // 情绪系统: 对话 → energy+, mood+
        ESP_LOGI(TAG, "LLM response displayed");
    }
}

/* Preset question buttons for keyboard-less device */
static void _on_quick_send(lv_event_t* e)
{
    const char* text = (const char*)lv_event_get_user_data(e);
    if (!text) return;
    page_chat_add_message(text, true);
    page_chat_set_thinking(true);
    esp_err_t err = chat_llm_send(text);
    if (err != ESP_OK) {
        page_chat_set_thinking(false);
        page_chat_add_message(_T("(LLM 不可用)", "(LLM not available)"), false);
    }
}

static void _on_test_send(lv_event_t* e)
{
    (void)e;
    /* Default greeting send */
    const char* text = "Hello!";
    page_chat_add_message(text, true);
    page_chat_set_thinking(true);
    esp_err_t err = chat_llm_send(text);
    if (err != ESP_OK) {
        page_chat_set_thinking(false);
        page_chat_add_message(_T("(请在设置中配置 API 密钥)", "(Configure API key in settings)"), false);
    }
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
    lv_obj_t* tb = xb_topbar_create(p, _T("对话", "Chat"), true);
    if (lv_obj_get_child_cnt(tb) >= 2)
        lv_obj_set_style_text_font(lv_obj_get_child(tb, 1), _F(), 0);
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
    _add_bubble(list, _T("你好！点个快捷问题开始吧。", "Hi! Tap a quick question below."), false);

    // Quick-send buttons (3 preset questions for keyboard-less device)
    static const char* quick_qs_cn[] = { "你好呀", "讲个笑话", "现在几点？" };
    static const char* quick_qs_en[] = { "How are you?", "Tell me a joke", "What time is it?" };
    extern bool s_lang_cn;
    const char** quick_qs = s_lang_cn ? quick_qs_cn : quick_qs_en;
    lv_obj_t* qbar = lv_obj_create(p);
    lv_obj_set_size(qbar, 320, 30);
    lv_obj_align(qbar, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_obj_set_style_bg_opa(qbar, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(qbar, 0, 0);
    lv_obj_set_style_pad_all(qbar, 0, 0);
    lv_obj_set_flex_flow(qbar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(qbar, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(qbar, LV_OBJ_FLAG_SCROLLABLE);

    for (int i = 0; i < 3; i++) {
        lv_obj_t* qb = lv_obj_create(qbar);
        lv_obj_set_size(qb, 96, 24);
        lv_obj_set_style_radius(qb, 12, 0);
        lv_obj_set_style_bg_color(qb, fg, 0);
        lv_obj_set_style_bg_opa(qb, LV_OPA_10, 0);
        lv_obj_set_style_border_color(qb, fg, 0);
        lv_obj_set_style_border_width(qb, 1, 0);
        lv_obj_set_style_border_opa(qb, LV_OPA_30, 0);
        lv_obj_add_flag(qb, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(qb, _on_quick_send, LV_EVENT_CLICKED, (void*)quick_qs[i]);

        lv_obj_t* ql = lv_label_create(qb);
        lv_label_set_text(ql, quick_qs[i]);
        lv_obj_set_style_text_color(ql, fg, 0);
        lv_obj_set_style_text_font(ql, _F(), 0);
        lv_obj_center(ql);
    }

    // Send button (circular, accent fill) — at bottom-right
    lv_obj_t* btn = lv_obj_create(p);
    lv_obj_set_size(btn, 32, 32);
    lv_obj_set_style_radius(btn, 16, 0);
    lv_obj_set_style_bg_color(btn, fg, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_RIGHT, -4, -4);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(btn, _on_test_send, LV_EVENT_CLICKED, NULL);
    lv_obj_t* arrow = lv_label_create(btn);
    lv_label_set_text(arrow, ">");
    lv_obj_set_style_text_color(arrow, bg, 0);
    lv_obj_center(arrow);

    // Poll timer: check for LLM response every 200ms
    ctx->poll_timer = lv_timer_create(_poll_response_cb, 200, NULL);

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
