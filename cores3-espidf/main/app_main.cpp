/*
 * 小伴 Mini V3.10 - 纯 LVGL 原生绘制
 *
 * V3.10 更新:
 * ✓ 眼睛: lv_draw_rect 竖椭圆 + 上半圆遮罩
 * ✓ 嘴巴: lv_draw_arc 弧线 (微笑/哭脸/惊讶O)
 * ✓ 发光: 柔光底层 (半透明椭圆)
 * ✓ 眩晕: 旋转重绘
 * ✓ 所有绘制集中在 LV_EVENT_DRAW_POST 回调
 * ✓ 代码精简: ~300 行 (从 ~500 行)
 */

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_random.h>
#include <M5Unified.h>
#include <lvgl.h>
#include <time.h>

#include "ui/theme_v3.h"
#include "ui/expressions.h"
#include "ui/interaction.h"
#include "ui/wifi_config.h"
#include "ui/font_zh_14.h"
#include "ui/screenshot.h"
#include "ui/qrcode.h"

static const char *TAG = "XIAOBAN";

// ========== LVGL 配置 ==========
#define LV_BUFFER_LINES 40
static lv_color_t lv_buf1[320 * LV_BUFFER_LINES];
static lv_color_t lv_buf2[320 * LV_BUFFER_LINES];

// ========== BottomBar 按钮定义 (对齐 BottomBar.tsx) ==========
// 顺序: 主题, 菜单, 随机, 对话, 开心, 眨眼, 调皮, 眩晕, 哭泣
#define BB_BTN_COUNT 9
static const char *bb_labels[] = {
    "**",     // 主题 (star)
    "=",      // 菜单 (hamburger)
    "?",      // 随机 (random)
    ":D",     // 对话 (talking mouth)
    "^_^",    // 开心 (happy face)
    ";)",     // 眨眼 (wink face)
    ">_<",    // 调皮 (naughty face)
    "@_@",    // 眩晕 (dizzy face)
    "T_T",    // 哭泣 (cry face)
};
static Expression bb_expr[] = {
    EXPR_IDLE,     // 主题 — 不触发表情
    EXPR_MENU,     // 菜单
    EXPR_IDLE,     // 随机 — handler 里随机
    EXPR_TALKING,  // 对话
    EXPR_HAPPY,    // 开心
    EXPR_WINK,     // 眨眼
    EXPR_NAUGHTY,  // 调皮
    EXPR_DIZZY,    // 眩晕
    EXPR_CRYING,   // 哭泣
};

// ========== 全局 UI 对象 ==========
static lv_obj_t *status_bar = NULL;
static lv_obj_t *bottom_bar = NULL;
static lv_obj_t *bb_buttons[BB_BTN_COUNT] = {NULL};
static bool bottom_bar_shown = false;

// StatusBar 标签引用（定时刷新用）
static lv_obj_t *sb_left_label = NULL;
static lv_obj_t *sb_center_label = NULL;
static lv_obj_t *sb_right_label = NULL;
static lv_timer_t *sb_refresh_timer = NULL;

// 双击检测
static uint32_t last_click_time = 0;
#define DOUBLE_CLICK_INTERVAL 400  // 两次点击最大间隔 ms

// Menu 浮层
static lv_obj_t *menu_overlay = NULL;
static bool menu_shown = false;
static lv_obj_t *brightness_val_label = NULL;
static lv_obj_t *volume_val_label = NULL;
static uint8_t current_brightness = 200;
static uint8_t current_volume = 100;

// 表情自动恢复状态（全局，点击事件回调使用）
static uint32_t expr_end_time = 0;
static bool expr_pending = false;

static void _lvgl_flush_callback(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    uint32_t w = (uint32_t)(area->x2 - area->x1 + 1);
    uint32_t h = (uint32_t)(area->y2 - area->y1 + 1);

    // M5GFX DMA 发送数据时，ESP32-S3 小端序导致字节顺序反转
    // GC9A01 期望 MSB-first，需要字节交换
    uint16_t *pixels = (uint16_t *)px_map;
    for (uint32_t i = 0; i < w * h; i++) {
        uint16_t p = pixels[i];
        pixels[i] = (p >> 8) | (p << 8);
    }

    M5.Display.pushImageDMA(area->x1, area->y1, w, h, (uint16_t *)px_map);
    lv_display_flush_ready(disp);
}

