# 语音交互设计方案 (Voice Interaction Design)

## 1. 调研结论 (Research Summary)

### 1.1 硬件能力
| 项目 | 规格 |
|------|------|
| MCU | ESP32-S3 (双核 240MHz) |
| Flash | 16MB |
| PSRAM | 8MB |
| 麦克风 | 内置 PDM 咪头 (M5.Mic, 16kHz/16bit) |
| 扬声器 | 内置喇叭 (M5.Speaker, 支持 playRaw PCM 播放) |

### 1.2 ESP-SR 本地语音框架
| 模块 | 功能 | PSRAM 占用 | CPU 占用 |
|------|------|-----------|---------|
| WakeNet9 | 唤醒词检测 | ~324 KB | ~9% 单核 |
| MultiNet7 CN | 命令词识别 (≤300条) | ~2.9 MB | ~11ms/帧 |
| ESP-TTS | 中文语音合成 | 依赖 voice_data | - |

**可用中文唤醒词**: 你好小智、小爱同学、你好小鑫、小美同学、Hi小星 等 30+ 个。

### 1.3 当前项目状态
- ❌ 麦克风未使用 (M5.Mic 未初始化)
- ❌ 无 ASR/STT
- ❌ 无 TTS (仅有 M5.Speaker.tone 蜂鸣)
- ✅ LLM 对话已实现 (claw_core, 云端)
- ✅ 表情系统 26 种 (含 EXPR_LISTENING / EXPR_THINKING / EXPR_TALKING)
- ✅ 对话气泡 DIALOG_VOICE_WAKE ("我在听...") 已定义但未调用

### 1.4 技术选型
| 环节 | 方案 | 理由 |
|------|------|------|
| 唤醒词 | **ESP-SR WakeNet9 (本地)** | 始终在线、低延迟、保护隐私 |
| ASR/STT | **云端 Whisper API** | 自由语音识别，本地 MultiNet 仅支持命令词 |
| TTS | **云端 TTS API** | 音质好、多语种；ESP-SR TTS 仅中文且音质一般 |
| 音频播放 | M5.Speaker.playRaw(int16_t*, 16kHz) | 直接播放 PCM |

> 说明：项目已使用云端 LLM (claw_core)，因此 ASR/TTS 复用同一 API Key 体系，架构一致。

---

## 2. 交互设计 (Interaction Design)

### 2.1 状态机

```
                    ┌──────────────────────────────────────┐
                    │                                      │
                    ▼                                      │
  ┌──────┐  唤醒词   ┌──────────┐  VAD结束  ┌────────────┐ │
  │ IDLE │ ────────► │ LISTENING│ ────────► │ RECOGNIZING│ │
  └──────┘           └──────────┘           └────────────┘ │
     ▲                                                │     │
     │                                                ▼     │
     │                                         ┌──────────┐ │
     │                                         │ THINKING │ │
     │                                         └──────────┘ │
     │                                                │     │
     │                                       LLM回复  ▼     │
     │                                         ┌──────────┐ │
     └─────────────────────────────────────────│ SPEAKING │ │
                          TTS播放完毕           └──────────┘ │
```

### 2.2 各状态表现

| 状态 | 表情 | 气泡 | 行为 |
|------|------|------|------|
| IDLE | EXPR_IDLE | - | WakeNet 监听唤醒词 |
| LISTENING | EXPR_LISTENING (兴奋) | "我在听..." | 录音，VAD 检测语音结束 |
| RECOGNIZING | EXPR_THINKING | "识别中..." | 上传音频到 Whisper API |
| THINKING | EXPR_THINKING | "思考中..." | 等待 LLM 回复 |
| SPEAKING | EXPR_TALKING | 回复文本 | TTS 播放，播放完毕回到 IDLE |

### 2.3 唤醒词
- **选用**: "你好小智" (wn9_nihaoxiaozhi_tts)
- **理由**: 预训练模型，识别率高；与"小伴"语义接近
- **未来**: 可训练自定义"你好小伴"唤醒词

### 2.4 超时处理
- 录音最长 10 秒，超时自动结束
- ASR 请求超时 15 秒
- LLM 超时 30 秒 (复用 claw_core)
- TTS 超时 15 秒

---

## 3. 技术方案 (Technical Solution)

### 3.1 组件结构

```
components/voice/
├── CMakeLists.txt
├── include/
│   ├── voice_manager.h      // 状态机 + 编排
│   ├── wake_word.h          // ESP-SR WakeNet 封装
│   ├── asr.h                // 云端 ASR (Whisper)
│   └── tts.h                // 云端 TTS
└── src/
    ├── voice_manager.cpp
    ├── wake_word.cpp
    ├── asr.cpp
    └── tts.cpp
```

### 3.2 核心 API

```c
// voice_manager.h
typedef enum {
    VOICE_STATE_IDLE,
    VOICE_STATE_LISTENING,
    VOICE_STATE_RECOGNIZING,
    VOICE_STATE_THINKING,
    VOICE_STATE_SPEAKING,
} voice_state_t;

typedef void (*voice_state_cb_t)(voice_state_t state, const char* text);

esp_err_t voice_manager_init(void);
void voice_manager_start(void);
void voice_manager_stop(void);
voice_state_t voice_manager_get_state(void);
void voice_manager_set_callback(voice_state_cb_t cb);
```

### 3.3 数据流

```
M5.Mic (16kHz PCM)
    │
    ▼
ESP-SR AFE ──(wakeup)──► voice_manager: IDLE → LISTENING
    │
    ▼ (继续录音)
VAD 检测语音结束
    │
    ▼
ASR (Whisper API) ──► text
    │
    ▼
LLM (claw_core) ──► response_text
    │
    ▼
TTS (OpenAI TTS API) ──► PCM
    │
    ▼
M5.Speaker.playRaw(PCM, 16kHz)
```

### 3.4 内存预算
| 项目 | 大小 |
|------|------|
| WakeNet9 模型 | 324 KB (PSRAM) |
| AFE 内部缓冲 | ~100 KB |
| 录音缓冲 (10s × 16kHz × 2B) | 320 KB |
| ASR/TTS HTTP 缓冲 | 128 KB |
| **合计** | **~870 KB PSRAM** |

### 3.5 Flash 预算
| 项目 | 大小 |
|------|------|
| App | 1.6 MB |
| ESP-SR 模型分区 | 1 MB (仅 WakeNet) |
| **合计** | **~2.6 MB / 16 MB** |

### 3.6 分区表修改
```csv
# Name,   Type, SubType, Offset,  Size, Flags
nvs,      data, nvs,     ,        0x6000,
phy_init, data, phy,     ,        0x1000,
factory,  app,  factory, ,        2000K,
model,    data, spiffs,  ,        1M,          # ESP-SR 模型
```

---

## 4. 实现计划 (Implementation Plan)

1. **添加 ESP-SR 依赖** + 分区表 + menuconfig 唤醒词
2. **wake_word.cpp**: AFE 初始化 + 唤醒词检测回调
3. **asr.cpp**: Whisper API 调用 (multipart/form-data)
4. **tts.cpp**: OpenAI TTS API 调用 + PCM 播放
5. **voice_manager.cpp**: 状态机 + 录音 + VAD + 编排
6. **集成到 app_main**: 初始化 + 状态回调更新 UI
7. **修改 page_chat**: 添加语音输入按钮
