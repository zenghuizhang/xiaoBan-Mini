enum { THEME_TECH = 0, THEME_CHILD = 1, THEME_DEV = 2 };
/* v5.0 NVS Memory */
#include "robot_memory.h"
#include <nvs_flash.h>
#include <nvs.h>
#include <esp_log.h>
#include <time.h>

static const char *TAG = "MEM";
static nvs_handle_t s_nvs;

void memory_init(void)
{
    esp_err_t err = nvs_open("xiaoban", NVS_READWRITE, &s_nvs);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "NVS open fail, will use defaults");
        s_nvs = 0;
    }
}

void memory_save_theme(int theme)
{
    if (!s_nvs) return;
    nvs_set_u8(s_nvs, "theme", (uint8_t)theme);
    nvs_commit(s_nvs);
    ESP_LOGI(TAG, "theme saved: %d", (int)theme);
}

int memory_load_theme(void)
{
    if (!s_nvs) return THEME_TECH;
    uint8_t v = THEME_TECH;
    nvs_get_u8(s_nvs, "theme", &v);
    if (v > 2) v = 0;
    ESP_LOGI(TAG, "theme loaded: %d", (int)v);
    return (int)v;
}

void memory_save_last_date(void)
{
    if (!s_nvs) return;
    time_t now; time(&now);
    struct tm *t = localtime(&now);
    uint32_t ymd = (t->tm_year + 1900) * 10000 + (t->tm_mon + 1) * 100 + t->tm_mday;
    nvs_set_u32(s_nvs, "lastdate", ymd);
    nvs_commit(s_nvs);
}

bool memory_should_morning_greet(void)
{
    time_t now; time(&now);
    struct tm *t = localtime(&now);
    int h = t->tm_hour;
    if (h < 6 || h > 10) return false;

    uint32_t today = (t->tm_year+1900)*10000 + (t->tm_mon+1)*100 + t->tm_mday;
    uint32_t last = 0;
    if (s_nvs) nvs_get_u32(s_nvs, "lastdate", &last);
    return (today != last);
}

void memory_record_interaction(void)
{
    if (!s_nvs) return;
    uint32_t count = 0;
    nvs_get_u32(s_nvs, "interact", &count);
    nvs_set_u32(s_nvs, "interact", count + 1);
    nvs_commit(s_nvs);
}

int memory_get_interaction_count(void)
{
    if (!s_nvs) return 0;
    uint32_t count = 0;
    nvs_get_u32(s_nvs, "interact", &count);
    return (int)count;
}
