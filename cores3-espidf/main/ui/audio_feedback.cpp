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
    }
    M5.Speaker.tone(freq, dur);
    vTaskDelay(pdMS_TO_TICKS(dur + 10));
}

void audio_init(void)
{
    M5.Speaker.setVolume(128);  // 50%
}
