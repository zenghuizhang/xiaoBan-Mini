// xb_event.c — in-process pub/sub. Swap for claw_event_router on device.
#include <string.h>
#include "xb_event.h"

#define MAX_SUBS 12  // v7.6: bumped from 8 because new pages subscribe more aggressively

typedef struct { xb_event_cb_t cb; void* user; } sub_t;
static sub_t g_subs[XB_EVT_MAX][MAX_SUBS];

void xb_event_init(void) {
    memset(g_subs, 0, sizeof(g_subs));
}

void xb_event_subscribe(xb_event_id_t id, xb_event_cb_t cb, void* user) {
    if (id >= XB_EVT_MAX || !cb) return;
    for (int i = 0; i < MAX_SUBS; ++i) {
        if (g_subs[id][i].cb == 0) {
            g_subs[id][i].cb = cb;
            g_subs[id][i].user = user;
            return;
        }
    }
}

void xb_event_post(xb_event_id_t id, void* payload) {
    if (id >= XB_EVT_MAX) return;
    for (int i = 0; i < MAX_SUBS; ++i)
        if (g_subs[id][i].cb) g_subs[id][i].cb(id, payload, g_subs[id][i].user);
}

void xb_event_unsubscribe(xb_event_cb_t cb, void* user) {
    for (int e = 0; e < XB_EVT_MAX; ++e)
        for (int i = 0; i < MAX_SUBS; ++i)
            if (g_subs[e][i].cb == cb && g_subs[e][i].user == user) {
                g_subs[e][i].cb = NULL;
                g_subs[e][i].user = NULL;
            }
}
