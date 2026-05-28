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
#include <nvs_flash.h>
#include <nvs_flash.h>

#include "ui/theme_v3.h"
#include "ui/expressions.h"
#include "ui/interaction.h"
#include "ui/wifi_config.h"
#include "ui/font_zh_14.h"
#include "ui/screenshot.h"
#include "ui/qrcode.h"
#include "ui/boot_anim.h"
#include "ui/radial_menu.h"
#include "ui/dialog_bubble.h"
#include "audio_feedback.h"
#include "ui/xb_widgets.h"
#include "ui/page_ota.h"
#include "ui/page_settings.h"
#include "robot_memory.h"
#include "motion_controller.h"
#include "ui/icons/icons.h"
#include "ui/scenario_overlay.h"
#include "ui/page_console.h"
#include "ui/page_chat.h"
#include "ui/page_skills.h"
#include "ui/page_skill_detail.h"
#include "ui/page_model_picker.h"
#include "ui/page_theme_picker.h"
#include "ui/page_persona_grid.h"
#include "ui/page_memory_browser.h"
#include "ui/tech_ui.h"
#include <math.h>

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
// (brightness/volume 变量在 settings_show 函数内 static)

// 表情自动恢复状态（全局，点击事件回调使用）
static uint32_t expr_end_time = 0;
static bool expr_pending = false;

static void _lvgl_flush_callback(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    uint32_t w = (uint32_t)(area->x2 - area->x1 + 1);
    uint32_t h = (uint32_t)(area->y2 - area->y1 + 1);

    // GC9A01 期望 MSB-first，需字节交换
    uint16_t *pixels = (uint16_t *)px_map;
    for (uint32_t i = 0; i < w * h; i++) {
        uint16_t p = pixels[i];
        pixels[i] = (p >> 8) | (p << 8);
    }

    // 截图抓swap之后的数据（和屏幕看到的一致）
    screenshot_feed(area->x1, area->y1, w, h, pixels);

    M5.Display.pushImageDMA(area->x1, area->y1, w, h, pixels);
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
        ThemeV3 n = (cur == THEME_TECH) ? THEME_CHILD : (cur == THEME_CHILD) ? THEME_COCOA : THEME_TECH;
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

        // v6.0: 双击 → 切换 MenuOverlay (对齐 RobotUI handleDoubleClick)
        if (now - last_click_time > 150 && now - last_click_time < 600) {
            ESP_LOGI(TAG, "双击 → 切换菜单");
            if (menu_shown) {
                _menu_close_cb(NULL);
            } else {
                _create_menu_overlay();
            }
            last_click_time = 0;
        } else {
            last_click_time = now;
        }
    }
}

// ========== v6.0 MenuOverlay (6扇区, 对齐 menuall.jpg 设计) ==========
#define RM_R  72
#define RM_BTN 52
bool s_lang_cn = true;  // true=中文, false=English (extern for page_settings.c)

static void _settings_show(void);

static const struct {
    int ang, idx;
} v6_items[] = {
    {-90, 0}, {-30, 1}, {30, 2}, {90, 3}, {150, 4}, {210, 5},
};

static void _menu_close_cb(lv_event_t *e)
{
    if (menu_overlay) {
        
        lv_obj_delete(menu_overlay);
        menu_overlay = NULL;
        menu_shown = false;
        expression_set_drawing_enabled(true);
    }
}

static void _v6_menu_cb(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    // v7.6 §6: 0=Chat 1=Model 2=Theme 3=Settings 4=Persona 5=Memory
    switch (idx) {
    case 0: _menu_close_cb(NULL); page_chat_create(lv_screen_active()); return;
    case 1: _menu_close_cb(NULL); page_model_picker_create(lv_screen_active()); return;
    case 2: _menu_close_cb(NULL); page_theme_picker_create(lv_screen_active()); return;
    case 3: _menu_close_cb(NULL); _settings_show(); return;
    case 4: _menu_close_cb(NULL); page_persona_grid_create(lv_screen_active()); return;
    case 5: _menu_close_cb(NULL); page_memory_browser_create(lv_screen_active()); return;
    }
    _menu_close_cb(NULL);
}

