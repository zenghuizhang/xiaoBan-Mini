/*
 * menu_icons.c
 * 集中导出 6 扇区图标指针表，供 radial_menu.c 按 sector index 取用。
 */
#include "menu_icons.h"

const lv_image_dsc_t * const menu_icons[MENU_ICON_COUNT] = {
    &icon_smile,     /* 0 */
    &icon_message,   /* 1 */
    &icon_settings,  /* 2 */
    &icon_palette,   /* 3 */
    &icon_puzzle,    /* 4 */
    &icon_shuffle,   /* 5 */
};
