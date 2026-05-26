# UI AI 设计简报 v7.6(对齐 React 原型「陪伴机器人 UI 原型 v7.6」)

> 直接复制给 UI 设计 AI / 工程实现 AI。本简报已自包含:画布尺寸、token、信息架构、组件清单、14 张 mock 任务、不变项白名单。
> **工程基线**:M5Stack CoreS3(320×240,圆角矩形),LVGL 9.5.0 + esp_lvgl_port 2.4+,ESP-Claw `claw_core@v1.0`。
> 本版与 v6.3.0 系列的差异已在 §0.1 列出,**以本文为准**。

---

## 0. 一句话项目背景

桌面陪伴机器人,屏 320×240,中心是 **face_engine**(双眼 + 嘴的几何动画,~24 个表情 variant),通过 ESP-Claw 接云端 LLM。v7.6 把信息架构从 v6.3 的"6 扇区都是终端语义"小调:**Skills 从扇区降级为 Settings 子项**,腾出 4 点位给 **Theme(主题切换)**,主题数仍是 4 但配色全换。

### 0.1 v7.6 相对 v6.3.0 的关键变更

| 模块 | v6.3.0 | **v7.6** |
|---|---|---|
| 4 套主题 | Tech / Warm / Child / Dev | **Tech / Lavender / Child / Cocoa** |
| Menu 4 点扇区 | Skills(Package) | **Theme(Palette)** |
| Skills 入口 | Menu 扇区 | **Settings → 拓展功能 → 技能插件** |
| Settings 语言 | 英文 | **中文**(通用 / 显示与声音 / 拓展功能 / 系统 / 开发者选项) |
| Developer 解锁 | 5-tap About | **默认可见**(组标题旁徽章「测试」) |
| Console.System 自测 | 3 英文按钮 Shake/Flip/Cover | **6 中文按钮**:前倾/后仰/左倾/右倾/摇晃/旋转 |
| Console.Service 顶部卡 | 3 张 Event Inject/Audio Loop/OTA Force | **无**(整页直接 6 剧本按钮) |
| Console.Service 6 剧本 | 英文 emoji | **中文 emoji**:🎤 语音唤醒 / 🤖 OTA模拟 / 📵 报错状态 / 📱 语音通话 / 🔋 低电量 / 🌡 过热 |
| Boot 动画 | 未对齐 | **统一动画**(无主题分支):眼睛揉醒 → 嘴打哈欠 → 全睁 |
| StatusBar 模式 | idle / transient / critical | **+ always**(所有子页常驻) |
| DialogBubble | 未对齐 | text / suggest 两种,顶部 85% 宽圆角卡 |
| 底部 RGB 灯带 | 概念 | **1.5px 高**,主题色脉动,场景态(error/ota/voice_wake/angry...)覆盖色相 |

---

## 1. 画布与基础规范(全部 mock 必须遵守)

| 项 | 值 |
|---|---|
| 物理分辨率 | 320×240 px |
| 设计稿渲染 | **640×480 px**(2× 出图) |
| 屏幕形态 | 圆角矩形(视觉描边代表设备框) |
| 色彩模式 | RGB 565 兼容,出图 RGB 8888 |
| 安全区 | 顶部 22 px 状态栏,左右各 0 |
| 字体 | DejaVu Sans / 思源黑体 CN;标题 Bold,正文 Regular |
| 字号(物理像素) | 16 标题 / 14 正文 / 12 副 / 11 图标标签 / 10 minicaption |
| 圆角 | 卡 4 / 按钮 4 / 头像 全圆 |
| 图标 | **lucide** viewBox 24,strokeWidth **2.4**,linecap round |

### 1.1 主题色 Token(4 套:Tech 默认 / Lavender / Child / Cocoa)

> v7.6 的 4 套主题完全替换 v6.3 的 Warm/Dev,工程 `theme_tokens.h` 须重新生成。

| Token | **Tech(默认)** | **Lavender** | **Child** | **Cocoa** |
|---|---|---|---|---|
| `bg`         | `#000000` | `#FAF5FF` | `#FFF9E6` | `#2D1B0E` |
| `panel`      | `#0a1e28` | `#FFFFFF` | `#FFFFFF` | `#3F2B20` |
| `accent`     | **`#22D3EE`** | **`#9333EA`** | **`#FF7F50`** | **`#FB7185`** |
| `accent_hi`  | `#33C5FF` | `#A855F7` | `#FFAA78` | `#FDA4AF` |
| `text`       | `#b4ebff` | `#4C1D95` | `#c85a28` | `#FEF3C7` |
| `border`     | `#14506e` | `#E9D5FF` | `#FFC8A0` | `#573D2C` |
| `danger`     | `#F43F5E` | `#EF4444` | `#F43F5E` | `#EF4444` |

