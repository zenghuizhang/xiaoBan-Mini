// xb_widgets.h — shared widgets: statusbar, top-app-bar, dot loading, error toast
#pragma once
#include "lvgl.h"

lv_obj_t* xb_statusbar_create(lv_obj_t* parent);          // 28px global top bar
lv_obj_t* xb_topbar_create(lv_obj_t* parent, const char* title, bool back);

lv_obj_t* xb_dot_loading_create(lv_obj_t* parent);        // 3-dot 8x8 200ms wave
lv_obj_t* xb_toast(lv_obj_t* parent, const char* text, uint32_t ms);
lv_obj_t* xb_error_inline(lv_obj_t* parent, const char* text);

lv_obj_t* xb_button(lv_obj_t* parent, const char* text, lv_event_cb_t cb);
lv_obj_t* xb_card(lv_obj_t* parent);                      // panel @ opa 30%
