/* esp-claw app_config — standalone implementation */
#include "app_config.h"
#include "robot_memory.h"
#include <string.h>
#include <esp_log.h>

static const char *TAG = "APPCFG";

void app_config_init(void) {}

esp_err_t app_config_load(app_config_t *cfg) {
    memset(cfg, 0, sizeof(*cfg));
    cfg->theme = memory_load_theme();
    ESP_LOGI(TAG, "loaded: theme=%d", cfg->theme);
    return ESP_OK;
}

esp_err_t app_config_save(const app_config_t *cfg) {
    memory_save_theme(cfg->theme);
    return ESP_OK;
}

esp_err_t app_config_validate_wifi(const app_config_t *cfg, const char **msg) {
    if (!cfg->wifi_ssid[0]) { *msg = "SSID required"; return ESP_ERR_INVALID_ARG; }
    return ESP_OK;
}
