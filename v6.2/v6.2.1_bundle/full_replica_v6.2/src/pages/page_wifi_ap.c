// page_wifi_ap.c — AP + Captive portal pairing flow.
// 4 visual states: ap (showing SSID/QR), connecting (spinner), success, error.
#include "xb_pages.h"
#include "../core/xb_theme.h"
#include "../core/xb_event.h"
#include "../widgets/xb_widgets.h"

extern const lv_image_dsc_t ic_wifi, ic_qr, ic_check_circle, ic_x_circle;

typedef enum { AP_IDLE, AP_CONNECTING, AP_SUCCESS, AP_ERROR } ap_state_t;

typedef struct {
    lv_obj_t* body;
    ap_state_t state;
} ap_ctx_t;

static void render(ap_ctx_t* ctx);

static void on_wifi(xb_event_id_t id, void* payload, void* user) {
    (void)id;
    ap_ctx_t* ctx = (ap_ctx_t*)user;
    int s = (int)(intptr_t)payload;
    ctx->state = (s == 2) ? AP_SUCCESS : (s == 1) ? AP_CONNECTING : AP_IDLE;
    render(ctx);
}

static void on_back(lv_event_t* e) { (void)e; xb_router_back(); }

static void on_retry(lv_event_t* e) {
    ap_ctx_t* ctx = (ap_ctx_t*)lv_obj_get_user_data(lv_event_get_target(e));
    ctx->state = AP_IDLE;
    render(ctx);
}

static void on_delete(lv_event_t* e) {
    ap_ctx_t* ctx = (ap_ctx_t*)lv_event_get_user_data(e);
    if (ctx) lv_free(ctx);
}

static void render(ap_ctx_t* ctx) {
    const theme_t* th = xb_theme_get();
    lv_obj_clean(ctx->body);

    switch (ctx->state) {
    case AP_IDLE: {
        lv_obj_t* card = xb_card(ctx->body);
        lv_obj_set_size(card, 280, 130);
        lv_obj_center(card);
        lv_obj_t* t = lv_label_create(card);
        lv_label_set_text(t, "Connect to AP:");
        lv_obj_set_style_text_color(t, th->text_dim, 0);
        lv_obj_align(t, LV_ALIGN_TOP_LEFT, 6, 4);
        lv_obj_t* ssid = lv_label_create(card);
        lv_label_set_text(ssid, "xiaobao-AP-FE21");
        lv_obj_set_style_text_color(ssid, th->accent, 0);
        lv_obj_align(ssid, LV_ALIGN_TOP_LEFT, 6, 26);
        lv_obj_t* url = lv_label_create(card);
        lv_label_set_text(url, "Open 192.168.4.1");
        lv_obj_set_style_text_color(url, th->text, 0);
        lv_obj_align(url, LV_ALIGN_TOP_LEFT, 6, 56);
        lv_obj_t* qr = lv_image_create(card);
        lv_image_set_src(qr, &ic_qr);
        lv_obj_set_style_image_recolor(qr, th->accent, 0);
        lv_obj_set_style_image_recolor_opa(qr, LV_OPA_COVER, 0);
        lv_obj_align(qr, LV_ALIGN_RIGHT_MID, -6, 0);
        break;
    }
    case AP_CONNECTING: {
        lv_obj_t* dots = xb_dot_loading_create(ctx->body);
        lv_obj_center(dots);
        lv_obj_t* l = lv_label_create(ctx->body);
        lv_label_set_text(l, "Joining your network…");
        lv_obj_set_style_text_color(l, th->text, 0);
        lv_obj_align(l, LV_ALIGN_CENTER, 0, 28);
        break;
    }
    case AP_SUCCESS: {
        lv_obj_t* ic = lv_image_create(ctx->body);
        lv_image_set_src(ic, &ic_check_circle);
        lv_obj_set_style_image_recolor(ic, th->success, 0);
        lv_obj_set_style_image_recolor_opa(ic, LV_OPA_COVER, 0);
        lv_obj_align(ic, LV_ALIGN_CENTER, 0, -16);
        lv_obj_t* l = lv_label_create(ctx->body);
        lv_label_set_text(l, "Connected");
        lv_obj_set_style_text_color(l, th->success, 0);
        lv_obj_align(l, LV_ALIGN_CENTER, 0, 22);
        break;
    }
    case AP_ERROR: {
        xb_error_inline(ctx->body, "Auth failed");
        lv_obj_t* btn = xb_button(ctx->body, "Retry", on_retry);
        lv_obj_set_user_data(btn, ctx);
        lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -10);
        break;
    }
    }
}

lv_obj_t* page_wifi_ap_create(lv_obj_t* parent) {
    const theme_t* th = xb_theme_get();
    ap_ctx_t* ctx = lv_malloc(sizeof(ap_ctx_t));
    ctx->state = AP_IDLE;

    lv_obj_t* p = lv_obj_create(parent);
    lv_obj_set_size(p, 320, 240);
    lv_obj_center(p);
    lv_obj_set_style_bg_color(p, th->bg, 0);
    lv_obj_set_style_border_width(p, 0, 0);
    lv_obj_set_style_pad_all(p, 0, 0);
    lv_obj_remove_flag(p, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(p, on_delete, LV_EVENT_DELETE, ctx);

    xb_statusbar_create(p);
    lv_obj_t* tb = xb_topbar_create(p, "Wi-Fi", true);
    lv_obj_add_flag(tb, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(tb, on_back, LV_EVENT_CLICKED, NULL);

    ctx->body = lv_obj_create(p);
    lv_obj_set_size(ctx->body, 320, 190);
    lv_obj_align(ctx->body, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_set_style_bg_opa(ctx->body, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ctx->body, 0, 0);
    lv_obj_remove_flag(ctx->body, LV_OBJ_FLAG_SCROLLABLE);

    render(ctx);
    xb_event_subscribe(XB_EVT_WIFI_STATE, on_wifi, ctx);
    return p;
}
