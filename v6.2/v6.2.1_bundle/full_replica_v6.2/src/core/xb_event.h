// xb_event.h — minimal event bus (wraps claw_event_router)
#pragma once
#include <stdint.h>

typedef enum {
    XB_EVT_THEME_CHANGED,
    XB_EVT_WIFI_STATE,        // payload: int (0 disconnected,1 connecting,2 connected)
    XB_EVT_BATTERY,           // payload: int %
    XB_EVT_AI_STATE,          // payload: enum {idle, thinking, talking}
    XB_EVT_MCP_ACTIVE,        // payload: bool
    XB_EVT_OTA_PROGRESS,      // payload: int %
    XB_EVT_FACE_REQUEST,      // payload: face_state_t
    XB_EVT_CHAT_DELTA,        // payload: const char* (token)
    XB_EVT_CHAT_DONE,
    XB_EVT_CHAT_ERROR,
    XB_EVT_MAX
} xb_event_id_t;

typedef void (*xb_event_cb_t)(xb_event_id_t id, void* payload, void* user);

void xb_event_init(void);
void xb_event_post(xb_event_id_t id, void* payload);
void xb_event_subscribe(xb_event_id_t id, xb_event_cb_t cb, void* user);
