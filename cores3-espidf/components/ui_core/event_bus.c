/* v6.2 Event Bus (esp-claw 模式: 发布/订阅, 解耦组件通信) */
#include "event_bus.h"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <esp_log.h>
#include <string.h>

static const char *TAG = "EVENT";
#define MAX_HANDLERS 4

static struct {
    event_handler_t handlers[MAX_HANDLERS];
    int count;
} subscribers[EVT_COUNT];
static SemaphoreHandle_t s_mutex = NULL;

void event_bus_init(void)
{
    s_mutex = xSemaphoreCreateMutex();
    memset(subscribers, 0, sizeof(subscribers));
    ESP_LOGI(TAG, "event bus ready (%d types)", EVT_COUNT);
}

void event_bus_subscribe(EventType type, event_handler_t handler)
{
    if (type >= EVT_COUNT || !handler) return;
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    if (subscribers[type].count < MAX_HANDLERS) {
        subscribers[type].handlers[subscribers[type].count++] = handler;
    }
    xSemaphoreGive(s_mutex);
}

void event_bus_publish(EventType type, void *data)
{
    if (type >= EVT_COUNT) return;
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    for (int i = 0; i < subscribers[type].count; i++) {
        if (subscribers[type].handlers[i]) {
            subscribers[type].handlers[i](type, data);
        }
    }
    xSemaphoreGive(s_mutex);
}
