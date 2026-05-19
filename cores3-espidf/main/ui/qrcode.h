/*
 * QR 码显示组件 — WiFi 扫码配网
 * 用 lv_draw_rect 在屏幕上绘制 QR 码
 */
#pragma once
#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

/** 创建并显示 WiFi 配网 QR 码界面 (http://192.168.4.1/) */
lv_obj_t *qrcode_create(lv_obj_t *parent);

/** 关闭 QR 码界面 */
void qrcode_close(void);

/** 检查 QR 码是否正在显示 */
bool qrcode_is_shown(void);

#ifdef __cplusplus
}
#endif
