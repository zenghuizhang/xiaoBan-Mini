// main.c — ESP-IDF app entry. Boots LVGL + xb_ui on M5Stack CoreS3.
//
// Wiring assumption (CoreS3 default):
//   ST7789  via esp_lvgl_port_display_create()
//   FT6336U touch via esp_lcd_touch_ft5x06
//   I2C   = SDA12 / SCL11
//   MPU6886 IMU on internal I2C
//
// The xb_ui component is purely LVGL — it has no direct HW dependency
// other than NVS for theme/persona persistence.
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "nvs_flash.h"
#include "lvgl.h"

#include "core/xb_event.h"
#include "core/xb_theme.h"
#include "sim/xb_persona.h"
#include "sim/xb_memory_store.h"
#include "pages/xb_pages.h"

static const char* TAG = "xb_app";

void app_main(void) {
    ESP_LOGI(TAG, "claw_xb v7.6 starting…");

    // 1. NVS (theme + persona need it)
    esp_err_t r = nvs_flash_init();
    if (r == ESP_ERR_NVS_NO_FREE_PAGES || r == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    // 2. xb_ui subsystems
    xb_event_init();
    xb_theme_init();
    xb_persona_init();
    xb_memory_init();

    // 3. LVGL (display + touch via BSP — wire this in your board layer)
    //    lv_init() and lvgl_port_init() are called by your bsp_init().
    extern void bsp_init(void);  // implemented in main/bsp_corS3.c
    bsp_init();

    // 4. Bring up UI on the active screen
    lv_obj_t* scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, xb_theme_get()->bg, 0);
    xb_router_init(scr);

    ESP_LOGI(TAG, "boot done · theme=%s persona=%s",
             xb_theme_name(), xb_persona_active()->id);
}
