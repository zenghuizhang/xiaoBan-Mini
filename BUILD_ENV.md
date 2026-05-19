# xiaoBan-Mini 陪伴机器人 - 开发环境记录

## 编译工具
- **arduino-cli** v1.4.1（不使用 PlatformIO）
- **ESP-IDF** v5.2.1 — 已安装在 `/home/zzh/esp-idf`

---

## 🚀 ESP-IDF 编译环境（cores3-espidf 项目）

### 环境初始化
每次打开新终端时执行：
```bash
source /home/zzh/esp-idf/export.sh
```

### 编译指令（M5Stack CoreS3）
```bash
cd xiaoBan-Mini/cores3-espidf

# 配置项目（首次或需要修改配置时）
idf.py menuconfig

# 编译
idf.py build

# 编译 + 烧录
idf.py flash

# 编译 + 烧录 + 监视器
idf.py flash monitor

# 仅监视器
idf.py monitor

# 清理构建
idf.py clean
```

### 烧录端口
- **Linux**: `/dev/ttyACM0`
- 端口自动检测，一般不需要手动指定

### 已集成组件（ESP-IDF Component Registry）
- **lvgl** v9.x — UI 图形库
- **m5unified** v0.2.x — M5Stack 硬件驱动

---

## Arduino 编译环境（cores3-sketch 项目）

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
