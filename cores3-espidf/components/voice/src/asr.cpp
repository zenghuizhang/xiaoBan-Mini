/* asr.cpp — 云端 ASR (OpenAI Whisper 兼容 API) */
#include "asr.h"
#include "app_config.h"
#include <esp_log.h>
#include <esp_http_client.h>
#include <cJSON.h>
#include <string.h>
#include <stdlib.h>

static const char *TAG = "ASR";

/* 用于接收 HTTP 响应的缓冲区 */
typedef struct {
    char   *buf;
    size_t  len;
    size_t  cap;
} http_resp_t;

static esp_err_t _http_event(esp_http_client_event_t *evt)
{
    http_resp_t *r = (http_resp_t *)evt->user_data;
    if (evt->event_id == HTTP_EVENT_ON_DATA) {
        if (r->len + evt->data_len + 1 > r->cap) {
            r->cap = (r->len + evt->data_len + 1) * 2;
            r->buf = (char *)realloc(r->buf, r->cap);
        }
        memcpy(r->buf + r->len, evt->data, evt->data_len);
        r->len += evt->data_len;
        r->buf[r->len] = '\0';
    }
    return ESP_OK;
}

esp_err_t asr_speech_to_text(const int16_t *pcm_data, size_t pcm_len,
                             char *out_text, size_t out_size)
{
    if (!pcm_data || pcm_len == 0 || !out_text || out_size == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    out_text[0] = '\0';

    app_config_t cfg;
    app_config_load(&cfg);
    if (!cfg.api_key[0]) {
        ESP_LOGE(TAG, "No API key configured");
        return ESP_ERR_INVALID_STATE;
    }

    /* 构建 URL: {base_url}/audio/transcriptions */
    char url[256];
    const char *base = cfg.base_url[0] ? cfg.base_url : "https://api.openai.com/v1";
    snprintf(url, sizeof(url), "%s/audio/transcriptions", base);

    /* 构建 multipart/form-data body */
    const char *boundary = "----xiaoBanBoundary";
    char header[256];
    int header_len = snprintf(header, sizeof(header),
        "--%s\r\n"
        "Content-Disposition: form-data; name=\"file\"; filename=\"audio.pcm\"\r\n"
        "Content-Type: audio/pcm\r\n\r\n", boundary);

    char footer[64];
    int footer_len = snprintf(footer, sizeof(footer),
        "\r\n--%s--\r\n", boundary);

    size_t body_len = header_len + pcm_len + footer_len;
    char *body = (char *)malloc(body_len);
    if (!body) return ESP_ERR_NO_MEM;
    memcpy(body, header, header_len);
    memcpy(body + header_len, pcm_data, pcm_len);
    memcpy(body + header_len + pcm_len, footer, footer_len);

    /* HTTP 请求 */
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
    esp_http_client_set_header(client, "Content-Type",
        "multipart/form-data; boundary=----xiaoBanBoundary");

    esp_http_client_set_post_field(client, body, body_len);

    esp_err_t err = esp_http_client_perform(client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "HTTP perform failed: %s", esp_err_to_name(err));
        free(body);
        esp_http_client_cleanup(client);
        return err;
    }

    int status = esp_http_client_get_status_code(client);
    if (status != 200) {
        ESP_LOGE(TAG, "ASR HTTP status %d", status);
        free(body);
        if (resp.buf) ESP_LOGW(TAG, "resp: %s", resp.buf);
        free(resp.buf);
        esp_http_client_cleanup(client);
        return ESP_FAIL;
    }

    /* 解析 JSON: {"text": "..."} */
    if (resp.buf) {
        cJSON *json = cJSON_Parse(resp.buf);
        if (json) {
            cJSON *text = cJSON_GetObjectItem(json, "text");
            if (text && cJSON_IsString(text)) {
                strlcpy(out_text, text->valuestring, out_size);
            }
            cJSON_Delete(json);
        }
        free(resp.buf);
    }

    free(body);
    esp_http_client_cleanup(client);

    ESP_LOGI(TAG, "ASR result: \"%s\"", out_text);
    return out_text[0] ? ESP_OK : ESP_FAIL;
}
