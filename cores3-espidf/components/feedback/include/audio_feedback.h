/* v5.0 Audio Feedback (对齐 audioFeedback.ts) */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    AUDIO_DOUBLE_CLICK,   // 800Hz, 100ms
    AUDIO_MENU_OPEN,      // 600Hz, 80ms
    AUDIO_MENU_SELECT,    // 1200Hz, 80ms
    AUDIO_MENU_SLIDE,     // 递增音阶
} AudioEvent;

void audio_play(AudioEvent event);
void audio_init(void);

#ifdef __cplusplus
}
#endif
