// main.c — app entry. Sets up display via esp_lvgl_port, initializes the
// theme + event bus + face engine, then hands off to the router.
//
// On a host PC build (sdl) the harness in tools/sim_main.c calls xb_app_start.
#include "lvgl.h"
#include "core/xb_event.h"
#include "core/xb_theme.h"
#include "core/xb_face.h"
#include "pages/xb_pages.h"

void xb_app_start(void) {
    xb_event_init();
    xb_theme_set("tech");

    lv_obj_t* scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, xb_theme_get()->bg, 0);

    xb_router_init(scr);
}

#ifdef ESP_PLATFORM
#include "esp_log.h"
#include "esp_lvgl_port.h"
void app_main(void) {
    // Display + LVGL init handled by board package (omitted — see INTEGRATION.md).
    xb_app_start();
}
#endif
