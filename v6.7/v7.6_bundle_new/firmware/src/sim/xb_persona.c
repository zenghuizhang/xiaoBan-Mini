// xb_persona.c — 6 persona registry (matches React prototype constants).
#include "xb_persona.h"
#include "../core/xb_event.h"
#include <string.h>

#ifdef ESP_PLATFORM
#include "nvs.h"
#include "nvs_flash.h"
#define NS  "xb_persona"
#define KEY "active"
#endif

const persona_t XB_PERSONAS[PERSONA_COUNT] = {
    { "lyra", "Lyra", "温柔陪伴 · 慢声细语",     "#A855F7" },
    { "echo", "Echo", "回声助手 · 高效专注",     "#22D3EE" },
    { "nova", "Nova", "活泼能量 · 阳光积极",     "#F97316" },
    { "sage", "Sage", "知识导师 · 沉稳博学",     "#34D399" },
    { "pico", "Pico", "童趣小友 · 软萌可爱",     "#FB7185" },
    { "doc",  "Doc",  "工程伙伴 · 严谨理性",     "#94A3B8" },
};

static const persona_t* g_active = &XB_PERSONAS[0];
static char g_active_id[16] = "lyra";

const persona_t* xb_persona_lookup(const char* id) {
    if (!id) return NULL;
    for (int i = 0; i < PERSONA_COUNT; ++i)
        if (strcmp(XB_PERSONAS[i].id, id) == 0) return &XB_PERSONAS[i];
    return NULL;
}

void xb_persona_init(void) {
#ifdef ESP_PLATFORM
    nvs_handle_t h;
    if (nvs_open(NS, NVS_READONLY, &h) == ESP_OK) {
        size_t sz = sizeof(g_active_id);
        if (nvs_get_str(h, KEY, g_active_id, &sz) != ESP_OK)
            strcpy(g_active_id, "lyra");
        nvs_close(h);
    }
#endif
    const persona_t* p = xb_persona_lookup(g_active_id);
    g_active = p ? p : &XB_PERSONAS[0];
}

const persona_t* xb_persona_active(void) { return g_active; }

void xb_persona_set(const char* id) {
    const persona_t* p = xb_persona_lookup(id);
    if (!p || p == g_active) return;
    g_active = p;
    strncpy(g_active_id, p->id, sizeof(g_active_id) - 1);
    g_active_id[sizeof(g_active_id) - 1] = 0;
#ifdef ESP_PLATFORM
    nvs_handle_t h;
    if (nvs_open(NS, NVS_READWRITE, &h) == ESP_OK) {
        nvs_set_str(h, KEY, g_active_id);
        nvs_commit(h);
        nvs_close(h);
    }
#endif
    xb_event_post(XB_EVT_PERSONA_CHANGED, (void*)g_active->id);
}
