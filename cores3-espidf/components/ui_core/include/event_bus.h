/* v6.2 Event Bus — 对齐 esp-claw 事件驱动架构 */
#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    EVT_BOOT_DONE,         // 开机完成
    EVT_WIFI_CONNECTED,    // WiFi 连接
    EVT_WIFI_DISCONNECTED, // WiFi 断连
    EVT_TOUCH_DOUBLE,      // 双击屏幕
    EVT_TOUCH_SINGLE,      // 单击屏幕
    EVT_IMU_TILT_FWD,      // 前倾
    EVT_IMU_TILT_BACK,     // 后仰
    EVT_IMU_TILT_LEFT,     // 左倾
    EVT_IMU_TILT_RIGHT,    // 右倾
    EVT_IMU_SHAKE,         // 摇晃
    EVT_IMU_TAP,           // 轻敲
    EVT_EXPR_CHANGED,      // 表情切换
    EVT_THEME_CHANGED,     // 主题切换
    EVT_LANG_CHANGED,      // 语言切换
    EVT_COUNT
} EventType;

typedef void (*event_handler_t)(EventType type, void *data);

void event_bus_init(void);
void event_bus_subscribe(EventType type, event_handler_t handler);
void event_bus_publish(EventType type, void *data);

#ifdef __cplusplus
}
#endif
