// page_todo.cpp — Todo list page (add / toggle / delete / clear-done).
#include "page_todo.h"
#include "xb_widgets.h"
#include "theme_v3.h"
#include "expressions.h"
#include "font_zh_14.h"
#include "todo_store.h"
#include "page_text_input.h"
#include <string.h>
#include <esp_log.h>

static const char* TAG = "TODO";

static const char* _T(const char* cn, const char* en) {
    extern bool s_lang_cn; return s_lang_cn ? cn : en;
}
static const lv_font_t* _F(void) {
    extern bool s_lang_cn;
    return s_lang_cn ? (const lv_font_t*)&font_zh_14 : &lv_font_montserrat_14;
}

typedef struct {
    lv_obj_t* page;
    lv_obj_t* list;   // scrollable flex-column container
} todo_ctx_t;

static todo_ctx_t* s_ctx = NULL;

// Forward decls
static void _rebuild_rows(void);
static void _open_add(void);

static void _on_delete(lv_event_t* e)
{
    todo_ctx_t* ctx = (todo_ctx_t*)lv_event_get_user_data(e);
    if (ctx) {
        if (s_ctx == ctx) s_ctx = NULL;
        lv_free(ctx);
    }
    expression_set_drawing_enabled(true);
}

static void _close(void)
{
    if (s_ctx && s_ctx->page) lv_obj_delete(s_ctx->page);
}

static void _on_back(lv_event_t* e)
{
    (void)e;
    _close();
}

// Row carries its index in user_data.
static void _on_row_click(lv_event_t* e)
{
    if (!s_ctx) return;
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    todo_store_toggle(idx);
    _rebuild_rows();
}

static void _on_del_click(lv_event_t* e)
{
    if (!s_ctx) return;
    // index is stored on the delete button's own user_data
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    todo_store_delete(idx);
    _rebuild_rows();
}

static void _on_add_click(lv_event_t* e)
{
    (void)e;
    _open_add();
}

static void _on_clear_click(lv_event_t* e)
{
    (void)e;
    if (!s_ctx) return;
    todo_store_clear_done();
    _rebuild_rows();
}

// Callback from page_text_input after the user types a new todo.
static void _on_add_done(const char* text, void* ctx)
{
    (void)ctx;
    todo_store_add(text);
    // page_text_input already closed; re-create the todo page on the screen.
    page_todo_create(lv_screen_active());
}

static void _open_add(void)
{
    if (!s_ctx) return;
    _close();
    page_text_input_create(lv_screen_active(),
        _T("新建待办", "New Todo"),
        "", TODO_TEXT_MAX - 1, false,
        _on_add_done, NULL);
}

// Build one row: [✓/○] text ............ [×]
static void _add_row(lv_obj_t* list, int idx, bool done, const char* text)
{
    lv_color_t fg = theme_fg();
    lv_color_t dim = theme_get_colors()->text_dim;

    lv_obj_t* row = xb_card(list);
    lv_obj_set_width(row, 296);
    lv_obj_set_height(row, 30);
    lv_obj_set_style_pad_all(row, 4, 0);
    lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(row, _on_row_click, LV_EVENT_CLICKED, (void*)(intptr_t)idx);

    // Status marker
    lv_obj_t* st = lv_label_create(row);
    lv_label_set_text(st, done ? LV_SYMBOL_OK : LV_SYMBOL_REFRESH);
    lv_obj_set_style_text_color(st, done ? dim : fg, 0);
    lv_obj_align(st, LV_ALIGN_LEFT_MID, 4, 0);

    // Text
    lv_obj_t* t = lv_label_create(row);
    lv_label_set_long_mode(t, LV_LABEL_LONG_DOT);
    lv_obj_set_width(t, 230);
    lv_label_set_text(t, text);
    lv_obj_set_style_text_color(t, done ? dim : fg, 0);
    lv_obj_set_style_text_font(t, _F(), 0);
    lv_obj_align(t, LV_ALIGN_LEFT_MID, 24, 0);

    // Delete button (×) — swallow its own click so it doesn't toggle the row.
    lv_obj_t* del = lv_label_create(row);
    lv_label_set_text(del, LV_SYMBOL_CLOSE);
    lv_obj_set_style_text_color(del, dim, 0);
    lv_obj_align(del, LV_ALIGN_RIGHT_MID, -4, 0);
    lv_obj_add_flag(del, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(del, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_event_cb(del, _on_del_click, LV_EVENT_CLICKED, (void*)(intptr_t)idx);
}

static void _rebuild_rows(void)
{
    if (!s_ctx || !s_ctx->list) return;
    lv_obj_clean(s_ctx->list);

    int n = todo_store_count();
    for (int i = 0; i < n; i++) {
        bool done = false;
        const char* text = todo_store_get(i, &done);
        if (text) _add_row(s_ctx->list, i, done, text);
    }

    if (n == 0) {
        lv_color_t fg = theme_fg();
        lv_obj_t* empty = lv_label_create(s_ctx->list);
        lv_label_set_text(empty, _T("暂无待办，点「添加」", "No todos. Tap Add."));
        lv_obj_set_style_text_color(empty, fg, 0);
        lv_obj_set_style_text_font(empty, _F(), 0);
        lv_obj_set_style_text_opa(empty, LV_OPA_60, 0);
        lv_obj_set_style_pad_top(empty, 30, 0);
        lv_obj_set_width(empty, 296);
        lv_label_set_long_mode(empty, LV_LABEL_LONG_WRAP);
        lv_obj_set_style_text_align(empty, LV_TEXT_ALIGN_CENTER, 0);
    }
}

lv_obj_t* page_todo_create(lv_obj_t* parent)
{
    if (s_ctx && s_ctx->page) _close();

    lv_color_t bg = theme_bg();
    lv_color_t fg = theme_fg();

    todo_ctx_t* ctx = (todo_ctx_t*)lv_malloc(sizeof(todo_ctx_t));
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
    lv_obj_t* tb = xb_topbar_create(p, _T("待办", "Todo"), true);
    if (lv_obj_get_child_cnt(tb) >= 2)
        lv_obj_set_style_text_font(lv_obj_get_child(tb, 1), _F(), 0);
    lv_obj_add_flag(tb, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(tb, _on_back, LV_EVENT_CLICKED, NULL);

    // Scrollable list (50..200)
    lv_obj_t* list = lv_obj_create(p);
    lv_obj_set_size(list, 320, 150);
    lv_obj_align(list, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(list, 0, 0);
    lv_obj_set_style_pad_all(list, 8, 0);
    lv_obj_set_style_pad_gap(list, 4, 0);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_add_flag(list, LV_OBJ_FLAG_SCROLLABLE);
    ctx->list = list;

    _rebuild_rows();

    // Bottom bar: [添加] [清除已完成]
    lv_obj_t* bar = lv_obj_create(p);
    lv_obj_set_size(bar, 320, 36);
    lv_obj_align(bar, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(bar, bg, 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(bar, 0, 0);
    lv_obj_set_style_pad_all(bar, 0, 0);
    lv_obj_set_flex_flow(bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bar, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(bar, 10, 0);
    lv_obj_remove_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

    auto make_btn = [&](const char* txt, lv_event_cb_t cb) {
        lv_obj_t* b = lv_obj_create(bar);
        lv_obj_set_size(b, 130, 28);
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
        return b;
    };
    make_btn(_T("添加", "Add"), _on_add_click);
    make_btn(_T("清除已完成", "Clear Done"), _on_clear_click);

    expression_set_drawing_enabled(false);
    ESP_LOGI(TAG, "todo page created (%d items)", todo_store_count());
    return p;
}
