// xb_widgets.c — v7.6 implementation.
#include "xb_widgets.h"
#include "../core/xb_theme.h"
#include "../core/xb_event.h"
#include <string.h>

// ============================================================================
// card
// ============================================================================
lv_obj_t* xb_card(lv_obj_t* parent, int w, int h) {
    const theme_t* th = xb_theme_get();
    lv_obj_t* o = lv_obj_create(parent);
    lv_obj_set_size(o, w, h);
    lv_obj_set_style_radius(o, 16, 0);
    lv_obj_set_style_bg_color(o, th->panel, 0);
    lv_obj_set_style_bg_opa(o, xb_card_panel_opa(), 0);   // v7.6 rule
    lv_obj_set_style_border_color(o, th->border, 0);
    lv_obj_set_style_border_width(o, 1, 0);
    lv_obj_set_style_pad_all(o, 12, 0);
    lv_obj_remove_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    return o;
}

// ============================================================================
// button
// ============================================================================
lv_obj_t* xb_button(lv_obj_t* parent, const char* label, lv_event_cb_t cb) {
    const theme_t* th = xb_theme_get();
    lv_obj_t* b = lv_btn_create(parent);
    lv_obj_set_height(b, 36);
    lv_obj_set_style_radius(b, 18, 0);
    lv_obj_set_style_bg_color(b, th->panel, 0);
    lv_obj_set_style_bg_opa(b, xb_card_panel_opa(), 0);
    lv_obj_set_style_border_color(b, th->accent, 0);
    lv_obj_set_style_border_width(b, 1, 0);
    lv_obj_set_style_pad_hor(b, 14, 0);

    lv_obj_t* l = lv_label_create(b);
    lv_label_set_text(l, label ? label : "");
    lv_obj_set_style_text_color(l, th->text, 0);
    lv_obj_center(l);

    if (cb) lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, NULL);
    return b;
}

