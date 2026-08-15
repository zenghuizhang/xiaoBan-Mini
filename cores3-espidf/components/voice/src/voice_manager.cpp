/* voice_manager.cpp — 语音交互状态机 */
#include "voice_manager.h"
#include "wake_word.h"
#include "asr.h"
#include "tts.h"
#include <M5Unified.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string.h>

static const char *TAG = "VOICE";

static voice_state_t s_state = VOICE_STATE_IDLE;
static voice_state_cb_t s_cb = NULL;
static voice_llm_cb_t s_llm_cb = NULL;
static TaskHandle_t s_task = NULL;
static volatile bool s_listen_trigger = false;

#define MAX_RECORD_MS   10000   // 最长录音 10 秒
#define SAMPLE_RATE     16000
#define VAD_SILENCE_MS  1500    // 静音 1.5 秒判定结束

static void _set_state(voice_state_t st, const char *text)
{
    s_state = st;
    if (s_cb) s_cb(st, text);
}

/* 唤醒词回调: IDLE → LISTENING */
static void _on_wake_detected(void)
{
    ESP_LOGI(TAG, "Wake word detected, entering LISTENING");
    s_listen_trigger = true;
}

/* 主语音任务: 处理状态流转 */
static void _voice_task(void *arg)
{
    (void)arg;
    int16_t *pcm_buf = (int16_t *)heap_caps_malloc(
        MAX_RECORD_MS * SAMPLE_RATE / 1000 * sizeof(int16_t),
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!pcm_buf) {
        ESP_LOGE(TAG, "pcm_buf alloc failed");
        vTaskDelete(NULL);
        return;
    }

    while (1) {
        /* 等待唤醒 (由 wake_word 回调设置 s_listen_trigger) */
        while (!s_listen_trigger) {
            vTaskDelay(pdMS_TO_TICKS(50));
        }
        s_listen_trigger = false;

        /* ===== LISTENING: 录音 ===== */
        _set_state(VOICE_STATE_LISTENING, NULL);
        wake_word_stop();   // 停止唤醒词任务, 释放麦克风
        wake_word_reset();

        size_t total_samples = 0;
        size_t max_samples = MAX_RECORD_MS * SAMPLE_RATE / 1000;
        int silence_frames = 0;
        int max_silence_frames = VAD_SILENCE_MS / 32;  // 32ms per frame
        int chunk = 512;

        uint32_t start_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
        while (total_samples < max_samples) {
            int n = M5.Mic.record(pcm_buf + total_samples, chunk);
            if (n <= 0) break;
            total_samples += n;

            /* 简易 VAD: 计算帧能量 */
            int32_t energy = 0;
            for (int i = 0; i < n; i++) {
                energy += abs(pcm_buf[total_samples - n + i]);
            }
            energy /= n;

            if (energy < 300) {  // 静音阈值
                silence_frames++;
                if (silence_frames >= max_silence_frames && total_samples > SAMPLE_RATE) {
                    ESP_LOGI(TAG, "VAD: silence detected, stop recording");
                    break;
                }
            } else {
                silence_frames = 0;
            }

            uint32_t elapsed = (xTaskGetTickCount() * portTICK_PERIOD_MS) - start_ms;
            if (elapsed >= MAX_RECORD_MS) break;
        }

        ESP_LOGI(TAG, "Recorded %d samples (%d ms)",
                 (int)total_samples, (int)(total_samples * 1000 / SAMPLE_RATE));

        if (total_samples < SAMPLE_RATE / 2) {
            ESP_LOGW(TAG, "Audio too short, skip");
            _set_state(VOICE_STATE_IDLE, NULL);
            wake_word_start();
            continue;
        }

        /* ===== RECOGNIZING: ASR ===== */
        _set_state(VOICE_STATE_RECOGNIZING, NULL);
        char asr_text[512] = {0};
        esp_err_t err = asr_speech_to_text(pcm_buf, total_samples * sizeof(int16_t),
                                           asr_text, sizeof(asr_text));
        if (err != ESP_OK || !asr_text[0]) {
            ESP_LOGE(TAG, "ASR failed");
            _set_state(VOICE_STATE_IDLE, NULL);
            wake_word_start();
            continue;
        }
        ESP_LOGI(TAG, "ASR: %s", asr_text);

        /* ===== THINKING: LLM ===== */
        _set_state(VOICE_STATE_THINKING, asr_text);
        char llm_resp[1024] = {0};
        esp_err_t llm_err = ESP_FAIL;
        if (s_llm_cb) {
            llm_err = s_llm_cb(asr_text, llm_resp, sizeof(llm_resp));
        }
        if (llm_err != ESP_OK || !llm_resp[0]) {
            ESP_LOGE(TAG, "LLM failed");
            _set_state(VOICE_STATE_IDLE, NULL);
            wake_word_start();
            continue;
        }

        /* ===== SPEAKING: TTS ===== */
        _set_state(VOICE_STATE_SPEAKING, llm_resp);
        tts_speak(llm_resp);

        /* 回到 IDLE, 重新开始监听唤醒词 */
        _set_state(VOICE_STATE_IDLE, NULL);
        wake_word_start();
    }

    free(pcm_buf);
    vTaskDelete(NULL);
}

esp_err_t voice_manager_init(void)
{
    esp_err_t err = wake_word_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "wake_word_init failed: %s", esp_err_to_name(err));
        return err;
    }
    wake_word_set_callback(_on_wake_detected);

    xTaskCreatePinnedToCore(_voice_task, "voice_mgr", 16384, NULL, 5, &s_task, 1);
    ESP_LOGI(TAG, "voice_manager initialized");
    return ESP_OK;
}

void voice_manager_start(void)
{
    wake_word_start();
}

void voice_manager_stop(void)
{
    wake_word_stop();
}

voice_state_t voice_manager_get_state(void)
{
    return s_state;
}

void voice_manager_set_callback(voice_state_cb_t cb)
{
    s_cb = cb;
}

void voice_manager_set_llm_callback(voice_llm_cb_t cb)
{
    s_llm_cb = cb;
}

void voice_manager_trigger_listen(void)
{
    s_listen_trigger = true;
}
