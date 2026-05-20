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
// #include "ui/screenshot.h"  // 截图已禁用
#include "ui/qrcode.h"
#include "ui/boot_anim.h"
#include "ui/radial_menu.h"
#include "ui/dialog_bubble.h"
#include "ui/audio_feedback.h"
#include "ui/robot_memory.h"
#include "ui/motion_controller.h"

static const char *TAG = "XIAOBAN";

// ========== LVGL 配置 ==========
#define LV_BUFFER_LINES 40
static lv_color_t lv_buf1[320 * LV_BUFFER_LINES];
static lv_color_t lv_buf2[320 * LV_BUFFER_LINES];

// ========== BottomBar 按钮定义 (对齐 BottomBar.tsx) ==========
// v5.0: 16 按钮 (核心表情 + 功能)

// ========== 全局 UI 对象 ==========

// StatusBar 标签引用（定时刷新用）

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



// Forward declarations
static void _menu_close_cb(lv_event_t *e);
static void _create_menu_overlay(void);

// v5.0 径向菜单回调
static void _radial_menu_cb(RadialMenuAction action)
{
    // 简单处理 (不做耗时操作: 音频/NVS 会卡 UI)
    switch (action) {
    case RM_ACTION_EXPRESSIONS:
        expression_set(EXPR_HAPPY, true);
        break;
    case RM_ACTION_DIALOGUE:
        expression_set(EXPR_TALKING, true);
        break;
    case RM_ACTION_SETTINGS:
        _create_menu_overlay();
        break;
    case RM_ACTION_THEME: {
        ThemeV3 cur = theme_v3_get_current();
        ThemeV3 n = (cur == THEME_TECH) ? THEME_CHILD : (cur == THEME_CHILD) ? THEME_DEV : THEME_TECH;
        theme_v3_switch(n);
        expression_refresh_theme();
        break;
    }
    case RM_ACTION_EXTENSIONS:  // "扩展" → 随机表情 (底部栏已移除)
    case RM_ACTION_RANDOM:
        expression_set((Expression)(EXPR_IDLE + 1 + (esp_random() % 15)), true);
        break;
    case RM_ACTION_CLOSE:
        break;
    }
    radial_menu_close();
}

static volatile bool g_boot_done = false;

// 开机动画完成后 → 设标志 (不在 lv_anim 回调内操作 LVGL)
static void _on_boot_done(void)
{
    g_boot_done = true;
    ESP_LOGI(TAG, "boot anim finished");
}

static void _screen_face_click_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;

        // 关闭已打开的覆盖层
        if (menu_shown) { _menu_close_cb(NULL); return; }
        if (radial_menu_is_shown()) { radial_menu_close(); return; }

        // v5.0: 双击 → 径向菜单 (150-500ms 间隔)
        if (now - last_click_time > 150 && now - last_click_time < 500) {
            ESP_LOGI(TAG, "双击 → 径向菜单");
            radial_menu_show(lv_screen_active(), _radial_menu_cb);
            last_click_time = 0;
        } else {
            last_click_time = now;
        }
    }
}

// ========== Menu 浮层 - 对齐原型 ==========
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

    bool is_tech = (theme_v3_get_current() == THEME_TECH);
    bool is_dev = (theme_v3_get_current() == THEME_DEV);
    lv_color_t bg = is_tech ? lv_color_hex(0x000000) : is_dev ? lv_color_hex(0x0A0A0A) : lv_color_hex(0xFFFBF0);
    lv_color_t panel_bg = is_tech ? lv_color_hex(0x18181B) : lv_color_hex(0xFFFEF7);
    lv_color_t text = is_tech ? lv_color_hex(0xFFFFFF) : lv_color_hex(0x000000);
    lv_color_t accent = is_tech ? lv_color_hex(0x22D3EE) : is_dev ? lv_color_hex(0x22C55E) : lv_color_hex(0xFF7F50);
    lv_color_t sub_text = is_tech ? lv_color_hex(0xA1A1AA) : lv_color_hex(0x78716C);
    lv_color_t item_bg = is_tech ? lv_color_hex(0x27272A) : lv_color_hex(0xFFFBF0);
    lv_color_t item_border = is_tech ? lv_color_hex(0x083344) : is_dev ? lv_color_hex(0x14532D) : lv_color_hex(0xFDE68A);

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
    lv_obj_set_style_bg_color(btn_close, is_tech ? lv_color_hex(0x27272A) : lv_color_hex(0xFEF3C7), 0);
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

