/*
 * QR 码显示 (V2 — lv_draw_rect 绘制, 零额外 obj)
 * URL: http://192.168.4.1/
 */
#include "qrcode.h"
#include "theme_v3.h"
#include "expressions.h"
#include <esp_log.h>

#define QR_SIZE 31
#define QR_MODULE 4  // 31×4=124px + border

static const uint8_t qr_matrix[QR_SIZE][QR_SIZE] = {
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,1,1,1,1,1,1,1,0,0,1,1,1,0,1,0,1,1,0,0,0,0,0,1,1,1,1,1,1,1,0},
    {0,1,0,0,0,0,0,1,0,0,0,1,0,0,1,0,1,0,0,0,0,1,0,1,0,0,0,0,0,1,0},
    {0,1,0,1,1,1,0,1,0,0,1,0,0,0,0,1,1,1,1,0,0,1,0,1,0,1,1,1,0,1,0},
    {0,1,0,1,1,1,0,1,0,0,1,1,0,0,1,0,0,1,0,0,0,0,0,1,0,1,1,1,0,1,0},
    {0,1,0,1,1,1,0,1,0,0,0,1,0,1,0,1,1,0,0,0,0,1,0,1,0,1,1,1,0,1,0},
    {0,1,0,0,0,0,0,1,0,1,1,1,0,0,0,0,1,1,1,0,1,1,0,1,0,0,0,0,0,1,0},
    {0,1,1,1,1,1,1,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,1,1,1,1,1,1,0},
    {0,0,0,0,0,0,0,0,0,0,1,1,0,1,1,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0},
    {0,1,0,0,1,0,1,1,0,1,0,0,1,0,0,1,0,1,0,1,0,1,1,0,1,0,0,0,0,0,0},
    {0,0,0,1,1,1,0,0,1,1,1,1,1,0,1,0,0,1,0,0,0,0,0,1,0,0,1,1,1,0,0},
    {0,0,1,1,0,0,0,1,0,1,0,1,0,1,1,1,0,1,0,0,1,0,0,0,1,0,0,1,1,0,0},
    {0,1,1,1,1,0,1,0,0,1,0,1,0,1,1,0,1,0,0,1,0,1,0,1,0,0,1,1,1,1,0},
    {0,1,0,1,1,1,1,1,0,1,0,1,1,0,0,1,1,1,0,0,0,0,0,1,1,0,1,0,1,1,0},
    {0,0,1,1,0,0,1,0,0,1,0,0,1,1,1,1,0,0,1,0,1,0,0,0,0,0,0,1,1,1,0},
    {0,0,1,0,0,0,0,1,1,1,1,0,0,1,1,1,0,1,0,1,0,0,0,0,0,1,0,1,1,1,0},
    {0,1,1,0,0,1,0,0,0,1,0,1,0,0,0,0,0,1,1,0,1,0,0,1,1,0,0,0,0,0,0},
    {0,0,1,0,0,1,1,1,1,0,1,1,0,1,1,0,0,0,1,0,1,0,1,0,0,0,0,1,0,1,0},
    {0,0,1,0,1,1,0,0,1,0,0,0,0,1,1,1,0,1,1,0,1,0,0,0,1,0,0,1,1,0,0},
    {0,1,0,1,0,0,0,1,1,0,0,0,0,1,0,1,0,1,1,0,0,0,0,1,0,1,0,1,1,1,0},
    {0,0,0,0,1,0,1,0,0,0,0,1,0,1,0,1,0,0,1,1,1,0,0,0,1,0,0,0,0,0,0},
    {0,1,0,0,0,1,1,1,0,1,0,1,0,0,0,0,1,1,0,0,0,1,1,1,1,1,1,1,1,1,0},
    {0,0,0,0,0,0,0,0,0,1,1,0,1,1,0,1,0,1,0,1,0,1,0,0,0,1,1,1,0,0,0},
    {0,1,1,1,1,1,1,1,0,0,0,0,1,1,0,0,0,1,0,1,1,1,0,1,0,1,1,0,1,0,0},
    {0,1,0,0,0,0,0,1,0,1,0,0,1,1,0,1,0,1,0,0,0,1,0,0,0,1,0,1,0,0,0},
    {0,1,0,1,1,1,0,1,0,0,1,1,0,0,0,0,1,1,1,1,0,1,1,1,1,1,0,0,1,1,0},
    {0,1,0,1,1,1,0,1,0,1,0,1,0,1,1,0,0,0,0,0,1,0,1,1,0,1,1,1,1,0,0},
    {0,1,0,1,1,1,0,1,0,0,0,0,1,1,1,0,0,0,1,0,1,0,1,0,0,1,0,1,0,1,0},
    {0,1,0,0,0,0,0,1,0,0,0,0,1,1,0,0,0,0,1,1,1,0,1,1,0,1,0,0,1,0,0},
    {0,1,1,1,1,1,1,1,0,1,0,1,1,1,0,0,0,0,0,0,0,1,0,1,1,0,1,0,1,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
};

