/* wake_word.cpp — ESP-SR WakeNet 唤醒词检测 */
#include "wake_word.h"
#include <M5Unified.h>
#include <esp_log.h>
#include <esp_afe_sr_iface.h>
#include <esp_afe_sr_models.h>
#include <esp_afe_config.h>
#include <model_path.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string.h>

static const char *TAG = "WAKE";

static const esp_afe_sr_iface_t *s_afe_handle = NULL;
static esp_afe_sr_data_t        *s_afe_data   = NULL;
static afe_config_t             *s_afe_config = NULL;
static srmodel_list_t           *s_models     = NULL;
static wake_word_detected_cb_t   s_cb = NULL;
static TaskHandle_t              s_task = NULL;
static volatile bool             s_running = false;

/* 唤醒词检测任务: 持续从 M5.Mic 读取音频并喂给 AFE */
static void _wake_task(void *arg)
{
    (void)arg;
    int chunk_size = s_afe_handle->get_feed_chunksize(s_afe_data);
    int channels   = s_afe_handle->get_feed_channel_num(s_afe_data);
    ESP_LOGI(TAG, "AFE chunk_size=%d channels=%d", chunk_size, channels);

    int16_t *feed_buf = (int16_t *)heap_caps_malloc(chunk_size * channels * sizeof(int16_t),
                                                    MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!feed_buf) {
        ESP_LOGE(TAG, "feed_buf alloc failed");
        vTaskDelete(NULL);
        return;
    }

    while (s_running) {
        /* 从 M5.Mic 读取一帧音频 */
        int n = M5.Mic.record(feed_buf, chunk_size * channels);
        if (n <= 0) {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        /* 喂给 AFE */
        s_afe_handle->feed(s_afe_data, feed_buf);

        /* 拉取处理结果 */
        afe_fetch_result_t *res = s_afe_handle->fetch(s_afe_data);
        if (res && res->wakeup_state == WAKENET_DETECTED) {
            ESP_LOGI(TAG, "Wake word detected!");
            if (s_cb) s_cb();
            /* 唤醒后暂停检测，由 voice_manager 控制恢复 */
            s_running = false;
            break;
        }
    }

    free(feed_buf);
    vTaskDelete(NULL);
}

esp_err_t wake_word_init(void)
{
    /* 初始化麦克风 */
    M5.Mic.begin();
    M5.Mic.setSampleRate(16000);

    /* 加载模型 (从 "model" 分区) */
    s_models = esp_srmodel_init("model");
    if (!s_models) {
        ESP_LOGE(TAG, "esp_srmodel_init failed, no model partition?");
        return ESP_FAIL;
    }

    /* 配置 AFE: 单麦克风输入 "M" */
    s_afe_config = afe_config_init("M", s_models, AFE_TYPE_SR, AFE_MODE_LOW_COST);
    if (!s_afe_config) {
        ESP_LOGE(TAG, "afe_config_init failed");
        return ESP_FAIL;
    }

    /* 获取 AFE handle */
    s_afe_handle = esp_afe_handle_from_config(s_afe_config);
    if (!s_afe_handle) {
        ESP_LOGE(TAG, "esp_afe_handle_from_config failed");
        return ESP_FAIL;
    }

    /* 创建 AFE 数据 */
    s_afe_data = s_afe_handle->create_from_config(s_afe_config);
    if (!s_afe_data) {
        ESP_LOGE(TAG, "create_from_config failed");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "WakeNet initialized");
    return ESP_OK;
}

esp_err_t wake_word_start(void)
{
    if (s_running) return ESP_OK;
    s_running = true;
    xTaskCreatePinnedToCore(_wake_task, "wake_word", 8192, NULL, 5, &s_task, 1);
    return ESP_OK;
}

void wake_word_stop(void)
{
    s_running = false;
    if (s_task) {
        vTaskDelay(pdMS_TO_TICKS(100));
        s_task = NULL;
    }
}

void wake_word_set_callback(wake_word_detected_cb_t cb)
{
    s_cb = cb;
}

int wake_word_fetch_audio(int16_t *buf, int max_samples)
{
    if (!s_afe_data || !s_afe_handle) return 0;
    afe_fetch_result_t *res = s_afe_handle->fetch(s_afe_data);
    if (res && res->data_size > 0) {
        int n = res->data_size / sizeof(int16_t);
        if (n > max_samples) n = max_samples;
        memcpy(buf, res->data, n * sizeof(int16_t));
        return n;
    }
    return 0;
}

void wake_word_reset(void)
{
    if (s_afe_handle && s_afe_data) {
        s_afe_handle->disable_wakenet(s_afe_data);
        vTaskDelay(pdMS_TO_TICKS(50));
        s_afe_handle->enable_wakenet(s_afe_data);
    }
}