static void _create_menu_overlay(void)
{
    if (menu_overlay) return;
    menu_shown = true;
    expression_set_drawing_enabled(false);

    lv_color_t fg = theme_fg();

    // 纯黑实底 (对齐设计稿 #000000)
    menu_overlay = lv_obj_create(lv_screen_active());
    lv_obj_set_size(menu_overlay, 320, 240);
    lv_obj_set_pos(menu_overlay, 0, 0);
    lv_obj_set_style_bg_color(menu_overlay, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(menu_overlay, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(menu_overlay, 0, 0);
    lv_obj_set_style_pad_all(menu_overlay, 0, 0);
    lv_obj_add_flag(menu_overlay, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(menu_overlay, _menu_close_cb, LV_EVENT_CLICKED, NULL);

    int cx = 160, cy = 120;
    for (int i = 0; i < 6; i++) {
        float rad = v6_items[i].ang * M_PI / 180.0f;
        int bx = cx + (int)(cosf(rad) * RM_R) - RM_BTN/2;
        int by = cy + (int)(sinf(rad) * RM_R) - RM_BTN/2;

        // 外圈发光环
        lv_obj_t *ring = lv_obj_create(menu_overlay);
        lv_obj_set_size(ring, RM_BTN+4, RM_BTN+4);
        lv_obj_set_pos(ring, bx-2, by-2);
        lv_obj_set_style_radius(ring, (RM_BTN+4)/2, 0);
        lv_obj_set_style_bg_opa(ring, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_color(ring, fg, 0);
        lv_obj_set_style_border_width(ring, 1, 0);
        lv_obj_set_style_border_opa(ring, LV_OPA_40, 0);
        lv_obj_set_style_shadow_color(ring, fg, 0);
        lv_obj_set_style_shadow_width(ring, 14, 0);
        lv_obj_set_style_shadow_opa(ring, LV_OPA_40, 0);

        // 按钮实体
        lv_obj_t *btn = lv_obj_create(menu_overlay);
        lv_obj_set_size(btn, RM_BTN, RM_BTN);
        lv_obj_set_pos(btn, bx, by);
        lv_obj_set_style_radius(btn, RM_BTN/2, 0);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x001A24), 0);  // 暗青填充
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(btn, fg, 0);
        lv_obj_set_style_border_width(btn, 2, 0);
        lv_obj_set_style_shadow_color(btn, fg, 0);
        lv_obj_set_style_shadow_width(btn, 8, 0);
        lv_obj_set_style_shadow_opa(btn, LV_OPA_50, 0);
        lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(btn, _v6_menu_cb, LV_EVENT_CLICKED, (void*)(intptr_t)v6_items[i].idx);

        // v6.2 官方图标 (32x32 ARGB8888 + image_recolor 主题着色)
        lv_obj_t *icon_img = lv_image_create(btn);
        lv_image_set_src(icon_img, menu_icons[i]);
        lv_obj_set_style_image_recolor(icon_img, fg, 0);
        lv_obj_set_style_image_recolor_opa(icon_img, LV_OPA_COVER, 0);
        lv_obj_center(icon_img);
    }

    ESP_LOGI(TAG, "v6.0 Menu: 6扇区 (52px btn, glow ring)");
}

// ========== v6.2 SettingsOverlay → page_settings.c ==========
static uint8_t s_bright = 180, s_vol = 140;

static void _settings_show(void) {
    if (menu_overlay) { lv_obj_delete(menu_overlay); menu_overlay = NULL; menu_shown = false; }
    page_settings_create(lv_screen_active());
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

    // ESP-IDF 5.5: 必须先初始化 NVS
    esp_err_t nvs_err = nvs_flash_init();
    if (nvs_err == ESP_ERR_NVS_NO_FREE_PAGES || nvs_err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }
    ESP_LOGI(TAG, "NVS init OK (err=%d)", (int)nvs_err);

    // 启动各子系统
    memory_init();
    audio_init();
    motion_init();

    // 默认 Tech 主题 (后续可通过 NVS 恢复偏好)
    ThemeV3 saved_theme = THEME_TECH;  // memory_load_theme();

    // ==============================================
    // v5.0 开机动画: 3.5s (对齐 BootAnimation.tsx)
    // ==============================================
    theme_v3_init((ThemeV3)saved_theme);
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
    ESP_LOGI(TAG, "face live, Chinese font OK");

    // v6.1: 自然呼吸亮度 (8s周期, cubic easing)
    lv_timer_create([](lv_timer_t *t){
        static int phase = 0;
        phase = (phase + 1) % 80;
        float p = phase * M_PI / 40.0f;
        float s = sinf(p);
        float eased = s * s * (s < 0 ? -1.0f : 1.0f);
        int b = 155 + (int)(25 * eased);
        M5.Display.setBrightness(b);
    }, 100, NULL);

    expression_start_carousel();

    // 屏幕点击事件
    lv_obj_add_event_cb(lv_screen_active(), _screen_face_click_cb, LV_EVENT_CLICKED, NULL);

    // ==============================================
    // 初始化 WiFi - 必须放最后！避免初始化顺序冲突
    // ==============================================
    wifi_init();

    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "系统启动完成! xiaoBan v6.7");
    ESP_LOGI(TAG, "========================================");

    // ==============================================
    // 主循环
    // ==============================================
    static int _loop_cnt = 0;
    while (1) {
        M5.update();

        // 心跳日志 (每 2 秒一次)
        if (++_loop_cnt % 400 == 0) {
            ESP_LOGI(TAG, "heartbeat #%d, free heap: %d", _loop_cnt/400,
                     (int)esp_get_free_heap_size());
        }

        uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;

        // 菜单打开时只刷 LVGL, 跳过所有后台逻辑 (防卡死)
        if (menu_shown) {
            lv_timer_handler();
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        // 表情自动恢复
        if (expr_pending && now >= expr_end_time) {
            expression_set(EXPR_IDLE, true);
            expr_pending = false;
        }

        // 表情轮播 (暂时禁用——隔离崩溃源)
        expression_process_pending();
        wifi_process_pending_ui();
        screenshot_capture();

        // v5.0: IMU 体感检测 (暂时禁用——隔离崩溃源)
        MotionAction ma = MOTION_NONE; // motion_poll();
        if (ma != MOTION_NONE) {
            switch (ma) {
            case MOTION_TILT_FORWARD: expression_set(EXPR_CURIOUS, true); break;
            case MOTION_TILT_BACKWARD: expression_set(EXPR_YAWN, true); break;
            case MOTION_TILT_LEFT:  expression_set(EXPR_LOOK_LEFT, true); break;
            case MOTION_TILT_RIGHT: expression_set(EXPR_LOOK_RIGHT, true); break;
            case MOTION_SHAKE: expression_set(EXPR_DIZZY, true); break;
            case MOTION_TAP: expression_set(EXPR_WINK, true); break;
            default: break;
            }
        }

        // v5.0: 早安问候检测 (暂时禁用)
        //static bool morning_checked = false;
        //if (!morning_checked && now > 10000) {
        //    morning_checked = true;
        //    if (memory_should_morning_greet()) {
        //        dialog_bubble_show(lv_screen_active(), DIALOG_MORNING, 4000);
        //        expression_set(EXPR_MORNING, true);
        //    }
        //}

        // ✅ LVGL 统一处理触屏输入
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}
