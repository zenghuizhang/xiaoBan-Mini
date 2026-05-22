# xiaoBan-Mini 陪伴机器人 - 开发环境记录

> **最后更新**: 2026-05-22
> **当前版本**: v6.1 (ESP-IDF 5.5.4 + LVGL 9.5.0)
> **状态**: ✅ 完整稳定版本，所有功能正常运行

## 截图系统

### PC 侧接收
```bash
# 方式1: 被动等待自动截图 (设备启动 ~8s 后自动发)
python3 tools/capture.py /dev/ttyACM0 /tmp/screen.png --wait

# 方式2: 主动触发 (USB Serial 发 's')
python3 tools/capture.py /dev/ttyACM0 /tmp/screen.png
```

### 工作原理
1. `screenshot_request()` 在主循环设 volatile flag
2. `screenshot_process_pending()` 调用 `lv_snapshot_take_to_buf` (需要 `CONFIG_LV_USE_SNAPSHOT=y`)
3. 快照数据传入独立 FreeRTOS task 做 base64 编码
4. 编码结果通过 ESP_LOGI("SNAP", "SS:...") 输出
5. PC 侧 `capture.py` 从串口接收 base64 → 解码 RGB565 → 保存 PNG

### 触发方式
| 方式 | 说明 |
|------|------|
| 启动自动 | `app_main` 在 idle 后 2s 调用 `screenshot_request()` |
| 串口 `'s'` | USB Serial 发送 `'s'` 触发截图 (需主控制台=USB Serial JTAG) |
| 串口 `'d'` | 禁用截图触发 (持久化到 NVS, 发货版本用) |
| 串口 `'e'` | 重新启用截图触发 (持久化到 NVS) |
| ~~双击~~ | 已移除（避免 UI 卡顿） |

## v6.1 文件结构

```
cores3-espidf/main/
├── app_main.cpp          # 主入口, UI 层级, 交互逻辑
├── CMakeLists.txt        # 编译配置
├── idf_component.yml     # 组件依赖 (LVGL, M5Unified)
└── ui/
    ├── theme_v3.h/cpp    # 3主题 (Tech/Child/Dev), scale=1.6
    ├── expressions.h/cpp # 20+ 种表情 + 呼吸 + 眨眼动画
    ├── boot_anim.h/c     # 开机动画 (3.5s, 3主题)
    ├── radial_menu.h/c   # 径向菜单 (6按钮, 按钮A触发)
    ├── dialog_bubble.h/c # 对话气泡 (早安/建议/晚安)
    ├── motion_controller.h/cpp # IMU 体感 (BMI270)
    ├── scenario_overlay.h/c   # 场景叠加层
    ├── tech_ui.h/c       # 科技风 UI
    ├── wifi_config.h/cpp # WiFi 配网 (AP+DNS劫持+HTTP)
    ├── qrcode.h/c        # QR 码显示 (lv_draw_rect)
    ├── font_zh_14.h/c    # 14号中文字体 (2bpp, Noto Sans SC)
    ├── font_zh_22.h/c    # 22号中文字体
    └── icons/            # UI 图标资源
```

## v5.0 交互流程
```
开机 → BootAnim(3.5s) → idle(呼吸+瞳孔微动)
  ↓ 双击
RadialMenu (6按钮)
  ├ 表情  → 随机表情
  ├ 对话  → TALKING
  ├ 设置  → MenuOverlay (WiFi/亮度/音量)
  ├ 主题  → Tech↔Child↔Dev 循环
  ├ 扩展  → BottomBar (16按钮, 各表情)
  └ 随机  → 随机表情
  ↓ IMU 体感
倾斜/摇晃/轻敲 → curious/yawn/dizzy/wink
  ↓ 时间
6-10点首次 → Morning 气泡问候
```

## v5.0 设计对齐状态

| v5.0 功能 | 文件 | 状态 |
|----------|------|------|
| BootAnimation (3主题) | boot_anim.c | ✅ |
| RadialMenu (6按钮) | radial_menu.c | ✅ |
| Face (20种表情) | expressions.cpp | ✅ |
| 呼吸动画 + 瞳孔微动 | expressions.cpp | ✅ |
| scale=1.6 | expressions.h | ✅ |
| 3主题 Tech/Child/Dev | theme_v3.cpp | ✅ |
| Wink 单次播放 | expressions.cpp | ✅ |
| DialogBubble | dialog_bubble.c | ✅ |
| IMU 体感 | motion_controller.cpp | ✅ |
| 音效反馈 | audio_feedback.cpp | ✅ |
| NVS 记忆 | robot_memory.c | ✅ |
| WiFi QR 配网 | qrcode.c+wifi_config.cpp | ✅ |
| BottomBar (16按钮) | app_main.cpp | ✅ |
| 截图回传 | screenshot.c | ✅ |
| Morning 问候 | app_main.cpp | ✅ |
| MenuOverlay | app_main.cpp | ✅ |
| StatusBar | ❌ 设计稿已移除 | — |
| 粒子特效 | ⏳ ESP32 性能限制 | — |

