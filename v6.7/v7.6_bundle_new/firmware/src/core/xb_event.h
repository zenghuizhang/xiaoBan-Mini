// xb_event.h — minimal event bus (wraps claw_event_router on real device)
// v7.6: added FACE/STATUSBAR/SCENE/PERSONA/MEMORY/RGB topics for new pages.
#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    XB_EVT_THEME_CHANGED,       // payload: const char* new theme name
    XB_EVT_WIFI_STATE,          // payload: int (0 disconnected,1 connecting,2 connected)
    XB_EVT_BATTERY,             // payload: int %
    XB_EVT_PLUG,                // payload: bool (true = charging)
    XB_EVT_AI_STATE,            // payload: enum {idle,thinking,talking}
    XB_EVT_MCP_ACTIVE,          // payload: bool
    XB_EVT_OTA_PROGRESS,        // payload: int %  (-1 = idle)
    XB_EVT_FACE_REQUEST,        // payload: face_variant_t
    XB_EVT_SCENE_REQUEST,       // payload: scene_t  (drives face+bubble+rgb)
    XB_EVT_BUBBLE,              // payload: bubble_evt_t*
    XB_EVT_RGB_SCENE,           // payload: rgb_scene_t
    XB_EVT_STATUSBAR_MODE,      // payload: statusbar_mode_t
    XB_EVT_STATUSBAR_PEEK,      // payload: NULL  (transient peek 2.4s)
    XB_EVT_PERSONA_CHANGED,     // payload: const char* persona_id
    XB_EVT_MODEL_CHANGED,       // payload: const char* model_id
    XB_EVT_MEMORY_CHANGED,      // payload: NULL  (memory list dirty)
    XB_EVT_IMU,                 // payload: imu_event_t
    XB_EVT_CHAT_DELTA,          // payload: const char* (token)
    XB_EVT_CHAT_DONE,
    XB_EVT_CHAT_ERROR,
    XB_EVT_MAX
} xb_event_id_t;

typedef void (*xb_event_cb_t)(xb_event_id_t id, void* payload, void* user);

void xb_event_init(void);
void xb_event_post(xb_event_id_t id, void* payload);
void xb_event_subscribe(xb_event_id_t id, xb_event_cb_t cb, void* user);
void xb_event_unsubscribe(xb_event_cb_t cb, void* user);  // v7.6: explicit teardown
