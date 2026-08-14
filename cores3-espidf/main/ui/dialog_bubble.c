/* v7.6 DialogBubble — 使用 theme tokens, 支持 8 种场景 */
#include "dialog_bubble.h"
#include "font_zh_14.h"
#include <esp_log.h>

static lv_obj_t *bubble = NULL;
static lv_timer_t *bubble_timer = NULL;
extern bool s_lang_cn;  // 同步全局语言设置 (app_main.cpp)

void dialog_bubble_set_lang(bool cn) { s_lang_cn = cn; }
bool dialog_bubble_get_lang(void) { return s_lang_cn; }

static void _auto_close(lv_timer_t *t)
{
    dialog_bubble_close();
}

// v7.6: 使用 theme tokens 取色, 不再硬编码 per-theme
static void _bubble_style(lv_color_t *bg, lv_color_t *fg, lv_color_t *border)
{
    const theme_colors_t *th = theme_get_colors();
    *bg     = th->panel;
    *fg     = th->text;
    *border = th->border;
}

lv_obj_t *dialog_bubble_show(lv_obj_t *parent, DialogType type, uint32_t duration_ms)
{
    if (bubble) dialog_bubble_close();

    const char *text = "";
    switch (type) {
    case DIALOG_MORNING:    text = s_lang_cn ? "早上好呀！今天也是充满能量的一天。" : "Good morning! Full of energy today."; break;
    case DIALOG_SUGGEST:    text = s_lang_cn ? "要不要试试调皮表情？" : "Try a naughty face?"; break;
    case DIALOG_SLEEP:      text = s_lang_cn ? "该休息了哦~" : "Time to rest~"; break;
    case DIALOG_LONELY:     text = s_lang_cn ? "好无聊哦，陪我玩一会吧..." : "I'm bored... play with me?"; break;
    case DIALOG_OTA:        text = s_lang_cn ? "OTA 系统更新中..." : "OTA updating..."; break;
    case DIALOG_ERROR:      text = s_lang_cn ? "系统发生异常错误！" : "System error!"; break;
    case DIALOG_VOICE_WAKE: text = s_lang_cn ? "我在听..." : "I'm listening..."; break;
    case DIALOG_CUSTOM:     text = ""; break;
    }

    lv_color_t bg, fg, border;
    _bubble_style(&bg, &fg, &border);

    bubble = lv_obj_create(parent);
    lv_obj_set_size(bubble, 280, 40);
    lv_obj_set_style_bg_color(bubble, bg, 0);
    lv_obj_set_style_bg_opa(bubble, LV_OPA_90, 0);
    lv_obj_set_style_border_color(bubble, border, 0);
    lv_obj_set_style_border_width(bubble, 1, 0);
    lv_obj_set_style_radius(bubble, 14, 0);
    lv_obj_set_style_pad_all(bubble, 8, 0);
    lv_obj_align(bubble, LV_ALIGN_BOTTOM_MID, 0, -50);

    lv_obj_t *label = lv_label_create(bubble);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, fg, 0);
    lv_obj_set_style_text_font(label, s_lang_cn ? (const lv_font_t*)&font_zh_14 : &lv_font_montserrat_14, 0);
    lv_obj_center(label);

    if (duration_ms > 0)
        bubble_timer = lv_timer_create(_auto_close, duration_ms, NULL);

    return bubble;
}

lv_obj_t *dialog_bubble_show_text(lv_obj_t *parent, const char *cn, const char *en, uint32_t duration_ms)
{
    if (bubble) dialog_bubble_close();

    const char *text = s_lang_cn ? cn : en;

    lv_color_t bg, fg, border;
    _bubble_style(&bg, &fg, &border);

    bubble = lv_obj_create(parent);
    lv_obj_set_size(bubble, 280, 40);
    lv_obj_set_style_bg_color(bubble, bg, 0);
    lv_obj_set_style_bg_opa(bubble, LV_OPA_90, 0);
    lv_obj_set_style_border_color(bubble, border, 0);
    lv_obj_set_style_border_width(bubble, 1, 0);
    lv_obj_set_style_radius(bubble, 14, 0);
    lv_obj_set_style_pad_all(bubble, 8, 0);
    lv_obj_align(bubble, LV_ALIGN_BOTTOM_MID, 0, -50);

    lv_obj_t *label = lv_label_create(bubble);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, fg, 0);
    lv_obj_set_style_text_font(label, s_lang_cn ? (const lv_font_t*)&font_zh_14 : &lv_font_montserrat_14, 0);
    lv_obj_center(label);

    if (duration_ms > 0)
        bubble_timer = lv_timer_create(_auto_close, duration_ms, NULL);

    return bubble;
}

void dialog_bubble_close(void)
{
    if (bubble_timer) { lv_timer_delete(bubble_timer); bubble_timer = NULL; }
    if (bubble) { lv_obj_delete(bubble); bubble = NULL; }
}

bool dialog_bubble_is_shown(void) { return bubble != NULL; }
