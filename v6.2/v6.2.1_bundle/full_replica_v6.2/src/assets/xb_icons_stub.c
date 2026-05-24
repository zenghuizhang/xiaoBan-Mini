// xb_icons_stub.c — placeholder lv_image_dsc_t entries.
//
// On the real device, run `lv_img_conv` against assets/icons/*.svg (after PNG
// rasterization) to produce per-icon arrays, then drop this file from build.
// All names here match what the page code references.
//
// Each stub is a 16x16 fully-transparent ARGB8888 image so the UI renders
// without crashing during early bring-up. Recolor + opa work normally.
#include "lvgl.h"

static const uint8_t XB_ICON_BLANK_16[16 * 16 * 4] = {0};

#define XB_ICON(name) \
const lv_image_dsc_t name = { \
    .header = { .magic = LV_IMAGE_HEADER_MAGIC, .cf = LV_COLOR_FORMAT_ARGB8888, \
                .w = 16, .h = 16, .stride = 64 }, \
    .data_size = sizeof(XB_ICON_BLANK_16), \
    .data = XB_ICON_BLANK_16, \
}

// Brand
XB_ICON(ic_brand_mark);
XB_ICON(ic_loading_ring);

// Statusbar / chrome
XB_ICON(ic_chevron_left);
XB_ICON(ic_battery);
XB_ICON(ic_wifi);
XB_ICON(ic_ai_dot);
XB_ICON(ic_plug);

// Menu
XB_ICON(ic_message_circle);
XB_ICON(ic_layers);
XB_ICON(ic_sparkles);
XB_ICON(ic_terminal);
XB_ICON(ic_settings);

// Chat
XB_ICON(ic_mic);
XB_ICON(ic_send);
XB_ICON(ic_stop);
XB_ICON(ic_zap);

// Wi-Fi
XB_ICON(ic_qr);
XB_ICON(ic_check_circle);
XB_ICON(ic_x_circle);

// OTA
XB_ICON(ic_download);

// Skills
XB_ICON(ic_cloud);
XB_ICON(ic_book_open);
XB_ICON(ic_music);
XB_ICON(ic_clock);
XB_ICON(ic_thermometer);
XB_ICON(ic_shield);

// Settings extras
XB_ICON(ic_palette);
XB_ICON(ic_globe);
XB_ICON(ic_sun);
XB_ICON(ic_volume);
XB_ICON(ic_eye_off);
XB_ICON(ic_mic_off);
XB_ICON(ic_info);
XB_ICON(ic_alert_triangle);

// Console
XB_ICON(ic_compass);
XB_ICON(ic_play);
XB_ICON(ic_radio);
XB_ICON(ic_cpu);
XB_ICON(ic_activity);
