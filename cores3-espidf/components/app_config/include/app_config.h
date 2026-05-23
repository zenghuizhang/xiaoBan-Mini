/* esp-claw app_config wrapper */
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char wifi_ssid[33];
    char wifi_password[65];
    int theme;
    int language;  // 0=CN, 1=EN
    int brightness;
    int volume;
} app_config_t;

void app_config_init(void);
esp_err_t app_config_load(app_config_t *cfg);
esp_err_t app_config_save(const app_config_t *cfg);

#ifdef __cplusplus
}
#endif
