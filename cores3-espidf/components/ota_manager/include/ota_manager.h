/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <stddef.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 5-stage OTA state machine
 *
 * CHECKING    - fetching /v1/ota/manifest
 * DOWNLOADING - esp_https_ota download with resume via Range: bytes=
 * VERIFYING   - SHA-256 + RSA-3072 signature check
 * APPLYING    - writing to inactive ota_X partition + marking bootable
 * DONE        - reboot countdown complete, esp_restart() pending
 * FAILED      - error encountered, rollback triggered
 */
typedef enum {
    OTA_STAGE_IDLE = 0,
    OTA_STAGE_CHECKING,
    OTA_STAGE_DOWNLOADING,
    OTA_STAGE_VERIFYING,
    OTA_STAGE_APPLYING,
    OTA_STAGE_DONE,
    OTA_STAGE_FAILED,
} ota_stage_t;

/**
 * @brief Progress callback signature
 *
 * @param stage   Current OTA stage
 * @param pct     Progress percentage (0-100) within current stage
 * @param msg     Human-readable status message
 * @param ctx     User context pointer
 */
typedef void (*ota_progress_cb_t)(ota_stage_t stage, int pct, const char *msg, void *ctx);

/**
 * @brief Initialize the OTA manager
 *
 * Allocates internal state, registers the progress callback.
 * Must be called once before any other OTA API.
 *
 * @param manifest_url  Base URL for /v1/ota/manifest endpoint
 * @param cb            Progress callback (may be NULL)
 * @param ctx           User context forwarded to callback
 * @return ESP_OK on success
 */
esp_err_t ota_manager_init(const char *manifest_url, ota_progress_cb_t cb, void *ctx);

/**
 * @brief Check for available firmware update
 *
 * GETs the manifest endpoint. Returns 304 (no update) by comparing
 * current firmware version against remote metadata.
 *
 * @return ESP_OK if an update is available, ESP_ERR_NOT_FOUND if up-to-date
 */
esp_err_t ota_check_update(void);

/**
 * @brief Start firmware download
 *
 * Initiates esp_https_ota download to the inactive OTA partition.
 * Supports resume via Range header.  Progress is reported through
 * the callback registered during init.
 *
 * @param firmware_url      Direct URL to firmware binary
 * @param expected_sha256   Expected SHA-256 hex digest (may be NULL to skip)
 * @return ESP_OK if download started, ESP_ERR_INVALID_STATE if not in appropriate stage
 */
esp_err_t ota_start_download(const char *firmware_url, const char *expected_sha256);

/**
 * @brief Abort an in-progress OTA operation
 *
 * Cancels download/verification and returns to IDLE state.
 * Safe to call at any stage.
 *
 * @return ESP_OK
 */
esp_err_t ota_manager_abort(void);

/**
 * @brief Get current OTA progress
 *
 * @param out_stage  Current stage (may be NULL)
 * @param out_pct    Current progress percentage (may be NULL)
 * @return ESP_OK
 */
esp_err_t ota_get_progress(ota_stage_t *out_stage, int *out_pct);

#ifdef __cplusplus
}
#endif
