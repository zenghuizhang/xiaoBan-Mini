#pragma once
#include "lvgl.h"
#ifdef __cplusplus
extern "C" {
#endif
lv_obj_t* page_ota_create(lv_obj_t* parent);
void page_ota_update_progress(int pct);
#ifdef __cplusplus
}
#endif