**4 套主题语义定位**:

| 主题 | 中文名 | 定位 |
|---|---|---|
| **Tech**     | 科技青 | 默认,主品牌色 |
| **Lavender** | 柔紫   | 亮色调,温柔商务 |
| **Child**    | 温暖儿童 | 浅米 + 珊瑚橙,圆润亲和 |
| **Cocoa**    | 草莓可可 | 深可可 + 草莓粉,温暖高级 |

**Tech 是暗色,Cocoa 是暗色;Lavender 和 Child 是亮色** — 出 mock 时面板半透明规则需要区分:
- **暗色主题(Tech/Cocoa)**:卡片底色 `panel @ 30%`(`${panel}4D`),保留底层 face 隐约可见
- **亮色主题(Lavender/Child)**:卡片底色 **直接用 `panel`**(纯白),避免透出 bg 影响可读

### 1.2 通用组件外观

| 组件 | 规格 |
|---|---|
| 卡片 | bg=panel(亮)/ panel@30%(暗),border=`border` 1px,radius=4,内 6px |
| 按钮(主) | bg=`accent`,text=`bg`,radius=4,h=28,内 12px |
| 按钮(次/outline) | 透明 + border=`accent`,text=`accent` |
| 列表项 | h=36 单行 / 56 双行,左 20×20 图标 + 中文本 + 右 chevron-right(16) |
| 状态栏 | h=22,右对齐 4 槽:plug → ai_dot → wifi → battery |
| 子页顶部栏 | h=28,左 chevron-left(20×20,`accent`),中标题 |

---

## 2. 信息架构与导航

```
[Boot 3.5s]
   └→ [Idle / face 主屏]
        │ double-click → [Radial Menu]
        │
        └→ [Menu] 6 扇区:
            ├ 12 点 ★ 对话    → talking state
            ├ 2  点    模型    → ModelPicker
            ├ 4  点    主题    → ThemePicker
            ├ 6  点    设置    → SettingsOverlay
            ├ 8  点    人格    → PersonaGrid
            └ 10 点    记忆    → MemoryBrowser → Purge modal

[Settings] 5 组(中文):
   通用 / 显示与声音 / 拓展功能 / 系统 / 开发者选项「测试」
   ├ 拓展功能 → Wi-Fi网络(→ wifi_ap → connecting → success/error)
   │           技能插件(Skills 空页)
   └ 开发者选项 → 控制台
                 详细日志(toggle)

[Console] 4 tab(中文):
   系统状态 / AI引擎 / 显示调试 / 服务模拟
   ├ 系统状态 : 传感器状态卡 + 6 体感按钮(前倾/后仰/左倾/右倾/摇晃/旋转)
   ├ AI引擎   : 2×2 卡(运行日志/模型路由/记忆管理/技能调试)
   ├ 显示调试 : 2×2 卡(主题配置/静态表情/动态表情/表情调试)
   └ 服务模拟 : 6 剧本按钮 3×2(🎤 语音唤醒 · 🤖 OTA模拟 · 📵 报错状态
                              📱 语音通话 · 🔋 低电量 · 🌡 过热)
```

---

## 3. ★ 三态(+ always)状态栏

| 模式 | 触发 | 视觉 |
|---|---|---|
| **idle**     | home 静止 | 完全隐藏(opa=0) |
| **transient**| 单击 face / xb_statusbar_peek() | 180ms 淡入 → 2000ms 保持 → 220ms 淡出 |
| **critical** | 电量 ≤ 15% / Wi-Fi 断开 / OTA 进行中 | 常驻 + 危险色突出 |
| **always**   | 所有子页(Settings/Console/Model/Persona/Memory/Theme/Skills/Wifi) | 常驻 |

> 4 槽右起:`battery(可红)` → `wifi(可斜杠)` → `ai_dot` → `plug(充电时)`。

---

## 4. 标志性视觉

