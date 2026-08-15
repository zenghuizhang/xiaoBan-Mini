/* voice_manager.h — 语音交互状态机 (wake → listen → ASR → LLM → TTS) */
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    VOICE_STATE_IDLE,          // 待机: 监听唤醒词
    VOICE_STATE_LISTENING,     // 聆听: 录音中
    VOICE_STATE_RECOGNIZING,   // 识别: ASR 上传中
    VOICE_STATE_THINKING,      // 思考: LLM 回复中
    VOICE_STATE_SPEAKING,      // 说话: TTS 播放中
} voice_state_t;

/* 状态变化回调: state=新状态, text=关联文本 (ASR结果/LLM回复/NULL) */
typedef void (*voice_state_cb_t)(voice_state_t state, const char *text);

/* LLM 文本生成回调: 输入用户文本, 输出回复文本 (由 app 层注册) */
typedef esp_err_t (*voice_llm_cb_t)(const char *user_text, char *resp_buf, size_t resp_size);

/** 初始化语音系统 (AFE + 回调) */
esp_err_t voice_manager_init(void);

/** 启动唤醒词监听 */
void voice_manager_start(void);

/** 停止语音系统 */
void voice_manager_stop(void);

/** 获取当前状态 */
voice_state_t voice_manager_get_state(void);

/** 注册状态变化回调 */
void voice_manager_set_callback(voice_state_cb_t cb);

/** 注册 LLM 回调 (由 app 层提供, 如 chat_llm) */
void voice_manager_set_llm_callback(voice_llm_cb_t cb);

/** 手动触发聆听 (例如按钮唤醒) */
void voice_manager_trigger_listen(void);

#ifdef __cplusplus
}
#endif
