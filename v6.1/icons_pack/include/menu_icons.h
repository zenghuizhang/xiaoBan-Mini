/*
 * menu_icons.h
 * --------------------------------------------------------------------
 * 径向菜单 6 扇区图标声明（LVGL 9 / ARGB8888 / 32x32）
 *
 * 来源：lucide-react 官方 SVG  -> cairosvg 32x32 PNG -> LVGL C 数组
 * 颜色：白色描边（alpha 编码笔画形状），运行时通过
 *       lv_obj_set_style_image_recolor + lv_obj_set_style_image_recolor_opa
 *       做主题色重着色，无需为每个主题再生成一份资源。
 *
 * 不要手工编辑 *.c 文件；如需重新生成，运行 build_icons.py。
 * --------------------------------------------------------------------
 */
#ifndef MENU_ICONS_H
#define MENU_ICONS_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

LV_IMG_DECLARE(icon_smile);     /* 扇区 0：表情 / 互动 */
LV_IMG_DECLARE(icon_message);   /* 扇区 1：对话 */
LV_IMG_DECLARE(icon_settings);  /* 扇区 2：设置 */
LV_IMG_DECLARE(icon_palette);   /* 扇区 3：主题 */
LV_IMG_DECLARE(icon_puzzle);    /* 扇区 4：扩展 / 插件 */
LV_IMG_DECLARE(icon_shuffle);   /* 扇区 5：随机 / 切换 */

/* 与 MenuOverlay.tsx (v6.1) 的扇区顺序保持一致 */
#define MENU_ICON_COUNT 6
extern const lv_image_dsc_t * const menu_icons[MENU_ICON_COUNT];

#ifdef __cplusplus
}
#endif

#endif /* MENU_ICONS_H */
