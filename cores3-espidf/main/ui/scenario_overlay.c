/* v6.0 ScenarioOverlay: 场景模拟控制台
 * 左下角"+"→开发者控制台: 早安/建议/休息/奖励/生气/寂寞/庆祝/兴奋/好奇
 */
#include "scenario_overlay.h"
#include "expressions.h"
#include "dialog_bubble.h"
#include <esp_log.h>

static const char *TAG = "SCENARIO";
static lv_obj_t *s_overlay = NULL;
static void (*s_on_close)(void) = NULL;

typedef struct { const char *label; Expression expr; DialogType dialog; } Scenario;
static const Scenario scenarios[] = {
    {"Morning",  EXPR_SLEEP_WAKE, DIALOG_MORNING},
    {"Suggest",  EXPR_IDLE,       DIALOG_SUGGEST},
    {"Sleep",    EXPR_YAWN,       DIALOG_SLEEP},
    {"Reward",   EXPR_CELEBRATE,  DIALOG_MORNING},   // reuse type, show OK
    {"Angry",    EXPR_ANGRY,      DIALOG_MORNING},
    {"Lonely",   EXPR_LOST,       DIALOG_SUGGEST},
    {"Celebrate",EXPR_CELEBRATE,  DIALOG_MORNING},
    {"Excited",  EXPR_EXCITED,    DIALOG_MORNING},
    {"Curious",  EXPR_CURIOUS,    DIALOG_MORNING},
    {"Thinking", EXPR_THINKING,   DIALOG_MORNING},
    {"Surprised",EXPR_SURPRISED,  DIALOG_MORNING},
    {"Sad",      EXPR_SAD,        DIALOG_MORNING},
};
#define SC_COUNT (sizeof(scenarios)/sizeof(scenarios[0]))

static void _sc_close_cb(lv_event_t *e)
{
    scenario_overlay_close();
    if (s_on_close) s_on_close();
}

static void _sc_cb(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    if (idx < 0 || idx >= (int)SC_COUNT) return;
    // 先关 overlay, 在面部显示表情+气泡
    scenario_overlay_close();
    if (s_on_close) s_on_close();
    expression_set(scenarios[idx].expr, true);
    dialog_bubble_show(lv_screen_active(), scenarios[idx].dialog, 3000);
}

lv_obj_t *scenario_overlay_create(lv_obj_t *parent, void (*on_close)(void))
{
    if (s_overlay) return s_overlay;
    s_on_close = on_close;

    bool is_tech = (theme_v3_get_current() == THEME_TECH);
    lv_color_t fg = theme_fg();
    lv_color_t bg = lv_color_hex(0x0F172A);
    lv_color_t btn_bg = lv_color_hex(0x1E293B);

    s_overlay = lv_obj_create(parent);
    lv_obj_set_size(s_overlay, 320, 240);
    lv_obj_set_pos(s_overlay, 0, 0);
    lv_obj_set_style_bg_color(s_overlay, bg, 0);
    lv_obj_set_style_bg_opa(s_overlay, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_overlay, 0, 0);
    lv_obj_set_style_pad_all(s_overlay, 8, 0);
    lv_obj_add_flag(s_overlay, LV_OBJ_FLAG_CLICKABLE);

    // 标题
    lv_obj_t *title = lv_label_create(s_overlay);
    lv_label_set_text(title, "Dev Console / Scenario Sim");
    lv_obj_set_style_text_color(title, fg, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

    // 关闭按钮
    lv_obj_t *cls = lv_btn_create(s_overlay);
    lv_obj_set_size(cls, 40, 24);
    lv_obj_set_style_radius(cls, 12, 0);
    lv_obj_set_style_bg_color(cls, fg, 0);
    lv_obj_align(cls, LV_ALIGN_TOP_RIGHT, -8, 4);
    lv_obj_t *cl = lv_label_create(cls);
    lv_label_set_text(cl, "X");
    lv_obj_set_style_text_color(cl, lv_color_hex(0x000000), 0);
    lv_obj_set_style_text_font(cl, &lv_font_montserrat_14, 0);
    lv_obj_center(cl);
    lv_obj_add_event_cb(cls, _sc_close_cb, LV_EVENT_CLICKED, NULL);

    // 场景按钮网格 (3列 × N行)
    int x0 = 8, y0 = 30, bw = 96, bh = 34, gap = 4;
    for (int i = 0; i < (int)SC_COUNT; i++) {
        int col = i % 3;
        int row = i / 3;
        lv_obj_t *btn = lv_btn_create(s_overlay);
        lv_obj_set_size(btn, bw, bh);
        lv_obj_set_pos(btn, x0 + col * (bw + gap), y0 + row * (bh + gap));
        lv_obj_set_style_radius(btn, 6, 0);
        lv_obj_set_style_bg_color(btn, btn_bg, 0);
        lv_obj_set_style_border_color(btn, fg, 0);
        lv_obj_set_style_border_width(btn, 1, 0);
        lv_obj_set_style_text_color(btn, fg, 0);
        lv_obj_add_event_cb(btn, _sc_cb, LV_EVENT_CLICKED, (void*)(intptr_t)i);
        lv_obj_t *lb = lv_label_create(btn);
        lv_label_set_text(lb, scenarios[i].label);
        lv_obj_set_style_text_font(lb, &lv_font_montserrat_14, 0);
        lv_obj_center(lb);
    }

    // 体感状态标签
    lv_obj_t *imu_lbl = lv_label_create(s_overlay);
    lv_label_set_text(imu_lbl, "IMU: tilt=curious/yawn/look  shake=dizzy  tap=wink");
    lv_obj_set_style_text_color(imu_lbl, lv_color_hex(0x64748B), 0);
    lv_obj_set_style_text_font(imu_lbl, &lv_font_montserrat_14, 0);
    lv_obj_align(imu_lbl, LV_ALIGN_BOTTOM_MID, 0, -4);

    ESP_LOGI(TAG, "Scenario overlay: %d scenarios", (int)SC_COUNT);
    return s_overlay;
}

void scenario_overlay_close(void)
{
    if (!s_overlay) return;
    lv_obj_delete(s_overlay);
    s_overlay = NULL;
}
