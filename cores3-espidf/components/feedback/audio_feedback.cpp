/* v5.0 Audio Feedback (M5.Speaker) */
#include "audio_feedback.h"
#include <M5Unified.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

void audio_play(AudioEvent event)
{
    int freq = 800, dur = 100;
    switch (event) {
    case AUDIO_DOUBLE_CLICK: freq = 800; dur = 100; break;
    case AUDIO_MENU_OPEN:    freq = 600; dur = 80;  break;
    case AUDIO_MENU_SELECT:  freq = 1200; dur = 80; break;
    case AUDIO_MENU_SLIDE:   freq = 900; dur = 50;  break;
    // 表情音效
    case AUDIO_EXPR_HAPPY:     M5.Speaker.tone(800, 60); vTaskDelay(70); M5.Speaker.tone(1200, 80); return;
    case AUDIO_EXPR_SURPRISED: M5.Speaker.tone(1500, 40); vTaskDelay(50); M5.Speaker.tone(1800, 60); return;
    case AUDIO_EXPR_SAD:       M5.Speaker.tone(400, 150); return;
    case AUDIO_EXPR_SLEEP:     M5.Speaker.tone(300, 200); return;
    case AUDIO_EXPR_DIZZY:     M5.Speaker.tone(600, 50); vTaskDelay(60); M5.Speaker.tone(900, 50); vTaskDelay(60); M5.Speaker.tone(600, 50); return;
    }
    M5.Speaker.tone(freq, dur);
    vTaskDelay(pdMS_TO_TICKS(dur + 10));
}

void audio_init(void)
{
    M5.Speaker.setVolume(128);  // 50%
}
