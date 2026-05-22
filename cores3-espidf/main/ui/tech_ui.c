#include <stdio.h>
#include "tech_ui.h"
#include "font_zh_14.h"

static lv_obj_t *status_label;
static lv_obj_t *battery_label;
static lv_obj_t *wifi_icon;
static lv_obj_t *time_label;

// 创建带圆角的卡片
static lv_obj_t *create_card(lv_obj_t *parent, lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, w, h);
    lv_obj_set_pos(card, x, y);
    
    // 样式设置
    lv_obj_set_style_bg_color(card, TECH_COLOR_BG_CARD, 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_80, 0);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, TECH_COLOR_PRIMARY, 0);
    lv_obj_set_style_border_opa(card, LV_OPA_30, 0);
    lv_obj_set_style_shadow_width(card, 8, 0);
    lv_obj_set_style_shadow_color(card, TECH_COLOR_PRIMARY, 0);
    lv_obj_set_style_shadow_opa(card, LV_OPA_20, 0);
    lv_obj_set_style_pad_all(card, 8, 0);
    
    return card;
}

// 创建图标按钮
static lv_obj_t *create_icon_btn(lv_obj_t *parent, const char *icon, lv_coord_t x, lv_coord_t y)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 50, 50);
    lv_obj_set_pos(btn, x, y);
    
    lv_obj_set_style_bg_color(btn, TECH_COLOR_BG_LIGHT, 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_60, 0);
    lv_obj_set_style_radius(btn, 25, 0);
    lv_obj_set_style_border_width(btn, 2, 0);
    lv_obj_set_style_border_color(btn, TECH_COLOR_PRIMARY, 0);
    lv_obj_set_style_pad_all(btn, 0, 0);
    
    // 按下效果
    lv_obj_set_style_bg_color(btn, TECH_COLOR_PRIMARY, LV_STATE_PRESSED);
    lv_obj_set_style_text_color(btn, TECH_COLOR_BG, LV_STATE_PRESSED);
    
    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, icon);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(label, TECH_COLOR_PRIMARY, 0);
    lv_obj_center(label);
    
    return btn;
}

