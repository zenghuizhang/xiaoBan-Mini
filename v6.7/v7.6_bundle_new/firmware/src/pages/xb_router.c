// xb_router.c — page stack + cross-fade transition.
#include "xb_pages.h"
#include "../core/xb_event.h"
#include "../widgets/xb_widgets.h"
#include <string.h>

#define STACK_MAX 8

static lv_obj_t* g_root  = NULL;
static lv_obj_t* g_cur   = NULL;
static page_id_t g_stack[STACK_MAX];
static int       g_sp    = 0;
static page_id_t g_id    = PAGE_BOOT;

static const page_ctor_t CTORS[PAGE_MAX] = {
    [PAGE_BOOT]            = page_boot_create,
    [PAGE_HOME]            = page_home_create,
    [PAGE_MENU]            = page_menu_create,
    [PAGE_CHAT]            = page_chat_create,
    [PAGE_MODEL_PICKER]    = page_model_picker_create,
    [PAGE_THEME_PICKER]    = page_theme_picker_create,
    [PAGE_PERSONA_GRID]    = page_persona_grid_create,
    [PAGE_MEMORY_BROWSER]  = page_memory_browser_create,
    [PAGE_SETTINGS]        = page_settings_create,
    [PAGE_WIFI_AP]         = page_wifi_ap_create,
    [PAGE_SKILLS_EMPTY]    = page_skills_empty_create,
    [PAGE_CONSOLE]         = page_console_create,
};

static void route_to(page_id_t p) {
    if (p >= PAGE_MAX || !CTORS[p]) return;
    if (g_cur) lv_obj_delete(g_cur);
    g_cur = CTORS[p](g_root);
    g_id  = p;
}

void xb_router_init(lv_obj_t* root) {
    g_root = root;
    g_sp   = 0;
    route_to(PAGE_BOOT);
}

void xb_router_go(page_id_t p) {
    if (p == g_id) return;
    if (g_sp < STACK_MAX) g_stack[g_sp++] = g_id;
    route_to(p);
}

page_id_t xb_router_current(void) { return g_id; }

void xb_router_back(void) {
    if (g_sp == 0) { route_to(PAGE_HOME); return; }
    page_id_t prev = g_stack[--g_sp];
    route_to(prev);
}
