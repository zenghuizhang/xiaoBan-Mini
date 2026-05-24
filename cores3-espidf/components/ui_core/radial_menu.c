/* v5.0 径向菜单 — 加大按钮, 单字母清晰标识 */
#include "radial_menu.h"
#include <esp_log.h>
#include <math.h>

static const char *TAG = "RMENU";
static lv_obj_t *rm_screen = NULL;
static void (*s_on_select)(RadialMenuAction) = NULL;

// v5.0: radius=75, button=56×56 (原 70/46, 加大更清晰)
#define RM_RADIUS 75
#define RM_BTN_SIZE 56

static const struct {
    const char *letter;   // 单个大写字母, Montserrat 渲染
    RadialMenuAction action;
    int angle_deg;
} rm_items[] = {
    {"E", RM_ACTION_EXPRESSIONS, -90},   // 表情 Expression
    {"D", RM_ACTION_DIALOGUE,   -30},   // 对话 Dialogue
    {"S", RM_ACTION_SETTINGS,    30},   // 设置 Settings
    {"T", RM_ACTION_THEME,       90},   // 主题 Theme
    {"+", RM_ACTION_EXTENSIONS, 150},   // 扩展
    {"?", RM_ACTION_RANDOM,     210},   // 随机
};

static void _bg_close_cb(lv_event_t *e)
{
    radial_menu_close();
    if (s_on_select) s_on_select(RM_ACTION_CLOSE);
}

static void _btn_cb(lv_event_t *e)
{
    RadialMenuAction action = (RadialMenuAction)(intptr_t)lv_event_get_user_data(e);
    if (s_on_select) s_on_select(action);
}

lv_obj_t *radial_menu_show(lv_obj_t *parent, void (*on_select)(RadialMenuAction action))
{
    if (rm_screen) return rm_screen;
    s_on_select = on_select;

    ThemeV3 t = theme_v3_get_current();
    lv_color_t fg = theme_fg();
    lv_color_t overlay_bg = lv_color_hex(0x000000);

    // 每个按钮的实心底色 (与主题匹配)
    lv_color_t btn_bg, btn_press;
    if (t == THEME_CHILD) {
        btn_bg = lv_color_hex(0xFFF5E0);
        btn_press = lv_color_hex(0xFF7F50);
    } else if (t == THEME_DEV) {
        btn_bg = lv_color_hex(0x0A1F0A);
        btn_press = lv_color_hex(0x22C55E);
    } else {
        btn_bg = lv_color_hex(0x081828);
        btn_press = lv_color_hex(0x22D3EE);
    }

    // 半透明遮罩
    rm_screen = lv_obj_create(parent);
    lv_obj_set_size(rm_screen, 320, 240);
    lv_obj_set_pos(rm_screen, 0, 0);
    lv_obj_set_style_bg_color(rm_screen, overlay_bg, 0);
    lv_obj_set_style_bg_opa(rm_screen, LV_OPA_80, 0);
    lv_obj_set_style_border_width(rm_screen, 0, 0);
    lv_obj_add_flag(rm_screen, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(rm_screen, _bg_close_cb, LV_EVENT_CLICKED, NULL);

    int cx = 160, cy = 120;

    // 中心返回按钮 (更大, 更清晰)
    lv_obj_t *center = lv_obj_create(rm_screen);
    lv_obj_set_size(center, 48, 48);
    lv_obj_set_style_radius(center, 24, 0);
    lv_obj_set_style_bg_color(center, btn_bg, 0);
    lv_obj_set_style_bg_opa(center, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(center, fg, 0);
    lv_obj_set_style_border_width(center, 2, 0);
    lv_obj_set_style_shadow_color(center, fg, 0);
    lv_obj_set_style_shadow_width(center, 12, 0);
    lv_obj_set_style_shadow_opa(center, LV_OPA_40, 0);
    lv_obj_set_pos(center, cx - 24, cy - 24);
    lv_obj_add_flag(center, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(center, _btn_cb, LV_EVENT_CLICKED, (void*)(intptr_t)RM_ACTION_CLOSE);

    lv_obj_t *cl = lv_label_create(center);
    lv_label_set_text(cl, "X");
    lv_obj_set_style_text_color(cl, fg, 0);
    lv_obj_set_style_text_font(cl, &lv_font_montserrat_14, 0);
    lv_obj_center(cl);

    // 6个扇区 — 大圆 + 单字母
    for (int i = 0; i < 6; i++) {
        float rad = rm_items[i].angle_deg * M_PI / 180.0f;
        int bx = cx + (int)(cosf(rad) * RM_RADIUS) - RM_BTN_SIZE/2;
        int by = cy + (int)(sinf(rad) * RM_RADIUS) - RM_BTN_SIZE/2;

        // 用 lv_obj + CLICKABLE (不用 lv_btn, 避免主题干扰)
        lv_obj_t *btn = lv_obj_create(rm_screen);
        lv_obj_set_size(btn, RM_BTN_SIZE, RM_BTN_SIZE);
        lv_obj_set_style_radius(btn, RM_BTN_SIZE/2, 0);
        lv_obj_set_style_bg_color(btn, btn_bg, 0);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(btn, fg, 0);
        lv_obj_set_style_border_width(btn, 2, 0);
        lv_obj_set_style_shadow_color(btn, fg, 0);
        lv_obj_set_style_shadow_width(btn, 10, 0);
        lv_obj_set_style_shadow_opa(btn, LV_OPA_30, 0);
        lv_obj_set_pos(btn, bx, by);
        lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(btn, _btn_cb, LV_EVENT_CLICKED, (void*)(intptr_t)rm_items[i].action);

        // 按压变色
        lv_obj_set_style_bg_color(btn, btn_press, LV_STATE_PRESSED);

        // 单个大写字母居中
        lv_obj_t *letter = lv_label_create(btn);
        lv_label_set_text(letter, rm_items[i].letter);
        lv_obj_set_style_text_color(letter, fg, 0);
        lv_obj_set_style_text_font(letter, &lv_font_montserrat_14, 0);
        lv_obj_center(letter);
    }

    ESP_LOGI(TAG, "径向菜单: 6按钮, radius=%d, size=%d", RM_RADIUS, RM_BTN_SIZE);
    return rm_screen;
}

void radial_menu_close(void)
{
    if (!rm_screen) return;
    lv_obj_delete(rm_screen);
    rm_screen = NULL;
}

bool radial_menu_is_shown(void) { return rm_screen != NULL; }
