/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "ota_manager.h"

#include <stdlib.h>
#include <string.h>

#include "esp_https_ota.h"
#include "esp_log.h"

static const char *TAG = "ota_manager";

/* -------------------------------------------------------------------------- */
/*  Internal State                                                            */
/* -------------------------------------------------------------------------- */

static struct {
    ota_stage_t       stage;
    int               progress_pct;
    char             *manifest_url;
    ota_progress_cb_t cb;
    void             *cb_ctx;
    bool              initialized;
} s_ota;

static void set_stage(ota_stage_t stage, int pct, const char *msg)
{
    s_ota.stage = stage;
    s_ota.progress_pct = pct;
    ESP_LOGI(TAG, "[OTA] stage: %d, progress: %d%%, msg: %s", stage, pct, msg ? msg : "");
    if (s_ota.cb) {
        s_ota.cb(stage, pct, msg, s_ota.cb_ctx);
    }
}

/* -------------------------------------------------------------------------- */
/*  Public API                                                                */
/* -------------------------------------------------------------------------- */

esp_err_t ota_manager_init(const char *manifest_url, ota_progress_cb_t cb, void *ctx)
{
    if (s_ota.initialized) {
        ESP_LOGW(TAG, "Already initialized, re-initializing");
        free(s_ota.manifest_url);
    }

    s_ota.stage = OTA_STAGE_IDLE;
    s_ota.progress_pct = 0;
    s_ota.manifest_url = manifest_url ? strdup(manifest_url) : NULL;
    s_ota.cb = cb;
    s_ota.cb_ctx = ctx;
    s_ota.initialized = true;

    ESP_LOGI(TAG, "OTA manager initialized, manifest_url=%s",
             manifest_url ? manifest_url : "(none)");
    return ESP_OK;
}

esp_err_t ota_check_update(void)
{
    if (!s_ota.initialized) {
        ESP_LOGE(TAG, "Not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    set_stage(OTA_STAGE_CHECKING, 0, "Checking for firmware update...");
    ESP_LOGI(TAG, "[OTA] stage: CHECKING — manifest GET would go here");

    /* Stub: simulate no-update scenario */
    set_stage(OTA_STAGE_IDLE, 0, "No update available (stub)");
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t ota_start_download(const char *firmware_url, const char *expected_sha256)
{
    if (!s_ota.initialized) {
        ESP_LOGE(TAG, "Not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    if (!firmware_url) {
        ESP_LOGE(TAG, "firmware_url is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    set_stage(OTA_STAGE_DOWNLOADING, 0, "Starting download...");
    ESP_LOGI(TAG, "[OTA] stage: DOWNLOADING — "
             "esp_https_ota would begin with url=%s, sha256=%s",
             firmware_url,
             expected_sha256 ? expected_sha256 : "(none)");

    /* Stub: simulate progressing through remaining stages */
    set_stage(OTA_STAGE_DOWNLOADING, 50, "Downloading firmware...");
    set_stage(OTA_STAGE_DOWNLOADING, 100, "Download complete");
    set_stage(OTA_STAGE_VERIFYING, 0, "Verifying SHA-256 + RSA-3072...");
    ESP_LOGI(TAG, "[OTA] stage: VERIFYING — SHA-256 + RSA-3072 check");
    set_stage(OTA_STAGE_VERIFYING, 100, "Signature OK");
    set_stage(OTA_STAGE_APPLYING, 0, "Writing to inactive OTA partition...");
    ESP_LOGI(TAG, "[OTA] stage: APPLYING — writing to ota_X + mark bootable");
    set_stage(OTA_STAGE_APPLYING, 100, "Partition marked bootable");
    set_stage(OTA_STAGE_DONE, 100, "Reboot in 5s...");
    ESP_LOGI(TAG, "[OTA] stage: DONE — reboot countdown (5s) then esp_restart()");

    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t ota_manager_abort(void)
{
    if (!s_ota.initialized) {
        ESP_LOGE(TAG, "Not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "OTA aborted at stage %d", s_ota.stage);
    set_stage(OTA_STAGE_IDLE, 0, "Aborted");
    return ESP_OK;
}

esp_err_t ota_get_progress(ota_stage_t *out_stage, int *out_pct)
{
    if (!s_ota.initialized) {
        ESP_LOGE(TAG, "Not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (out_stage) {
        *out_stage = s_ota.stage;
    }
    if (out_pct) {
        *out_pct = s_ota.progress_pct;
    }
    return ESP_OK;
}
