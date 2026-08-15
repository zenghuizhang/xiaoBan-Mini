/* asr.h — 云端 ASR (OpenAI Whisper 兼容 API) */
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 将 PCM 音频发送到 Whisper API 进行语音识别
 * @param pcm_data   16kHz 16-bit mono PCM 数据
 * @param pcm_len    PCM 数据字节数
 * @param out_text   输出文本缓冲区
 * @param out_size   缓冲区大小
 * @return ESP_OK 成功
 */
esp_err_t asr_speech_to_text(const int16_t *pcm_data, size_t pcm_len,
                             char *out_text, size_t out_size);

#ifdef __cplusplus
}
#endif