// ============================================================================
// topbar
// ============================================================================
lv_obj_t* xb_topbar(lv_obj_t* parent, const char* title, lv_event_cb_t back_cb) {
    const theme_t* th = xb_theme_get();
    lv_obj_t* bar = lv_obj_create(parent);
    lv_obj_set_size(bar, 320, 28);
    lv_obj_set_pos(bar, 0, 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(bar, 0, 0);
    lv_obj_set_style_pad_all(bar, 4, 0);
    lv_obj_remove_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* back = lv_btn_create(bar);
    lv_obj_set_size(back, 28, 22);
    lv_obj_set_pos(back, 4, 2);
    lv_obj_set_style_bg_opa(back, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(back, 0, 0);
    lv_obj_t* bl = lv_label_create(back);
    lv_label_set_text(bl, LV_SYMBOL_LEFT);
    lv_obj_set_style_text_color(bl, th->accent, 0);
    lv_obj_center(bl);
    if (back_cb) lv_obj_add_event_cb(back, back_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t* t = lv_label_create(bar);
    lv_label_set_text(t, title ? title : "");
    lv_obj_set_style_text_color(t, th->text, 0);
    lv_obj_align(t, LV_ALIGN_CENTER, 0, 0);
    return bar;
}

// ============================================================================
// statusbar — 4 modes, auto-hide transient timer
// ============================================================================
static struct {
    lv_obj_t* root;
    lv_obj_t* lbl_wifi;
    lv_obj_t* lbl_bat;
    lv_obj_t* lbl_ai;
    statusbar_mode_t mode;
    lv_timer_t* hide_timer;
    int wifi_state;     // 0/1/2
    int battery;        // %
    bool plug;
} S = {0};

static void sb_repaint(void) {
    if (!S.root) return;
    const theme_t* th = xb_theme_get();
    bool full   = (S.mode != XB_STATUSBAR_IDLE);
    bool danger = (S.mode == XB_STATUSBAR_CRITICAL);

    lv_color_t col = danger ? th->danger : th->text_dim;
    char buf[16];

    // WiFi dot
    if (S.lbl_wifi) {
        lv_label_set_text(S.lbl_wifi,
            S.wifi_state == 2 ? LV_SYMBOL_WIFI :
            S.wifi_state == 1 ? LV_SYMBOL_LOOP : LV_SYMBOL_CLOSE);
        lv_obj_set_style_text_color(S.lbl_wifi, col, 0);
    }
    // Battery
    if (S.lbl_bat) {
        snprintf(buf, sizeof(buf), "%s %d%%",
                 S.plug ? LV_SYMBOL_CHARGE :
                 (S.battery > 60 ? LV_SYMBOL_BATTERY_FULL :
                  S.battery > 30 ? LV_SYMBOL_BATTERY_2    :
                                   LV_SYMBOL_BATTERY_EMPTY),
                 S.battery);
        lv_label_set_text(S.lbl_bat, buf);
        lv_obj_set_style_text_color(S.lbl_bat, col, 0);
        lv_obj_set_style_opa(S.lbl_bat, full ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
    }
    // AI dot
    if (S.lbl_ai) {
        lv_obj_set_style_text_color(S.lbl_ai, th->accent_hi, 0);
        lv_obj_set_style_opa(S.lbl_ai, LV_OPA_COVER, 0);
    }
}

static void sb_hide_cb(lv_timer_t* t) {
    lv_timer_del(t);
    S.hide_timer = NULL;
    S.mode = XB_STATUSBAR_IDLE;
    sb_repaint();
}

void xb_statusbar_set_mode(statusbar_mode_t m) {
    S.mode = m;
    if (S.hide_timer) { lv_timer_del(S.hide_timer); S.hide_timer = NULL; }
    if (m == XB_STATUSBAR_TRANSIENT) {
        S.hide_timer = lv_timer_create(sb_hide_cb, 2400, NULL);
    }
    sb_repaint();
}

static void sb_evt(xb_event_id_t id, void* p, void* u) {
    (void)u;
    switch (id) {
        case XB_EVT_WIFI_STATE:    S.wifi_state = (int)(intptr_t)p; break;
        case XB_EVT_BATTERY:       S.battery    = (int)(intptr_t)p; break;
        case XB_EVT_PLUG:          S.plug       = (bool)(intptr_t)p; break;
        case XB_EVT_STATUSBAR_MODE: xb_statusbar_set_mode((statusbar_mode_t)(intptr_t)p); return;
        case XB_EVT_STATUSBAR_PEEK: xb_statusbar_set_mode(XB_STATUSBAR_TRANSIENT); return;
        case XB_EVT_THEME_CHANGED: break;
        default: return;
    }
    sb_repaint();
}

lv_obj_t* xb_statusbar(lv_obj_t* parent) {
    S.root = lv_obj_create(parent);
    lv_obj_set_size(S.root, 320, 16);
    lv_obj_set_pos(S.root, 0, 0);
    lv_obj_set_style_bg_opa(S.root, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(S.root, 0, 0);
    lv_obj_set_style_pad_all(S.root, 2, 0);
    lv_obj_remove_flag(S.root, LV_OBJ_FLAG_SCROLLABLE);

    S.lbl_wifi = lv_label_create(S.root); lv_obj_set_pos(S.lbl_wifi, 4, 0);
    S.lbl_ai   = lv_label_create(S.root); lv_obj_set_pos(S.lbl_ai,  150, 0);
    lv_label_set_text(S.lbl_ai, LV_SYMBOL_BELL);
    S.lbl_bat  = lv_label_create(S.root); lv_obj_align(S.lbl_bat, LV_ALIGN_RIGHT_MID, -4, 0);
    lv_label_set_text(S.lbl_bat, "");

    S.mode = XB_STATUSBAR_IDLE;
    S.wifi_state = 2; S.battery = 100; S.plug = false;

    xb_event_subscribe(XB_EVT_WIFI_STATE,     sb_evt, NULL);
    xb_event_subscribe(XB_EVT_BATTERY,        sb_evt, NULL);
    xb_event_subscribe(XB_EVT_PLUG,           sb_evt, NULL);
    xb_event_subscribe(XB_EVT_STATUSBAR_MODE, sb_evt, NULL);
    xb_event_subscribe(XB_EVT_STATUSBAR_PEEK, sb_evt, NULL);
    xb_event_subscribe(XB_EVT_THEME_CHANGED,  sb_evt, NULL);
    sb_repaint();
    return S.root;
}

// ============================================================================
// dialog bubble — top-right floating panel
// ============================================================================
lv_obj_t* xb_dialog_bubble(lv_obj_t* parent, const bubble_evt_t* ev) {
    const theme_t* th = xb_theme_get();
    lv_obj_t* b = lv_obj_create(parent);
    lv_obj_set_size(b, 196, ev && ev->kind == XB_BUBBLE_SUGGEST ? 68 : 52);
    lv_obj_align(b, LV_ALIGN_TOP_RIGHT, -12, 22);
    lv_obj_set_style_radius(b, 12, 0);
    lv_obj_set_style_bg_color(b, th->panel, 0);
    lv_obj_set_style_bg_opa(b, xb_card_panel_opa(), 0);
    lv_obj_set_style_border_color(b, th->accent_dim, 0);
    lv_obj_set_style_border_width(b, 1, 0);
    lv_obj_set_style_pad_all(b, 8, 0);
    lv_obj_remove_flag(b, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* l = lv_label_create(b);
    lv_label_set_text(l, ev && ev->text ? ev->text : "");
    lv_obj_set_style_text_color(l, th->text, 0);
    lv_label_set_long_mode(l, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(l, 180);

    if (ev && ev->kind == XB_BUBBLE_SUGGEST && ev->suggest_label) {
        lv_obj_t* s = lv_btn_create(b);
        lv_obj_set_size(s, 80, 22);
        lv_obj_align(s, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
        lv_obj_set_style_radius(s, 11, 0);
        lv_obj_set_style_bg_color(s, th->accent, 0);
        lv_obj_t* sl = lv_label_create(s);
        lv_label_set_text(sl, ev->suggest_label);
        lv_obj_center(sl);
    }
    return b;
}

// ============================================================================
// RGB strip
// ============================================================================
static struct { lv_obj_t* bar; rgb_scene_t s; } R = {0};

static lv_color_t rgb_color(rgb_scene_t s) {
    const theme_t* th = xb_theme_get();
    switch (s) {
        case XB_RGB_LONELY:   return lv_color_make(0xFA, 0xCC, 0x15);  // #FACC15
        case XB_RGB_OTA:      return lv_color_make(0xA8, 0x55, 0xF7);  // #A855F7
        case XB_RGB_ERROR:    return lv_color_make(0xEF, 0x44, 0x44);  // #EF4444
        case XB_RGB_CALL:     return lv_color_make(0x22, 0xC5, 0x5E);  // #22C55E
        case XB_RGB_LOW_BAT:  return lv_color_make(0xF9, 0x73, 0x16);  // #F97316
        case XB_RGB_OVERHEAT: return lv_color_make(0xDC, 0x26, 0x26);  // #DC2626
        case XB_RGB_IDLE:
        default:              return th->accent_hi;
    }
}

void xb_rgb_strip_set(rgb_scene_t s) {
    R.s = s;
    if (!R.bar) return;
    lv_color_t c = rgb_color(s);
    lv_obj_set_style_bg_color(R.bar, c, 0);
    lv_obj_set_style_shadow_color(R.bar, c, 0);
}

static void rgb_evt(xb_event_id_t id, void* p, void* u) {
    (void)id; (void)u;
    xb_rgb_strip_set((rgb_scene_t)(intptr_t)p);
}

lv_obj_t* xb_rgb_strip(lv_obj_t* parent) {
    R.bar = lv_obj_create(parent);
    lv_obj_set_size(R.bar, 320, 2);     // ~1.5px on CoreS3
    lv_obj_set_pos(R.bar, 0, 0);
    lv_obj_set_style_border_width(R.bar, 0, 0);
    lv_obj_set_style_radius(R.bar, 1, 0);
    lv_obj_set_style_shadow_width(R.bar, 8, 0);
    lv_obj_set_style_shadow_opa(R.bar, 0x80, 0);
    lv_obj_set_style_pad_all(R.bar, 0, 0);
    lv_obj_remove_flag(R.bar, LV_OBJ_FLAG_SCROLLABLE);
    xb_rgb_strip_set(XB_RGB_IDLE);
    xb_event_subscribe(XB_EVT_RGB_SCENE, rgb_evt, NULL);
    return R.bar;
}

// ============================================================================
// dot loading
// ============================================================================
static void dot_anim(void* var, int32_t v) {
    lv_obj_set_style_opa((lv_obj_t*)var, (lv_opa_t)v, 0);
}

lv_obj_t* xb_dot_loading(lv_obj_t* parent) {
    const theme_t* th = xb_theme_get();
    lv_obj_t* row = lv_obj_create(parent);
    lv_obj_set_size(row, 60, 14);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    for (int i = 0; i < 3; ++i) {
        lv_obj_t* d = lv_obj_create(row);
        lv_obj_set_size(d, 8, 8);
        lv_obj_set_pos(d, i * 18, 3);
        lv_obj_set_style_radius(d, 4, 0);
        lv_obj_set_style_bg_color(d, th->accent_hi, 0);
        lv_obj_set_style_border_width(d, 0, 0);

        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, d);
        lv_anim_set_values(&a, 0x30, 0xFF);
        lv_anim_set_duration(&a, 600);
        lv_anim_set_playback_duration(&a, 600);
        lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
        lv_anim_set_delay(&a, i * 150);
        lv_anim_set_exec_cb(&a, dot_anim);
        lv_anim_start(&a);
    }
    return row;
}

// ============================================================================
// toast
// ============================================================================
static void toast_close_cb(lv_timer_t* t) {
    lv_obj_t* o = (lv_obj_t*)lv_timer_get_user_data(t);
    if (o) lv_obj_delete(o);
    lv_timer_del(t);
}

void xb_toast(const char* msg, int ms) {
    if (!msg) return;
    const theme_t* th = xb_theme_get();
    lv_obj_t* scr = lv_screen_active();
    lv_obj_t* o = lv_obj_create(scr);
    lv_obj_set_size(o, 220, 32);
    lv_obj_align(o, LV_ALIGN_BOTTOM_MID, 0, -24);
    lv_obj_set_style_radius(o, 16, 0);
    lv_obj_set_style_bg_color(o, th->panel, 0);
    lv_obj_set_style_bg_opa(o, 0xE0, 0);
    lv_obj_set_style_border_color(o, th->accent_dim, 0);
    lv_obj_set_style_border_width(o, 1, 0);
    lv_obj_remove_flag(o, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* l = lv_label_create(o);
    lv_label_set_text(l, msg);
    lv_obj_set_style_text_color(l, th->text, 0);
    lv_obj_center(l);

    lv_timer_t* t = lv_timer_create(toast_close_cb, ms > 0 ? ms : 2200, NULL);
    lv_timer_set_user_data(t, o);
    lv_timer_set_repeat_count(t, 1);
}

// ============================================================================
// inline error
// ============================================================================
lv_obj_t* xb_error_inline(lv_obj_t* parent, const char* msg, lv_event_cb_t retry_cb) {
    const theme_t* th = xb_theme_get();
    lv_obj_t* box = xb_card(parent, 280, 64);
    lv_obj_set_style_border_color(box, th->danger, 0);

    lv_obj_t* l = lv_label_create(box);
    lv_label_set_text(l, msg ? msg : "Error");
    lv_obj_set_style_text_color(l, th->danger, 0);
    lv_obj_align(l, LV_ALIGN_LEFT_MID, 0, 0);

    if (retry_cb) {
        lv_obj_t* r = xb_button(box, "重试", retry_cb);
        lv_obj_align(r, LV_ALIGN_RIGHT_MID, 0, 0);
    }
    return box;
}

// ============================================================================
// modal
// ============================================================================
typedef struct { lv_event_cb_t ok, cancel; } modal_cbs_t;

static void modal_ok_cb(lv_event_t* e) {
    lv_obj_t* btn = lv_event_get_target_obj(e);
    lv_obj_t* m = lv_obj_get_parent(btn);
    modal_cbs_t* cbs = (modal_cbs_t*)lv_obj_get_user_data(m);
    if (cbs && cbs->ok) cbs->ok(e);
    xb_modal_close(m);
}
static void modal_cancel_cb(lv_event_t* e) {
    lv_obj_t* btn = lv_event_get_target_obj(e);
    lv_obj_t* m = lv_obj_get_parent(btn);
    modal_cbs_t* cbs = (modal_cbs_t*)lv_obj_get_user_data(m);
    if (cbs && cbs->cancel) cbs->cancel(e);
    xb_modal_close(m);
}

lv_obj_t* xb_modal(const char* title, const char* body,
                   lv_event_cb_t on_ok, lv_event_cb_t on_cancel) {
    const theme_t* th = xb_theme_get();
    lv_obj_t* m = lv_obj_create(lv_screen_active());
    lv_obj_set_size(m, 260, 140);
    lv_obj_center(m);
    lv_obj_set_style_radius(m, 16, 0);
    lv_obj_set_style_bg_color(m, th->panel, 0);
    lv_obj_set_style_bg_opa(m, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(m, th->accent, 0);
    lv_obj_set_style_border_width(m, 1, 0);
    lv_obj_set_style_pad_all(m, 12, 0);
    lv_obj_remove_flag(m, LV_OBJ_FLAG_SCROLLABLE);

    modal_cbs_t* cbs = lv_malloc(sizeof(modal_cbs_t));
    cbs->ok = on_ok; cbs->cancel = on_cancel;
    lv_obj_set_user_data(m, cbs);

    lv_obj_t* th_lbl = lv_label_create(m);
    lv_label_set_text(th_lbl, title ? title : "");
    lv_obj_set_style_text_color(th_lbl, th->accent_hi, 0);
    lv_obj_set_pos(th_lbl, 0, 0);

    lv_obj_t* bd = lv_label_create(m);
    lv_label_set_text(bd, body ? body : "");
    lv_obj_set_style_text_color(bd, th->text, 0);
    lv_label_set_long_mode(bd, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(bd, 236);
    lv_obj_set_pos(bd, 0, 24);

    lv_obj_t* ok = xb_button(m, "确认", modal_ok_cb);
    lv_obj_align(ok, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    lv_obj_t* ca = xb_button(m, "取消", modal_cancel_cb);
    lv_obj_align(ca, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    return m;
}

void xb_modal_close(lv_obj_t* modal) {
    if (!modal) return;
    modal_cbs_t* cbs = (modal_cbs_t*)lv_obj_get_user_data(modal);
    if (cbs) lv_free(cbs);
    lv_obj_delete(modal);
}
