/* esp-claw wifi_manager API wrapper for xiaoBan */
#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "esp_netif.h"
#include "esp_wifi.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*wifi_manager_state_cb_t)(bool connected, void *user_ctx);

typedef struct {
    const char *sta_ssid;
    const char *sta_password;
    uint32_t max_retry;
} wifi_manager_config_t;

typedef struct {
    bool sta_connected;
    bool sta_configured;
    const char *sta_ip;
    char sta_ssid[33];
    int8_t rssi;
} wifi_manager_status_t;

typedef struct {
    char ssid[33];
    int8_t rssi;
    wifi_auth_mode_t authmode;
} wifi_manager_scan_record_t;

esp_err_t wifi_manager_init(void);
esp_err_t wifi_manager_start(const wifi_manager_config_t *config);
esp_err_t wifi_manager_apply_sta_config(const wifi_manager_config_t *config);
void wifi_manager_get_status(wifi_manager_status_t *status);
esp_err_t wifi_manager_scan_aps(wifi_manager_scan_record_t *records,
                                uint16_t max_records, uint16_t *out_count);

#ifdef __cplusplus
}
#endif
