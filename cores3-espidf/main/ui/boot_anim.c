/* v5.0 开机动画 */
#include "boot_anim.h"
#include <esp_log.h>

static const char *TAG = "BOOT";
static lv_obj_t *anim_screen = NULL;
static void (*s_on_done)(void) = NULL;

static void _anim_done_cb(lv_anim_t *a)
{
    if (anim_screen) { lv_obj_delete(anim_screen); anim_screen = NULL; }
    if (s_on_done) s_on_done();
    ESP_LOGI(TAG, "boot done");
}

/* Tech: cyan scanline + SYSTEM BOOTING + dots */
static void _boot_tech(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);

    // Scanline
    lv_obj_t *line = lv_obj_create(parent);
    lv_obj_set_size(line, 2, 2);
    lv_obj_set_style_bg_color(line, lv_color_hex(0x22D3EE), 0);
    lv_obj_set_style_bg_opa(line, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(line, 0, 0);
    lv_obj_set_style_radius(line, 0, 0);
    lv_obj_set_pos(line, 30, 110);
    lv_obj_set_style_shadow_width(line, 20, 0);
    lv_obj_set_style_shadow_color(line, lv_color_hex(0x22D3EE), 0);
    lv_obj_set_style_shadow_opa(line, LV_OPA_80, 0);

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, line);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_width);
    lv_anim_set_values(&a, 2, 240);
    lv_anim_set_time(&a, 500);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_start(&a);

    // Text
    lv_obj_t *txt = lv_label_create(parent);
    lv_label_set_text(txt, "SYSTEM BOOTING...");
    lv_obj_set_style_text_color(txt, lv_color_hex(0x22D3EE), 0);
    lv_obj_set_style_text_font(txt, &lv_font_montserrat_14, 0);
    lv_obj_align(txt, LV_ALIGN_CENTER, 0, 20);

    lv_anim_init(&a);
    lv_anim_set_var(&a, txt);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_style_opa);
    lv_anim_set_values(&a, LV_OPA_0, LV_OPA_COVER);
    lv_anim_set_time(&a, 300);
    lv_anim_set_delay(&a, 1000);
    lv_anim_start(&a);
    // Fade in-out
    lv_anim_init(&a);
    lv_anim_set_var(&a, txt);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_style_opa);
    lv_anim_set_values(&a, LV_OPA_COVER, LV_OPA_50);
    lv_anim_set_time(&a, 800);
    lv_anim_set_delay(&a, 1500);
    lv_anim_set_playback_time(&a, 800);
    lv_anim_set_repeat_count(&a, 2);
    lv_anim_start(&a);

    // Dots
    for (int i = 0; i < 5; i++) {
        lv_obj_t *dot = lv_obj_create(parent);
        lv_obj_set_size(dot, 14, 6);
        lv_obj_set_style_bg_color(dot, lv_color_hex(0x22D3EE), 0);
        lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(dot, 0, 0);
        lv_obj_set_style_radius(dot, 3, 0);
        lv_obj_set_style_shadow_width(dot, 8, 0);
        lv_obj_set_style_shadow_color(dot, lv_color_hex(0x22D3EE), 0);
        lv_obj_set_style_shadow_opa(dot, LV_OPA_50, 0);
        lv_obj_align(dot, LV_ALIGN_BOTTOM_MID, (i-2)*22, -40);

        lv_anim_init(&a);
        lv_anim_set_var(&a, dot);
        lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_style_opa);
        lv_anim_set_values(&a, LV_OPA_0, LV_OPA_COVER);
        lv_anim_set_time(&a, 100);
        lv_anim_set_delay(&a, 1500 + i * 200);
        lv_anim_start(&a);
    }

    // Done timer
    lv_anim_init(&a);
    lv_anim_set_var(&a, parent);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_y);
    lv_anim_set_values(&a, 0, 0);
    lv_anim_set_time(&a, 1);
    lv_anim_set_delay(&a, 3500);
    lv_anim_set_ready_cb(&a, _anim_done_cb);
    lv_anim_start(&a);
}

