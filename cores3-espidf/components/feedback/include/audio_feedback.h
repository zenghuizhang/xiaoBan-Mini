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
    // 表情音效 (竞品对齐: Vector/Loona 声音反馈)
    AUDIO_EXPR_HAPPY,     // 轻快上升音
    AUDIO_EXPR_SURPRISED, // 惊叹短音
    AUDIO_EXPR_SAD,       // 低沉下降音
    AUDIO_EXPR_SLEEP,     // 柔和低音
    AUDIO_EXPR_DIZZY,     // 旋转音
} AudioEvent;

void audio_play(AudioEvent event);
void audio_init(void);

#ifdef __cplusplus
}
#endif
