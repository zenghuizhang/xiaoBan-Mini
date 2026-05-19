#pragma once

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 中文字体初始化
 * 加载包含常用中文字符的字体
 */
void font_chinese_init(void);

/**
 * 获取小号中文字体 (16px)
 */
const lv_font_t* font_chinese_16(void);

/**
 * 获取中号中文字体 (20px)
 */
const lv_font_t* font_chinese_20(void);

/**
 * 获取大号中文字体 (24px)
 */
const lv_font_t* font_chinese_24(void);

/**
 * 为标签自动设置中文字体
 * 如果文本包含中文则使用中文字体，否则使用默认字体
 */
void label_set_auto_font(lv_obj_t *label, int size);

#ifdef __cplusplus
}
#endif
