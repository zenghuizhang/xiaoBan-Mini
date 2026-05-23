/* esp-claw wifi_manager stub */
#include "wifi_manager.h"
#include <string.h>
#include <esp_log.h>

static const char *TAG = "WIFIMGR";

esp_err_t wifi_manager_init(void) { ESP_LOGI(TAG, "init"); return ESP_OK; }
esp_err_t wifi_manager_start(const wifi_manager_config_t *config) { return ESP_OK; }
esp_err_t wifi_manager_apply_sta_config(const wifi_manager_config_t *config) { return ESP_OK; }

void wifi_manager_get_status(wifi_manager_status_t *status) {
    memset(status, 0, sizeof(*status));
}

esp_err_t wifi_manager_scan_aps(wifi_manager_scan_record_t *records,
                                uint16_t max_records, uint16_t *out_count) {
    *out_count = 0;
    return ESP_OK;
}
