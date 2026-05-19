/*
 * UXV3.6 - 100% 对齐 Face.tsx
 *  ✅ #22D3EE 发光效果（box-shadow）
 *  ✅ 无白色描边、无黄色高光
 */

#include "theme_v3.h"
#include <esp_log.h>

static const char *TAG = "THEME_V3";
static ThemeV3 current_theme = THEME_ADULT;

static void _theme_update_screen_bg(void)
{
    lv_obj_t *screen = lv_screen_active();
    if (current_theme == THEME_ADULT) {
        lv_obj_set_style_bg_color(screen, ADULT_BG, 0);
    } else {
        lv_obj_set_style_bg_color(screen, CHILD_BG, 0);
    }
}

void theme_v3_init(ThemeV3 default_theme)
{
    current_theme = default_theme;
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  UXV3.8 主题系统 - 对齐 Face.tsx");
    ESP_LOGI(TAG, "========================================");

    if (default_theme == THEME_ADULT) {
        ESP_LOGI(TAG, "  默认主题: 成人模式");
        ESP_LOGI(TAG, "  面部: cyan-400 #22D3EE");
    } else {
        ESP_LOGI(TAG, "  默认主题: 儿童模式");
        ESP_LOGI(TAG, "  面部: orange-400 #FB923C");
        ESP_LOGI(TAG, "  圆角: 50%% 完全椭圆");
    }

    _theme_update_screen_bg();

    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  ✓ 主题系统初始化完成");
    ESP_LOGI(TAG, "========================================");
}

void theme_v3_switch(ThemeV3 theme)
{
    if (theme == current_theme) return;

    current_theme = theme;
    _theme_update_screen_bg();
    ESP_LOGI(TAG, "切换主题: %s", theme == THEME_ADULT ? "成人模式" : "儿童模式");
}

ThemeV3 theme_v3_get_current(void)
{
    return current_theme;
}

void theme_v3_draw_eye(lv_obj_t *parent, int cx, int cy, bool is_left)
{
    // ✅ 对齐 Face.tsx: idle 32×40, r=16
    const int w = 32;
    const int h = 40;
    int r = 16;

    lv_color_t fg_color;

    if (current_theme == THEME_ADULT) {
        fg_color = ADULT_FG;
    } else {
        fg_color = CHILD_FG;
        r = w < h ? w : h;  // child: 50% → fully rounded
    }

    // ✅ V3.6：单层渲染 + box-shadow 发光（对齐 Face.tsx）
    lv_obj_t *eye = lv_obj_create(parent);
    lv_obj_set_size(eye, w, h);
    lv_obj_set_pos(eye, cx - w/2, cy - h/2);
    lv_obj_set_style_radius(eye, r, 0);
    lv_obj_set_style_bg_color(eye, fg_color, 0);
    lv_obj_set_style_bg_opa(eye, LV_OPA_100, 0);
    lv_obj_set_style_border_width(eye, 0, 0);

    // 发光效果：box-shadow 对齐
    if (current_theme == THEME_ADULT) {
        lv_obj_set_style_shadow_width(eye, ADULT_SHADOW_W, 0);
        lv_obj_set_style_shadow_color(eye, fg_color, 0);
        lv_obj_set_style_shadow_opa(eye, ADULT_SHADOW_OPA, 0);
    } else {
        lv_obj_set_style_shadow_width(eye, 10, 0);
        lv_obj_set_style_shadow_color(eye, fg_color, 0);
        lv_obj_set_style_shadow_opa(eye, CHILD_SHADOW_OPA, 0);
    }
    lv_obj_set_style_shadow_ofs_x(eye, 0, 0);
    lv_obj_set_style_shadow_ofs_y(eye, 0, 0);

    ESP_LOGD(TAG, "绘制眼睛: (%d, %d) %s, 圆角=%d",
             cx, cy, is_left ? "左眼" : "右眼", r);
}

void theme_v3_draw_mouth(lv_obj_t *parent, int cx, int cy)
{
    // ✅ 对齐 Face.tsx: idle 24×4, r=2
    const int w = 24;
    const int h = 4;
    const int r = 2;

    lv_color_t fg_color;
    if (current_theme == THEME_ADULT) {
        fg_color = ADULT_FG;
    } else {
        fg_color = CHILD_FG;
    }

    lv_obj_t *mouth = lv_obj_create(parent);
    lv_obj_set_size(mouth, w, h);
    lv_obj_set_pos(mouth, cx - w/2, cy - h/2);
    lv_obj_set_style_radius(mouth, r, 0);
    lv_obj_set_style_bg_color(mouth, fg_color, 0);
    lv_obj_set_style_border_width(mouth, 0, 0);

    if (current_theme == THEME_ADULT) {
        lv_obj_set_style_shadow_width(mouth, ADULT_SHADOW_W, 0);
        lv_obj_set_style_shadow_color(mouth, ADULT_SHADOW, 0);
        lv_obj_set_style_shadow_opa(mouth, ADULT_SHADOW_OPA, 0);
        lv_obj_set_style_shadow_ofs_x(mouth, 0, 0);
        lv_obj_set_style_shadow_ofs_y(mouth, 0, 0);
    } else {
        lv_obj_set_style_shadow_width(mouth, 8, 0);
        lv_obj_set_style_shadow_color(mouth, fg_color, 0);
        lv_obj_set_style_shadow_opa(mouth, CHILD_SHADOW_OPA, 0);
    }

    ESP_LOGD(TAG, "绘制嘴巴: (%d, %d)", cx, cy);
}
