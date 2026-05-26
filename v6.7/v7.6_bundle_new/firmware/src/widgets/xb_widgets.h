// xb_widgets.h — v7.6 shared widget toolkit.
// All widgets are theme-aware: they re-paint on XB_EVT_THEME_CHANGED.
#pragma once
#include "lvgl.h"

// ---- card -------------------------------------------------------------------
// Returns a panel container with v7.6 alpha rule applied (opaque on light
// themes, 30% on dark themes). Use as parent for page content.
lv_obj_t* xb_card(lv_obj_t* parent, int w, int h);

// ---- button -----------------------------------------------------------------
// Pill button with accent border + icon-aware text.
lv_obj_t* xb_button(lv_obj_t* parent, const char* label, lv_event_cb_t cb);

// ---- topbar -----------------------------------------------------------------
// Page header: back chevron + centered title.
lv_obj_t* xb_topbar(lv_obj_t* parent, const char* title, lv_event_cb_t back_cb);

// ---- statusbar --------------------------------------------------------------
// 4 visibility modes (v7.6):
//   IDLE       — only WiFi/AI state dots (default home)
//   TRANSIENT  — full row, auto-hides after 2.4s
//   CRITICAL   — full row + danger color (low battery / no wifi)
//   ALWAYS     — full row, never hides (settings / wifi pages)
typedef enum {
    XB_STATUSBAR_IDLE = 0,
    XB_STATUSBAR_TRANSIENT,
    XB_STATUSBAR_CRITICAL,
    XB_STATUSBAR_ALWAYS,
} statusbar_mode_t;

lv_obj_t* xb_statusbar(lv_obj_t* parent);     // subscribes to events; auto-destroys on parent delete
void      xb_statusbar_set_mode(statusbar_mode_t m);

// ---- dialog bubble ----------------------------------------------------------
typedef enum { XB_BUBBLE_TEXT, XB_BUBBLE_SUGGEST } bubble_kind_t;
typedef struct {
    bubble_kind_t kind;
    const char*   text;
    const char*   suggest_label;  // only for SUGGEST
} bubble_evt_t;

lv_obj_t* xb_dialog_bubble(lv_obj_t* parent, const bubble_evt_t* ev);

// ---- RGB strip --------------------------------------------------------------
// 1.5px high strip at top edge. Scene-mapped color (lonely=#FACC15, ota=#A855F7,
// error=#EF4444, idle=accent_hi).
typedef enum {
    XB_RGB_IDLE = 0, XB_RGB_LONELY, XB_RGB_OTA,
    XB_RGB_ERROR, XB_RGB_CALL, XB_RGB_LOW_BAT, XB_RGB_OVERHEAT,
} rgb_scene_t;

lv_obj_t* xb_rgb_strip(lv_obj_t* parent);
void      xb_rgb_strip_set(rgb_scene_t s);

// ---- dot loading ------------------------------------------------------------
lv_obj_t* xb_dot_loading(lv_obj_t* parent);

// ---- toast ------------------------------------------------------------------
// Pops a transient banner. Auto-hides after `ms`. Pass 0 for default 2200ms.
void xb_toast(const char* msg, int ms);

// ---- inline error -----------------------------------------------------------
lv_obj_t* xb_error_inline(lv_obj_t* parent, const char* msg, lv_event_cb_t retry_cb);

// ---- modal ------------------------------------------------------------------
// Centered modal with title + body + OK/Cancel.
lv_obj_t* xb_modal(const char* title, const char* body,
                   lv_event_cb_t on_ok, lv_event_cb_t on_cancel);
void      xb_modal_close(lv_obj_t* modal);
