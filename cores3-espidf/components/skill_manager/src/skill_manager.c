/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "skill_manager.h"

#include <stdlib.h>
#include <string.h>

#include "esp_log.h"

static const char *TAG = "skill_manager";

/* -------------------------------------------------------------------------- */
/*  Demo Skill Registry (Voice, Weather, Timer, Translate, Parrot)            */
/* -------------------------------------------------------------------------- */

static skill_info_t s_demo_skills[SKILL_DEMO_COUNT] = {
    {
        .id         = "voice",
        .name       = "Voice",
        .summary    = "Voice control and speech recognition",
        .version    = "1.0.0",
        .author     = "xiaoBan Team",
        .size_bytes = 4096,
        .installed  = true,
        .enabled    = false,
        .updated_at = 1714809600,
    },
    {
        .id         = "weather_today",
        .name       = "Weather",
        .summary    = "Real-time weather query and forecast",
        .version    = "1.2.0",
        .author     = "xiaoBan Team",
        .size_bytes = 6144,
        .installed  = true,
        .enabled    = false,
        .updated_at = 1714809600,
    },
    {
        .id         = "timer",
        .name       = "Timer",
        .summary    = "Countdown timer and alarm management",
        .version    = "1.0.1",
        .author     = "xiaoBan Team",
        .size_bytes = 3584,
        .installed  = true,
        .enabled    = false,
        .updated_at = 1714809600,
    },
    {
        .id         = "translate",
        .name       = "Translate",
        .summary    = "Multi-language text translation",
        .version    = "1.1.0",
        .author     = "xiaoBan Team",
        .size_bytes = 5120,
        .installed  = true,
        .enabled    = false,
        .updated_at = 1714809600,
    },
    {
        .id         = "parrot",
        .name       = "Parrot",
        .summary    = "Echo user input back as a parrot character",
        .version    = "1.0.0",
        .author     = "xiaoBan Team",
        .size_bytes = 2048,
        .installed  = true,
        .enabled    = false,
        .updated_at = 1714809600,
    },
};

/* -------------------------------------------------------------------------- */
/*  Internal State                                                            */
/* -------------------------------------------------------------------------- */

static struct {
    bool initialized;
} s_sm;

/* -------------------------------------------------------------------------- */
/*  Public API                                                                */
/* -------------------------------------------------------------------------- */

esp_err_t skill_manager_init(void)
{
    if (s_sm.initialized) {
        ESP_LOGW(TAG, "Already initialized");
        return ESP_OK;
    }

    s_sm.initialized = true;

    ESP_LOGI(TAG, "Skill manager initialized, %d demo skills in registry",
             SKILL_DEMO_COUNT);
    for (int i = 0; i < SKILL_DEMO_COUNT; i++) {
        ESP_LOGI(TAG, "  [%d] %s (%s) — installed=%d enabled=%d",
                 i,
                 s_demo_skills[i].name,
                 s_demo_skills[i].id,
                 s_demo_skills[i].installed,
                 s_demo_skills[i].enabled);
    }

    return ESP_OK;
}

esp_err_t skill_list_installed(skill_info_t **out_list, size_t *out_count)
{
    if (!s_sm.initialized) {
        ESP_LOGE(TAG, "Not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    if (!out_list || !out_count) {
        return ESP_ERR_INVALID_ARG;
    }

    size_t num = SKILL_DEMO_COUNT;
    skill_info_t *list = calloc(num, sizeof(skill_info_t));
    if (!list) {
        ESP_LOGE(TAG, "Failed to allocate list for %zu skills", num);
        return ESP_ERR_NO_MEM;
    }

    memcpy(list, s_demo_skills, num * sizeof(skill_info_t));
    *out_list  = list;
    *out_count = num;

    ESP_LOGI(TAG, "Listed %zu installed skills", num);
    return ESP_OK;
}

esp_err_t skill_install(const char *skill_id)
{
    if (!s_sm.initialized) {
        ESP_LOGE(TAG, "Not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    if (!skill_id) {
        return ESP_ERR_INVALID_ARG;
    }

    /* Check if skill is already in the demo registry */
    skill_info_t info;
    esp_err_t err = skill_get_info(skill_id, &info);
    if (err == ESP_OK && info.installed) {
        ESP_LOGW(TAG, "Skill '%s' is already installed", skill_id);
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Install skill '%s' — stub: "
             "would GET /v1/skills/%s/pkg, verify RSA, extract to SPIFFS",
             skill_id, skill_id);
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t skill_toggle(const char *skill_id, bool enable)
{
    if (!s_sm.initialized) {
        ESP_LOGE(TAG, "Not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    if (!skill_id) {
        return ESP_ERR_INVALID_ARG;
    }

    /* Try to find in demo registry and toggle */
    for (int i = 0; i < SKILL_DEMO_COUNT; i++) {
        if (strcmp(s_demo_skills[i].id, skill_id) == 0) {
            s_demo_skills[i].enabled = enable;
            ESP_LOGI(TAG, "Toggled skill '%s' (%s) -> %s",
                     s_demo_skills[i].name, skill_id,
                     enable ? "enabled" : "disabled");
            ESP_LOGI(TAG, "  stub: would call claw_skill_%s_for_session()",
                     enable ? "activate" : "deactivate");
            return ESP_OK;
        }
    }

    ESP_LOGW(TAG, "Skill '%s' not found in demo registry", skill_id);
    return ESP_ERR_NOT_FOUND;
}

esp_err_t skill_get_info(const char *skill_id, skill_info_t *out_info)
{
    if (!s_sm.initialized) {
        ESP_LOGE(TAG, "Not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    for (int i = 0; i < SKILL_DEMO_COUNT; i++) {
        if (strcmp(s_demo_skills[i].id, skill_id) == 0) {
            if (out_info) {
                memcpy(out_info, &s_demo_skills[i], sizeof(skill_info_t));
            }
            return ESP_OK;
        }
    }

    return ESP_ERR_NOT_FOUND;
}
