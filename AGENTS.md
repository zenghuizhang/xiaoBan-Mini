# AGENTS.md — xiaoBan-Mini 陪伴机器人

> v5.0 | 2026-05-20 | M5Stack CoreS3 (ESP32-S3, 320×240)

## 环境初始化
```bash
# ESP-IDF v5.5.4
IDF_PATH=/home/zzh/esp-idf
IDF_PYTHON=/home/zzh/.espressif/python_env/idf5.5_py3.10_env/bin/python
alias idf.py="$IDF_PYTHON $IDF_PATH/tools/idf.py"
cd /home/zzh/.openclaw-autumn/workspace/xiaoBan-Mini/cores3-espidf
```

## 编译与烧录
```bash
idf.py build                            # 编译
idf.py -p /dev/ttyACM0 flash            # 烧录
```

## 截图系统
```bash
# 被动等待 (设备启动 ~8s 后自动截图)
python3 tools/capture.py /dev/ttyACM0 /tmp/screen.png --wait

# 主动触发 (发送 's' 触发截图)
python3 tools/capture.py /dev/ttyACM0 /tmp/screen.png

# 控制命令: 's'=截图  'd'=禁用触发器  'e'=启用触发器 (均持久化到 NVS)
```
原理: `screenshot.c` → `lv_snapshot_take_to_buf` (需 `CONFIG_LV_USE_SNAPSHOT=y`) → PSRAM buffer → FreeRTOS task 做 base64 → ESP_LOGI("SNAP", "SS:...") → PC capture.py 解码 RGB565 → PNG

## v5.0 交互流程
```
开机 → BootAnim(3.5s, 3主题) → idle(呼吸+瞳孔微动)
  ↓ 双击
RadialMenu (6按钮圆形)
  ├ 表情  → random expression
  ├ 对话  → TALKING
  ├ 设置  → MenuOverlay (WiFi/亮度/音量)
  ├ 主题  → Tech↔Child↔Dev
  ├ 扩展  → BottomBar(16按钮)
  └ 随机  → random expression
  ↓ IMU
倾斜/摇晃/轻敲 → curious/yawn/dizzy/wink
  ↓ 时间
6-10点首次 → Morning 气泡 "Good morning!"
```

## 源代码结构
```
cores3-espidf/main/
├── app_main.cpp          # 主入口, UI层, 交互逻辑
├── CMakeLists.txt
└── ui/
    ├── theme_v3.h/cpp    # 3主题: Tech(cyan)/Child(coral #FF7F50)/Dev(green #22C55E), scale=1.6
    ├── expressions.h/cpp # 20表情 + 呼吸动画 + 瞳孔微动
    ├── boot_anim.h/c     # 开机动画 (3.5s)
    ├── radial_menu.h/c   # 径向菜单 (6按钮, 双击触发)
    ├── dialog_bubble.h/c # 对话气泡
    ├── audio_feedback.h/cpp  # 音效反馈
    ├── motion_controller.h/cpp  # IMU体感
    ├── robot_memory.h/c  # NVS记忆
    ├── wifi_config.h/cpp # WiFi配网 (AP+DNS劫持+HTTP)
    ├── qrcode.h/c        # QR码 (DRAW_POST绘制)
    ├── screenshot.h/c    # 截图回传 (异步base64)
    └── font_zh_14.h/c    # 中文 Noto Sans SC (2bpp)

v5.0/                       # React 原型 (设计参考)
```

## 关键配置
- **LVGL 9.5**, `LV_COLOR_DEPTH=16`, `LV_USE_SNAPSHOT=1`
- **PSRAM 8MB** — 用于截图 buffer
- **Color fix**: flush 回调做 byte swap `(p>>8)|(p<<8)` — GC9A01 MSB-first vs ESP32 小端序
- **Face绘制**: `lv_draw_rect` 填充 + `LV_EVENT_DRAW_POST` on `face_container` (320×240, z-order 低于 BottomBar)
- **BottomBar**: `lv_obj` + `LV_OBJ_FLAG_CLICKABLE` (不用 `lv_btn`, 避免主题干扰)
- **屏不可滚动**: `lv_obj_remove_flag(lv_screen_active(), LV_OBJ_FLAG_SCROLLABLE)`

## 注意事项
- **串口输入** 用 `fgetc(stdin)` + `O_NONBLOCK` — 安全非阻塞，不要用 `usb_serial_jtag_read_bytes`
- **禁止** FreeRTOS timer 回调中调用 LVGL API — 栈溢出
- **禁止** `lv_btn_create` (默认主题会覆盖样式)
- ESP32-S3 为小端序, `malloc` 默认不用 PSRAM; 截图用 `heap_caps_malloc(MALLOC_CAP_SPIRAM)`
- Montserrat 字体不支持中文; 颜文字标签: `** = ? :) ^_^ ;) >_< @_@ T_T`

## 设计参考
- `/home/zzh/.openclaw-autumn/workspace/xiaoBan-Mini/桌面机器人UI_完整设计文档_v5.0.md`
- `/home/zzh/.openclaw-autumn/workspace/xiaoBan-Mini/v5.0/codebase/src/`
- 关键文件: `Face.tsx`, `RobotUI.tsx`, `RadialMenu.tsx`, `BootAnimation.tsx`, `BottomBar.tsx`