// ========== UX 动画: 底部栏平滑滑动 ==========
static void _bottom_bar_anim_cb(void *obj, int32_t y)
{
    lv_obj_set_y((lv_obj_t*)obj, y);
}

static void _bottom_bar_anim_done_cb(lv_anim_t *a)
{
    // 动画结束后强制全屏刷新，避免残留白条
    lv_obj_invalidate(lv_screen_active());
}

static void _animate_bottom_bar(bool show)
{
    static lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, bottom_bar);
    lv_anim_set_exec_cb(&anim, _bottom_bar_anim_cb);
    lv_anim_set_ready_cb(&anim, _bottom_bar_anim_done_cb);
    lv_anim_set_time(&anim, 300);
    lv_anim_set_path_cb(&anim, lv_anim_path_ease_out);

    int32_t current_y = lv_obj_get_y(bottom_bar);
    lv_anim_set_values(&anim, current_y, show ? 200 : 310);

    lv_anim_start(&anim);
    bottom_bar_shown = show;
}

// Forward declarations for menu
static void _menu_close_cb(lv_event_t *e);
static void _create_menu_overlay(void);

static void _screen_face_click_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;

        // 如果菜单已打开，单击关闭菜单
        if (menu_shown) {
            _menu_close_cb(NULL);
            return;
        }

        lv_indev_t *indev = lv_indev_get_act();
        if (!indev) return;

        lv_point_t p;
        lv_indev_get_point(indev, &p);

        // 只在面部区域响应（y < 192, 避开状态栏和底部栏区域）
        if (p.y < 192) {
            // V3.9: 双击检测 — 对齐原型双击呼出/隐藏 BottomBar
            if (now - last_click_time < DOUBLE_CLICK_INTERVAL) {
                ESP_LOGI(TAG, "双击面部区域: 切换底部栏");
                _animate_bottom_bar(!bottom_bar_shown);
                screenshot_request();  // 每次切换底部栏都截一张图
                last_click_time = 0;  // 重置，避免三击

                expression_set(EXPR_HAPPY, true);
                expr_end_time = now + 800;
                expr_pending = true;
            } else {
                last_click_time = now;
            }
        }
    }
}

static void _bottom_bar_button_cb(lv_event_t *e)
{
    int btn_idx = (int)(intptr_t)lv_event_get_user_data(e);
    ESP_LOGI(TAG, "底部栏按钮点击: %s (idx=%d)", bb_labels[btn_idx], btn_idx);

    switch (btn_idx) {
        case 0: {  // 主题切换 — 对齐 BottomBar.tsx toggleTheme
            ThemeV3 new_theme = (theme_v3_get_current() == THEME_ADULT) ? THEME_CHILD : THEME_ADULT;
            theme_v3_switch(new_theme);
            expression_refresh_theme();

            bool is_adult = (new_theme == THEME_ADULT);

            // BottomBar 容器颜色 — 对齐 BottomBar.tsx
            lv_obj_set_style_bg_color(bottom_bar,
                is_adult ? lv_color_hex(0x18181B) : lv_color_hex(0xFEF3C7), 0);
            lv_obj_set_style_border_color(bottom_bar,
                is_adult ? lv_color_hex(0x27272A) : lv_color_hex(0xFDE68A), 0);

            // BottomBar 按钮颜色 — 同步更新
            for (int i = 0; i < BB_BTN_COUNT; i++) {
                lv_obj_t *label = lv_obj_get_child(bb_buttons[i], 0);
                if (is_adult) {
                    if (label) lv_obj_set_style_text_color(label, lv_color_hex(0xE4E4E7), 0);
                    lv_obj_set_style_bg_opa(bb_buttons[i], LV_OPA_TRANSP, 0);
                    lv_obj_set_style_border_width(bb_buttons[i], 0, 0);
                    lv_obj_set_style_bg_color(bb_buttons[i], lv_color_hex(0x083344), LV_STATE_PRESSED);
                    lv_obj_set_style_text_color(bb_buttons[i], lv_color_hex(0x22D3EE), LV_STATE_PRESSED);
                } else {
                    if (label) lv_obj_set_style_text_color(label, lv_color_hex(0x7C2D12), 0);
                    lv_obj_set_style_bg_opa(bb_buttons[i], LV_OPA_TRANSP, 0);
                    lv_obj_set_style_border_width(bb_buttons[i], 0, 0);
                    lv_obj_set_style_bg_color(bb_buttons[i], lv_color_hex(0xFB923C), LV_STATE_PRESSED);
                    lv_obj_set_style_text_color(bb_buttons[i], lv_color_hex(0xFFFFFF), LV_STATE_PRESSED);
                }
            }

            // StatusBar 颜色 — 对齐 StatusBar.tsx
            lv_obj_set_style_bg_color(status_bar,
                is_adult ? lv_color_hex(0x18181B) : lv_color_hex(0xFDE68A), 0);
            lv_obj_set_style_text_color(status_bar,
                is_adult ? lv_color_hex(0xA1A1AA) : lv_color_hex(0xB45309), 0);
            break;
        }
        case 1:  // 菜单 — 对齐 BottomBar.tsx menu state
            _create_menu_overlay();
            break;
        case 2: {  // 随机 — 对齐 BottomBar.tsx random state
            Expression faces[] = {EXPR_HAPPY, EXPR_WINK, EXPR_NAUGHTY, EXPR_DIZZY, EXPR_TALKING};
            expression_set(faces[esp_random() % 5], true);
            break;
        }
        case 3:  // 对话
        case 4:  // 开心
        case 5:  // 眨眼
        case 6:  // 调皮
        case 7:  // 眩晕
        case 8:  // 哭泣
            expression_set(bb_expr[btn_idx], true);
            break;
    }
}

