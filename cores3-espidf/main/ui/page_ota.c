// page_ota.c — 5-stage OTA: check/download/verify/apply/done
// 对齐 full_replica_v6.2 page_ota.c
#include "xb_widgets.h"
#include "theme_v3.h"
#include "expressions.h"
#include <stdio.h>
#include <esp_log.h>

static const char* TAG = "OTA";
static const char* STAGES[5] = {"Checking", "Downloading", "Verifying", "Applying", "Done"};

typedef struct {
    lv_obj_t* page;
    lv_obj_t* bar;
    lv_obj_t* stage_lbl;
    lv_obj_t* pct_lbl;
    int stage;
    int pct;
} ota_ctx_t;

static ota_ctx_t* s_ota_ctx = NULL;

static void _ota_render(ota_ctx_t* ctx)
{
    lv_bar_set_value(ctx->bar, ctx->pct, LV_ANIM_ON);
    lv_label_set_text(ctx->stage_lbl, STAGES[ctx->stage]);
    char b[8]; snprintf(b, sizeof(b), "%d%%", ctx->pct);
    lv_label_set_text(ctx->pct_lbl, b);
}

static void on_back(lv_event_t* e) {
    (void)e;
    if (s_ota_ctx && s_ota_ctx->page) {
        lv_obj_delete(s_ota_ctx->page);
        s_ota_ctx = NULL;
    }
}

static void on_delete(lv_event_t* e) {
    ota_ctx_t* ctx = (ota_ctx_t*)lv_event_get_user_data(e);
    if (ctx) { lv_free(ctx); s_ota_ctx = NULL; }
}

static void on_start(lv_event_t* e) {
    (void)e;
    ESP_LOGI(TAG, "OTA simulation started");
    // Simulation: advance through stages
}

lv_obj_t* page_ota_create(lv_obj_t* parent) {
    lv_color_t fg = theme_fg();
    lv_color_t bg = theme_bg();
    ota_ctx_t* ctx = (ota_ctx_t*)lv_malloc(sizeof(ota_ctx_t));
    ctx->stage = 0;
    ctx->pct = 0;
    s_ota_ctx = ctx;

    lv_obj_t* p = lv_obj_create(parent);
    lv_obj_set_size(p, 320, 240);
    lv_obj_center(p);
    lv_obj_set_style_bg_color(p, bg, 0);
    lv_obj_set_style_border_width(p, 0, 0);
    lv_obj_set_style_pad_all(p, 0, 0);
    lv_obj_remove_flag(p, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(p, on_delete, LV_EVENT_DELETE, ctx);
    ctx->page = p;

    xb_statusbar_create(p);
    lv_obj_t* tb = xb_topbar_create(p, "Update", true);
    lv_obj_add_flag(tb, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(tb, on_back, LV_EVENT_CLICKED, NULL);

    // Card
    lv_obj_t* card = xb_card(p);
    lv_obj_set_size(card, 280, 130);
    lv_obj_align(card, LV_ALIGN_CENTER, 0, 12);

    // Download icon (label placeholder)
    lv_obj_t* ic = lv_label_create(card);
    lv_label_set_text(ic, ">>");
    lv_obj_set_style_text_color(ic, fg, 0);
    lv_obj_align(ic, LV_ALIGN_TOP_LEFT, 6, 6);

    ctx->stage_lbl = lv_label_create(card);
    lv_label_set_text(ctx->stage_lbl, STAGES[0]);
    lv_obj_set_style_text_color(ctx->stage_lbl, fg, 0);
    lv_obj_align(ctx->stage_lbl, LV_ALIGN_TOP_LEFT, 36, 8);

    ctx->pct_lbl = lv_label_create(card);
    lv_label_set_text(ctx->pct_lbl, "0%");
    lv_obj_set_style_text_color(ctx->pct_lbl, fg, 0);
    lv_obj_align(ctx->pct_lbl, LV_ALIGN_TOP_RIGHT, -6, 8);

    // Progress bar
    ctx->bar = lv_bar_create(card);
    lv_obj_set_size(ctx->bar, 256, 8);
    lv_obj_align(ctx->bar, LV_ALIGN_CENTER, 0, 8);
    lv_obj_set_style_bg_color(ctx->bar, fg, 0);
    lv_obj_set_style_bg_opa(ctx->bar, LV_OPA_20, 0);
    lv_obj_set_style_bg_color(ctx->bar, fg, LV_PART_INDICATOR);
    lv_bar_set_range(ctx->bar, 0, 100);

    // Tip
    lv_obj_t* tip = lv_label_create(card);
    lv_label_set_text(tip, "Keep device powered.");
    lv_obj_set_style_text_color(tip, fg, 0);
    lv_obj_set_style_text_opa(tip, LV_OPA_50, 0);
    lv_obj_align(tip, LV_ALIGN_BOTTOM_MID, 0, -6);

    // Start button
    lv_obj_t* btn = xb_button(p, "Start Update", on_start);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -40);

    ESP_LOGI(TAG, "OTA page created");
    return p;
}

// Public API for updating progress from OTA task
void page_ota_update_progress(int pct) {
    if (!s_ota_ctx) return;
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    s_ota_ctx->pct = pct;
    s_ota_ctx->stage = pct < 5 ? 0 : pct < 70 ? 1 : pct < 90 ? 2 : pct < 100 ? 3 : 4;
    _ota_render(s_ota_ctx);
}