## 编译工具
- **ESP-IDF** v5.5.4 — 已安装在 `/home/zzh/esp-idf`
- **工具链**: GCC 14.2.0 + GDB 16.3
- **LVGL**: v9.5.0 (通过组件管理器安装)

---

## 🚀 ESP-IDF 编译环境（cores3-espidf 项目）

### 环境初始化
```bash
# ESP-IDF v5.5.4 — 使用 export.sh 或直接指定 Python
source /home/zzh/esp-idf/export.sh
# 或手动设置
IDF_PATH=/home/zzh/esp-idf
IDF_PYTHON=/home/zzh/.espressif/python_env/idf5.5_py3.10_env/bin/python
alias idf.py="$IDF_PYTHON $IDF_PATH/tools/idf.py"
```

### 编译指令（M5Stack CoreS3）
```bash
cd /home/zzh/.openclaw-autumn/workspace/xiaoBan-Mini/cores3-espidf

# 编译
idf.py build
# 烧录
idf.py -p /dev/ttyACM0 flash
# 清理
idf.py clean
```

### ⚠️ 分区表配置

**重要**：V6.1 固件约 1.2MB，默认 1MB 分区不够。

已在 `sdkconfig` 中启用：
```
CONFIG_PARTITION_TABLE_SINGLE_APP_LARGE=y
```

**各分区大小**：
- factory: 1.5MB (0x10000 ~ 0x187000)
- 固件占用: ~1.2MB
- 剩余空间: ~300KB

### 烧录端口
- **Linux**: `/dev/ttyACM0`
- 端口自动检测，一般不需要手动指定

### 已集成组件（ESP-IDF Component Registry）
- **lvgl** v9.x — UI 图形库
- **m5unified** v0.2.x — M5Stack 硬件驱动

---

---

## 📊 ESP-IDF 升级记录 (v6.1)

### 5.2.1 → 5.5.4 升级要点

| 项目 | 变更 |
|------|------|
| GCC 版本 | 12.2 → 14.2 |
| GDB 版本 | 12.1 → 16.3 |
| 工具链架构 | 芯片独立工具链 → 统一工具链 |
| 分区表 | SINGLE_APP → SINGLE_APP_LARGE |
| 固件大小 | ~1.1MB → ~1.2MB |

### 兼容性

✅ **完全兼容零代码修改**
- LVGL 9.5.0 组件直接使用
- M5Unified 0.2.13 正常工作
- 所有现有功能无需调整

---

## ⚠️ Arduino 编译环境（已废弃，仅供参考）

## ⚠️ 正确的编译指令

### M5Stack CoreS3（当前目标硬件）
```bash
# 编译
arduino-cli compile --fqbn m5stack:esp32:m5stack_cores3 xiaoBan-Mini/cores3-sketch/

# 烧录（USB 口）
arduino-cli upload -p /dev/ttyACM0 --fqbn m5stack:esp32:m5stack_cores3 xiaoBan-Mini/cores3-sketch/

# 串口监视器
arduino-cli monitor -p /dev/ttyACM0 -c baudrate=115200
```

### ❌ 不要用这些（之前用错了）
```bash
# 错误：这是通用 ESP32 核心，未安装且与 CoreS3 不兼容
arduino-cli compile --fqbn esp32:esp32:esp32 ...

# 错误：这是 M5Stack 初代 Core（普通 ESP32），不是 CoreS3
arduino-cli compile --fqbn m5stack:esp32:m5stack_core ...
```

## Board Core
| Core | 版本 | FQBN | 用途 |
|------|------|------|------|
| m5stack:esp32 | 3.2.6 | `m5stack:esp32:m5stack_cores3` | M5Stack CoreS3 (ESP32-S3) |

## 已安装库
| 库名 | 版本 | 用途 |
|------|------|------|
| M5Unified | 0.2.13 | M5Stack 统一硬件驱动（显示/触摸/IMU/电源） |
| U8g2 | 2.35.30 | OLED 驱动（旧版 ESP32 用，CoreS3 不需要） |
| ESP32Servo | 3.1.3 | 舵机控制（旧版用，CoreS3 不需要） |

