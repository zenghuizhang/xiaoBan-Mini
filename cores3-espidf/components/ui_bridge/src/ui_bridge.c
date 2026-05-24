/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "ui_bridge.h"

#include <stdlib.h>
#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

static const char *TAG = "ui_bridge";

/* -------------------------------------------------------------------------- */
/*  Cross-Core Message Types                                                  */
/* -------------------------------------------------------------------------- */

/**
 * @brief Opcodes for messages posted into the FreeRTOS queue from Core 0.
 */
typedef enum {
    BRIDGE_MSG_CHAT_DELTA = 0,
    BRIDGE_MSG_CHAT_DONE,
    BRIDGE_MSG_CHAT_ERROR,
    BRIDGE_MSG_EXPRESSION,
} bridge_msg_type_t;

/**
 * @brief Payload for a single cross-core bridge message.
 */
typedef struct {
    bridge_msg_type_t type;
    union {
        char delta[256];       /* BRIDGE_MSG_CHAT_DELTA  */
        char err_msg[128];     /* BRIDGE_MSG_CHAT_ERROR  */
        int  expr_id;          /* BRIDGE_MSG_EXPRESSION  */
    };
} bridge_msg_t;

/* -------------------------------------------------------------------------- */
/*  Internal State                                                            */
/* -------------------------------------------------------------------------- */

#define UI_BRIDGE_MAX_PAGES   8
#define UI_BRIDGE_QUEUE_LEN  16

static struct {
    bool                  initialized;
    ui_bridge_page_cbs_t  pages[UI_BRIDGE_MAX_PAGES];
    int                   page_count;
    QueueHandle_t         msg_queue;
} s_bridge;

/* -------------------------------------------------------------------------- */
/*  Internal Helpers                                                          */
/* -------------------------------------------------------------------------- */

static ui_bridge_page_cbs_t *find_page(ui_page_id_t page_id)
{
    for (int i = 0; i < s_bridge.page_count; i++) {
        if (s_bridge.pages[i].page_id == page_id) {
            return &s_bridge.pages[i];
        }
    }
    return NULL;
}

static void dispatch_message(const bridge_msg_t *msg)
{
    ui_page_id_t target_page = UI_PAGE_CHAT; /* default target */

    /* Dispatch to registered callbacks */
    for (int i = 0; i < s_bridge.page_count; i++) {
        const ui_bridge_page_cbs_t *p = &s_bridge.pages[i];

        switch (msg->type) {
        case BRIDGE_MSG_CHAT_DELTA:
            if (p->on_chat_delta) {
                p->on_chat_delta(msg->delta, p->user_ctx);
            }
            break;
        case BRIDGE_MSG_CHAT_DONE:
            if (p->on_chat_done) {
                p->on_chat_done(p->user_ctx);
            }
            break;
        case BRIDGE_MSG_CHAT_ERROR:
            if (p->on_chat_error) {
                p->on_chat_error(msg->err_msg, p->user_ctx);
            }
            break;
        case BRIDGE_MSG_EXPRESSION:
            if (p->on_expression) {
                p->on_expression(msg->expr_id, p->user_ctx);
            }
            break;
        }
    }
}

/* -------------------------------------------------------------------------- */
/*  Public API                                                                */
/* -------------------------------------------------------------------------- */

esp_err_t ui_bridge_init(void)
{
    if (s_bridge.initialized) {
        ESP_LOGW(TAG, "Already initialized");
        return ESP_OK;
    }

    memset(&s_bridge, 0, sizeof(s_bridge));
    s_bridge.msg_queue = xQueueCreate(UI_BRIDGE_QUEUE_LEN, sizeof(bridge_msg_t));
    if (!s_bridge.msg_queue) {
        ESP_LOGE(TAG, "Failed to create message queue");
        return ESP_ERR_NO_MEM;
    }

    s_bridge.initialized = true;
    ESP_LOGI(TAG, "UI bridge initialized, queue_len=%d, max_pages=%d",
             UI_BRIDGE_QUEUE_LEN, UI_BRIDGE_MAX_PAGES);
    return ESP_OK;
}

esp_err_t ui_bridge_register_page(const ui_bridge_page_cbs_t *cbs)
{
    if (!s_bridge.initialized) {
        ESP_LOGE(TAG, "Not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    if (!cbs) {
        return ESP_ERR_INVALID_ARG;
    }

    /* Replace existing registration for the same page_id */
    ui_bridge_page_cbs_t *existing = find_page(cbs->page_id);
    if (existing) {
        ESP_LOGI(TAG, "Replacing callbacks for page_id=%d", cbs->page_id);
        memcpy(existing, cbs, sizeof(*cbs));
        return ESP_OK;
    }

    if (s_bridge.page_count >= UI_BRIDGE_MAX_PAGES) {
        ESP_LOGE(TAG, "Max pages reached (%d)", UI_BRIDGE_MAX_PAGES);
        return ESP_ERR_NO_MEM;
    }

    s_bridge.pages[s_bridge.page_count++] = *cbs;
    ESP_LOGI(TAG, "Registered page_id=%d", cbs->page_id);
    return ESP_OK;
}

esp_err_t ui_bridge_unregister_page(ui_page_id_t page_id)
{
    if (!s_bridge.initialized) {
        ESP_LOGE(TAG, "Not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    for (int i = 0; i < s_bridge.page_count; i++) {
        if (s_bridge.pages[i].page_id == page_id) {
            /* Compact the array */
            int tail = s_bridge.page_count - i - 1;
            if (tail > 0) {
                memmove(&s_bridge.pages[i], &s_bridge.pages[i + 1],
                        tail * sizeof(ui_bridge_page_cbs_t));
            }
            s_bridge.page_count--;
            ESP_LOGI(TAG, "Unregistered page_id=%d", page_id);
            return ESP_OK;
        }
    }

    ESP_LOGW(TAG, "page_id=%d not found for unregister", page_id);
    return ESP_ERR_NOT_FOUND;
}

esp_err_t ui_bridge_notify_chat(const char *delta)
{
    if (!s_bridge.initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    if (!delta) {
        return ESP_ERR_INVALID_ARG;
    }

    bridge_msg_t msg = { .type = BRIDGE_MSG_CHAT_DELTA };
    strncpy(msg.delta, delta, sizeof(msg.delta) - 1);
    msg.delta[sizeof(msg.delta) - 1] = '\0';

    /* Non-blocking send from Core 0; UI will dequeue on Core 1 */
    if (xQueueSend(s_bridge.msg_queue, &msg, 0) != pdTRUE) {
        ESP_LOGW(TAG, "Queue full, dropping delta");
    }
    return ESP_OK;
}

esp_err_t ui_bridge_notify_expression(int expr_id)
{
    if (!s_bridge.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    bridge_msg_t msg = {
        .type = BRIDGE_MSG_EXPRESSION,
        .expr_id = expr_id,
    };

    if (xQueueSend(s_bridge.msg_queue, &msg, 0) != pdTRUE) {
        ESP_LOGW(TAG, "Queue full, dropping expression");
    }
    return ESP_OK;
}
