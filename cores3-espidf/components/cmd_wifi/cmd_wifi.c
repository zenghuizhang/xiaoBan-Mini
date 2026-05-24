/* esp-claw cmd_wifi — simplified (no console deps) */
#include "cmd_wifi.h"
#include "app_config.h"
#include "wifi_manager.h"
#include <esp_log.h>
#include <string.h>

static const char *TAG = "CMD_WIFI";

int cmd_wifi_scan(void) {
    wifi_manager_scan_record_t records[20];
    uint16_t count = 0;
    esp_err_t err = wifi_manager_scan_aps(records, 20, &count);
    ESP_LOGI(TAG, "scan: %d APs found (err=%d)", (int)count, (int)err);
    return (int)count;
}

int cmd_wifi_connect(const char *ssid, const char *password) {
    app_config_t cfg;
    app_config_load(&cfg);
    strlcpy(cfg.wifi_ssid, ssid, sizeof(cfg.wifi_ssid));
    strlcpy(cfg.wifi_password, password, sizeof(cfg.wifi_password));
    app_config_save(&cfg);
    
    wifi_manager_config_t wcfg = { .sta_ssid = ssid, .sta_password = password };
    return (int)wifi_manager_apply_sta_config(&wcfg);
}
