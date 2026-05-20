/* v5.0 DialogBubble: 对话气泡 (对齐 RobotUI.tsx DialogBubble) */
#pragma once
#include <lvgl.h>
#include "theme_v3.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DIALOG_MORNING,   // 早安问候
    DIALOG_SUGGEST,   // 表情建议
    DIALOG_SLEEP,     // 晚安提示
} DialogType;

/** 显示对话气泡, duration_ms 后自动关闭 (0=手动关闭) */
lv_obj_t *dialog_bubble_show(lv_obj_t *parent, DialogType type, uint32_t duration_ms);

/** 关闭对话气泡 */
void dialog_bubble_close(void);

/** 是否正在显示 */
bool dialog_bubble_is_shown(void);

#ifdef __cplusplus
}
#endif
