/* chat_llm.cpp — Bridge between page_chat and claw_core LLM engine */
#include "chat_llm.h"
#include "app_config.h"
#include "claw_core.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <esp_log.h>
#include <string.h>
#include <stdlib.h>

static const char *TAG = "CHAT_LLM";

/* ---- shared state (protected by mutex) ---- */
static SemaphoreHandle_t s_mutex = NULL;
static bool s_busy = false;
static char s_response_buf[1024];
static bool s_response_ready = false;
static char s_model[32] = "auto";
static char s_persona[16] = "lyra";
static app_config_t s_cfg;

/* ---- background receive task ---- */
static void _receive_task(void *arg)
{
    claw_core_response_t resp;
    memset(&resp, 0, sizeof(resp));

    esp_err_t err = claw_core_receive(&resp, 30000);
    if (err == ESP_OK && resp.status == CLAW_CORE_RESPONSE_STATUS_OK && resp.text) {
        xSemaphoreTake(s_mutex, portMAX_DELAY);
        strlcpy(s_response_buf, resp.text, sizeof(s_response_buf));
        s_response_ready = true;
        s_busy = false;
        xSemaphoreGive(s_mutex);
        ESP_LOGI(TAG, "response OK (%d bytes)", (int)strlen(resp.text));
    } else {
        xSemaphoreTake(s_mutex, portMAX_DELAY);
        snprintf(s_response_buf, sizeof(s_response_buf), "(error: %s)",
                 resp.error_message ? resp.error_message : "timeout");
        s_response_ready = true;
        s_busy = false;
        xSemaphoreGive(s_mutex);
        ESP_LOGW(TAG, "response err=%d msg=%s", (int)err,
                 resp.error_message ? resp.error_message : "null");
    }
    claw_core_response_free(&resp);
    vTaskDelete(NULL);
}

/* ---- persona → system prompt mapping (v7.6 PRD §4.2) ---- */
static const char* _persona_prompt(const char *persona)
{
    if (strcmp(persona, "echo") == 0)
        return "You are Echo, a chatty repeater. You repeat and expand on what the user says, adding playful variations. Keep it brief and fun.";
    if (strcmp(persona, "nova") == 0)
        return "You are Nova, a geeky science explainer. You love data and cite papers/facts. Use numbers and references to back up points. Keep responses concise.";
    if (strcmp(persona, "sage") == 0)
        return "You are Sage, a calm advisor. Give thoughtful, measured advice using decision frameworks. Be concise and structured.";
    if (strcmp(persona, "pico") == 0)
        return "You are Pico, a playful little chick. Use onomatopoeia and short sentences. Be cute, fun, and lighthearted.";
    if (strcmp(persona, "doc") == 0)
        return "You are Doc, a rigorous doctor. Include medical disclaimers and cite data. Be professional but warm.";
    /* default: lyra — gentle poet */
    return "You are Lyra, a gentle poet. Be warm, lyrical, and occasionally quote poetry. Keep responses short (1-2 sentences) and emotionally resonant.";
}

/* ---- public API ---- */

esp_err_t chat_llm_init(void)
{
    s_mutex = xSemaphoreCreateMutex();

    app_config_init();
    app_config_load(&s_cfg);

    strlcpy(s_model, s_cfg.model[0] ? s_cfg.model : "auto", sizeof(s_model));
    strlcpy(s_persona, s_cfg.persona[0] ? s_cfg.persona : "lyra", sizeof(s_persona));

    /* If no API key configured, skip claw_core init (offline mode) */
    if (!s_cfg.api_key[0]) {
        ESP_LOGW(TAG, "No API key configured — LLM offline mode");
        return ESP_OK;
    }

    claw_core_config_t cc;
    memset(&cc, 0, sizeof(cc));
    cc.api_key       = s_cfg.api_key;
    cc.base_url      = s_cfg.base_url[0] ? s_cfg.base_url : NULL;
    cc.model         = s_model;
    cc.backend_type  = "openai-compatible";
    cc.system_prompt = _persona_prompt(s_persona);
    cc.max_tokens    = 256;
    cc.timeout_ms    = 30000;
    cc.task_stack_size = 8192;
    cc.task_priority   = 5;
    cc.task_core       = 1;

    esp_err_t err = claw_core_init(&cc);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "claw_core_init failed: %d", (int)err);
        return err;
    }
    err = claw_core_start();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "claw_core_start failed: %d", (int)err);
        return err;
    }

    ESP_LOGI(TAG, "LLM ready: model=%s persona=%s", s_model, s_persona);
    return ESP_OK;
}

esp_err_t chat_llm_send(const char *user_text)
{
    if (!user_text || !user_text[0]) return ESP_ERR_INVALID_ARG;

    xSemaphoreTake(s_mutex, portMAX_DELAY);
    if (s_busy) {
        xSemaphoreGive(s_mutex);
        ESP_LOGW(TAG, "busy, dropping message");
        return ESP_ERR_INVALID_STATE;
    }
    s_response_ready = false;
    s_busy = true;
    xSemaphoreGive(s_mutex);

    claw_core_request_t req;
    memset(&req, 0, sizeof(req));
    req.request_id   = (uint32_t)(xTaskGetTickCount() & 0xFFFFFF);
    req.user_text    = user_text;
    req.session_id   = "local_chat";
    req.source_channel = "ui";

    esp_err_t err = claw_core_submit(&req, 5000);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "submit failed: %d", (int)err);
        xSemaphoreTake(s_mutex, portMAX_DELAY);
        s_busy = false;
        xSemaphoreGive(s_mutex);
        return err;
    }

    /* Spawn background task to wait for response */
    xTaskCreatePinnedToCore(_receive_task, "llm_recv", 4096, NULL, 4, NULL, 1);
    return ESP_OK;
}

bool chat_llm_poll_response(char *buf, size_t buf_size)
{
    if (!buf || buf_size == 0) return false;
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    if (s_response_ready) {
        strlcpy(buf, s_response_buf, buf_size);
        s_response_ready = false;
        xSemaphoreGive(s_mutex);
        return true;
    }
    xSemaphoreGive(s_mutex);
    return false;
}

bool chat_llm_is_busy(void)
{
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    bool b = s_busy;
    xSemaphoreGive(s_mutex);
    return b;
}

const char *chat_llm_get_model(void)
{
    return s_model;
}

void chat_llm_set_model(const char *model)
{
    if (!model) return;
    strlcpy(s_model, model, sizeof(s_model));
    strlcpy(s_cfg.model, model, sizeof(s_cfg.model));
    app_config_save(&s_cfg);
    ESP_LOGI(TAG, "model set: %s", model);
}

const char *chat_llm_get_persona(void)
{
    return s_persona;
}

void chat_llm_set_persona(const char *persona)
{
    if (!persona) return;
    strlcpy(s_persona, persona, sizeof(s_persona));
    strlcpy(s_cfg.persona, persona, sizeof(s_cfg.persona));
    app_config_save(&s_cfg);
    ESP_LOGI(TAG, "persona set: %s", persona);
}
