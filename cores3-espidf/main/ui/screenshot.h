#pragma once

// ========== 截图调测开关 ==========
// 注释下面这行即可完全禁用截图功能（零开销）
#define SCREENSHOT_ENABLE  1

#include <esp_http_server.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

#if SCREENSHOT_ENABLE
void screenshot_feed(int x1, int y1, int w, int h, const uint16_t *data);
void screenshot_capture(void);
void screenshot_register(httpd_handle_t server);
void screenshot_sta_start(void);
void screenshot_sta_stop(void);
#else
static inline void screenshot_feed(int x1,int y1,int w,int h,const uint16_t *d){(void)x1;(void)y1;(void)w;(void)h;(void)d;}
static inline void screenshot_capture(void){}
static inline void screenshot_register(httpd_handle_t s){(void)s;}
static inline void screenshot_sta_start(void){}
static inline void screenshot_sta_stop(void){}
#endif

#ifdef __cplusplus
}
#endif
