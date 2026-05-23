/* v6.2 Breathing Engine — 5级呼吸状态 + cubic easing */
#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BREATH_DEEP_SLEEP = 0,  // 8000ms, 5%
    BREATH_LIGHT_REST,      // 6000ms, 8%
    BREATH_IDLE,            // 4000ms, 10%
    BREATH_ALERT,           // 2500ms, 15%
    BREATH_EXCITED,         // 1500ms, 18%
} BreathLevel;

void breathing_init(void);
void breathing_set_level(BreathLevel level);

/** 获取当前呼吸值 (0~255, cubic eased sin) */
int32_t breathing_get_value(void);

/** 获取呼吸缩放因子 (0.88~1.12) */
float breathing_get_scale(void);

/** 获取亮度值 (130~180) */
int32_t breathing_get_brightness(void);

#ifdef __cplusplus
}
#endif