- **face_engine** = 两眼 + 嘴 3 个圆角矩形 div,**颜色 = `accent_hi`**(不是 `accent`),`box-shadow: 0 0 20px ${accent_hi}B3`
- **眨眼**:idle/breath/talking/look_*/curious/thinking/alert/lost/angry 等会随机 3–5s 触发 150ms 闭眼;happy/wink/celebrate/excited/dizzy/crying/naughty/sleep_wake/yawn/surprised/deep_sleep 不眨
- **径向菜单**:6 个 56×56 圆形按钮,半径 78,角度 −90/−30/30/90/150/210°,中心 ✕ 40×40
- **底部 RGB 灯带**:高 1.5px,默认 `accent_hi` 脉动 4s;场景态自定义颜色(见 §5)
- **不要**拟物、不要 3D、不要渐变光泽——扁平 + glow

---

## 5. 场景态 → 表情 / 灯带映射(已在 `RobotUI.tsx` 写死)

| state(URL) | face | bubble | RGB strip |
|---|---|---|---|
| `idle` / `breath` / `happy` / `talking` / `dizzy` ... | 同名 variant | 无 | `accent_hi` 4s 脉动 |
| `morning`   | `sleep_wake` | "早上好呀!今天也是充满能量的一天。" | 默认 |
| `lonely_3`  | `lost`       | "好无聊哦,陪我玩一会吧..." | **`#FACC15`** 4s 慢脉动 |
| `reward`    | `celebrate`  | 无 | `accent_hi` 0.5s 快脉动 |
| `angry`     | `angry`      | 无 | **`danger`** 1.2s 中速 |
| `ota`       | `deep_sleep` | "OTA 系统更新中..." | **`#A855F7`** 1.5s 紫 |
| `error`     | `dizzy`      | "系统发生异常错误!" | **`danger`** 0.5s 急促 |
| `voice_wake`| `excited`    | "我在听..." | `accent_hi` 0.5s 高强度 |
| `suggest`   | `idle`       | "要不要试试调皮表情?" + ✓/✗ | 默认 |

---

## 6. 14 张 Mock 任务清单(命名 `pNN_<page>_<state>.png`,2× 渲染 640×480)

> **默认主题 = Tech**;只有 #M11 一张需要单独出 4 主题对比页。

### #M01 `p01_boot.png`
Boot 揉眼起床动画的"中段"定格 — 两眼半睁(高 16),嘴张开打哈欠(28×36 圆),无状态栏,bg = `tech.bg`。

### #M02 `p02_home_idle_clean.png`
全屏 idle face(两眼 32×64 + 嘴 24×4),**无状态栏**,bg = `tech.bg`,底部 1.5px 灯带 `accent_hi` 脉动中(画 60% opacity)。

### #M03 `p02_home_peek.png`
同 #M02,**顶部 22px 状态栏完全可见**:🔌 充电图标 + AI ●(青) + WiFi + 88% 电池图标。

### #M04 `p02_home_critical_lowbat.png`
同 #M02 + 状态栏常驻 + 电量 12% 红色 + `BatteryWarning` 图标;face 改成 `lost` variant(双眼下垂、嘴下弯小弧)。

### #M05 `p03_menu.png`
Radial Menu **6 扇区**(中心 ✕ 40×40,按钮 56×56,半径 78):

| 角度 | 扇区 | lucide | label |
|---|---|---|---|
| −90° | 12 点 | `MessageSquare` | 对话 |
| −30° | 2 点  | `Sparkles`      | 模型 |
| 30°  | 4 点  | `Palette`       | 主题 |
| 90°  | 6 点  | `Settings`      | 设置 |
| 150° | 8 点  | `UserSquare`    | 人格 |
| 210° | 10 点 | `Brain`         | 记忆 |

bg 覆盖 `${tech.bg}CC`(80% 黑) + backdrop-blur。单出一张「12 点 对话 hover」:该按钮 scale 1.1、border 加亮、外发光 20px,中心区域顶部浮出 label 文字"对话"`text-shadow: 0 0 8px ${accent}CC`。

### #M06 `p06_model_picker.png`
顶栏 `← 大模型选择`(h=28)。下方 6 行,行高 36,左侧 12×12 选中圆点(实心 = `accent`,空心 = border):
- ⓘ Auto (智能路由) — **选中,右侧 ✓**
- 💎 GPT-4o          副 "云端 · 专业版"
- 🎭 Claude 3.5 Sonnet 副 "云端 · 专业版"
- 🚀 Doubao Pro      副 "云端 · 标准版"
- 🦾 Qwen-2.5 1.5B   副 "本地 · 极速版"
- 📱 Phi-3 mini      副 "本地 · 极速版"

底栏 h=28 居中小字:"切换会立即生效,正在进行的对话不打断"

