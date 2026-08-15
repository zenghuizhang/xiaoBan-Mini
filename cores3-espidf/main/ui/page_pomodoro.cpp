// page_pomodoro.cpp — Pomodoro timer view (pure display).
//
// The countdown is advanced by pomodoro_tick() in the app main loop, so the
// timer keeps running even when this page is closed (true "desktop presence").
// This page only reads the engine accessors every 250ms to refresh the big
// MM:SS, the status/task line and the context-aware primary button. Mirrors
// page_todo's structure (topbar + theme + _T/_F + ctx + LV_EVENT_DELETE cleanup).
#include "page_pomodoro.h"
#include "xb_widgets.h"
#include "theme_v3.h"
#include "expressions.h"
#include "font_zh_14.h"
#include "font_zh_22.h"
#include "pomodoro.h"
#include "page_text_input.h"
#include <stdio.h>
#include <string.h>
#include <esp_log.h>

static const char* TAG = "POMO_PAGE";

static const char* _T(const char* cn, const char* en) {
    extern bool s_lang_cn; return s_lang_cn ? cn : en;
}
static const lv_font_t* _F(void) {
    extern bool s_lang_cn;
    return s_lang_cn ? (const lv_font_t*)&font_zh_14 : &lv_font_montserrat_14;
}

/* Pending task for the NEXT focus session. Page-local: survives page close
 * within a boot (cleared on reboot). While a session runs, the engine's own
 * task (pomodoro_task()) is what's displayed. */
static char s_pending_task[POMO_TASK_MAX] = {0};

typedef struct {
    lv_obj_t*   page;
    lv_obj_t*   time_label;     // big MM:SS
    lv_obj_t*   status_label;   // phase · task
    lv_obj_t*   primary_lbl;    // label inside the start/pause/resume/skip btn
    lv_timer_t* timer;
} pomo_ctx_t;

static pomo_ctx_t* s_ctx = NULL;

static void _refresh(lv_timer_t* t);

static const char* _primary_text(pomo_state_t st)
{
    extern bool s_lang_cn;
    if (s_lang_cn) {
        switch (st) {
            case POMO_IDLE:    return "开始";
            case POMO_RUNNING: return "暂停";
            case POMO_PAUSED:  return "继续";
            case POMO_BREAK:   return "跳过";
        }
    } else {
        switch (st) {
            case POMO_IDLE:    return "Start";
            case POMO_RUNNING: return "Pause";
            case POMO_PAUSED:  return "Resume";
            case POMO_BREAK:   return "Skip";
        }
    }
    return "?";
}

static void _on_delete(lv_event_t* e)
{
    pomo_ctx_t* ctx = (pomo_ctx_t*)lv_event_get_user_data(e);
    if (ctx) {
        if (ctx->timer) lv_timer_delete(ctx->timer);
        if (s_ctx == ctx) s_ctx = NULL;
        lv_free(ctx);
    }
    expression_set_drawing_enabled(true);
}

static void _close(void)
{
    if (s_ctx && s_ctx->page) lv_obj_delete(s_ctx->page);  // triggers _on_delete
}

static void _on_back(lv_event_t* e)
{
    (void)e;
    _close();
}

static void _on_primary(lv_event_t* e)
{
    (void)e;
    switch (pomodoro_state()) {
        case POMO_IDLE:    pomodoro_start(s_pending_task); break;
        case POMO_RUNNING: pomodoro_pause();               break;
        case POMO_PAUSED:  pomodoro_resume();              break;
        case POMO_BREAK:   pomodoro_reset();               break;  // skip break
    }
    _refresh(NULL);
}

static void _on_reset(lv_event_t* e)
{
    (void)e;
    pomodoro_reset();
    _refresh(NULL);
}

static void _on_task_done(const char* text, void* ctx)
{
    (void)ctx;
    if (text) strlcpy(s_pending_task, text, POMO_TASK_MAX);
    page_pomodoro_create(lv_screen_active());
}

static void _on_task(lv_event_t* e)
{
    (void)e;
    if (!s_ctx) return;
    /* Seed the input with the running task (if any), else the pending task. */
    const char* init = (pomodoro_state() != POMO_IDLE) ? pomodoro_task() : s_pending_task;
    _close();
    page_text_input_create(lv_screen_active(),
        _T("任务名称", "Task name"),
        init && init[0] ? init : "", POMO_TASK_MAX - 1, false,
        _on_task_done, NULL);
}

