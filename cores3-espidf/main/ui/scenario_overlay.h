/* v6.0 ScenarioOverlay: 场景模拟控制台 (开发者模式) */
#pragma once
#include <lvgl.h>
#include "theme_v3.h"
#include "dialog_bubble.h"

#ifdef __cplusplus
extern "C" {
#endif

lv_obj_t *scenario_overlay_create(lv_obj_t *parent, void (*on_close)(void));
void scenario_overlay_close(void);

#ifdef __cplusplus
}
#endif
