<div align="center">

# 🤖 小伴 Mini V7.6

**桌面萌宠 · 情绪搭子 · 你的专属陪伴机器人**

[![ESP-IDF](https://img.shields.io/badge/ESP--IDF-v5.2.1-blue)](https://docs.espressif.com/projects/esp-idf/)
[![LVGL](https://img.shields.io/badge/LVGL-v9.5.0-green)](https://lvgl.io/)
[![License](https://img.shields.io/badge/License-MIT-yellow)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-M5Stack%20CoreS3-orange)](https://docs.m5stack.com/en/core/CoreS3)
[![GitHub Stars](https://img.shields.io/github/stars/zenghuizhang/xiaoBan-Mini?style=social)](https://github.com/zenghuizhang/xiaoBan-Mini)

![小伴机器人预览](docs-v2/assets/preview.png)

---

### ✨ 「它不是工具，是坐在你桌面上的小伙伴」

</div>

---

## 🌟 最新亮点 V7.6

> 🎉 **2026年5月重大更新** - 更稳定、更可爱、更懂你

| 新特性 | 描述 |
|--------|------|
| 🖼️ **截图功能** | 一键保存屏幕画面，分享你的小伴瞬间 |
| 🎨 **主题预览** | 内置 HTML 主题预览器，可视化调试 |
| ⚡ **代码重构** | 移除旧版遗留代码，更轻量更稳定 |
| 🔧 **设置页面优化** | 更直观的配置界面 |
| 📶 **WiFi 配网升级** | 更稳定的连接体验 |

---

## 🎯 核心特性

### 🎨 表情系统
- **8+ 精心设计的动态表情**：开心、难过、眩晕、可爱、发呆、好奇、害羞、生气
- **表情随机轮播**：自动切换表情，模拟真实情绪变化
- **流畅动画过渡**：LVGL 硬件加速，60fps 丝滑体验
- **草青色发光眼睛**：标志性 #22D3EE 配色，科技感与萌感并存

### 🎭 双主题系统
- **🌙 深色主题**：科技感十足，适合夜间使用
- **☀️ 浅色主题**：清新明亮，适合白天使用
- **🔄 一键切换**：按钮快速切换，无需重启

### 🎮 交互方式
| 操作 | 反应 |
|------|------|
| 🅰️ 按钮 A | 切换主题 |
| 🅱️ 按钮 B | 开心表情 |
| 🆑 按钮 C | 眩晕表情 |
| 👆 双击屏幕 | 可爱表情 |

### 📶 WiFi 智能配网
- 内置 WiFi SmartConfig
- 二维码配网，手机一扫即连
- 网络状态实时显示

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
# 安装 ESP-IDF v5.2.1
# 参考：https://docs.espressif.com/projects/esp-idf/zh_CN/v5.2.1/

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

# 烧录（替换 /dev/ttyACM0 为你的串口）
idf.py -p /dev/ttyACM0 flash monitor
```

> 💡 **注意**：固件约 1.2MB，使用 `SINGLE_APP_LARGE` 分区表。

### 3. 运行
烧录完成后，设备自动重启，你将看到：
- 开机启动动画 ✨
- 草青色的眼睛亮起 👀
- 表情开始随机轮播 🎭

---

## 📁 目录结构

```
xiaoBan-Mini/
├── cores3-espidf/           # ESP-IDF 固件源码
│   ├── main/
│   │   ├── app_main.cpp     # 主程序入口
│   │   └── ui/
│   │       ├── page_settings.cpp/h   # 设置页面
│   │       ├── screenshot.c/h        # 截图功能
│   │       ├── wifi_config.cpp/h     # WiFi 配网
│   │       ├── expressions.cpp/h     # 表情系统
│   │       ├── tech_ui.c/h           # 科技风UI
│   │       └── icons/                # 图标资源
│   └── CMakeLists.txt
├── theme-preview/            # 主题预览 HTML 工具
├── docs-v2/                  # 文档与资源
├── tools/                    # 开发工具脚本
└── README.md                 # 你正在看的文件
```

---

## 📊 版本迭代

| 版本 | 时间 | 核心变更 |
|------|------|----------|
| **V7.6** | **2026.05** | ✨ 截图功能 + 主题预览工具 + 代码重构优化 |
| V7.0 | 2026.05 | 双主题系统 + 8种表情 + WiFi配网 |
| V6.0 | 2026.05 | ESP-IDF + LVGL9 完整重构 |
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

> **🌟 给个 Star 就是最大的支持！**

---

## 📄 许可证

本项目采用 **MIT License** 开源 - 详见 [LICENSE](LICENSE) 文件。

---

<div align="center">

### 💚 用科技，温暖每一个孤独的时刻

*Made with ❤️ by 小伴团队*

[![GitHub](https://img.shields.io/badge/GitHub-zenghuizhang%2FxiaoBan--Mini-181717?logo=github)](https://github.com/zenghuizhang/xiaoBan-Mini)

</div>
