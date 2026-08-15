/* tts.cpp — 云端 TTS (OpenAI TTS 兼容 API) + M5.Speaker 播放 */
#include "tts.h"
#include "app_config.h"
#include <M5Unified.h>
#include <esp_log.h>
#include <esp_http_client.h>
#include <string.h>
#include <stdlib.h>

static const char *TAG = "TTS";

static volatile bool s_speaking = false;

/* HTTP 响应缓冲 */
typedef struct {
    uint8_t *buf;
    size_t   len;
    size_t   cap;
} http_resp_t;

static esp_err_t _http_event(esp_http_client_event_t *evt)
{
    http_resp_t *r = (http_resp_t *)evt->user_data;
    if (evt->event_id == HTTP_EVENT_ON_DATA) {
        if (r->len + evt->data_len > r->cap) {
            r->cap = (r->len + evt->data_len) * 2;
            r->buf = (uint8_t *)realloc(r->buf, r->cap);
        }
        memcpy(r->buf + r->len, evt->data, evt->data_len);
        r->len += evt->data_len;
    }
    return ESP_OK;
}

esp_err_t tts_speak(const char *text)
{
    if (!text || !text[0]) return ESP_ERR_INVALID_ARG;

    app_config_t cfg;
    app_config_load(&cfg);
    if (!cfg.api_key[0]) {
        ESP_LOGE(TAG, "No API key configured");
        return ESP_ERR_INVALID_STATE;
    }

    /* 构建 URL */
    char url[256];
    const char *base = cfg.base_url[0] ? cfg.base_url : "https://api.openai.com/v1";
    snprintf(url, sizeof(url), "%s/audio/speech", base);

    /* 构建 JSON body */
    char *body = (char *)malloc(strlen(text) + 256);
    if (!body) return ESP_ERR_NO_MEM;
    snprintf(body, strlen(text) + 256,
        "{\"model\":\"tts-1\",\"input\":\"%s\",\"voice\":\"alloy\",\"response_format\":\"pcm\"}",
        text);

    http_resp_t resp = {0};
    esp_http_client_config_t http_cfg = {
        .url = url,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 15000,
        .event_handler = _http_event,
        .user_data = &resp,
    };
    esp_http_client_handle_t client = esp_http_client_init(&http_cfg);

    char auth[160];
    snprintf(auth, sizeof(auth), "Bearer %s", cfg.api_key);
    esp_http_client_set_header(client, "Authorization", auth);
    esp_http_client_set_header(client, "Content-Type", "application/json");

    esp_http_client_set_post_field(client, body, strlen(body));

    s_speaking = true;
    esp_err_t err = esp_http_client_perform(client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "HTTP perform failed: %s", esp_err_to_name(err));
        free(body);
        esp_http_client_cleanup(client);
        s_speaking = false;
        return err;
    }

    int status = esp_http_client_get_status_code(client);
    if (status != 200) {
        ESP_LOGE(TAG, "TTS HTTP status %d", status);
        free(body);
        free(resp.buf);
        esp_http_client_cleanup(client);
        s_speaking = false;
        return ESP_FAIL;
    }

    /* 播放 PCM (24kHz 16-bit mono) */
    if (resp.buf && resp.len > 0) {
        ESP_LOGI(TAG, "TTS PCM size: %d bytes", (int)resp.len);
        M5.Speaker.playRaw((const int16_t *)resp.buf, resp.len / 2, 24000);
        /* 等待播放完成 */
        while (M5.Speaker.isPlaying()) {
            vTaskDelay(pdMS_TO_TICKS(50));
        }
        free(resp.buf);
    }

    free(body);
    esp_http_client_cleanup(client);
    s_speaking = false;
    return ESP_OK;
}

void tts_stop(void)
{
    M5.Speaker.stop();
    s_speaking = false;
}

bool tts_is_speaking(void)
{
    return s_speaking;
}
