#include "app_config.h"
#include "robot_memory.h"
#include <string.h>
#include <esp_log.h>

static const char *TAG = "APPCFG";

void app_config_init(void) {}

esp_err_t app_config_load(app_config_t *cfg) {
    memset(cfg, 0, sizeof(*cfg));
    cfg->theme = memory_load_theme();
    ESP_LOGI(TAG, "config loaded: theme=%d", cfg->theme);
    return ESP_OK;
}

esp_err_t app_config_save(const app_config_t *cfg) {
    memory_save_theme(cfg->theme);
    ESP_LOGI(TAG, "config saved: theme=%d", cfg->theme);
    return ESP_OK;
}