## 硬件信息 - M5Stack CoreS3
- **芯片**: ESP32-S3 (Xtensa LX7 双核 240MHz)
- **Flash**: 16MB
- **PSRAM**: 8MB (QSPI)
- **屏幕**: 2.0" IPS LCD 320x240 (GC9A01 驱动)
- **触摸**: FT6336 电容式触摸屏
- **电源管理**: AXP2101（可读电量）
- **IMU**: BMI270 六轴
- **RTC**: BM8563
- **音频**: ES7210 麦克风 + AW88298 扬声器
- **USB**: `/dev/ttyACM0` (Hardware CDC/JTAG)

---

## 开发注意事项

### LVGL 与 GC9A01 字节序不匹配 — 需要字节交换而非 R↔B 通道交换 (2026-05-16)

**现象**：LVGL 中通过 `lv_color_hex()` 设置的颜色在 M5Stack CoreS3 屏幕上显示异常——深灰变粉红、青色变灰绿。

**根因**：ESP32-S3 是小端序 CPU，uint16_t 在内存中存为 `[低字节, 高字节]`。
GC9A01 显示驱动通过 SPI 接收数据时要求 **MSB-first**（即先收高字节）。
`M5GFX::pushImageDMA()` 直接将内存字节流发送到 SPI，导致每个像素的高低位字节颠倒。
这不是 R/B 通道交换问题，而是 **字节序（endianness）问题**。

**修复方式**：在 LVGL 的 flush 回调中，对每个 16 位像素做字节交换（16-bit byte swap），再传给 M5GFX。

```cpp
// cores3-espidf/main/app_main.cpp → _lvgl_flush_callback()
uint16_t *pixels = (uint16_t *)px_map;
for (uint32_t i = 0; i < w * h; i++) {
    uint16_t p = pixels[i];
    pixels[i] = (p >> 8) | (p << 8);  // byte swap (little-endian → big-endian)
}
```

**验证**：灰色 (R≈G≈B) 在修正后应正确显示为灰色而非粉红色；
青色 #22D3EE 应显示为正确的青色而非灰绿色。

**教训**：
- 不要将字节序问题误判为 R/B 通道排序问题，两者物理含义不同
- 检测方法：用 `lv_color_hex(0x18181B)`（中性深灰）测试，如果偏色则说明字节序不对
- R↔B 通道交换公式 `(p>>11)|(p&0x07E0)|(p<<11)` 在字节序问题上无效，必须用字节交换 `(p>>8)|(p<<8)`

### FreeRTOS Timer 回调中禁止调用 LVGL API

### FreeRTOS Timer 回调中禁止调用 LVGL API

**现象**：`Tmr Svc` 任务栈溢出 (Stack Overflow)，导致反复重启。

**根因**：FreeRTOS 软件定时器回调运行在独立的 Timer Service 任务上下文中，栈空间很小（约 2KB）。在回调中直接调用 `lv_anim_start()`、`lv_obj_set_pos()` 等 LVGL API 会迅速耗尽栈空间。所有 LVGL 操作必须在调用 `lv_timer_handler()` 的主任务中执行。

**修复方式**：定时器回调只设置 `volatile bool` 标志位，主循环在 `expression_process_pending()` 中检测标志并执行实际的 LVGL 操作。

```cpp
// ❌ 错误：在 FreeRTOS timer 回调中调 LVGL
static void blink_cb(TimerHandle_t t) {
    _render_expression(current_expr);  // 栈溢出！
}

// ✅ 正确：只设标志
static void blink_cb(TimerHandle_t t) {
    pending_blink = true;
    pending_blink_state = false;
}
```

## 项目结构约定 (arduino-cli)
arduino-cli 要求 `.ino` 文件放在同名文件夹中：
```
xiaoBan-Mini/
├── cores3-sketch/                    ← CoreS3 项目（arduino-cli）
│   ├── cores3-sketch.ino             ← 主文件（与文件夹同名）
│   ├── lgfx_config.h                 ← 显示驱动配置
│   ├── companion_robot.h             ← 全局配置与类型
│   ├── theme.h / theme.cpp           ← 主题系统
│   ├── expressions.h / expressions.cpp ← 表情绘制
│   ├── state_machine.h / state_machine.cpp ← 状态机
│   └── ui.h / ui.cpp                 ← 状态栏/底部栏/菜单
├── esp32-v2.5/                        ← 旧版 ESP32+OLED（PlatformIO）
├── m5stack-demo-arduino/              ← 历史 demo
├── BUILD_ENV.md                       ← 本文件
├── pr.md                              ← PRD 产品需求
├── light.png                          ← 白天模式设计参考
├── heise.png                          ← 黑夜模式设计参考
└── ux.jpg                             ← UX 总览图
```
