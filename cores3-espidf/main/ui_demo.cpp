/*
 * 科技风 UI 演示程序
 * 直接编译运行即可看到效果
 */

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <M5Unified.h>
#include <lvgl.h>

#include "ui/tech_ui.h"
#include "ui/font_zh_14.h"

static const char *TAG = "TECH_UI_DEMO";

#define LV_BUFFER_LINES 40
static lv_color_t lv_buf1[320 * LV_BUFFER_LINES];
static lv_color_t lv_buf2[320 * LV_BUFFER_LINES];

static void lv_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    uint32_t w = lv_area_get_width(area);
    uint32_t h = lv_area_get_height(area);
    M5.Display.startWrite();
    M5.Display.setAddrWindow(area->x1, area->y1, w, h);
    M5.Display.pushPixels((uint16_t *)px_map, w * h);
    M5.Display.endWrite();
    lv_display_flush_ready(disp);
}

static void lv_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    auto t = M5.Touch.getDetail();
    if (t.isPressed()) {
        data->point.x = t.x;
        data->point.y = t.y;
        data->state = LV_INDEV_STATE_PR;
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

static void ui_update_task(void *pv)
{
    int count = 0;
    while (1) {
        // 模拟时间更新
        char time_buf[16];
        snprintf(time_buf, sizeof(time_buf), "%02d:%02d", (count % 24), (count % 60));
        tech_ui_set_time(time_buf);
        
        // 模拟电池变化
        tech_ui_set_battery(50 + (count % 50));
        
        // 更新状态文字
        const char *statuses[] = {"陪伴中...", "思考中...", "开心~", "打哈欠...", "发呆中..."};
        tech_ui_update_status(statuses[count % 5]);
        
        count++;
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "=== 小伴 Mini 科技风 UI 演示 ===");
    
    // 初始化 M5Unified
    auto cfg = M5.config();
    M5.begin(cfg);
    M5.Display.setBrightness(200);
    M5.Display.setRotation(1);  // 横屏
    
    ESP_LOGI(TAG, "M5 初始化完成");
    
    // 初始化 LVGL
    lv_init();
    
    lv_display_t *disp = lv_display_create(320, 240);
    lv_display_set_flush_cb(disp, lv_flush_cb);
    lv_display_set_buffers(disp, lv_buf1, lv_buf2, sizeof(lv_buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);
    
    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, lv_read_cb);
    
    ESP_LOGI(TAG, "LVGL 初始化完成");
    
    // 创建科技风 UI
    tech_ui_create(lv_screen_active());
    
    ESP_LOGI(TAG, "科技风 UI 创建完成");
    
    // 启动 UI 更新任务
    xTaskCreate(ui_update_task, "ui_update", 4096, NULL, 5, NULL);
    
    // 主循环
    while (1) {
        M5.update();
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}
