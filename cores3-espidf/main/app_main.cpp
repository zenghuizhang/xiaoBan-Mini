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
// #include "ui/screenshot.h"  // 截图已禁用
#include "ui/qrcode.h"
#include "ui/boot_anim.h"
#include "ui/radial_menu.h"
#include "ui/dialog_bubble.h"
#include "audio_feedback.h"
#include "robot_memory.h"
#include "motion_controller.h"
#include "ui/icons/icons.h"
#include "ui/scenario_overlay.h"
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
static bool s_lang_cn = true;  // true=中文, false=English

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
    switch (idx) {
    case 0: expression_set(EXPR_HAPPY, true); break;
    case 1: expression_set(EXPR_TALKING, true); break;
    case 2: _menu_close_cb(NULL); _settings_show(); return;
    case 3: {
        ThemeV3 c = theme_v3_get_current();
        theme_v3_switch(c == THEME_TECH ? THEME_CHILD : c == THEME_CHILD ? THEME_DEV : THEME_TECH);
        expression_refresh_theme();
        break;
    }
    case 4:  // 扩展 → 场景模拟控制台
        scenario_overlay_create(menu_overlay, [](){
            _menu_close_cb(NULL);
        });
        return;
    case 5:
        expression_set((Expression)(EXPR_IDLE+1+(esp_random()%19)), true); break;
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

// ========== v6.2 SettingsOverlay (全屏设置面板, 对齐 SettingsOverlay.tsx) ==========
static lv_obj_t *settings_screen = NULL;
static uint8_t s_bright = 180, s_vol = 140;

static void _settings_back_cb(lv_event_t *e) {
    if (settings_screen) { lv_obj_delete(settings_screen); settings_screen = NULL; }
}

static void _settings_bright_slider_cb(lv_event_t *e) {
    lv_obj_t *s = (lv_obj_t *)lv_event_get_target(e);
    s_bright = (uint8_t)lv_slider_get_value(s);
    M5.Display.setBrightness(s_bright);
}

static void _settings_vol_slider_cb(lv_event_t *e) {
    lv_obj_t *s = (lv_obj_t *)lv_event_get_target(e);
    s_vol = (uint8_t)lv_slider_get_value(s);
    M5.Speaker.setVolume(s_vol);
}

static void _settings_wifi_cb(lv_event_t *e) {
    if (wifi_get_state() == WIFI_CONNECTED) { expression_set(EXPR_HAPPY, true); }
    else { qrcode_create(lv_screen_active()); wifi_start_ap_config(); }
}

static void _settings_lang_btn_cb(lv_event_t *e) {
    s_lang_cn = !s_lang_cn;
    dialog_bubble_set_lang(s_lang_cn);
    if (settings_screen) { lv_obj_delete(settings_screen); settings_screen = NULL; }
    _settings_show();
}

static void _settings_show(void) {
    if (settings_screen) return;
    if (menu_overlay) { lv_obj_delete(menu_overlay); menu_overlay = NULL; menu_shown = false; }
    
    bool is_tech = (theme_v3_get_current() == THEME_TECH);
    bool is_dev  = (theme_v3_get_current() == THEME_DEV);
    lv_color_t bg = is_tech ? lv_color_hex(0x0A0A14) : is_dev ? lv_color_hex(0x0A140A) : lv_color_hex(0xFFF9E6);
    lv_color_t accent = theme_fg();
    lv_color_t txt = is_tech ? lv_color_hex(0xE2E8F0) : lv_color_hex(0x1E293B);
    lv_color_t sub_txt = is_tech ? lv_color_hex(0x64748B) : lv_color_hex(0x94A3B8);
    lv_color_t card_bg = is_tech ? lv_color_hex(0x0F172A) : is_dev ? lv_color_hex(0x0A1F0A) : lv_color_hex(0xFFF5E0);
    lv_color_t card_border = is_tech ? lv_color_hex(0x1E293B) : is_dev ? lv_color_hex(0x14532D) : lv_color_hex(0xFDE68A);

    settings_screen = lv_obj_create(lv_screen_active());
    lv_obj_set_size(settings_screen, 320, 240);
    lv_obj_set_pos(settings_screen, 0, 0);
    lv_obj_set_style_bg_color(settings_screen, bg, 0);
    lv_obj_set_style_bg_opa(settings_screen, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(settings_screen, 0, 0);
    lv_obj_set_style_pad_all(settings_screen, 12, 0);
    lv_obj_add_flag(settings_screen, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(settings_screen, LV_OBJ_FLAG_SCROLLABLE);

    // 顶部: < 返回 + 系统设置
    lv_obj_t *back_btn = lv_btn_create(settings_screen);
    lv_obj_set_size(back_btn, 36, 36);
    lv_obj_set_style_radius(back_btn, 18, 0);
    lv_obj_set_style_bg_color(back_btn, card_bg, 0);
    lv_obj_set_style_border_color(back_btn, card_border, 0);
    lv_obj_set_style_border_width(back_btn, 1, 0);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 0, 2);
    lv_obj_add_event_cb(back_btn, _settings_back_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *bi = lv_label_create(back_btn);
    lv_label_set_text(bi, "<");
    lv_obj_set_style_text_color(bi, sub_txt, 0);
    lv_obj_center(bi);

    lv_obj_t *title = lv_label_create(settings_screen);
    lv_label_set_text(title, "Settings");
    lv_obj_set_style_text_color(title, accent, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 12);

    int y = 52;
    // === Brightness ===
    lv_obj_t *bl = lv_label_create(settings_screen);
    lv_label_set_text(bl, "Brightness");
    lv_obj_set_style_text_color(bl, sub_txt, 0);
    lv_obj_set_style_text_font(bl, &lv_font_montserrat_14, 0);
    lv_obj_align(bl, LV_ALIGN_TOP_LEFT, 0, y);

    lv_obj_t *bs = lv_slider_create(settings_screen);
    lv_obj_set_width(bs, 200);
    lv_slider_set_range(bs, 10, 255);
    lv_slider_set_value(bs, s_bright, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bs, accent, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(bs, card_border, LV_PART_MAIN);
    lv_obj_set_style_bg_color(bs, accent, LV_PART_KNOB);
    lv_obj_align_to(bs, bl, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 2);
    lv_obj_add_event_cb(bs, _settings_bright_slider_cb, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t *bv = lv_label_create(settings_screen);
    lv_label_set_text_fmt(bv, "%d%%", s_bright * 100 / 255);
    lv_obj_set_style_text_color(bv, sub_txt, 0);
    lv_obj_align_to(bv, bs, LV_ALIGN_OUT_RIGHT_MID, 8, 0);

    // === Volume ===
    y += 42;
    lv_obj_t *vl = lv_label_create(settings_screen);
    lv_label_set_text(vl, "Volume");
    lv_obj_set_style_text_color(vl, sub_txt, 0);
    lv_obj_align(vl, LV_ALIGN_TOP_LEFT, 0, y);
    lv_obj_t *vs = lv_slider_create(settings_screen);
    lv_obj_set_width(vs, 200);
    lv_slider_set_range(vs, 0, 255);
    lv_slider_set_value(vs, s_vol, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(vs, accent, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(vs, card_border, LV_PART_MAIN);
    lv_obj_set_style_bg_color(vs, accent, LV_PART_KNOB);
    lv_obj_align_to(vs, vl, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 2);
    lv_obj_add_event_cb(vs, _settings_vol_slider_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_t *vv = lv_label_create(settings_screen);
    lv_label_set_text_fmt(vv, "%d%%", s_vol * 100 / 255);
    lv_obj_set_style_text_color(vv, sub_txt, 0);
    lv_obj_align_to(vv, vs, LV_ALIGN_OUT_RIGHT_MID, 8, 0);

    // === WiFi ===
    y += 42;
    lv_obj_t *wifi_btn = lv_btn_create(settings_screen);
    lv_obj_set_size(wifi_btn, 296, 34);
    lv_obj_set_style_radius(wifi_btn, 8, 0);
    lv_obj_set_style_bg_color(wifi_btn, card_bg, 0);
    lv_obj_set_style_border_color(wifi_btn, card_border, 0);
    lv_obj_set_style_border_width(wifi_btn, 1, 0);
    lv_obj_set_style_pad_left(wifi_btn, 10, 0);
    lv_obj_align(wifi_btn, LV_ALIGN_TOP_LEFT, 0, y);
    lv_obj_add_event_cb(wifi_btn, _settings_wifi_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *wl = lv_label_create(wifi_btn);
    lv_label_set_text_fmt(wl, "WiFi   %s", wifi_get_state() == WIFI_CONNECTED ? wifi_get_ssid() : "Not connected");
    lv_obj_set_style_text_color(wl, txt, 0);
    lv_obj_set_style_text_font(wl, &lv_font_montserrat_14, 0);
    lv_obj_align(wl, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_t *wr = lv_label_create(wifi_btn);
    lv_label_set_text(wr, ">");
    lv_obj_set_style_text_color(wr, sub_txt, 0);
    lv_obj_align(wr, LV_ALIGN_RIGHT_MID, -10, 0);

    // === Language ===
    y += 38;
    lv_obj_t *lang_btn = lv_btn_create(settings_screen);
    lv_obj_set_size(lang_btn, 296, 34);
    lv_obj_set_style_radius(lang_btn, 8, 0);
    lv_obj_set_style_bg_color(lang_btn, card_bg, 0);
    lv_obj_set_style_border_color(lang_btn, card_border, 0);
    lv_obj_set_style_border_width(lang_btn, 1, 0);
    lv_obj_set_style_pad_left(lang_btn, 10, 0);
    lv_obj_align(lang_btn, LV_ALIGN_TOP_LEFT, 0, y);
    lv_obj_add_event_cb(lang_btn, _settings_lang_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *ll = lv_label_create(lang_btn);
    lv_label_set_text_fmt(ll, "Language   %s", s_lang_cn ? "CN" : "EN");
    lv_obj_set_style_text_color(ll, txt, 0);
    lv_obj_set_style_text_font(ll, &lv_font_montserrat_14, 0);
    lv_obj_align(ll, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_t *lr = lv_label_create(lang_btn);
    lv_label_set_text(lr, ">");
    lv_obj_set_style_text_color(lr, sub_txt, 0);
    lv_obj_align(lr, LV_ALIGN_RIGHT_MID, -10, 0);

    // === 系统分组: 主题 ===
    y += 38;
    lv_obj_t *theme_btn = lv_btn_create(settings_screen);
    lv_obj_set_size(theme_btn, 296, 34);
    lv_obj_set_style_radius(theme_btn, 8, 0);
    lv_obj_set_style_bg_color(theme_btn, card_bg, 0);
    lv_obj_set_style_border_color(theme_btn, card_border, 0);
    lv_obj_set_style_border_width(theme_btn, 1, 0);
    lv_obj_set_style_pad_left(theme_btn, 10, 0);
    lv_obj_align(theme_btn, LV_ALIGN_TOP_LEFT, 0, y);
    lv_obj_add_event_cb(theme_btn, [](lv_event_t *e){
        ThemeV3 cur = theme_v3_get_current();
        ThemeV3 n = (cur == THEME_TECH) ? THEME_CHILD : (cur == THEME_CHILD) ? THEME_DEV : THEME_TECH;
        theme_v3_switch(n); expression_refresh_theme();
        if (settings_screen) { lv_obj_delete(settings_screen); settings_screen = NULL; }
        _settings_show();
    }, LV_EVENT_CLICKED, NULL);
    lv_obj_t *tl = lv_label_create(theme_btn);
    lv_label_set_text(tl, "Theme   Tech/Child/Dev");
    lv_obj_set_style_text_color(tl, txt, 0);
    lv_obj_set_style_text_font(tl, &lv_font_montserrat_14, 0);
    lv_obj_align(tl, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_t *tr = lv_label_create(theme_btn);
    lv_label_set_text(tr, ">");
    lv_obj_set_style_text_color(tr, sub_txt, 0);
    lv_obj_align(tr, LV_ALIGN_RIGHT_MID, -10, 0);

    // === 关于 ===
    y += 38;
    lv_obj_t *about_btn = lv_btn_create(settings_screen);
    lv_obj_set_size(about_btn, 296, 34);
    lv_obj_set_style_radius(about_btn, 8, 0);
    lv_obj_set_style_bg_color(about_btn, card_bg, 0);
    lv_obj_set_style_border_color(about_btn, card_border, 0);
    lv_obj_set_style_border_width(about_btn, 1, 0);
    lv_obj_set_style_pad_left(about_btn, 10, 0);
    lv_obj_align(about_btn, LV_ALIGN_TOP_LEFT, 0, y);
    lv_obj_t *al = lv_label_create(about_btn);
    lv_label_set_text(al, "About   xiaoBan Mini v6.2");
    lv_obj_set_style_text_color(al, txt, 0);
    lv_obj_set_style_text_font(al, &lv_font_montserrat_14, 0);
    lv_obj_align(al, LV_ALIGN_LEFT_MID, 0, 0);

    ESP_LOGI(TAG, "v6.2 SettingsOverlay (11 items, 5 groups)");
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
    ESP_LOGI(TAG, "系统启动完成! v5.0");
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
