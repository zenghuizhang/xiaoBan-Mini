// xb_widgets.h — 对齐 full_replica_v6.2 widgets
#pragma once
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

lv_obj_t* xb_card(lv_obj_t* parent);
lv_obj_t* xb_button(lv_obj_t* parent, const char* text, lv_event_cb_t cb);
lv_obj_t* xb_statusbar_create(lv_obj_t* parent);
lv_obj_t* xb_topbar_create(lv_obj_t* parent, const char* title, bool back);
lv_obj_t* xb_dot_loading_create(lv_obj_t* parent);
lv_obj_t* xb_error_inline(lv_obj_t* parent, const char* text);
lv_obj_t* xb_icon_create(lv_obj_t* parent, const char* label, lv_color_t color);

#ifdef __cplusplus
}
#endif
