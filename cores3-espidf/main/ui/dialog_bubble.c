/* v5.0 DialogBubble */
#include "dialog_bubble.h"
#include <esp_log.h>

static lv_obj_t *bubble = NULL;
static lv_timer_t *bubble_timer = NULL;

static void _auto_close(lv_timer_t *t)
{
    dialog_bubble_close();
}

lv_obj_t *dialog_bubble_show(lv_obj_t *parent, DialogType type, uint32_t duration_ms)
{
    if (bubble) dialog_bubble_close();

    bool is_tech = (theme_v3_get_current() == THEME_TECH);
    bool is_dev  = (theme_v3_get_current() == THEME_DEV);

    const char *text = "";
    switch (type) {
    case DIALOG_MORNING: text = "Good morning!"; break;
    case DIALOG_SUGGEST: text = "Try naughty face?"; break;
    case DIALOG_SLEEP:   text = "Time to rest~"; break;
    }

    lv_color_t bg = is_tech ? lv_color_hex(0x083344) : is_dev ? lv_color_hex(0x14532D) : lv_color_hex(0xFFF5E0);
    lv_color_t fg = is_tech ? lv_color_hex(0x67E8F9) : is_dev ? lv_color_hex(0x4ADE80) : lv_color_hex(0xFF7F50);
    lv_color_t border = is_tech ? lv_color_hex(0x0891B2) : is_dev ? lv_color_hex(0x16A34A) : lv_color_hex(0xFFD5B0);

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
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
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
