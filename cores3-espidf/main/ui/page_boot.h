// page_boot.h — boot animation page with StatusBar+TopBar layout (v6.2.1)
#pragma once
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

lv_obj_t* page_boot_create(lv_obj_t* parent, void (*on_done)(void));

#ifdef __cplusplus
}
#endif
