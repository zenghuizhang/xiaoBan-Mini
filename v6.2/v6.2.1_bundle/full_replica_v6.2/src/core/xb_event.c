// xb_event.c — minimal in-process pub/sub. Replace with claw_event_router on real device.
#include <string.h>
#include "xb_event.h"

#define MAX_SUBS 8
typedef struct { xb_event_cb_t cb; void* user; } sub_t;
static sub_t g_subs[XB_EVT_MAX][MAX_SUBS];

void xb_event_init(void) { memset(g_subs, 0, sizeof(g_subs)); }

void xb_event_subscribe(xb_event_id_t id, xb_event_cb_t cb, void* user) {
    if (id >= XB_EVT_MAX) return;
    for (int i = 0; i < MAX_SUBS; ++i) {
        if (g_subs[id][i].cb == 0) {
            g_subs[id][i].cb = cb; g_subs[id][i].user = user; return;
        }
    }
}

void xb_event_post(xb_event_id_t id, void* payload) {
    if (id >= XB_EVT_MAX) return;
    for (int i = 0; i < MAX_SUBS; ++i)
        if (g_subs[id][i].cb) g_subs[id][i].cb(id, payload, g_subs[id][i].user);
}
