#pragma once

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 交互状态
 */
typedef enum {
    INTERACT_NORMAL,     // 正常状态（无菜单）
    INTERACT_BOTTOM_BAR, // 底部栏显示
    INTERACT_MENU        // 菜单浮层
} InteractionState;

/**
 * 底部栏按钮定义
 */
typedef enum {
    BTN_MENU,
    BTN_THEME,
    BTN_CHAT,
    BTN_HAPPY,
    BTN_DIZZY,
    BTN_COUNT
} BottomBarButton;

/**
 * 初始化交互系统
 */
void interaction_init(void);

/**
 * 显示底部操作栏
 */
void interaction_show_bottom_bar(bool animate);

/**
 * 隐藏底部操作栏
 */
void interaction_hide_bottom_bar(bool animate);

/**
 * 显示菜单浮层
 */
void interaction_show_menu(void);

/**
 * 隐藏菜单浮层
 */
void interaction_hide_menu(void);

/**
 * 获取当前交互状态
 */
InteractionState interaction_get_state(void);

#ifdef __cplusplus
}
#endif
