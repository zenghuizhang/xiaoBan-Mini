#pragma once
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize claw_core LLM engine. Call once at boot after WiFi init. */
esp_err_t chat_llm_init(void);

/* Submit a user message. Returns ESP_OK if queued. */
esp_err_t chat_llm_send(const char *user_text);

/* Check if a response is ready (non-blocking). Returns true + copies text. */
bool chat_llm_poll_response(char *buf, size_t buf_size);

/* Check if currently waiting for LLM response. */
bool chat_llm_is_busy(void);

/* Get current model name. */
const char *chat_llm_get_model(void);

/* Set model (persisted to NVS). */
void chat_llm_set_model(const char *model);

/* Get current persona id. */
const char *chat_llm_get_persona(void);

/* Set persona (persisted to NVS). */
void chat_llm_set_persona(const char *persona);

#ifdef __cplusplus
}
#endif
