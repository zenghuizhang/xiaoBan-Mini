// xb_pages.h — page registry consumed by xb_router.
#pragma once
#include "lvgl.h"

typedef enum {
    PAGE_BOOT = 0,
    PAGE_HOME,
    PAGE_MENU,
    PAGE_CHAT,
    PAGE_MODEL_PICKER,
    PAGE_THEME_PICKER,
    PAGE_PERSONA_GRID,
    PAGE_MEMORY_BROWSER,
    PAGE_SETTINGS,
    PAGE_WIFI_AP,
    PAGE_SKILLS_EMPTY,
    PAGE_CONSOLE,
    PAGE_MAX
} page_id_t;

typedef lv_obj_t* (*page_ctor_t)(lv_obj_t* parent);

// Each page exposes one ctor; destructor is LVGL's lv_obj_delete on the root.
lv_obj_t* page_boot_create(lv_obj_t* p);
lv_obj_t* page_home_create(lv_obj_t* p);
lv_obj_t* page_menu_create(lv_obj_t* p);
lv_obj_t* page_chat_create(lv_obj_t* p);
lv_obj_t* page_model_picker_create(lv_obj_t* p);
lv_obj_t* page_theme_picker_create(lv_obj_t* p);
lv_obj_t* page_persona_grid_create(lv_obj_t* p);
lv_obj_t* page_memory_browser_create(lv_obj_t* p);
lv_obj_t* page_settings_create(lv_obj_t* p);
lv_obj_t* page_wifi_ap_create(lv_obj_t* p);
lv_obj_t* page_skills_empty_create(lv_obj_t* p);
lv_obj_t* page_console_create(lv_obj_t* p);

// Router
void       xb_router_init(lv_obj_t* root);
void       xb_router_go(page_id_t p);
page_id_t  xb_router_current(void);
void       xb_router_back(void);                 // pops history
