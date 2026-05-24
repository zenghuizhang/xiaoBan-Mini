/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Skill metadata descriptor
 *
 * Mirrors the structure defined in BACKEND_PLAN.md section 4.1.
 */
typedef struct {
    char     id[64];
    char     name[64];
    char     summary[256];
    char     version[16];
    char     author[64];
    size_t   size_bytes;
    bool     installed;
    bool     enabled;
    uint32_t updated_at;
} skill_info_t;

/**
 * @brief Demo skill IDs for the built-in registry.
 *
 * These 5 skills form the starter set shipped with the firmware.
 */
typedef enum {
    SKILL_DEMO_VOICE = 0,
    SKILL_DEMO_WEATHER,
    SKILL_DEMO_TIMER,
    SKILL_DEMO_TRANSLATE,
    SKILL_DEMO_PARROT,
    SKILL_DEMO_COUNT,
} skill_demo_id_t;

/**
 * @brief Initialize the skill manager
 *
 * Mounts the skills SPIFFS partition, loads the demo skill registry
 * (Voice, Weather, Timer, Translate, Parrot), and prepares
 * claw_skill / claw_memory integration stubs.
 *
 * Must be called after claw_skill_init() and claw_memory_init().
 *
 * @return ESP_OK on success
 */
esp_err_t skill_manager_init(void);

/**
 * @brief List all installed skills
 *
 * Allocates and populates a caller-freed array of skill_info_t
 * representing every skill currently in the registry.
 *
 * @param out_list   Pointer to receive allocated array (caller frees with free())
 * @param out_count  Pointer to receive element count
 * @return ESP_OK on success, ESP_ERR_NO_MEM on allocation failure
 */
esp_err_t skill_list_installed(skill_info_t **out_list, size_t *out_count);

/**
 * @brief Install a skill by ID
 *
 * Stub: in production this would GET /v1/skills/{id}/pkg, verify
 * RSA signature, extract to SPIFFS /skills/{id}/, then call
 * claw_skill_reload_registry().
 *
 * @param skill_id  Skill identifier (e.g. "weather_today")
 * @return ESP_OK on success
 */
esp_err_t skill_install(const char *skill_id);

/**
 * @brief Toggle a skill's enabled state
 *
 * Stub: in production this calls claw_skill_activate_for_session()
 * or claw_skill_deactivate_for_session() so the LLM context provider
 * includes/excludes the skill's SKILL.md prompt.
 *
 * @param skill_id  Skill identifier
 * @param enable    true to enable, false to disable
 * @return ESP_OK on success
 */
esp_err_t skill_toggle(const char *skill_id, bool enable);

/**
 * @brief Look up a skill's current info
 *
 * Searches the local registry for a skill by ID.
 *
 * @param skill_id  Skill identifier
 * @param out_info  Pointer to receive skill info (may be NULL)
 * @return ESP_OK on success, ESP_ERR_NOT_FOUND if unknown
 */
esp_err_t skill_get_info(const char *skill_id, skill_info_t *out_info);

#ifdef __cplusplus
}
#endif
