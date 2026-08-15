/* wake_word.h — ESP-SR WakeNet 唤醒词检测封装 */
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 唤醒词检测回调 */
typedef void (*wake_word_detected_cb_t)(void);

/** 初始化 AFE + WakeNet */
esp_err_t wake_word_init(void);

/** 启动唤醒词检测任务 */
esp_err_t wake_word_start(void);

/** 停止唤醒词检测 */
void wake_word_stop(void);

/** 注册唤醒回调 */
void wake_word_set_callback(wake_word_detected_cb_t cb);

/** 获取 AFE 处理后的单声道音频 (供 ASR 使用) */
int wake_word_fetch_audio(int16_t *buf, int max_samples);

/** 重置 AFE 状态 (开始新一轮录音前调用) */
void wake_word_reset(void);

#ifdef __cplusplus
}
#endif
