#ifndef TECH_UI_H
#define TECH_UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

// 科技风配色
#define TECH_COLOR_PRIMARY   lv_color_hex(0x22D3EE)  // 草青色（主色）
#define TECH_COLOR_SECONDARY lv_color_hex(0x3B82F6)  // 蓝色
#define TECH_COLOR_ACCENT    lv_color_hex(0x10B981)  // 绿色
#define TECH_COLOR_WARNING   lv_color_hex(0xF59E0B)  // 橙色
#define TECH_COLOR_DANGER    lv_color_hex(0xEF4444)  // 红色
#define TECH_COLOR_BG        lv_color_hex(0x0F172A)  // 深蓝背景
#define TECH_COLOR_BG_LIGHT lv_color_hex(0x1E293B)  // 稍浅背景
#define TECH_COLOR_BG_CARD   lv_color_hex(0x334155)  // 卡片背景
#define TECH_COLOR_TEXT      lv_color_hex(0xF1F5F9)  // 文字白色
#define TECH_COLOR_TEXT_SEC  lv_color_hex(0x94A3B8)  // 次要文字

void tech_ui_create(lv_obj_t *parent);
void tech_ui_update_status(const char *status_text);
void tech_ui_set_battery(int percent);
void tech_ui_set_wifi(bool connected);
void tech_ui_set_time(const char *time_str);

#ifdef __cplusplus
}
#endif

#endif // TECH_UI_H
