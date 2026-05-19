/*
 * 主题系统 - LVGL 9.5 4层发光效果实现
 * 这就是 LVGL 吊打 Arduino LGFX 的核心！
 */

#include "theme.h"
#include <esp_log.h>

static const char *TAG = "THEME";
static ThemeMode current_theme = THEME_DARK;

void theme_init(ThemeMode mode)
{
    current_theme = mode;
    ESP_LOGI(TAG, "主题系统初始化完成");
    ESP_LOGI(TAG, "  4层发光效果: 8px 光晕扩散");
    ESP_LOGI(TAG, "  层级透明度: 100%% → 50%% → 30%% → 15%%");
}

void theme_switch(ThemeMode mode)
{
    current_theme = mode;
    ESP_LOGI(TAG, "切换主题: %s", mode == THEME_DARK ? "黑夜模式" : "白天模式");
    
    lv_obj_t *screen = lv_screen_active();
    
    if (mode == THEME_DARK) {
        lv_obj_set_style_bg_color(screen, COLOR_DARK_BG, 0);
    } else {
        lv_obj_set_style_bg_color(screen, COLOR_LIGHT_BG, 0);
    }
}

ThemeMode theme_get_current(void)
{
    return current_theme;
}

lv_obj_t* theme_draw_glowing_pill(lv_obj_t *parent, int cx, int cy, int w, int h, int r)
{
    if (current_theme == THEME_LIGHT) {
        // 白天模式：无发光，单层即可
        lv_obj_t *obj = lv_obj_create(parent);
        lv_obj_set_size(obj, w, h);
        lv_obj_set_pos(obj, cx - w/2, cy - h/2);
        lv_obj_set_style_radius(obj, r, 0);
        lv_obj_set_style_bg_color(obj, COLOR_LIGHT_FG, 0);
        lv_obj_set_style_border_width(obj, 0, 0);
        return obj;
    }

    // ==============================================
    // 黑夜模式：4 层发光效果！
    // 从外到内：偏移 8 → 6 → 4 → 2 → 0
    // 透明度：15% → 30% → 50% → 100%
    // ==============================================

    // 第 4 层 - 最外层，最淡，最大范围
    lv_obj_t *glow4 = lv_obj_create(parent);
    lv_obj_set_size(glow4, w + GLOW_OFFSET_4 * 2, h + GLOW_OFFSET_4 * 2);
    lv_obj_set_pos(glow4, cx - w/2 - GLOW_OFFSET_4, cy - h/2 - GLOW_OFFSET_4);
    lv_obj_set_style_radius(glow4, r + GLOW_OFFSET_4, 0);
    lv_obj_set_style_bg_color(glow4, COLOR_GLOW_4, 0);
    lv_obj_set_style_bg_opa(glow4, GLOW_OPA_4, 0);
    lv_obj_set_style_border_width(glow4, 0, 0);

    // 第 3 层
    lv_obj_t *glow3 = lv_obj_create(parent);
    lv_obj_set_size(glow3, w + GLOW_OFFSET_3 * 2, h + GLOW_OFFSET_3 * 2);
    lv_obj_set_pos(glow3, cx - w/2 - GLOW_OFFSET_3, cy - h/2 - GLOW_OFFSET_3);
    lv_obj_set_style_radius(glow3, r + GLOW_OFFSET_3, 0);
    lv_obj_set_style_bg_color(glow3, COLOR_GLOW_3, 0);
    lv_obj_set_style_bg_opa(glow3, GLOW_OPA_3, 0);
    lv_obj_set_style_border_width(glow3, 0, 0);

    // 第 2 层
    lv_obj_t *glow2 = lv_obj_create(parent);
    lv_obj_set_size(glow2, w + GLOW_OFFSET_2 * 2, h + GLOW_OFFSET_2 * 2);
    lv_obj_set_pos(glow2, cx - w/2 - GLOW_OFFSET_2, cy - h/2 - GLOW_OFFSET_2);
    lv_obj_set_style_radius(glow2, r + GLOW_OFFSET_2, 0);
    lv_obj_set_style_bg_color(glow2, COLOR_GLOW_2, 0);
    lv_obj_set_style_bg_opa(glow2, GLOW_OPA_2, 0);
    lv_obj_set_style_border_width(glow2, 0, 0);

    // 第 1 层
    lv_obj_t *glow1 = lv_obj_create(parent);
    lv_obj_set_size(glow1, w + GLOW_OFFSET_1 * 2, h + GLOW_OFFSET_1 * 2);
    lv_obj_set_pos(glow1, cx - w/2 - GLOW_OFFSET_1, cy - h/2 - GLOW_OFFSET_1);
    lv_obj_set_style_radius(glow1, r + GLOW_OFFSET_1, 0);
    lv_obj_set_style_bg_color(glow1, COLOR_GLOW_1, 0);
    lv_obj_set_style_bg_opa(glow1, GLOW_OPA_1, 0);
    lv_obj_set_style_border_width(glow1, 0, 0);

    // 核心层 - 最亮
    lv_obj_t *core = lv_obj_create(parent);
    lv_obj_set_size(core, w, h);
    lv_obj_set_pos(core, cx - w/2, cy - h/2);
    lv_obj_set_style_radius(core, r, 0);
    lv_obj_set_style_bg_color(core, COLOR_CORE, 0);
    lv_obj_set_style_bg_opa(core, GLOW_OPA_1, 0);
    lv_obj_set_style_border_width(core, 0, 0);

    ESP_LOGI(TAG, "✓ 4层发光效果绘制完成");
    ESP_LOGI(TAG, "  光晕范围: ±%dpx", GLOW_OFFSET_4);
    ESP_LOGI(TAG, "  渲染对象: 5个 (4层光晕 + 1个核心)");

    return core;
}