void tech_ui_create(lv_obj_t *parent)
{
    // 设置背景
    lv_obj_set_style_bg_color(parent, TECH_COLOR_BG, 0);
    
    // ==================== 顶部状态栏 ====================
    lv_obj_t *status_bar = lv_obj_create(parent);
    lv_obj_set_size(status_bar, 320, 36);
    lv_obj_set_pos(status_bar, 0, 0);
    lv_obj_set_style_bg_opa(status_bar, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(status_bar, 0, 0);
    lv_obj_set_style_pad_all(status_bar, 0, 0);
    lv_obj_set_style_radius(status_bar, 0, 0);
    
    // 时间显示（左边）
    time_label = lv_label_create(status_bar);
    lv_label_set_text(time_label, "23:04");
    lv_obj_set_style_text_font(time_label, &font_zh_14, 0);
    lv_obj_set_style_text_color(time_label, TECH_COLOR_TEXT, 0);
    lv_obj_align(time_label, LV_ALIGN_LEFT_MID, 12, 0);
    
    // WiFi图标（中间偏右）
    wifi_icon = lv_label_create(status_bar);
    lv_label_set_text(wifi_icon, "📶");
    lv_obj_set_style_text_color(wifi_icon, TECH_COLOR_PRIMARY, 0);
    lv_obj_align(wifi_icon, LV_ALIGN_RIGHT_MID, -50, 0);
    
    // 电池显示（最右边）
    battery_label = lv_label_create(status_bar);
    lv_label_set_text(battery_label, "🔋 85%");
    lv_obj_set_style_text_font(battery_label, &font_zh_14, 0);
    lv_obj_set_style_text_color(battery_label, TECH_COLOR_TEXT, 0);
    lv_obj_align(battery_label, LV_ALIGN_RIGHT_MID, -10, 0);
    
    // 分割线
    lv_obj_t *divider = lv_obj_create(parent);
    lv_obj_set_size(divider, 296, 1);
    lv_obj_set_pos(divider, 12, 38);
    lv_obj_set_style_bg_color(divider, TECH_COLOR_PRIMARY, 0);
    lv_obj_set_style_bg_opa(divider, LV_OPA_20, 0);
    lv_obj_set_style_border_width(divider, 0, 0);
    lv_obj_set_style_radius(divider, 0, 0);
    
    // ==================== 主内容区 ====================
    
    // 左上卡片 - 表情状态
    lv_obj_t *card1 = create_card(parent, 12, 50, 144, 100);
    lv_obj_t *card1_title = lv_label_create(card1);
    lv_label_set_text(card1_title, "🤖 状态");
    lv_obj_set_style_text_font(card1_title, &font_zh_14, 0);
    lv_obj_set_style_text_color(card1_title, TECH_COLOR_PRIMARY, 0);
    lv_obj_align(card1_title, LV_ALIGN_TOP_MID, 0, 0);
    
    status_label = lv_label_create(card1);
    lv_label_set_text(status_label, "陪伴中...");
    lv_obj_set_style_text_font(status_label, &font_zh_14, 0);
    lv_obj_set_style_text_color(status_label, TECH_COLOR_TEXT, 0);
    lv_obj_align(status_label, LV_ALIGN_CENTER, 0, 8);
    
    // 右上卡片 - 环境信息
    lv_obj_t *card2 = create_card(parent, 164, 50, 144, 100);
    lv_obj_t *card2_title = lv_label_create(card2);
    lv_label_set_text(card2_title, "🌡️ 环境");
    lv_obj_set_style_text_font(card2_title, &font_zh_14, 0);
    lv_obj_set_style_text_color(card2_title, TECH_COLOR_PRIMARY, 0);
    lv_obj_align(card2_title, LV_ALIGN_TOP_MID, 0, 0);
    
    lv_obj_t *temp_label = lv_label_create(card2);
    lv_label_set_text(temp_label, "25°C  45%");
    lv_obj_set_style_text_font(temp_label, &font_zh_14, 0);
    lv_obj_set_style_text_color(temp_label, TECH_COLOR_TEXT, 0);
    lv_obj_align(temp_label, LV_ALIGN_CENTER, 0, 8);
    
    lv_obj_t *temp_sub = lv_label_create(card2);
    lv_label_set_text(temp_sub, "温度  湿度");
    lv_obj_set_style_text_font(temp_sub, &font_zh_14, 0);
    lv_obj_set_style_text_color(temp_sub, TECH_COLOR_TEXT_SEC, 0);
    lv_obj_align(temp_sub, LV_ALIGN_CENTER, 0, 26);
    
    // ==================== 底部导航栏 ====================
    lv_obj_t *nav_bar = lv_obj_create(parent);
    lv_obj_set_size(nav_bar, 320, 70);
    lv_obj_set_pos(nav_bar, 0, 170);
    lv_obj_set_style_bg_opa(nav_bar, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(nav_bar, 0, 0);
    lv_obj_set_style_pad_all(nav_bar, 0, 0);
    lv_obj_set_style_radius(nav_bar, 0, 0);
    
    // 创建5个图标按钮
    create_icon_btn(nav_bar, "👁️", 10, 10);   // 表情
    create_icon_btn(nav_bar, "💬", 74, 10);   // 对话
    create_icon_btn(nav_bar, "⚙️", 138, 10);  // 设置
    create_icon_btn(nav_bar, "🎨", 202, 10);  // 主题
    create_icon_btn(nav_bar, "🔄", 266, 10);  // 随机
    
    // 按钮标签
    const char *btn_labels[] = {"表情", "对话", "设置", "主题", "随机"};
    for (int i = 0; i < 5; i++) {
        lv_obj_t *label = lv_label_create(nav_bar);
        lv_label_set_text(label, btn_labels[i]);
        lv_obj_set_style_text_font(label, &font_zh_14, 0);
        lv_obj_set_style_text_color(label, TECH_COLOR_TEXT_SEC, 0);
        lv_obj_set_pos(label, 15 + i * 64, 58);
    }
}

void tech_ui_update_status(const char *status_text)
{
    if (status_label) {
        lv_label_set_text(status_label, status_text);
    }
}

void tech_ui_set_battery(int percent)
{
    if (battery_label) {
        char buf[16];
        const char *icon = (percent > 20) ? "🔋" : "🪫";
        snprintf(buf, sizeof(buf), "%s %d%%", icon, percent);
        lv_label_set_text(battery_label, buf);
    }
}

void tech_ui_set_wifi(bool connected)
{
    if (wifi_icon) {
        lv_label_set_text(wifi_icon, connected ? "📶" : "❌");
    }
}

void tech_ui_set_time(const char *time_str)
{
    if (time_label) {
        lv_label_set_text(time_label, time_str);
    }
}
