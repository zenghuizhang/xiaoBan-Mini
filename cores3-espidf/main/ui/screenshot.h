/*
 * 截图回传系统
 * 通过 LVGL snapshot 捕获屏幕 → base64 编码 → printf 输出
 * Python capture.py 接收并还原 PNG
 */
#pragma once
#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

/** 在 main 主循环中调用，检查是否有截图请求并执行 */
void screenshot_process_pending(void);

/** 请求一次截图 (可从任意任务调用, 线程安全) */
void screenshot_request(void);

/** 初始化截图系统 */
void screenshot_init(void);

/** 启动后自动截图 (约 500ms 延迟) */
void screenshot_boot_trigger(void);

#ifdef __cplusplus
}
#endif
