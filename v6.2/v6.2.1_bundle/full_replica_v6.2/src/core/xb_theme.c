#include <string.h>
#include "xb_theme.h"
#include "xb_event.h"

static const theme_t* g_th = &THEME_TECH;

const theme_t* xb_theme_get(void) { return g_th; }

void xb_theme_set(const char* name) {
    g_th = xb_theme_lookup(name);
    xb_event_post(XB_EVT_THEME_CHANGED, (void*)g_th);
}
