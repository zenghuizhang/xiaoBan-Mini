/* tts.h — 云端 TTS (OpenAI TTS 兼容 API) + PCM 播放 */
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 将文本转为语音并播放
 * @param text  要合成的文本
 * @return ESP_OK 成功
 */
esp_err_t tts_speak(const char *text);

/** 停止当前播放 */
void tts_stop(void);

/** 是否正在播放 */
bool tts_is_speaking(void);

#ifdef __cplusplus
}
#endif