static void _refresh(lv_timer_t* t)
{
    (void)t;
    if (!s_ctx || !s_ctx->time_label) return;

    pomo_state_t st = pomodoro_state();
    int rem = pomodoro_remaining_seconds();
    int mm = rem / 60, ss = rem % 60;

    lv_label_set_text_fmt(s_ctx->time_label, "%02d:%02d", mm, ss);

    const char* phase = (st == POMO_RUNNING) ? _T("专注中", "Focusing")
                      : (st == POMO_BREAK)   ? _T("休息一下", "On break")
                      : (st == POMO_PAUSED)  ? _T("已暂停", "Paused")
                      :                         _T("就绪", "Ready");
    const char* task = (st != POMO_IDLE) ? pomodoro_task() : s_pending_task;

    static char status[96];
    if (task && task[0])
        snprintf(status, sizeof(status), "%s · %s", phase, task);
    else
        snprintf(status, sizeof(status), "%s", phase);
    lv_label_set_text(s_ctx->status_label, status);

    lv_label_set_text(s_ctx->primary_lbl, _primary_text(st));
}

static lv_obj_t* _make_btn(lv_obj_t* bar, const char* txt, lv_event_cb_t cb,
                           lv_obj_t** out_lbl)
{
    lv_color_t fg = theme_fg();
    lv_obj_t* b = lv_obj_create(bar);
    lv_obj_set_size(b, 92, 28);
    lv_obj_set_style_radius(b, 6, 0);
    lv_obj_set_style_bg_color(b, fg, 0);
    lv_obj_set_style_bg_opa(b, LV_OPA_10, 0);
    lv_obj_set_style_border_color(b, fg, 0);
    lv_obj_set_style_border_width(b, 1, 0);
    lv_obj_set_style_border_opa(b, LV_OPA_40, 0);
    lv_obj_add_flag(b, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t* l = lv_label_create(b);
    lv_label_set_text(l, txt);
    lv_obj_set_style_text_color(l, fg, 0);
    lv_obj_set_style_text_font(l, _F(), 0);
    lv_obj_center(l);
    if (out_lbl) *out_lbl = l;
    return b;
}

lv_obj_t* page_pomodoro_create(lv_obj_t* parent)
{
    if (s_ctx && s_ctx->page) _close();

    lv_color_t bg = theme_bg();
    lv_color_t fg = theme_fg();

    pomo_ctx_t* ctx = (pomo_ctx_t*)lv_malloc(sizeof(pomo_ctx_t));
    memset(ctx, 0, sizeof(*ctx));
    s_ctx = ctx;

    lv_obj_t* p = lv_obj_create(parent);
    lv_obj_set_size(p, 320, 240);
    lv_obj_center(p);
    lv_obj_set_style_bg_color(p, bg, 0);
    lv_obj_set_style_border_width(p, 0, 0);
    lv_obj_set_style_pad_all(p, 0, 0);
    lv_obj_remove_flag(p, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(p, _on_delete, LV_EVENT_DELETE, ctx);
    ctx->page = p;

    xb_statusbar_create(p);
    lv_obj_t* tb = xb_topbar_create(p, _T("番茄钟", "Pomodoro"), true);
    if (lv_obj_get_child_cnt(tb) >= 2)
        lv_obj_set_style_text_font(lv_obj_get_child(tb, 1), _F(), 0);
    lv_obj_add_flag(tb, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(tb, _on_back, LV_EVENT_CLICKED, NULL);

    // Big MM:SS countdown (font_zh_22 covers ASCII digits + colon).
    ctx->time_label = lv_label_create(p);
    lv_obj_set_style_text_font(ctx->time_label, &font_zh_22, 0);
    lv_obj_set_style_text_color(ctx->time_label, fg, 0);
    lv_obj_align(ctx->time_label, LV_ALIGN_TOP_MID, 0, 72);

    // Status / task line
    ctx->status_label = lv_label_create(p);
    lv_obj_set_style_text_font(ctx->status_label, _F(), 0);
    lv_obj_set_style_text_color(ctx->status_label, fg, 0);
    lv_obj_set_style_text_opa(ctx->status_label, LV_OPA_80, 0);
    lv_obj_set_width(ctx->status_label, 296);
    lv_label_set_long_mode(ctx->status_label, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(ctx->status_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(ctx->status_label, LV_ALIGN_TOP_MID, 0, 132);

    // Button row
    lv_obj_t* bar = lv_obj_create(p);
    lv_obj_set_size(bar, 320, 40);
    lv_obj_align(bar, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(bar, bg, 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(bar, 0, 0);
    lv_obj_set_style_pad_all(bar, 0, 0);
    lv_obj_set_flex_flow(bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bar, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(bar, 8, 0);
    lv_obj_remove_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

    _make_btn(bar, _primary_text(pomodoro_state()), _on_primary, &ctx->primary_lbl);
    _make_btn(bar, _T("重置", "Reset"), _on_reset, NULL);
    _make_btn(bar, _T("任务", "Task"), _on_task, NULL);

    // 250ms refresh (display only; ticking is in the main loop)
    ctx->timer = lv_timer_create(_refresh, 250, NULL);
    _refresh(NULL);

    expression_set_drawing_enabled(false);
    ESP_LOGI(TAG, "pomodoro page created (state=%d)", (int)pomodoro_state());
    return p;
}
