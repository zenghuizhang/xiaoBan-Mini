// star_reward.c — Star reward persistence (NVS key "stars").
#include "star_reward.h"
#include "settings_store.h"

#include <esp_log.h>
#include <stdlib.h>
#include <string.h>

static const char *TAG = "STAR";
#define STAR_KEY "stars"

static int s_stars = 0;

void star_reward_init(void)
{
    char buf[16];
    settings_store_get_string(STAR_KEY, buf, sizeof(buf), "0");
    s_stars = atoi(buf);
    ESP_LOGI(TAG, "stars loaded: %d", s_stars);
}

int star_reward_get(void)
{
    return s_stars;
}

void star_reward_add(int n)
{
    s_stars += n;
    if (s_stars < 0) s_stars = 0;
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", s_stars);
    settings_store_set_string(STAR_KEY, buf);
}

void star_reward_reset(void)
{
    s_stars = 0;
    settings_store_set_string(STAR_KEY, "0");
}
