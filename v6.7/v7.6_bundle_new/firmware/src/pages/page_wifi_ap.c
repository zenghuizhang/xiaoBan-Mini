// page_wifi_ap.c — softAP onboarding (QR + state machine).
//   states: AP_OPEN → CONNECTING → SUCCESS / ERROR
// Real device hosts AP claw_xb_xxxx and a captive page;
// this UI just displays the QR + status and reacts to XB_EVT_WIFI_STATE.
#include "xb_pages.h"
#include "../core/xb_theme.h"
#include "../core/xb_event.h"
#include "../widgets/xb_widgets.h"

static lv_obj_t* g_status = NULL;

static void on_wifi(xb_event_id_t id, void* p, void* u) {
    (void)id; (void)u;
    if (!g_status) return;
    int s = (int)(intptr_t)p;
    const char* msg = s == 0 ? "等待手机扫码…"
                    : s == 1 ? "正在连接路由…"
                             : "连接成功";
    lv_label_set_text(g_status, msg);
    if (s == 2) xb_toast("WiFi 已连接", 0);
}

static void back_cb(lv_event_t* e) { (void)e; xb_router_back(); }

lv_obj_t* page_wifi_ap_create(lv_obj_t* parent) {
    const theme_t* th = xb_theme_get();
    lv_obj_t* root = lv_obj_create(parent);
    lv_obj_set_size(root, 320, 240);
    lv_obj_center(root);
    lv_obj_set_style_bg_opa(root, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_set_style_pad_all(root, 0, 0);

    xb_topbar(root, "WiFi 配网", back_cb);
    xb_statusbar(root);
    xb_statusbar_set_mode(XB_STATUSBAR_ALWAYS);

    // QR placeholder (real device fills with the captive URL)
    lv_obj_t* qr = lv_obj_create(root);
    lv_obj_set_size(qr, 110, 110);
    lv_obj_align(qr, LV_ALIGN_LEFT_MID, 20, 6);
    lv_obj_set_style_bg_color(qr, lv_color_white(), 0);
    lv_obj_set_style_border_color(qr, th->accent, 0);
    lv_obj_set_style_border_width(qr, 2, 0);
    lv_obj_set_style_radius(qr, 8, 0);

    lv_obj_t* hint = lv_label_create(root);
    lv_label_set_text(hint,
        "1. 手机连接热点\n   claw_xb_xxxx\n"
        "2. 浏览器自动弹出\n   配网页面\n"
        "3. 输入家中 WiFi 密码");
    lv_obj_set_style_text_color(hint, th->text, 0);
    lv_obj_align(hint, LV_ALIGN_RIGHT_MID, -12, -8);

    g_status = lv_label_create(root);
    lv_label_set_text(g_status, "等待手机扫码…");
    lv_obj_set_style_text_color(g_status, th->accent_hi, 0);
    lv_obj_align(g_status, LV_ALIGN_BOTTOM_MID, 0, -4);

    xb_event_subscribe(XB_EVT_WIFI_STATE, on_wifi, NULL);
    return root;
}