static void _touch_read_callback(lv_indev_t *indev, lv_indev_data_t *data)
{
    auto touch = M5.Touch.getDetail();

    if (touch.wasPressed() || touch.isPressed()) {
        data->point.x = touch.x;
        data->point.y = touch.y;
        data->state = LV_INDEV_STATE_PRESSED;
        // 每 50 次 press 打印一次 (避免刷屏)
        static int cnt = 0;
        if (++cnt % 50 == 1) ESP_LOGI("TOUCH", "press at (%d,%d)", touch.x, touch.y);
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

    // 启动各子系统 (截图编译级禁用)
    memory_init();
    audio_init();
    motion_init();

    // 默认 Tech 主题 (后续可通过 NVS 恢复偏好)
    ThemeV3 saved_theme = THEME_TECH;  // memory_load_theme();

    // ==============================================
    // v5.0 开机动画: 3.5s (对齐 BootAnimation.tsx)
    // ==============================================
    theme_v3_init(saved_theme);
    expressions_init();  // 先创建 face_container (但被 boot 覆盖)
    expression_set_drawing_enabled(false);  // 开机时隐藏面部

    boot_anim_play(lv_screen_active(), _on_boot_done);

    // 等待开机动画完成 (轮询 M5 + LVGL, 保持触摸响应)
    // boot_anim 内部 3.5s 后回调 _on_boot_done → 设 g_boot_done=true
    for (int i = 0; i < 500 && !g_boot_done; i++) {
        M5.update();  // 保持触摸面板刷新
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    // 开机完成, 在干净的上下文启动面部
    expression_set_drawing_enabled(true);
    expression_set(EXPR_IDLE, false);
    lv_obj_invalidate(lv_screen_active());
    // 刷新 30 帧确保面部立刻出现
    for (int i = 0; i < 30; i++) {
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    ESP_LOGI(TAG, "face live");

    // v5.0: 不创建 StatusBar (设计稿已移除)
    expression_start_carousel();

    // ✅ 屏幕背景点击事件（替代主循环中的双重 Touch 读取）
    lv_obj_add_event_cb(lv_screen_active(), _screen_face_click_cb, LV_EVENT_CLICKED, NULL);

    // ==============================================
    // 初始化 WiFi - 必须放最后！避免初始化顺序冲突
    // ==============================================
    wifi_init();

    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "系统启动完成! v5.0");
    ESP_LOGI(TAG, "========================================");

    // ==============================================
    // 主循环
    // ==============================================
    while (1) {
        M5.update();

        uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;

        // 表情自动恢复
        if (expr_pending && now >= expr_end_time) {
            expression_set(EXPR_IDLE, true);
            expr_pending = false;
        }

        // 表情轮播
        expression_process_pending();

        // v5.0: IMU 体感检测
        MotionAction ma = motion_poll();
        if (ma != MOTION_NONE) {
            switch (ma) {
            case MOTION_TILT_FORWARD: expression_set(EXPR_CURIOUS, true); break;
            case MOTION_TILT_BACKWARD: expression_set(EXPR_YAWN, true); break;
            case MOTION_TILT_LEFT: case MOTION_TILT_RIGHT: expression_set(EXPR_LOOK_AROUND, true); break;
            case MOTION_SHAKE: expression_set(EXPR_DIZZY, true); break;
            case MOTION_TAP: expression_set(EXPR_WINK, true); break;
            default: break;
            }
        }

        // v5.0: 早安问候检测
        static bool morning_checked = false;
        if (!morning_checked && now > 10000) {  // 10s 后检查
            morning_checked = true;
            if (memory_should_morning_greet()) {
                dialog_bubble_show(lv_screen_active(), DIALOG_MORNING, 4000);
                expression_set(EXPR_MORNING, true);
            }
        }

        // ✅ LVGL 统一处理触屏输入
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}