/* Child: coral circle with :) bouncing + HELLO! */
static void _boot_child(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, lv_color_hex(0xFFF9E6), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);

    // Coral circle
    lv_obj_t *circle = lv_obj_create(parent);
    lv_obj_set_size(circle, 60, 60);
    lv_obj_set_style_bg_color(circle, lv_color_hex(0xFF7F50), 0);
    lv_obj_set_style_bg_opa(circle, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(circle, 0, 0);
    lv_obj_set_style_radius(circle, 30, 0);
    lv_obj_set_style_shadow_width(circle, 16, 0);
    lv_obj_set_style_shadow_color(circle, lv_color_hex(0xFF7F50), 0);
    lv_obj_set_style_shadow_opa(circle, LV_OPA_50, 0);
    lv_obj_align(circle, LV_ALIGN_CENTER, 0, -10);

    lv_obj_t *face = lv_label_create(circle);
    lv_label_set_text(face, ":)");
    lv_obj_set_style_text_color(face, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(face, &lv_font_montserrat_14, 0);
    lv_obj_center(face);

    // Bounce
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, circle);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_y);
    lv_anim_set_values(&a, lv_obj_get_y(circle), lv_obj_get_y(circle) - 15);
    lv_anim_set_time(&a, 500);
    lv_anim_set_playback_time(&a, 500);
    lv_anim_set_repeat_count(&a, 2);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
    lv_anim_start(&a);

    // HELLO!
    lv_obj_t *txt = lv_label_create(parent);
    lv_label_set_text(txt, "HELLO!");
    lv_obj_set_style_text_color(txt, lv_color_hex(0xFF7F50), 0);
    lv_obj_set_style_text_font(txt, &lv_font_montserrat_14, 0);
    lv_obj_align(txt, LV_ALIGN_BOTTOM_MID, 0, -50);

    lv_anim_init(&a);
    lv_anim_set_var(&a, txt);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_style_opa);
    lv_anim_set_values(&a, LV_OPA_0, LV_OPA_COVER);
    lv_anim_set_time(&a, 500);
    lv_anim_set_delay(&a, 1200);
    lv_anim_start(&a);

    // Done
    lv_anim_init(&a);
    lv_anim_set_var(&a, parent);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_y);
    lv_anim_set_values(&a, 0, 0);
    lv_anim_set_time(&a, 1);
    lv_anim_set_delay(&a, 3500);
    lv_anim_set_ready_cb(&a, _anim_done_cb);
    lv_anim_start(&a);
}

/* Dev: terminal boot text */
static void _boot_dev(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);

    const char *lines[] = {
        "> INITIALIZING KERNEL...",
        "> LOADING PERSONALITY...",
        "> WAKING UP AI...",
    };
    for (int i = 0; i < 3; i++) {
        lv_obj_t *t = lv_label_create(parent);
        lv_label_set_text(t, lines[i]);
        lv_obj_set_style_text_color(t, lv_color_hex(0x22C55E), 0);
        lv_obj_set_style_text_font(t, &lv_font_montserrat_14, 0);
        lv_obj_align(t, LV_ALIGN_LEFT_MID, 30, (i-1) * 20);

        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, t);
        lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_style_opa);
        lv_anim_set_values(&a, LV_OPA_0, LV_OPA_COVER);
        lv_anim_set_time(&a, 100);
        lv_anim_set_delay(&a, i * 800);
        lv_anim_start(&a);
    }

    // Blinking cursor
    lv_obj_t *cursor = lv_obj_create(parent);
    lv_obj_set_size(cursor, 12, 16);
    lv_obj_set_style_bg_color(cursor, lv_color_hex(0x22C55E), 0);
    lv_obj_set_style_bg_opa(cursor, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(cursor, 0, 0);
    lv_obj_set_style_radius(cursor, 0, 0);
    lv_obj_align(cursor, LV_ALIGN_LEFT_MID, 230, 20);

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, cursor);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_style_opa);
    lv_anim_set_values(&a, LV_OPA_COVER, LV_OPA_0);
    lv_anim_set_time(&a, 400);
    lv_anim_set_playback_time(&a, 400);
    lv_anim_set_delay(&a, 2200);
    lv_anim_set_repeat_count(&a, 3);
    lv_anim_start(&a);

    // Done
    lv_anim_init(&a);
    lv_anim_set_var(&a, parent);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_y);
    lv_anim_set_values(&a, 0, 0);
    lv_anim_set_time(&a, 1);
    lv_anim_set_delay(&a, 3500);
    lv_anim_set_ready_cb(&a, _anim_done_cb);
    lv_anim_start(&a);
}

void boot_anim_play(lv_obj_t *parent, void (*on_done)(void))
{
    s_on_done = on_done;
    anim_screen = lv_obj_create(parent);
    lv_obj_set_size(anim_screen, 320, 240);
    lv_obj_set_pos(anim_screen, 0, 0);
    lv_obj_set_style_border_width(anim_screen, 0, 0);
    lv_obj_set_style_pad_all(anim_screen, 0, 0);

    ThemeV3 t = theme_v3_get_current();
    if (t == THEME_CHILD) _boot_child(anim_screen);
    else if (t == THEME_COCOA) _boot_dev(anim_screen);
    else _boot_tech(anim_screen);
}
