// page_text_input.cpp — Reusable full-screen keyboard input page (ASCII only)
//
// First use of lv_keyboard in this codebase. Layout (320×240):
//   topbar (0..28, back "<" = cancel) + textarea (y≈52) + Save button (y≈94)
//   + lv_keyboard pinned to bottom (~110px). Mirrors page_chat/page_model_picker
//   lifecycle: expression_set_drawing_enabled(false) on open, true on close.
#include "page_text_input.h"
#include "xb_widgets.h"
#include "theme_v3.h"
#include "expressions.h"
#include "font_zh_14.h"
#include <string.h>
#include <esp_log.h>

static const char* TAG = "TEXT_INPUT";

static const char* _T(const char* cn, const char* en) {
    extern bool s_lang_cn; return s_lang_cn ? cn : en;
}
static const lv_font_t* _F(void) {
    extern bool s_lang_cn;
    return s_lang_cn ? (const lv_font_t*)&font_zh_14 : &lv_font_montserrat_14;
}

typedef struct {
    lv_obj_t* page;
    lv_obj_t* ta;
    lv_obj_t* kb;
    page_text_input_done_cb on_done;
    void* user_ctx;
} ti_ctx_t;

static ti_ctx_t* s_ctx = NULL;

static void _on_delete(lv_event_t* e)
{
    ti_ctx_t* ctx = (ti_ctx_t*)lv_event_get_user_data(e);
    if (ctx) {
        if (s_ctx == ctx) s_ctx = NULL;
        lv_free(ctx);
    }
    // Return face drawing to the idle screen (covers cancel + save paths).
    expression_set_drawing_enabled(true);
}

static void _close(ti_ctx_t* ctx)
{
    if (ctx && ctx->page) {
        lv_obj_delete(ctx->page);  // triggers _on_delete (frees ctx, nulls s_ctx)
    }
}

/* Back "<" / cancel: close without invoking the callback. */
static void _on_back(lv_event_t* e)
{
    (void)e;
    _close(s_ctx);
}

/* Save: snapshot text, close the page, then run the callback.
 * The textarea's internal buffer is freed when the page is deleted, so we copy
 * into a stack buffer before closing. */
static void _on_save(lv_event_t* e)
{
    (void)e;
    if (!s_ctx) return;

    char buf[256];
    const char* t = s_ctx->ta ? lv_textarea_get_text(s_ctx->ta) : NULL;
    strlcpy(buf, t ? t : "", sizeof(buf));

    page_text_input_done_cb cb = s_ctx->on_done;
    void* uc = s_ctx->user_ctx;

    _close(s_ctx);  // frees ctx; expression drawing re-enabled in _on_delete

    if (cb) cb(buf, uc);
}

lv_obj_t* page_text_input_create(lv_obj_t* parent,
                                 const char* title,
                                 const char* initial_value,
                                 int max_len,
                                 bool password_mode,
                                 page_text_input_done_cb on_done_cb,
                                 void* user_ctx)
{
    // Only one input page at a time.
    if (s_ctx && s_ctx->page) _close(s_ctx);

    lv_color_t bg = theme_bg();
    lv_color_t fg = theme_fg();

    ti_ctx_t* ctx = (ti_ctx_t*)lv_malloc(sizeof(ti_ctx_t));
    memset(ctx, 0, sizeof(*ctx));
    ctx->on_done  = on_done_cb;
    ctx->user_ctx = user_ctx;
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

    // Statusbar + Topbar (back "<" = cancel)
    xb_statusbar_create(p);
    lv_obj_t* tb = xb_topbar_create(p, title ? title : _T("输入", "Input"), true);
    if (lv_obj_get_child_cnt(tb) >= 2)
        lv_obj_set_style_text_font(lv_obj_get_child(tb, 1), _F(), 0);
    lv_obj_add_flag(tb, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(tb, _on_back, LV_EVENT_CLICKED, NULL);

    // Textarea: single-line, password-capable
    lv_obj_t* ta = lv_textarea_create(p);
    lv_obj_set_size(ta, 296, 36);
    lv_obj_align(ta, LV_ALIGN_TOP_MID, 0, 52);
    lv_textarea_set_one_line(ta, true);
    if (max_len > 0) lv_textarea_set_max_length(ta, max_len);
    lv_textarea_set_password_mode(ta, password_mode);
    lv_textarea_set_placeholder_text(ta, _T("点此输入", "Tap to type"));
    lv_obj_set_style_text_font(ta, _F(), 0);
    lv_obj_set_style_text_color(ta, fg, 0);
    lv_obj_set_style_border_color(ta, fg, 0);
    lv_obj_set_style_border_opa(ta, LV_OPA_40, 0);
    lv_obj_set_style_bg_color(ta, bg, 0);
    lv_obj_set_style_bg_opa(ta, LV_OPA_COVER, 0);
    if (initial_value && initial_value[0]) lv_textarea_set_text(ta, initial_value);
    ctx->ta = ta;

    // Save button (accent fill, centered)
    lv_obj_t* save = lv_obj_create(p);
    lv_obj_set_size(save, 120, 30);
    lv_obj_align(save, LV_ALIGN_TOP_MID, 0, 94);
    lv_obj_set_style_radius(save, 6, 0);
    lv_obj_set_style_bg_color(save, fg, 0);
    lv_obj_set_style_bg_opa(save, LV_OPA_10, 0);
    lv_obj_set_style_border_color(save, fg, 0);
    lv_obj_set_style_border_width(save, 1, 0);
    lv_obj_set_style_border_opa(save, LV_OPA_40, 0);
    lv_obj_add_flag(save, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(save, _on_save, LV_EVENT_CLICKED, NULL);
    lv_obj_t* sl = lv_label_create(save);
    lv_label_set_text(sl, _T("保存", "Save"));
    lv_obj_set_style_text_color(sl, fg, 0);
    lv_obj_set_style_text_font(sl, _F(), 0);
    lv_obj_center(sl);

    // Keyboard pinned to bottom, bound to the textarea
    lv_obj_t* kb = lv_keyboard_create(p);
    lv_obj_set_size(kb, 320, 110);
    lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_keyboard_set_textarea(kb, ta);
    ctx->kb = kb;

    expression_set_drawing_enabled(false);
    ESP_LOGI(TAG, "text input page created (pw=%d)", (int)password_mode);
    return p;
}