// ========== Menu 浮层 V3.9 - 对齐原型 ==========
static void _menu_item_wifi_cb(lv_event_t *e)
{
    _menu_close_cb(NULL);
    if (wifi_get_state() == WIFI_CONNECTED) {
        expression_set(EXPR_HAPPY, true);
    } else {
        // 显示 QR 码界面并启动 AP
        qrcode_create(lv_screen_active());
        expression_set_drawing_enabled(false);
        wifi_start_ap_config();
    }
}

static void _menu_brightness_slider_cb(lv_event_t *e)
{
    lv_obj_t *slider = (lv_obj_t *)lv_event_get_target(e);
    current_brightness = (uint8_t)lv_slider_get_value(slider);
    M5.Display.setBrightness(current_brightness);
    if (brightness_val_label) {
        lv_label_set_text_fmt(brightness_val_label, "%d", current_brightness);
    }
}

static void _menu_volume_slider_cb(lv_event_t *e)
{
    lv_obj_t *slider = (lv_obj_t *)lv_event_get_target(e);
    current_volume = (uint8_t)lv_slider_get_value(slider);
    M5.Speaker.setVolume(current_volume);
    if (volume_val_label) {
        lv_label_set_text_fmt(volume_val_label, "%d", current_volume);
    }
}

static void _menu_close_cb(lv_event_t *e)
{
    if (menu_overlay) {
        lv_obj_delete(menu_overlay);
        menu_overlay = NULL;
        brightness_val_label = NULL;
        volume_val_label = NULL;
        menu_shown = false;
        expression_set_drawing_enabled(true);
    }
}

