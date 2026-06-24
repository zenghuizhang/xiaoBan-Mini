/* esp-claw app_config — standalone implementation */
#include "app_config.h"
#include "robot_memory.h"
#include "settings_store.h"
#include <string.h>
#include <esp_log.h>

static const char *TAG = "APPCFG";

void app_config_init(void) {
    settings_store_init(&(settings_store_config_t){ .namespace_name = "xiaoban" });
}

esp_err_t app_config_load(app_config_t *cfg) {
    memset(cfg, 0, sizeof(*cfg));
    cfg->theme = memory_load_theme();
    settings_store_get_string("api_key",  cfg->api_key,  sizeof(cfg->api_key),  "");
    settings_store_get_string("base_url", cfg->base_url, sizeof(cfg->base_url), "");
    settings_store_get_string("model",    cfg->model,    sizeof(cfg->model),    "auto");
    settings_store_get_string("persona",  cfg->persona,  sizeof(cfg->persona),  "lyra");
    ESP_LOGI(TAG, "loaded: theme=%d model=%s persona=%s", cfg->theme, cfg->model, cfg->persona);
    return ESP_OK;
}

esp_err_t app_config_save(const app_config_t *cfg) {
    memory_save_theme(cfg->theme);
    settings_store_set_string("api_key",  cfg->api_key);
    settings_store_set_string("base_url", cfg->base_url);
    settings_store_set_string("model",    cfg->model);
    settings_store_set_string("persona",  cfg->persona);
    return ESP_OK;
}

esp_err_t app_config_validate_wifi(const app_config_t *cfg, const char **msg) {
    if (!cfg->wifi_ssid[0]) { *msg = "SSID required"; return ESP_ERR_INVALID_ARG; }
    return ESP_OK;
}
