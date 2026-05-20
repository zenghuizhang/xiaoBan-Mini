/* v5.0 RadialMenu: 6扇区圆形菜单 (对齐 RadialMenu.tsx) */
#pragma once
#include <lvgl.h>
#include "theme_v3.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    RM_ACTION_EXPRESSIONS,  // 表情
    RM_ACTION_DIALOGUE,     // 对话
    RM_ACTION_SETTINGS,     // 设置
    RM_ACTION_THEME,        // 主题
    RM_ACTION_EXTENSIONS,   // 扩展 (→BottomBar)
    RM_ACTION_RANDOM,       // 随机
    RM_ACTION_CLOSE,        // 关闭/返回
} RadialMenuAction;

/** 创建并显示径向菜单, 回调返回用户选择的动作 */
lv_obj_t *radial_menu_show(lv_obj_t *parent, void (*on_select)(RadialMenuAction action));

/** 关闭径向菜单 */
void radial_menu_close(void);

/** 菜单是否正在显示 */
bool radial_menu_is_shown(void);

#ifdef __cplusplus
}
#endif
