/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Type of UI page that registered a callback.
 *
 * Used to route bridge events to the correct active page.
 */
typedef enum {
    UI_PAGE_NONE = 0,
    UI_PAGE_HOME,
    UI_PAGE_CHAT,
    UI_PAGE_MENU,
    UI_PAGE_WIFI,
    UI_PAGE_OTA,
    UI_PAGE_SKILLS,
    UI_PAGE_SETTINGS,
} ui_page_id_t;

/**
 * @brief Chat delta callback signature
 *
 * Called by the bridge when a new token arrives from claw_core SSE stream.
 * The callee MUST hold lvgl_port_lock(0) before touching LVGL objects
 * (rule 26 cross-core LVGL access).
 *
 * @param delta    Null-terminated token text
 * @param user_ctx Opaque context pointer passed at registration
 */
typedef void (*ui_bridge_on_chat_delta_cb_t)(const char *delta, void *user_ctx);

/**
 * @brief Chat stream complete callback
 *
 * Called when SSE stream finishes (data: [DONE]).
 *
 * @param user_ctx Opaque context pointer
 */
typedef void (*ui_bridge_on_chat_done_cb_t)(void *user_ctx);

/**
 * @brief Chat error callback
 *
 * Called on SSE error, timeout, or cancellation.
 *
 * @param err_msg  Human-readable error description
 * @param user_ctx Opaque context pointer
 */
typedef void (*ui_bridge_on_chat_error_cb_t)(const char *err_msg, void *user_ctx);

/**
 * @brief Expression change callback
 *
 * Called when the bridge receives an expression request from claw_core
 * or other components.  The callee should route to face_engine via
 * xb_face bridge (never call face_engine directly).
 *
 * @param expr_id  Expression ID (FACE_THINKING, FACE_TALKING, etc.)
 * @param user_ctx Opaque context pointer
 */
typedef void (*ui_bridge_on_expression_cb_t)(int expr_id, void *user_ctx);

/**
 * @brief Page callback registration block
 *
 * Each UI page can register a set of callbacks so the bridge knows
 * where to forward claw_core tokens, completion events, and errors.
 */
typedef struct {
    ui_page_id_t                    page_id;
    ui_bridge_on_chat_delta_cb_t    on_chat_delta;
    ui_bridge_on_chat_done_cb_t     on_chat_done;
    ui_bridge_on_chat_error_cb_t    on_chat_error;
    ui_bridge_on_expression_cb_t    on_expression;
    void                           *user_ctx;
} ui_bridge_page_cbs_t;

/**
 * @brief Initialize the UI bridge
 *
 * Creates the FreeRTOS cross-core message queue and prepares
 * internal state for receiving claw_core SSE callbacks.
 *
 * Must be called after LVGL and claw_core are initialized,
 * before any page registers its callbacks.
 *
 * @return ESP_OK on success
 */
esp_err_t ui_bridge_init(void);

/**
 * @brief Register a page's callback set
 *
 * Replaces any previously registered callbacks for the same page_id.
 * Only one page of a given type may be registered at a time.
 *
 * @param cbs  Callback block (copied internally)
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG if cbs is NULL
 */
esp_err_t ui_bridge_register_page(const ui_bridge_page_cbs_t *cbs);

/**
 * @brief Unregister a page's callbacks
 *
 * @param page_id  Page to unregister
 * @return ESP_OK on success
 */
esp_err_t ui_bridge_unregister_page(ui_page_id_t page_id);

/**
 * @brief Notify a chat delta token
 *
 * Typically called from claw_core on_token callback (Core 0).
 * The bridge posts the delta into the FreeRTOS queue so the
 * registered chat page (Core 1) can consume it in LVGL context.
 *
 * @param delta  Null-terminated token text
 * @return ESP_OK on success
 */
esp_err_t ui_bridge_notify_chat(const char *delta);

/**
 * @brief Notify an expression change
 *
 * Typically called from claw_core callback or face engine trigger
 * to request an expression change on the UI.
 *
 * @param expr_id  Expression identifier
 * @return ESP_OK on success
 */
esp_err_t ui_bridge_notify_expression(int expr_id);

#ifdef __cplusplus
}
#endif
