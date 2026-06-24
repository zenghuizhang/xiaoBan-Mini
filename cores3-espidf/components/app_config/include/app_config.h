/* esp-claw app_config API (standalone) */
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#ifdef __cplusplus
extern "C" {
#endif

#define APP_CONFIG_API_KEY_MAX    128
#define APP_CONFIG_BASE_URL_MAX   128
#define APP_CONFIG_MODEL_MAX      32
#define APP_CONFIG_PERSONA_MAX    16

typedef struct {
    char wifi_ssid[33];
    char wifi_password[65];
    int theme;
    int language;
    int brightness;
    int volume;
    /* LLM API config */
    char api_key[APP_CONFIG_API_KEY_MAX];
    char base_url[APP_CONFIG_BASE_URL_MAX];
    char model[APP_CONFIG_MODEL_MAX];
    char persona[APP_CONFIG_PERSONA_MAX];
} app_config_t;

void app_config_init(void);
esp_err_t app_config_load(app_config_t *cfg);
esp_err_t app_config_save(const app_config_t *cfg);
esp_err_t app_config_validate_wifi(const app_config_t *cfg, const char **msg);

#ifdef __cplusplus
}
#endif
