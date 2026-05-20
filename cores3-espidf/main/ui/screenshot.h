/* 截图回传系统 */
#pragma once
#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

void screenshot_process_pending(void);
void screenshot_request(void);
void screenshot_init(void);
void screenshot_boot_trigger(void);

#ifdef __cplusplus
}
#endif
