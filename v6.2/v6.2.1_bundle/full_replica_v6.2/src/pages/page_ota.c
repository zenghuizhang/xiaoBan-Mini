// page_ota.c — 5-stage OTA: check / download / verify / apply / done.
// Listens to XB_EVT_OTA_PROGRESS (0..100) and advances stage automatically.
#include "xb_pages.h"
#include "../core/xb_theme.h"
#include "../core/xb_event.h"
#include "../widgets/xb_widgets.h"
#include <stdio.h>

extern const lv_image_dsc_t ic_download, ic_check_circle;

static const char* STAGES[5] = { "Checking", "Downloading", "Verifying", "Applying", "Done" };

typedef struct {
    lv_obj_t* bar;
    lv_obj_t* stage_lbl;
    lv_obj_t* pct_lbl;
    int stage;
} ota_ctx_t;

static void on_progress(xb_event_id_t id, void* payload, void* user) {
    (void)id;
    ota_ctx_t* ctx = (ota_ctx_t*)user;
    int pct = (int)(intptr_t)payload;
    if (pct < 0) pct = 0; if (pct > 100) pct = 100;
    int stage = pct < 5 ? 0 : pct < 70 ? 1 : pct < 90 ? 2 : pct < 100 ? 3 : 4;
    ctx->stage = stage;
    lv_bar_set_value(ctx->bar, pct, LV_ANIM_ON);
    lv_label_set_text(ctx->stage_lbl, STAGES[stage]);
    char b[8]; snprintf(b, sizeof(b), "%d%%", pct);
    lv_label_set_text(ctx->pct_lbl, b);
}

static void on_back(lv_event_t* e) { (void)e; xb_router_back(); }
static void on_delete(lv_event_t* e) {
    ota_ctx_t* ctx = (ota_ctx_t*)lv_event_get_user_data(e);
    if (ctx) lv_free(ctx);
}

lv_obj_t* page_ota_create(lv_obj_t* parent) {
    const theme_t* th = xb_theme_get();
    ota_ctx_t* ctx = lv_malloc(sizeof(ota_ctx_t));
    ctx->stage = 0;

    lv_obj_t* p = lv_obj_create(parent);
    lv_obj_set_size(p, 320, 240);
    lv_obj_center(p);
    lv_obj_set_style_bg_color(p, th->bg, 0);
    lv_obj_set_style_border_width(p, 0, 0);
    lv_obj_set_style_pad_all(p, 0, 0);
    lv_obj_remove_flag(p, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(p, on_delete, LV_EVENT_DELETE, ctx);

    xb_statusbar_create(p);
    lv_obj_t* tb = xb_topbar_create(p, "Update", true);
    lv_obj_add_flag(tb, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(tb, on_back, LV_EVENT_CLICKED, NULL);

    lv_obj_t* card = xb_card(p);
    lv_obj_set_size(card, 280, 130);
    lv_obj_align(card, LV_ALIGN_CENTER, 0, 12);

    lv_obj_t* ic = lv_image_create(card);
    lv_image_set_src(ic, &ic_download);
    lv_obj_set_style_image_recolor(ic, th->accent, 0);
    lv_obj_set_style_image_recolor_opa(ic, LV_OPA_COVER, 0);
    lv_obj_align(ic, LV_ALIGN_TOP_LEFT, 6, 6);

    ctx->stage_lbl = lv_label_create(card);
    lv_label_set_text(ctx->stage_lbl, STAGES[0]);
    lv_obj_set_style_text_color(ctx->stage_lbl, th->text, 0);
    lv_obj_align(ctx->stage_lbl, LV_ALIGN_TOP_LEFT, 36, 8);

    ctx->pct_lbl = lv_label_create(card);
    lv_label_set_text(ctx->pct_lbl, "0%");
    lv_obj_set_style_text_color(ctx->pct_lbl, th->accent, 0);
    lv_obj_align(ctx->pct_lbl, LV_ALIGN_TOP_RIGHT, -6, 8);

    ctx->bar = lv_bar_create(card);
    lv_obj_set_size(ctx->bar, 256, 8);
    lv_obj_align(ctx->bar, LV_ALIGN_CENTER, 0, 8);
    lv_obj_set_style_bg_color(ctx->bar, th->panel, 0);
    lv_obj_set_style_bg_color(ctx->bar, th->accent, LV_PART_INDICATOR);
    lv_bar_set_range(ctx->bar, 0, 100);

    lv_obj_t* tip = lv_label_create(card);
    lv_label_set_text(tip, "Keep device powered.");
    lv_obj_set_style_text_color(tip, th->text_dim, 0);
    lv_obj_align(tip, LV_ALIGN_BOTTOM_MID, 0, -6);

    xb_event_subscribe(XB_EVT_OTA_PROGRESS, on_progress, ctx);
    return p;
}
