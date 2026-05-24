// xb_router.c — minimal stack-based page navigator (max depth 6).
#include "xb_pages.h"

#define MAX_DEPTH 6
static lv_obj_t* g_root = NULL;
static xb_page_id_t g_stack[MAX_DEPTH];
static int g_top = -1;
static lv_obj_t* g_current_obj = NULL;

static lv_obj_t* build(xb_page_id_t id) {
    switch (id) {
        case XB_PAGE_BOOT:         return page_boot_create(g_root);
        case XB_PAGE_HOME:         return page_home_create(g_root);
        case XB_PAGE_MENU:         return page_menu_create(g_root);
        case XB_PAGE_CHAT:         return page_chat_create(g_root);
        case XB_PAGE_WIFI_AP:      return page_wifi_ap_create(g_root);
        case XB_PAGE_WIFI_PAIR:    return page_wifi_pair_create(g_root);
        case XB_PAGE_OTA:          return page_ota_create(g_root);
        case XB_PAGE_SKILLS:       return page_skills_create(g_root);
        case XB_PAGE_SKILL_DETAIL: return page_skill_detail_create(g_root, "weather");
        case XB_PAGE_SETTINGS:     return page_settings_create(g_root);
        case XB_PAGE_CONSOLE:      return page_console_create(g_root);
    }
    return NULL;
}

void xb_router_init(lv_obj_t* root) {
    g_root = root;
    g_top = -1;
    g_current_obj = NULL;
    xb_router_goto(XB_PAGE_BOOT);
}

void xb_router_goto(xb_page_id_t id) {
    if (g_current_obj) { lv_obj_del(g_current_obj); g_current_obj = NULL; }
    if (g_top < MAX_DEPTH - 1) g_stack[++g_top] = id;
    g_current_obj = build(id);
}

void xb_router_back(void) {
    if (g_top <= 0) return;
    if (g_current_obj) { lv_obj_del(g_current_obj); g_current_obj = NULL; }
    g_top--;
    g_current_obj = build(g_stack[g_top]);
}