### #M07 `p07_persona_grid.png`
顶栏 `← 人格配置`。3×2 卡片,**96×84,gap=8**,居中:
- 🎵 **Lyra** 「温柔诗人」(**选中:`accent` 描边 2px,name 用 accent 色**)
- 🪞 Echo 「话痨复读机」
- 🌟 Nova 「极客科普」
- 🦉 Sage 「冷静顾问」
- 🐣 Pico 「童趣小鸡」
- 🩺 Doc  「严谨医师」

### #M08 `p08_memory_browser.png`
顶栏 `← 长程记忆`,右侧小按钮"清空"(border + panel)。6 行 h=56:
| 图标 | 文本 | 时间 |
|---|---|---|
| `MessageSquare` | chat · 「用户喜欢喝美式咖啡」 | 2 小时前 |
| `Settings`      | settings · 「主题=tech」      | 3 天前 |
| `Briefcase`     | skills · 「pomodoro 已安装」  | 1 周前 |
| `Activity`      | usage · 「用户偏好早晨活跃」  | 2 周前 |
| `Globe`         | net · 「家庭 Wi-Fi 自动连接」 | 1 个月前 |
| `MessageSquare` | chat · 「不爱吃香菜」         | 1 个月前 |

每行右侧 16×16 `Trash2` 图标(`danger` 色)。

### #M09 `p08_memory_purge_modal.png`
同 #M08 + 底部弹出 modal(底贴 panel,顶圆角 8,padding 16):
- 标题 16sp Bold:**清空所有长程记忆?**
- 副 12sp:这只清除记忆里的对话偏好/学习数据,不影响设置和 Wi-Fi 状态
- 按钮 2 列 gap-2,均 h=28:
  - **取消** outline(text/border = `accent`)
  - **全部清空** 实心 `danger` bg + 白字

### #M10 `p09_settings.png`(单张,5 组,Developer 默认可见)
顶栏标题居中 12sp Bold "系统设置",左上返回按钮(32×32 圆,内 chevron-left 16)。**纯黑半透明 bg `${bg}E6` + backdrop-blur**。5 组:

| 组 | 项 |
|---|---|
| 通用 | 账号绑定 / 语言设置 / 睡眠定时(右侧值 + chevron) |
| 显示与声音 | 亮度 slider(`Sun` 图标) / 音量 slider(`Volume2`) |
| 拓展功能 | Wi-Fi 网络(值 "未连接") / 技能插件(值 "3 个已安装") |
| 系统 | 系统更新(`isAlert`,红色 + "有新版本") / 清除偏好数据(红) / 关于系统(值 "v7.2") |
| 开发者选项「测试」 | 控制台 / 详细日志(右侧 toggle 开关,bg=`accent`) |

组标题 12sp `text@70%`;**「测试」徽章** = `bg=accent text=#000 px-1.5 py-0.5 rounded-sm text-[9px] Bold`。

### #M11 `p10_theme_picker_4themes.png`(**4 主题对比图,1 张**)
顶栏 `← 主题风格`。2×2 网格 304 宽 gap-8,每格 h=64:
| 格 | name | desc | dot |
|---|---|---|---|
| 选中(`accent` 2px 描边)| 科技青 | Tech     | `#22D3EE` |
| 普通 | 柔紫       | Lavender | `#9333EA` |
| 普通 | 温暖儿童   | Child    | `#FF7F50` |
| 普通 | 草莓可可   | Cocoa    | `#FB7185` |

> 此 mock 主题仍是 Tech;选中态用 Tech 自身 `accent` 描边即可。

### #M12 `p11_console_ai.png`
顶栏 `← 开发者控制台`,4 tab 等分(系统状态 / **AI引擎**(高亮 border-b-2 `accent`) / 显示调试 / 服务模拟)。下方 2×2 卡片 144×56:
- `Terminal` 运行日志 / 查看最近运行日志
- `Sparkles` 模型路由 / 强制指定大模型
- `Brain`    记忆管理 / 列表 / 删除 / 清空
- `Briefcase` 技能调试 / 调用及测试插件

### #M13 `p11_console_system.png`
4 tab,**系统状态**高亮。下方:
1. 卡片 304 宽 padding 8 + border:`传感器状态` 标题(11sp Bold)+ "IMU: 正常 / 光照: 420 lx"(10sp text@80%)
2. 标题 11sp Bold "体感测试"
3. **3×2 网格 6 按钮**(h=28 border 1px,bg=panel):前倾 / 后仰 / 左倾 / 右倾 / 摇晃 / 旋转

