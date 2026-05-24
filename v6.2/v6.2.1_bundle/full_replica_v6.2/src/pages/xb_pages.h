// xb_pages.h — entry points for every page in the v6.2 replica.
// Each xxx_create() builds an lv_obj_t under `parent` and returns the root.
// Each page is responsible for its own subscriptions and unsubscriptions
// (page lifetime tied to its root lv_obj via LV_EVENT_DELETE).
#pragma once
#include "lvgl.h"

lv_obj_t* page_boot_create        (lv_obj_t* parent);
lv_obj_t* page_home_create        (lv_obj_t* parent);
lv_obj_t* page_menu_create        (lv_obj_t* parent);   // 6-sector radial overlay
lv_obj_t* page_chat_create        (lv_obj_t* parent);
lv_obj_t* page_wifi_ap_create     (lv_obj_t* parent);
lv_obj_t* page_wifi_pair_create   (lv_obj_t* parent);
lv_obj_t* page_ota_create         (lv_obj_t* parent);
lv_obj_t* page_skills_create      (lv_obj_t* parent);
lv_obj_t* page_skill_detail_create(lv_obj_t* parent, const char* skill_id);
lv_obj_t* page_settings_create    (lv_obj_t* parent);
lv_obj_t* page_console_create     (lv_obj_t* parent);

// Tiny router — replace with real claw_navigator on device.
typedef enum {
    XB_PAGE_BOOT, XB_PAGE_HOME, XB_PAGE_MENU, XB_PAGE_CHAT,
    XB_PAGE_WIFI_AP, XB_PAGE_WIFI_PAIR, XB_PAGE_OTA,
    XB_PAGE_SKILLS, XB_PAGE_SKILL_DETAIL,
    XB_PAGE_SETTINGS, XB_PAGE_CONSOLE
} xb_page_id_t;

void xb_router_init(lv_obj_t* root);
void xb_router_goto(xb_page_id_t id);
void xb_router_back(void);
