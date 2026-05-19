/*
 * v5.0 主题系统: Tech (cyan) / Child (coral) / Dev (green)
 */
#include "theme_v3.h"
#include <esp_log.h>

static const char *TAG = "THEME";
static ThemeV3 current_theme = THEME_TECH;

static void _theme_update_screen_bg(void)
{
    lv_obj_t *screen = lv_screen_active();
    lv_color_t bg = TECH_BG;
    if (current_theme == THEME_CHILD) bg = CHILD_BG;
    else if (current_theme == THEME_DEV) bg = DEV_BG;
    lv_obj_set_style_bg_color(screen, bg, 0);
}

void theme_v3_init(ThemeV3 default_theme)
{
    current_theme = default_theme;
    _theme_update_screen_bg();
    ESP_LOGI(TAG, "v5.0 主题: %s",
        default_theme == THEME_TECH ? "Tech(cyan)" :
        default_theme == THEME_CHILD ? "Child(coral)" : "Dev(green)");
}

void theme_v3_switch(ThemeV3 theme)
{
    if (theme == current_theme) return;
    current_theme = theme;
    _theme_update_screen_bg();
    ESP_LOGI(TAG, "切换: %s",
        theme == THEME_TECH ? "Tech" : theme == THEME_CHILD ? "Child" : "Dev");
}

ThemeV3 theme_v3_get_current(void) { return current_theme; }

lv_color_t theme_fg(void)
{
    if (current_theme == THEME_CHILD) return CHILD_FG;
    if (current_theme == THEME_DEV)   return DEV_FG;
    return TECH_FG;
}

lv_color_t theme_bg(void)
{
    if (current_theme == THEME_CHILD) return CHILD_BG;
    return TECH_BG;  // tech 和 dev 都是黑背景
}
