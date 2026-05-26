#pragma once
#include "lvgl.h"
#ifdef __cplusplus
extern "C" {
#endif
// v7.6 §6.1: 6-sector icons from icons_v7.6/mono
LV_IMG_DECLARE(icon_chat);      // 0: 对话 MessageCircle
LV_IMG_DECLARE(icon_model);     // 1: 模型 Cpu
LV_IMG_DECLARE(icon_theme);     // 2: 主题 Palette
LV_IMG_DECLARE(icon_settings2); // 3: 设置 Settings
LV_IMG_DECLARE(icon_persona);   // 4: 人格 User
LV_IMG_DECLARE(icon_memory2);   // 5: 记忆 BookOpen
#define MENU_ICON_COUNT 6
extern const lv_image_dsc_t * const menu_icons[MENU_ICON_COUNT];
#ifdef __cplusplus
}
#endif
