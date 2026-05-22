<div align="center">

# 🤖 小伴 Mini V6.0

**桌面萌宠 · 情绪搭子 · 你的专属陪伴机器人**

[![ESP-IDF](https://img.shields.io/badge/ESP--IDF-v5.5.4-blue)](https://docs.espressif.com/projects/esp-idf/)
[![LVGL](https://img.shields.io/badge/LVGL-v9.5.0-green)](https://lvgl.io/)
[![License](https://img.shields.io/badge/License-MIT-yellow)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-M5Stack%20CoreS3-orange)](https://docs.m5stack.com/en/core/CoreS3)

![小伴机器人预览](docs-v2/assets/preview.png)

</div>

---

## ✨ 项目简介

小伴 Mini 是一个坐在桌面上的「情绪搭子」——它不是助手，不需要你命令它；它只是一个安静的小陪伴，会用小动作给你情绪反馈。

**视觉特色：** 标志性的草青色(#22D3EE)发光眼睛，科技感与萌感并存

---

## 🎯 核心特性

### 🎨 表情系统
- **20+ 动态表情**：开心、难过、愤怒、惊讶、困倦、眩晕、可爱、发呆、好奇、害羞、生气等
- **表情随机轮播**：自动切换表情，模拟真实情绪变化
- **流畅动画过渡**：LVGL 硬件加速，60fps 丝滑体验
- **微动作系统**：呼吸、眨眼、眼球转动等细节动画

### 🎭 双主题系统
- **深色主题**：科技感十足，适合夜间使用
- **浅色主题**：清新明亮，适合白天使用
- **一键切换**：按钮快速切换主题

### 🎮 交互方式
| 操作 | 反应 |
|------|------|
| 🅰️ 按钮 A | 打开径向菜单 |
| 🅱️ 按钮 B | 开心表情 |
| 🆑 按钮 C | 快捷功能 |
| 👆 单击屏幕 | 随机表情 |
| 👆 双击屏幕 | 可爱表情 |
| 📐 IMU 体感 | 摇晃有惊喜 |

### 💬 对话框气泡
- 支持中英文显示
- 内置 14px / 22px 两种中文字体
- 圆润气泡样式，自动适配内容长度

### 🎛️ 径向菜单系统
- 科技风环形菜单
- 6 个快捷功能入口
- WiFi 配网、设置、主题切换等

### 📶 WiFi 智能配网
- 内置 WiFi SmartConfig
- 二维码配网，手机一扫即连
- 支持网络状态实时显示

---

## 🛠️ 硬件要求

| 组件 | 规格 |
|------|------|
| **主控** | M5Stack CoreS3 (ESP32-S3) |
| **屏幕** | 2.0" IPS 320×240, ST7789V |
| **Flash** | 16MB QSPI Flash |
| **PSRAM** | 8MB Octal PSRAM |
| **电池** | 内置 500mAh 锂电池 |

> 💡 **提示**：本项目专为 M5Stack CoreS3 设计，其他 ESP32 开发板需要调整引脚配置。

---

## 🚀 快速开始

### 1. 环境搭建
```bash
# 安装 ESP-IDF v5.5.4
# 参考：https://docs.espressif.com/projects/esp-idf/zh_CN/v5.5.4/

# 克隆项目
git clone https://github.com/zenghuizhang/xiaoBan-Mini.git
cd xiaoBan-Mini/cores3-espidf
```

### 2. 编译与烧录
```bash
# 设置目标芯片
idf.py set-target esp32s3

# 编译
idf.py build
```

> 💡 **注意**：V6.0 固件约 1.2MB，需使用 `SINGLE_APP_LARGE` 分区表。

# 烧录（替换 /dev/ttyACM0 为你的串口）
idf.py -p /dev/ttyACM0 flash monitor
```

### 3. 运行
烧录完成后，设备自动重启，你将看到：
- 开机启动动画
- 草青色的眼睛亮起
- 表情开始随机轮播

---

## 📁 目录结构

```
xiaoBan-Mini/
├── cores3-espidf/           # ESP-IDF 固件源码
│   ├── main/
│   │   ├── app_main.cpp     # 主程序入口
│   │   ├── ui/
│   │   │   ├── expressions.cpp/h   # 表情系统
│   │   │   ├── dialog_bubble.c/h   # 对话框气泡
│   │   │   ├── font_zh_14.c/h      # 14号中文字体
│   │   │   ├── font_zh_22.c/h      # 22号中文字体
│   │   │   ├── tech_ui.c/h         # 科技风UI
│   │   │   ├── motion_controller.cpp/h  # 运动控制
│   │   │   ├── scenario_overlay.c/h     # 场景叠加层
│   │   │   └── icons/              # 图标资源
│   └── CMakeLists.txt
├── v6.0/                      # v6.0 版本设计文档
│   ├── codebase/             # UI 原型代码（TypeScript）
│   └── notebook/             # PRD 设计文档
├── docs-v2/                  # v2.0 通用文档
├── tools/                    # 开发工具脚本
└── README.md                 # 你正在看的文件
```

---

## 🎬 功能演示

| 功能 | 预览 |
|------|------|
| 表情轮播 | ![表情轮播](docs-v2/assets/expressions.gif) |
| 主题切换 | ![主题切换](docs-v2/assets/theme.gif) |
| 对话框 | ![对话框](docs-v2/assets/dialog.gif) |

> 📸 **截图征集**：欢迎提交你的小伴机器人照片！

---

## 📊 版本迭代

| 版本 | 时间 | 核心变更 |
|------|------|----------|
| V6.1 | 2026.05 | 升级 ESP-IDF 5.5.4，表情系统扩展到 20+ 种 |
| V6.0 | 2026.05 | ESP-IDF + LVGL9 重构，双主题系统 |
| V5.0 | 2026.03 | Arduino + LVGL8 实现，基础表情 |
| V2.0 | 2026.03 | 产品定位明确：情绪陪伴机器人 |
| V1.0 | 2025.12 | 初代原型，4舵机语音助手方案 |

---

## 🤝 参与贡献

欢迎各种形式的贡献！

- 🐛 提交 Issue 反馈 Bug
- 💡 提出新功能建议
- 📝 完善文档和注释
- 🔧 提交 PR 改进代码

---

## 📄 许可证

本项目采用 **MIT License** 开源 - 详见 [LICENSE](LICENSE) 文件。

---

<div align="center">

**用科技，温暖每一个孤独的时刻** 💚

*Made with ❤️ by 小伴团队*

</div>
