/* v5.0 MotionController (BMI270 IMU) */
#include "motion_controller.h"
#include <M5Unified.h>
#include <esp_log.h>
#include <math.h>

static const char *TAG = "MOTION";
static uint32_t last_detect_ms = 0;

void motion_init(void)
{
    ESP_LOGI(TAG, "IMU ready");
}

MotionAction motion_poll(void)
{
    uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
    if (now - last_detect_ms < 500) return MOTION_NONE; // 500ms debounce

    float ax, ay, az;
    M5.Imu.getAccel(&ax, &ay, &az);

    // Tilt detection (pitch from accel)
    float pitch = atan2f(-ax, sqrtf(ay*ay + az*az)) * 180.0f / M_PI;
    float roll  = atan2f(ay, az) * 180.0f / M_PI;

    // Magnitude for shake/tap
    float mag = sqrtf(ax*ax + ay*ay + az*az);

    if (mag > 2.0f) {
        ESP_LOGI(TAG, "shake! %.2fg → dizzy", mag);
        last_detect_ms = now;
        return MOTION_SHAKE;
    }
    if (mag > 1.5f && fabsf(az) > 1.2f) {
        ESP_LOGI(TAG, "tap! z=%.2f", az);
        last_detect_ms = now;
        return MOTION_TAP;
    }
    if (pitch > 15.0f) {
        last_detect_ms = now;
        return MOTION_TILT_FORWARD;
    }
    if (pitch < -15.0f) {
        last_detect_ms = now;
        return MOTION_TILT_BACKWARD;
    }
    if (roll > 15.0f) {
        last_detect_ms = now;
        return MOTION_TILT_LEFT;
    }
    if (roll < -15.0f) {
        last_detect_ms = now;
        return MOTION_TILT_RIGHT;
    }

    return MOTION_NONE;
}