### #M14 `p11_console_service.png`
4 tab,**服务模拟**高亮。下方:
1. 标题 11sp Bold "场景模拟"
2. **3×2 网格 6 按钮**(h=32 border + bg=panel,文字 10sp medium):
   - 🎤 语音唤醒  ·  🤖 OTA模拟  ·  📵 报错状态
   - 📱 语音通话  ·  🔋 低电量    ·  🌡 过热
3. 按下态(可选 #M14b):border + 文字色变 `accent`

### #M15(可选) `p11_console_display.png`
4 tab,显示调试高亮。2×2 卡:`Palette 主题配置` / `Smile 静态表情` / `Shuffle 动态表情` / `Bug 表情调试`。

### #M16(可选) `p_wifi_ap.png`
Wi-Fi AP 配网:白底 QR(80×80)+ "扫码配网"(`accent`) + "热点: Robot_AP_1234" + "模拟接收配置" 圆角按钮。后续 connecting(spinner)/ success(✓ accent)/ error(✗ danger + 返回/重试)可作辅助序列。

### #M17(可选) `p_dialog_bubble.png`
home 上方 `top-4 left-1/2 -translate-x-1/2 w-[85%]` 圆角 2xl bubble:`bg=${panel}CC + border ${border}80 + box-shadow ${accent}33`,内 11sp 文本居中。suggest 类型多 2 个圆按钮(✗ 灰底 / ✓ accent 33% 底)。

---

## 7. 不变项白名单(**严禁修改**)

| 项 | 锁定原因 |
|---|---|
| face_engine 24 帧 variant 几何参数(`scale=1.6`)| 已与 React 原型像素对齐 |
| 状态栏 4 槽顺序(右起 battery → wifi → ai_dot → plug) | 后端事件结构耦合 |
| 6 扇区角度 −90/−30/30/90/150/210° | `page_menu.c::SECTORS[]` 硬编码 |
| Menu 6 扇区 label 与 icon 映射(对话/模型/主题/设置/人格/记忆) | 路由表硬编码 |
| Console 4 tab 名称与顺序(系统状态/AI引擎/显示调试/服务模拟) | 路由表硬编码 |
| 4 套主题 token(见 §1.1) | 工程 THEME_TECH/LAVENDER/CHILD/COCOA 已固化 |
| 主题总数 = 4(无 Warm / Dev / Steel / Mono 等) | 已删除,留作技术债 |
| 卡片样式(暗主题 panel@30% / 亮主题 panel 100%) | 全局 `xb_card()` 约束 |
| 圆角 4 / 按钮 h=28 / 行高 36 或 56 | LVGL hit-area 联动 |
| 图标 viewBox 24 strokeWidth 2.4 lucide 风 | SVG 资产共用 |
| Developer 默认可见(无 5-tap) | v7.6 已下线,徽章改为"测试" |
| face 颜色 = `accent_hi`(不是 `accent`) | 与 React 实现一致 |
| 底部 RGB 灯带高 1.5 px | 物理 LED 条同高 |

---

## 8. 输出格式约定

```
### p02_home_idle_clean.png
[640×480 PNG]
描述:全屏 idle face,两眼 32×64 圆角 16,嘴 24×4 圆角 2,无状态栏,bg=tech.bg
图层:bg / left_eye / right_eye / mouth / rgb_strip(opa 60%)
token:bg=tech.bg, accent_hi=tech.accent_hi
```

---

## 9. 验收 checklist(PM 自测)

- [ ] 主题统一 Tech(除 #M11 4 主题对比)
- [ ] 6 扇区角度 −90/−30/30/90/150/210° 顺时针
- [ ] Menu **4 点位 = 主题(Palette)**,不是 Skills
- [ ] StatusBar 4 槽顺序正确,亮色主题下文字仍清晰
- [ ] Settings 5 组中文,开发者选项**默认可见**带"测试"徽章
- [ ] Console 4 tab 中文,**系统状态 = 6 体感按钮**,**服务模拟 = 6 中文 emoji 按钮**
- [ ] ThemePicker **2×2** 4 主题:科技青/柔紫/温暖儿童/草莓可可,圆点 `#22D3EE / #9333EA / #FF7F50 / #FB7185`
- [ ] 全文字段无残留英文 tab 名 / 无 5-tap 文案
- [ ] face 颜色 = `accent_hi`,不是 `accent`
- [ ] 所有 mock 640×480 PNG-24