static const char *TAG = "QRCODE";
static lv_obj_t *qr_screen = NULL;
static int qr_offset_x = 0, qr_offset_y = 0;
static void qrcode_close_cb(lv_event_t *e);

/* DRAW_POST: 用 lv_draw_rect 画 QR 码模块 (零额外 obj, 不耗内存) */
static void _qr_draw_cb(lv_event_t *e)
{
    lv_layer_t *layer = lv_event_get_layer(e);
    if (!layer) return;

    lv_draw_rect_dsc_t dsc;
    lv_draw_rect_dsc_init(&dsc);
    dsc.bg_color = lv_color_hex(0x000000);
    dsc.bg_opa = LV_OPA_COVER;
    dsc.radius = 0;
    dsc.border_width = 0;

    for (int r = 0; r < QR_SIZE; r++) {
        for (int c = 0; c < QR_SIZE; c++) {
            if (!qr_matrix[r][c]) continue;
            lv_area_t a;
            int x0 = qr_offset_x + c * QR_MODULE;
            int y0 = qr_offset_y + r * QR_MODULE;
            a.x1 = x0; a.y1 = y0;
            a.x2 = x0 + QR_MODULE - 1;
            a.y2 = y0 + QR_MODULE - 1;
            lv_draw_rect(layer, &dsc, &a);
        }
    }
}

lv_obj_t *qrcode_create(lv_obj_t *parent)
{
    if (qr_screen) return qr_screen;

    bool is_adult = (theme_v3_get_current() == THEME_ADULT);
    lv_color_t bg = is_adult ? ADULT_BG : CHILD_BG;
    lv_color_t fg = is_adult ? ADULT_FG : CHILD_FG;
    lv_color_t white = lv_color_hex(0xFFFFFF);

    qr_screen = lv_obj_create(parent);
    lv_obj_set_size(qr_screen, 320, 240);
    lv_obj_set_pos(qr_screen, 0, 0);
    lv_obj_set_style_bg_color(qr_screen, bg, 0);
    lv_obj_set_style_bg_opa(qr_screen, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(qr_screen, 0, 0);
    lv_obj_set_style_pad_all(qr_screen, 0, 0);  // 无内边距, DRAW_POST 坐标准确
    lv_obj_add_flag(qr_screen, LV_OBJ_FLAG_CLICKABLE);

    // QR 码模块大小和位置
    int qr_px = QR_SIZE * QR_MODULE;  // 27*5=135
    int border = 4;  // 白色边框
    int total_w = qr_px + border * 2;
    qr_offset_x = (320 - total_w) / 2 + border;
    qr_offset_y = (240 - total_w) / 2 + border;

    // 标题 (QR 码上方)
    lv_obj_t *title = lv_label_create(qr_screen);
    lv_label_set_text(title, "Scan to connect WiFi");
    lv_obj_set_style_text_color(title, fg, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 6);

    // 提示 (QR 码下方)
    lv_obj_t *hint = lv_label_create(qr_screen);
    lv_label_set_text(hint, "ESP32-S3-Box-Config\nThen visit 192.168.4.1");
    lv_obj_set_style_text_color(hint, is_adult ? lv_color_hex(0x71717A) : lv_color_hex(0xA8A29E), 0);
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_14, 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -6);

    // 白色背景
    lv_obj_t *qr_bg = lv_obj_create(qr_screen);
    lv_obj_set_size(qr_bg, total_w, total_w);
    lv_obj_set_pos(qr_bg, (320 - total_w) / 2, (240 - total_w) / 2);
    lv_obj_set_style_bg_color(qr_bg, white, 0);
    lv_obj_set_style_bg_opa(qr_bg, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(qr_bg, 0, 0);
    lv_obj_set_style_radius(qr_bg, 0, 0);
    lv_obj_set_style_pad_all(qr_bg, 0, 0);

    // QR 模块用 DRAW_POST 绘制 (零额外 obj!)
    lv_obj_add_event_cb(qr_screen, _qr_draw_cb, LV_EVENT_DRAW_POST, NULL);

    // 点击关闭
    lv_obj_add_event_cb(qr_screen, qrcode_close_cb, LV_EVENT_CLICKED, NULL);

    expression_set_drawing_enabled(false);

    ESP_LOGI(TAG, "QR 码已显示 (%dx%d, %d modules drawn)", qr_px, qr_px, QR_SIZE*QR_SIZE);
    return qr_screen;
}

static void qrcode_close_cb(lv_event_t *e) { qrcode_close(); }

void qrcode_close(void)
{
    if (!qr_screen) return;
    lv_obj_delete(qr_screen);
    qr_screen = NULL;
    expression_set_drawing_enabled(true);
    ESP_LOGI(TAG, "QR 码已关闭");
}

bool qrcode_is_shown(void)
{
    return qr_screen != NULL;
}
