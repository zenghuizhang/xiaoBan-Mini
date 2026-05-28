// screenshot.c — accumulated framebuffer from flush callback
#include "screenshot.h"
#if SCREENSHOT_ENABLE
#include <esp_log.h>
#include <esp_netif.h>
#include <esp_http_server.h>
#include <esp_heap_caps.h>
#include <string.h>
#include <stdlib.h>
#include <lvgl.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

static const char *TAG = "SCR";
static httpd_handle_t s_httpd = NULL;
static SemaphoreHandle_t s_mutex = NULL;
static uint8_t *s_bmp = NULL;
static int s_size = 0;
static uint16_t *s_fb = NULL;  // accumulated full framebuffer 320x240 RGB565
static int s_fb_w = 320, s_fb_h = 240;
static bool s_fb_dirty = false;

// Called from LVGL flush callback (main task) — accumulate flushed areas
void screenshot_feed(int x1, int y1, int w, int h, const uint16_t *data) {
    if (!s_fb) {
        s_fb = (uint16_t*)heap_caps_calloc(s_fb_w * s_fb_h, 2, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (!s_fb) return;
    }
    for (int y = 0; y < h; y++) {
        memcpy(&s_fb[(y1 + y) * s_fb_w + x1], &data[y * w], w * 2);
    }
    s_fb_dirty = true;
}

// Build BMP from accumulated framebuffer
static void _build_bmp(void) {
    if (!s_fb || !s_fb_dirty) return;
    s_fb_dirty = false;
    int row = ((s_fb_w * 24 + 31) / 32) * 4, img = row * s_fb_h, total = 54 + img;
    uint8_t *bmp = (uint8_t*)heap_caps_calloc(1, total, MALLOC_CAP_SPIRAM);
    if (!bmp) return;
    bmp[0]='B'; bmp[1]='M'; *(uint32_t*)(bmp+2)=total; *(uint32_t*)(bmp+10)=54; *(uint32_t*)(bmp+14)=40;
    *(int32_t*)(bmp+18)=s_fb_w; *(int32_t*)(bmp+22)=-(int32_t)s_fb_h;
    *(uint16_t*)(bmp+26)=1; *(uint16_t*)(bmp+28)=24; *(uint32_t*)(bmp+34)=img;
    for (int y = 0; y < s_fb_h; y++) { uint8_t *r = bmp + 54 + y * row;
        for (int x = 0; x < s_fb_w; x++) { uint16_t c = s_fb[y * s_fb_w + x];
            c = (c >> 8) | (c << 8); // undo GC9A01 byte swap for BMP
            uint8_t b5=c&0x1F,g6=(c>>5)&0x3F,r5=(c>>11)&0x1F;
            r[x*3+0]=(uint8_t)((b5*255+15)/31); r[x*3+1]=(uint8_t)((g6*255+31)/63); r[x*3+2]=(uint8_t)((r5*255+15)/31); }
    }
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    free(s_bmp); s_bmp = bmp; s_size = total;
    xSemaphoreGive(s_mutex);
}

void screenshot_capture(void) {
    if (!s_mutex) s_mutex = xSemaphoreCreateMutex();
    if (!s_mutex) return;
    if (!s_httpd) screenshot_sta_start();
    _build_bmp();
}

static esp_err_t _handler(httpd_req_t *req) {
    if (!s_mutex) { httpd_resp_send_500(req); return ESP_FAIL; }
    xSemaphoreTake(s_mutex, pdMS_TO_TICKS(5000));
    if (s_bmp && s_size > 0) {
        httpd_resp_set_type(req, "image/bmp");
        esp_err_t ret = httpd_resp_send(req, (const char*)s_bmp, s_size);
        xSemaphoreGive(s_mutex);
        return ret;
    }
    xSemaphoreGive(s_mutex);
    httpd_resp_send_500(req);
    return ESP_FAIL;
}

void screenshot_register(httpd_handle_t server) {
    httpd_uri_t u = {.uri="/screen.bmp", .method=HTTP_GET, .handler=_handler};
    httpd_register_uri_handler(server, &u);
    if (!s_mutex) s_mutex = xSemaphoreCreateMutex();
}

void screenshot_sta_start(void) {
    if (s_httpd) return;
    if (!s_mutex) s_mutex = xSemaphoreCreateMutex();
    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    cfg.server_port = 80; cfg.max_open_sockets = 3; cfg.lru_purge_enable = true;
    if (httpd_start(&s_httpd, &cfg) != ESP_OK) { ESP_LOGW(TAG,"httpd fail"); return; }
    httpd_uri_t u = {.uri="/screen.bmp", .method=HTTP_GET, .handler=_handler};
    httpd_register_uri_handler(s_httpd, &u);
    esp_netif_ip_info_t ip; esp_netif_t *nif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (nif && esp_netif_get_ip_info(nif,&ip)==ESP_OK)
        ESP_LOGI(TAG, "http://" IPSTR "/screen.bmp", IP2STR(&ip.ip));
}
void screenshot_sta_stop(void) {
    if(s_httpd){httpd_stop(s_httpd);s_httpd=NULL;}
}

#endif /* SCREENSHOT_ENABLE */
