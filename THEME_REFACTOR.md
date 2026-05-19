# 主题系统重构说明

## 概述

本次重构将 xiaoBan-Mini V2.5 的主题系统从"仅自动切换"升级为"自动 + 手动双模式"，并实现了完整的昼夜双主题视觉效果，严格遵循 PRD v2.4 设计规范和 light.png / heise.png 设计稿。

---

## 设计规范

### 白天模式 (light.png)
| 属性 | 值 |
|------|-----|
| **背景** | 浅色（填充白色，模拟柔和青色 `#F0FDFA`） |
| **表情** | 深色（黑色像素，模拟深青色 `#164E63`） |
| **发光效果** | 无 |
| **状态栏** | 深色文字 |

### 黑夜模式 (heise.png)
| 属性 | 值 |
|------|-----|
| **背景** | 纯黑（不填充） |
| **表情** | 亮色（白色像素，模拟青色 `#22D3EE`） |
| **发光效果** | 有（多层椭圆模拟光晕扩散） |
| **状态栏** | 白色文字 |

---

## 修改文件清单

### 1. `config.h`
- 新增 `PIN_BTN_THEME` 主题切换按钮引脚
- 新增 `TIMER_THEME_DEBOUNCE` 防抖时间
- 新增 `ANIM_THEME_TRANSITION_MS` 主题切换过渡动画时长
- 新增 `EEPROM_THEME_ADDR` / `EEPROM_MAGIC_ADDR` 主题持久化配置
- 新增 `DEBUG_THEME` 主题调试开关

### 2. `types.h`
- 新增 `ThemeColors` 结构体（bg / fg / fg_glow / status_text）
- 新增 `THEME_LIGHT_COLORS` 和 `THEME_DARK_COLORS` 常量配置

### 3. `eyes.h`
- 新增 `eyes_set_theme()` / `eyes_get_theme()` 主题设置/查询
- 新增 `eyes_apply_theme()` 应用主题（重绘当前表情）
- 新增 `eyes_transition_theme()` 主题切换过渡动画

### 4. `eyes.cpp`（核心重构）
- **主题感知绘制函数**：
  - `draw_themed_filled_eye()` — 填充眼睛（发光/普通）
  - `draw_themed_happy_eye()` — 弯月眼
  - `draw_themed_sleepy_eye()` — 半闭眼
  - `draw_themed_mouth_line/arc/circle()` — 嘴巴系列
  - `draw_themed_smirk()` — 歪嘴
  - `draw_themed_dizzy_mouth()` — 漩涡嘴巴
  - `draw_themed_status_bar()` — 状态栏

- **发光效果模拟**：
  ```cpp
  void draw_glow_ellipse(int cx, int cy, int rx, int ry) {
    // 外层光晕（最淡，最大）
    u8g2.drawEllipse(cx, cy, rx + 4, ry + 4);
    // 中层光晕
    u8g2.drawEllipse(cx, cy, rx + 2, ry + 2);
    // 核心（实心）
    u8g2.drawFilledEllipse(cx, cy, rx, ry);
  }
  ```

- **白天模式背景处理**：
  ```cpp
  void eyes_clear() {
    u8g2.clearBuffer();
    if (tc->bg == 1) {
      u8g2.setDrawColor(1);
      u8g2.drawBox(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);  // 填充白色
    }
  }
  ```

- **主题切换过渡动画**：快速闪烁 → 新主题渲染

### 5. `state_machine.h`（新建）
- 状态机接口定义
- 主题管理接口定义

### 6. `state_machine.cpp`
- 新增 `auto_theme_enabled` 自动切换开关
- 新增 `last_theme_toggle_time` 防抖计时
- `state_init()` 增加 EEPROM 主题加载
- `state_update()` 增加自动主题切换条件判断
- 新增主题管理函数：
  - `theme_toggle()` — 手动切换（带防抖 + 持久化）
  - `theme_set()` — 直接设置（带持久化）
  - `theme_get()` — 获取当前主题
  - `theme_set_auto()` — 开关自动切换
  - `theme_is_auto()` — 查询自动切换状态

### 7. `xiaoBan-Mini-v2.5.ino`
- 新增 `#include <EEPROM.h>`
- 新增主题按钮状态变量
- `setup()` 增加按钮引脚初始化
- `loop()` 增加按钮检测和主题切换触发

---

## 使用方式

### 自动切换（默认开启）
光敏传感器检测环境光线：
- `light > 600` → 白天模式
- `light < 200` → 黑夜模式
- 200-600 之间保持当前状态（滞回设计）

### 手动切换
1. **按钮切换**：按下连接到 `PIN_BTN_THEME` 的按钮
2. **代码切换**：
   ```cpp
   theme_toggle();              // 切换到另一个主题
   theme_set(THEME_LIGHT);      // 设置为白天
   theme_set(THEME_DARK);       // 设置为黑夜
   ```

### 关闭自动切换
```cpp
theme_set_auto(false);  // 关闭自动切换，仅手动
theme_set_auto(true);   // 重新开启自动切换
```

### 主题持久化
主题选择自动保存到 EEPROM，断电重启后恢复上次选择。首次使用默认黑夜模式。

---

## 硬件接线

| 引脚 | 功能 | 说明 |
|------|------|------|
| D7 | 主题切换按钮 | 接按钮到 GND，内部上拉 |

如果不想接按钮，也可以通过触摸长按或其他传感器触发 `theme_toggle()`。

---

## 视觉效果对比

### 黑夜模式（heise.png 风格）
```
┌──────────────────────────┐
│ 已连接    14:30    85%  │  ← 白色文字
│                          │
│     ◉         ◉         │  ← 青色发光眼睛（多层光晕）
│                          │
│        ─────             │  ← 青色发光嘴巴
│                          │
└──────────────────────────┘
  纯黑背景 + 发光表情
```

### 白天模式（light.png 风格）
```
┌──────────────────────────┐
│ 已连接    14:30    85%  │  ← 深色文字
│                          │
│     ●         ●         │  ← 深色眼睛（无发光）
│                          │
│        ─────             │  ← 深色嘴巴
│                          │
└──────────────────────────┘
  浅色背景 + 深色表情
```

---

## 测试要点

1. **主题切换**：按钮按下后背景和表情颜色平滑切换，无闪烁
2. **发光效果**：黑夜模式下眼睛和嘴巴有明显的光晕扩散效果
3. **白天模式**：浅色背景清晰，表情深色可辨识
4. **持久化**：切换主题后重启，恢复到切换后的主题
5. **自动切换**：遮挡光敏传感器，自动切换到黑夜模式
6. **表情轮播**：切换主题后随机表情轮播正常，颜色跟随主题
7. **状态栏**：连接状态、时间、电量文字颜色跟随主题

---

## 注意事项

1. OLED 为单色屏，无法真正显示青色，用白色像素模拟
2. 发光效果通过多层绘制模拟，会略微增加渲染时间（约 2-3ms）
3. EEPROM 有写入寿命限制（约 10 万次），主题切换已做防抖处理
4. 如果使用自动模式，光线在阈值附近波动时不会频繁切换（滞回设计）
