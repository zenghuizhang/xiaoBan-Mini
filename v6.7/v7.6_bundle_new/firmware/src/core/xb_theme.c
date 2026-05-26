// xb_theme.c — implementation.
#include "xb_theme.h"
#include "xb_event.h"
#include <string.h>

#ifdef ESP_PLATFORM
#include "nvs.h"
#include "nvs_flash.h"
#define NVS_NS "xb_theme"          // ≤ 15 chars
#define NVS_KEY "active"
#endif

static const theme_t* g_active = &THEME_TECH;
static char g_active_name[16]   = "tech";

static const theme_t* lookup(const char* name) {
    return xb_theme_lookup(name);   // defined in theme_tokens.h, returns &THEME_TECH for unknowns
}

void xb_theme_init(void) {
#ifdef ESP_PLATFORM
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READONLY, &h) == ESP_OK) {
        size_t sz = sizeof(g_active_name);
        if (nvs_get_str(h, NVS_KEY, g_active_name, &sz) != ESP_OK)
            strcpy(g_active_name, "tech");
        nvs_close(h);
    }
#endif
    g_active = lookup(g_active_name);
}

const theme_t* xb_theme_get(void)    { return g_active; }
const char*    xb_theme_name(void)   { return g_active_name; }
bool           xb_theme_is_light(void) { return g_active->is_light ? true : false; }

lv_opa_t xb_card_panel_opa(void) {
    return g_active->is_light ? LV_OPA_COVER : (lv_opa_t)0x4D;  // ~30%
}

void xb_theme_set(const char* name) {
    if (!name) return;
    const theme_t* t = lookup(name);
    if (t == g_active) return;          // no-op
    g_active = t;
    strncpy(g_active_name, name, sizeof(g_active_name) - 1);
    g_active_name[sizeof(g_active_name) - 1] = 0;
#ifdef ESP_PLATFORM
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READWRITE, &h) == ESP_OK) {
        nvs_set_str(h, NVS_KEY, g_active_name);
        nvs_commit(h);
        nvs_close(h);
    }
#endif
    xb_event_post(XB_EVT_THEME_CHANGED, (void*)g_active_name);
}
