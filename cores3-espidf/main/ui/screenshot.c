/*
 * 截图回传系统 (异步版本)
 * LVGL snapshot 在主循环 → 编码输出移到独立 FreeRTOS task
 * 不阻塞 UI 响应
 */
#include "screenshot.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <esp_log.h>
#include <esp_heap_caps.h>
#include <string.h>

static const char *TAG = "SNAP";
static volatile bool snap_requested = false;
static uint16_t *snap_buf = NULL;       // PSRAM buffer
static SemaphoreHandle_t snap_mutex = NULL;

/* LVGL snapshot → PSRAM buffer (主循环调用, 很快) */
static bool _do_snapshot(void)
{
    lv_obj_t *screen = lv_screen_active();
    lv_image_dsc_t dsc;
    lv_result_t res = lv_snapshot_take_to_buf(screen, LV_COLOR_FORMAT_RGB565, &dsc,
                                               snap_buf, 320 * 240 * 2);
    if (res != LV_RESULT_OK) {
        ESP_LOGE(TAG, "snapshot err: %d", (int)res);
        return false;
    }
    return true;
}

/* base64 编码 + 输出 (在独立 task 中运行, 不阻塞 UI) */
static const char B64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static void _output_task(void *arg)
{
    uint16_t *buf = (uint16_t *)arg;

    ESP_LOGI(TAG, "SCREENSHOT_START 153600");

    uint8_t *data = (uint8_t *)buf;
    char line[128];
    int pos = 0;

    for (size_t i = 0; i + 2 < 320 * 240 * 2; i += 3) {
        uint32_t v = ((uint32_t)data[i] << 16) | ((uint32_t)data[i+1] << 8) | data[i+2];
        line[pos++] = B64[(v >> 18) & 0x3F];
        line[pos++] = B64[(v >> 12) & 0x3F];
        line[pos++] = B64[(v >>  6) & 0x3F];
        line[pos++] = B64[ v        & 0x3F];
        if (pos >= 120) { line[pos] = '\0'; ESP_LOGI(TAG, "SS:%s", line); pos = 0; }
    }

    // final 2 bytes (153600 is divisible by 3, so no remainder)

    if (pos > 0) { line[pos] = '\0'; ESP_LOGI(TAG, "SS:%s", line); }

    ESP_LOGI(TAG, "SCREENSHOT_END");
    heap_caps_free(buf);
    xSemaphoreGive(snap_mutex);
    vTaskDelete(NULL);
}

void screenshot_process_pending(void)
{
    if (!snap_requested) return;
    snap_requested = false;

    // 尝试获取信号量 (非阻塞) — 如果上一张截图还在输出, 跳过
    if (xSemaphoreTake(snap_mutex, 0) != pdTRUE) {
        ESP_LOGW(TAG, "上一张截图还在处理, 跳过");
        return;
    }

    if (_do_snapshot()) {
        // 把 snapshot buffer 交给 output task, 主循环立刻继续
        xTaskCreate(_output_task, "snap_out", 4096, snap_buf, 1, NULL);
        // snap_mutex 在 output_task 完成时释放
    } else {
        xSemaphoreGive(snap_mutex);
    }
}

void screenshot_request(void)
{
    snap_requested = true;
}

void screenshot_init(void)
{
    // 分配 PSRAM 缓冲 (预分配, 避免每次截图都 alloc)
    snap_buf = (uint16_t *)heap_caps_malloc(320 * 240 * 2, MALLOC_CAP_SPIRAM);
    if (!snap_buf) {
        ESP_LOGE(TAG, "PSRAM alloc fail! 截图不可用");
        return;
    }

    snap_mutex = xSemaphoreCreateBinary();
    xSemaphoreGive(snap_mutex);

    ESP_LOGI(TAG, "截图系统就绪 (双击触发, 异步编码不卡UI)");
}

void screenshot_boot_trigger(void)
{
    // 不再自动触发 — 避免阻塞启动后的 UI 响应
}