static void _create_menu_overlay(void)
{
    if (menu_overlay) return;
    menu_shown = true;
    expression_set_drawing_enabled(false);

    bool is_adult = (theme_v3_get_current() == THEME_ADULT);
    lv_color_t bg = is_adult ? lv_color_hex(0x000000) : lv_color_hex(0xFFFBF0);
    lv_color_t panel_bg = is_adult ? lv_color_hex(0x18181B) : lv_color_hex(0xFFFEF7);
    lv_color_t text = is_adult ? lv_color_hex(0xFFFFFF) : lv_color_hex(0x000000);
    lv_color_t accent = is_adult ? lv_color_hex(0x22D3EE) : lv_color_hex(0x9D6BFF);
    lv_color_t sub_text = is_adult ? lv_color_hex(0xA1A1AA) : lv_color_hex(0x78716C);
    lv_color_t item_bg = is_adult ? lv_color_hex(0x27272A) : lv_color_hex(0xFFFBF0);
    lv_color_t item_border = is_adult ? lv_color_hex(0x083344) : lv_color_hex(0xFDE68A);

    // 全屏遮罩
    menu_overlay = lv_obj_create(lv_screen_active());
    lv_obj_set_size(menu_overlay, 320, 240);
    lv_obj_set_pos(menu_overlay, 0, 0);
    lv_obj_set_style_bg_color(menu_overlay, bg, 0);
    lv_obj_set_style_bg_opa(menu_overlay, LV_OPA_90, 0);
    lv_obj_set_style_border_width(menu_overlay, 0, 0);
    lv_obj_set_style_radius(menu_overlay, 0, 0);
    lv_obj_set_style_pad_all(menu_overlay, 0, 0);
    lv_obj_add_flag(menu_overlay, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(menu_overlay, _menu_close_cb, LV_EVENT_CLICKED, NULL);

    // 面板
    lv_obj_t *panel = lv_obj_create(menu_overlay);
    lv_obj_set_size(panel, 288, 208);
    lv_obj_set_pos(panel, 16, 16);
    lv_obj_set_style_bg_color(panel, panel_bg, 0);
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(panel, 16, 0);
    lv_obj_set_style_border_width(panel, 1, 0);
    lv_obj_set_style_border_color(panel, accent, 0);
    lv_obj_set_style_border_opa(panel, LV_OPA_30, 0);
    lv_obj_set_style_pad_all(panel, 12, 0);
    lv_obj_set_style_layout(panel, LV_LAYOUT_FLEX, 0);
    lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_add_flag(panel, LV_OBJ_FLAG_CLICKABLE);

    // 标题行：标题 + 关闭按钮
    lv_obj_t *header = lv_obj_create(panel);
    lv_obj_set_size(header, 264, 28);
    lv_obj_set_style_bg_opa(header, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_pad_all(header, 0, 0);
    lv_obj_set_style_layout(header, LV_LAYOUT_FLEX, 0);
    lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *title = lv_label_create(header);
    lv_label_set_text(title, "Menu");
    lv_obj_set_style_text_color(title, accent, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);

    lv_obj_t *btn_close = lv_btn_create(header);
    lv_obj_set_size(btn_close, 24, 24);
    lv_obj_set_style_radius(btn_close, 12, 0);
    lv_obj_set_style_bg_color(btn_close, is_adult ? lv_color_hex(0x27272A) : lv_color_hex(0xFEF3C7), 0);
    lv_obj_set_style_border_width(btn_close, 0, 0);
    lv_obj_add_event_cb(btn_close, _menu_close_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *x_label = lv_label_create(btn_close);
    lv_label_set_text(x_label, "X");
    lv_obj_center(x_label);
    lv_obj_set_style_text_color(x_label, accent, 0);

    lv_obj_set_style_pad_bottom(header, 8, 0);

    // 分隔线
    lv_obj_t *sep = lv_obj_create(panel);
    lv_obj_set_size(sep, 264, 1);
    lv_obj_set_style_bg_color(sep, accent, 0);
    lv_obj_set_style_bg_opa(sep, LV_OPA_20, 0);
    lv_obj_set_style_border_width(sep, 0, 0);
    lv_obj_set_style_pad_all(sep, 0, 0);
    lv_obj_set_style_margin_bottom(sep, 8, 0);

    // === 菜单项：WiFi 设置 ===
    lv_obj_t *item_wifi = lv_btn_create(panel);
    lv_obj_set_size(item_wifi, 264, 36);
    lv_obj_set_style_radius(item_wifi, 8, 0);
    lv_obj_set_style_bg_color(item_wifi, item_bg, 0);
    lv_obj_set_style_border_width(item_wifi, 1, 0);
    lv_obj_set_style_border_color(item_wifi, item_border, 0);
    lv_obj_set_style_border_opa(item_wifi, LV_OPA_50, 0);
    lv_obj_set_style_pad_left(item_wifi, 10, 0);
    lv_obj_set_style_pad_right(item_wifi, 10, 0);
    lv_obj_add_event_cb(item_wifi, _menu_item_wifi_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *wifi_label = lv_label_create(item_wifi);
    if (wifi_get_state() == WIFI_CONNECTED) {
        lv_label_set_text_fmt(wifi_label, "WiFi  %s", wifi_get_ssid());
    } else {
        lv_label_set_text(wifi_label, "WiFi  Not connected");
    }
    lv_obj_set_style_text_color(wifi_label, text, 0);
    lv_obj_center(wifi_label);

    // === 菜单项：亮度 ===
    lv_obj_t *item_bright = lv_btn_create(panel);
    lv_obj_set_size(item_bright, 264, 36);
    lv_obj_set_style_radius(item_bright, 8, 0);
    lv_obj_set_style_bg_color(item_bright, item_bg, 0);
    lv_obj_set_style_border_width(item_bright, 1, 0);
    lv_obj_set_style_border_color(item_bright, item_border, 0);
    lv_obj_set_style_border_opa(item_bright, LV_OPA_50, 0);
    lv_obj_set_style_pad_all(item_bright, 4, 0);
    lv_obj_set_style_layout(item_bright, LV_LAYOUT_FLEX, 0);
    lv_obj_set_flex_flow(item_bright, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(item_bright, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_add_flag(item_bright, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *bright_lbl = lv_label_create(item_bright);
    lv_label_set_text(bright_lbl, "Bright");
    lv_obj_set_style_text_color(bright_lbl, text, 0);
    lv_obj_set_style_pad_left(bright_lbl, 10, 0);

    lv_obj_t *bright_row = lv_obj_create(item_bright);
    lv_obj_set_size(bright_row, 180, 28);
    lv_obj_set_style_bg_opa(bright_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(bright_row, 0, 0);
    lv_obj_set_style_pad_all(bright_row, 0, 0);
    lv_obj_set_style_layout(bright_row, LV_LAYOUT_FLEX, 0);
    lv_obj_set_flex_flow(bright_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bright_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *bright_slider = lv_slider_create(bright_row);
    lv_obj_set_width(bright_slider, 140);
    lv_slider_set_range(bright_slider, 10, 255);
    lv_slider_set_value(bright_slider, current_brightness, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bright_slider, accent, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(bright_slider, lv_color_hex(0x3F3F46), LV_PART_MAIN);
    lv_obj_set_style_bg_color(bright_slider, accent, LV_PART_KNOB);
    lv_obj_add_event_cb(bright_slider, _menu_brightness_slider_cb, LV_EVENT_VALUE_CHANGED, NULL);

    brightness_val_label = lv_label_create(bright_row);
    lv_label_set_text_fmt(brightness_val_label, "%d", current_brightness);
    lv_obj_set_style_text_color(brightness_val_label, sub_text, 0);
    lv_obj_set_width(brightness_val_label, 36);
    lv_obj_set_style_text_align(brightness_val_label, LV_TEXT_ALIGN_RIGHT, 0);

    // === 菜单项：音量 ===
    lv_obj_t *item_vol = lv_btn_create(panel);
    lv_obj_set_size(item_vol, 264, 36);
    lv_obj_set_style_radius(item_vol, 8, 0);
    lv_obj_set_style_bg_color(item_vol, item_bg, 0);
    lv_obj_set_style_border_width(item_vol, 1, 0);
    lv_obj_set_style_border_color(item_vol, item_border, 0);
    lv_obj_set_style_border_opa(item_vol, LV_OPA_50, 0);
    lv_obj_set_style_pad_all(item_vol, 4, 0);
    lv_obj_set_style_layout(item_vol, LV_LAYOUT_FLEX, 0);
    lv_obj_set_flex_flow(item_vol, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(item_vol, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_add_flag(item_vol, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *vol_lbl = lv_label_create(item_vol);
    lv_label_set_text(vol_lbl, "Volume");
    lv_obj_set_style_text_color(vol_lbl, text, 0);
    lv_obj_set_style_pad_left(vol_lbl, 10, 0);

    lv_obj_t *vol_row = lv_obj_create(item_vol);
    lv_obj_set_size(vol_row, 180, 28);
    lv_obj_set_style_bg_opa(vol_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(vol_row, 0, 0);
    lv_obj_set_style_pad_all(vol_row, 0, 0);
    lv_obj_set_style_layout(vol_row, LV_LAYOUT_FLEX, 0);
    lv_obj_set_flex_flow(vol_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(vol_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *vol_slider = lv_slider_create(vol_row);
    lv_obj_set_width(vol_slider, 140);
    lv_slider_set_range(vol_slider, 0, 255);
    lv_slider_set_value(vol_slider, current_volume, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(vol_slider, accent, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(vol_slider, lv_color_hex(0x3F3F46), LV_PART_MAIN);
    lv_obj_set_style_bg_color(vol_slider, accent, LV_PART_KNOB);
    lv_obj_add_event_cb(vol_slider, _menu_volume_slider_cb, LV_EVENT_VALUE_CHANGED, NULL);

    volume_val_label = lv_label_create(vol_row);
    lv_label_set_text_fmt(volume_val_label, "%d", current_volume);
    lv_obj_set_style_text_color(volume_val_label, sub_text, 0);
    lv_obj_set_width(volume_val_label, 36);
    lv_obj_set_style_text_align(volume_val_label, LV_TEXT_ALIGN_RIGHT, 0);

    // === 关于 ===
    lv_obj_set_style_pad_top(panel, 4, 0);
    lv_obj_t *about = lv_label_create(panel);
    lv_label_set_text(about, "xiaoBan Mini V3.9  M5Stack CoreS3");
    lv_obj_set_style_text_color(about, sub_text, 0);
    lv_obj_set_style_text_font(about, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_align(about, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_pad_top(about, 8, 0);

    ESP_LOGI(TAG, "Menu V3.9 浮层已打开 - 原型对齐");
}

static void _create_bottom_bar(void)
{
    // 对齐 BottomBar.tsx: h-10(40px), px-1.5(6px pad), gap-1(4px)
    const int bar_height = 40;
    const int btn_w = 34;
    const int btn_h = 32;

    bottom_bar = lv_obj_create(lv_screen_active());
    lv_obj_set_size(bottom_bar, 320, bar_height);
    lv_obj_set_pos(bottom_bar, 0, 300);  // 默认隐藏在屏幕外
    lv_obj_set_style_radius(bottom_bar, 0, 0);
    lv_obj_set_style_border_width(bottom_bar, 0, 0);
    lv_obj_set_style_pad_top(bottom_bar, 4, 0);
    lv_obj_set_style_pad_bottom(bottom_bar, 4, 0);
    lv_obj_set_style_pad_left(bottom_bar, 6, 0);
    lv_obj_set_style_pad_right(bottom_bar, 6, 0);
    lv_obj_set_style_layout(bottom_bar, LV_LAYOUT_FLEX, 0);
    lv_obj_set_flex_flow(bottom_bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bottom_bar, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // 对齐 BottomBar.tsx 容器颜色
    if (theme_v3_get_current() == THEME_ADULT) {
        lv_obj_set_style_bg_opa(bottom_bar, LV_OPA_90, 0);
        lv_obj_set_style_bg_color(bottom_bar, lv_color_hex(0x18181B), 0);  // zinc-900/95
        lv_obj_set_style_border_width(bottom_bar, 1, 0);
        lv_obj_set_style_border_color(bottom_bar, lv_color_hex(0x27272A), 0);  // border-zinc-800
        lv_obj_set_style_border_opa(bottom_bar, LV_OPA_COVER, 0);
        lv_obj_set_style_text_color(bottom_bar, lv_color_hex(0xA1A1AA), 0);  // zinc-400
    } else {
        lv_obj_set_style_bg_opa(bottom_bar, LV_OPA_90, 0);
        lv_obj_set_style_bg_color(bottom_bar, lv_color_hex(0xFEF3C7), 0);  // amber-100/95
        lv_obj_set_style_border_width(bottom_bar, 1, 0);
        lv_obj_set_style_border_color(bottom_bar, lv_color_hex(0xFDE68A), 0);  // border-amber-200
        lv_obj_set_style_border_opa(bottom_bar, LV_OPA_COVER, 0);
        lv_obj_set_style_text_color(bottom_bar, lv_color_hex(0xB45309), 0);  // amber-700
    }

    for (int i = 0; i < BB_BTN_COUNT; i++) {
        // 使用 lv_obj + CLICKABLE, 不用 lv_btn (避免主题样式干扰)
        bb_buttons[i] = lv_obj_create(bottom_bar);
        lv_obj_set_size(bb_buttons[i], btn_w, btn_h);
        lv_obj_add_flag(bb_buttons[i], LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_pad_all(bb_buttons[i], 0, 0);
        lv_obj_set_style_radius(bb_buttons[i], 6, 0);
        lv_obj_set_style_bg_opa(bb_buttons[i], LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(bb_buttons[i], 0, 0);
        lv_obj_set_style_shadow_width(bb_buttons[i], 0, 0);
        lv_obj_set_style_bg_color(bb_buttons[i], lv_color_hex(0x083344), LV_STATE_PRESSED);
        lv_obj_set_style_bg_opa(bb_buttons[i], LV_OPA_COVER, LV_STATE_PRESSED);

        lv_obj_add_event_cb(bb_buttons[i], _bottom_bar_button_cb, LV_EVENT_CLICKED, (void*)(intptr_t)i);

        // 标签: 简单 ASCII, Montserrat 肯定能渲染
        lv_obj_t *label = lv_label_create(bb_buttons[i]);
        lv_label_set_text(label, bb_labels[i]);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
        lv_obj_center(label);

        // 显式文本色 (亮色, 高对比)
        if (theme_v3_get_current() == THEME_ADULT) {
            lv_obj_set_style_text_color(label, lv_color_hex(0xE4E4E7), 0);  // zinc-200 bright
            lv_obj_set_style_text_color(bb_buttons[i], lv_color_hex(0x22D3EE), LV_STATE_PRESSED);
            lv_obj_set_style_bg_color(bb_buttons[i], lv_color_hex(0x083344), LV_STATE_PRESSED);
        } else {
            lv_obj_set_style_text_color(label, lv_color_hex(0x7C2D12), 0);  // amber-900 dark
            lv_obj_set_style_text_color(bb_buttons[i], lv_color_hex(0xFFFFFF), LV_STATE_PRESSED);
            lv_obj_set_style_bg_color(bb_buttons[i], lv_color_hex(0xFB923C), LV_STATE_PRESSED);
        }
    }

    ESP_LOGI(TAG, "✓ 底部栏: 9个按钮 (颜文字标签), 原型对齐");
    for (int i = 0; i < BB_BTN_COUNT; i++) {
        ESP_LOGI(TAG, "  btn[%d] = '%s'", i, bb_labels[i]);
    }
}

static void _status_bar_refresh(lv_timer_t *timer)
{
    if (!sb_left_label || !sb_center_label || !sb_right_label) return;

    // WiFi 状态 - V3.9: 文本图标替代 emoji
    if (wifi_get_state() == WIFI_CONNECTED) {
        lv_label_set_text_fmt(sb_left_label, "WiFi %s", wifi_get_ssid());
    } else if (wifi_get_state() == WIFI_CONFIG_AP_MODE) {
        lv_label_set_text(sb_left_label, "AP Mode");
    } else {
        lv_label_set_text(sb_left_label, "No WiFi");
    }

    // 时间
    time_t now;
    time(&now);
    struct tm *t = localtime(&now);
    lv_label_set_text_fmt(sb_center_label, "%02d:%02d", t->tm_hour, t->tm_min);

    // 电量
    int bat = M5.Power.getBatteryLevel();
    bool charging = (M5.Power.isCharging() != 0);
    if (charging) {
        lv_label_set_text_fmt(sb_right_label, "%d%%+", bat);
    } else {
        lv_label_set_text_fmt(sb_right_label, "%d%%", bat);
    }
}

static void _create_status_bar(void)
{
    // V3.9: 对齐原型 StatusBar 设计
    status_bar = lv_obj_create(lv_screen_active());
    lv_obj_set_size(status_bar, 320, 18);
    lv_obj_set_pos(status_bar, 0, 0);
    lv_obj_set_style_radius(status_bar, 0, 0);
    lv_obj_set_style_border_width(status_bar, 0, 0);
    lv_obj_set_style_pad_left(status_bar, 8, 0);
    lv_obj_set_style_pad_right(status_bar, 8, 0);
    lv_obj_set_style_layout(status_bar, LV_LAYOUT_FLEX, 0);
    lv_obj_set_flex_flow(status_bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(status_bar, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(status_bar, LV_OBJ_FLAG_CLICKABLE);

    // 对齐 StatusBar.tsx 颜色
    if (theme_v3_get_current() == THEME_ADULT) {
        lv_obj_set_style_bg_color(status_bar, lv_color_hex(0x18181B), 0);  // zinc-900
        lv_obj_set_style_text_color(status_bar, lv_color_hex(0xA1A1AA), 0); // zinc-400
    } else {
        lv_obj_set_style_bg_color(status_bar, lv_color_hex(0xFDE68A), 0);  // amber-200
        lv_obj_set_style_text_color(status_bar, lv_color_hex(0xB45309), 0); // amber-700
    }

    sb_left_label = lv_label_create(status_bar);
    lv_obj_set_style_text_font(sb_left_label, &lv_font_montserrat_14, 0);
    sb_center_label = lv_label_create(status_bar);
    lv_obj_set_style_text_font(sb_center_label, &lv_font_montserrat_14, 0);
    sb_right_label = lv_label_create(status_bar);
    lv_obj_set_style_text_font(sb_right_label, &lv_font_montserrat_14, 0);

    _status_bar_refresh(NULL);
    sb_refresh_timer = lv_timer_create(_status_bar_refresh, 30000, NULL);

    ESP_LOGI(TAG, "StatusBar V3.9 创建完成");
}

static void _touch_read_callback(lv_indev_t *indev, lv_indev_data_t *data)
{
    auto touch = M5.Touch.getDetail();

    if (touch.wasPressed() || touch.isPressed()) {
        // M5.Touch 已经按 setRotation(1) 处理好坐标，直接使用
        data->point.x = touch.x;
        data->point.y = touch.y;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

/**
 * LVGL 9.x 系统时间源 - 使用 FreeRTOS tick
 * 必须在 lv_init() 之前调用！否则 lv_tick_get() 永远返回 0
 */
static uint32_t _lv_tick_get_source(void)
{
    return xTaskGetTickCount() * portTICK_PERIOD_MS;
}

// ========== 主入口 ==========
extern "C" void app_main(void)
{
    auto m5_config = M5.config();
    M5.begin(m5_config);
    M5.Display.setRotation(1);
    M5.Display.startWrite();

    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  小伴 Mini V3.10 - 原生绘制引擎");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "✓ 原生绘制: lv_draw_rect眼 + lv_draw_arc嘴");
    ESP_LOGI(TAG, "✓ 8种表情 + 柔光 + 泪水动画");
    ESP_LOGI(TAG, "✓ BottomBar: 9按钮 tab式 / 双击手势");
    ESP_LOGI(TAG, "✓ Menu: WiFi/亮度/音量/关于");

    // RGB 自检
    M5.Display.fillScreen(TFT_RED);
    vTaskDelay(pdMS_TO_TICKS(30));
    M5.Display.fillScreen(TFT_GREEN);
    vTaskDelay(pdMS_TO_TICKS(30));
    M5.Display.fillScreen(TFT_BLUE);
    vTaskDelay(pdMS_TO_TICKS(30));
    M5.Display.fillScreen(TFT_BLACK);

    // ==============================================
    // LVGL 初始化
    // ==============================================
    lv_init();
    lv_tick_set_cb(_lv_tick_get_source);  // ✅ 配置时间源！否则崩溃

    lv_display_t *disp = lv_display_create(320, 240);
    lv_display_set_flush_cb(disp, _lvgl_flush_callback);
    lv_display_set_buffers(disp, lv_buf1, lv_buf2, sizeof(lv_buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);

    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, _touch_read_callback);

    ESP_LOGI(TAG, "✓ LVGL 9.5 初始化完成");

    // 让屏幕可点击（CLICKED 事件），但禁止滚动（对齐 RobotUI.tsx overflow-hidden）
    lv_obj_add_flag(lv_screen_active(), LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(lv_screen_active(), LV_OBJ_FLAG_SCROLLABLE);

    // 启动截图系统 (串口监听 's' 命令)
    screenshot_init();

    // ==============================================
    // 初始化 UI 层级
    // ==============================================
    theme_v3_init(THEME_ADULT);
    expressions_init();

    // UXV3.3 完整架构
    _create_status_bar();
    _create_bottom_bar();
    expression_start_carousel();

    // ✅ 屏幕背景点击事件（替代主循环中的双重 Touch 读取）
    lv_obj_add_event_cb(lv_screen_active(), _screen_face_click_cb, LV_EVENT_CLICKED, NULL);

    // ==============================================
    // 初始化 WiFi - 必须放最后！避免初始化顺序冲突
    // ==============================================
    wifi_init();

    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "系统启动完成! V3.9 原型对齐迭代");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  StatusBar: WiFi/时间/电量 30秒刷新");
    ESP_LOGI(TAG, "  表情: 8种 + 动态动画 + 眨眼 + 轮播");
    ESP_LOGI(TAG, "  BottomBar: 9按钮 tab式 (双击面部呼出)");
    ESP_LOGI(TAG, "  Menu: WiFi/亮度/音量/关于");
    ESP_LOGI(TAG, "  截图: 串口发 's' 触发, 或启动后自动截一张");

    // 启动截图延迟触发 (等渲染完成)
    screenshot_boot_trigger();

    // ==============================================
    // 主循环
    // ==============================================
    while (1) {
        M5.update();  // ✅ 只调用一次 M5.update()！

        uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;

        // 表情自动恢复
        if (expr_pending && now >= expr_end_time) {
            expression_set(EXPR_IDLE, true);
            expr_pending = false;
        }

        // 表情轮播
        expression_process_pending();

        // 截图回传 (串口 's' 命令)
        screenshot_process_pending();

        // ✅ LVGL 统一处理触屏输入
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}
