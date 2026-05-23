/* v6.2 Breathing Engine — Modified Sine + cubic easing, 5 levels */
#include "breathing.h"
#include <math.h>

static BreathLevel s_level = BREATH_IDLE;
static int32_t s_value = 0;
static int s_phase = 0;

// 5级呼吸参数 (周期ms, 振幅)
static const struct {
    int period_ms;    // 完整周期
    float amplitude;   // 0~1
} s_params[] = {
    {8000, 0.05f},    // DEEP_SLEEP
    {6000, 0.08f},    // LIGHT_REST
    {4000, 0.10f},    // IDLE
    {2500, 0.15f},    // ALERT
    {1500, 0.18f},    // EXCITED
};

void breathing_init(void) { s_level = BREATH_IDLE; }

void breathing_set_level(BreathLevel level) { s_level = level; }

int32_t breathing_get_value(void)
{
    int period = s_params[s_level].period_ms;
    // Advance phase: each call is ~10ms (driven by lv_timer_handler)
    s_phase = (s_phase + 1) % (period / 10);
    float t = (float)s_phase / (period / 10);  // 0~1
    // cubic eased sin: more natural than pure sin
    float raw = sinf(t * 2.0f * M_PI);
    float eased = raw * raw * raw;  // cubic
    s_value = (int32_t)(eased * 255);
    return s_value;
}

float breathing_get_scale(void)
{
    float amp = s_params[s_level].amplitude;
    float raw = sinf(s_phase * 2.0f * M_PI / (s_params[s_level].period_ms / 10));
    float eased = raw * raw * raw;
    return 1.0f + amp * eased;
}

int32_t breathing_get_brightness(void)
{
    float raw = sinf(s_phase * 2.0f * M_PI / (s_params[s_level].period_ms / 10));
    float eased = raw * raw * (raw < 0 ? -1.0f : 1.0f);
    return 155 + (int32_t)(25 * eased);
}
