# 桌面陪伴机器人 UX 迭代规划 v3.0(配套 v7.6)

## 文档信息

| 项 | 值 |
|---|---|
| 版本 | **v3.0** |
| 基线 | React 原型 v7.6 |
| 上版 | v2.0(配 v6.3 原型) |
| 角色 | UX 主导改版,设计师 AI 执行,工程同步落地 |
| 节奏 | 双周迭代,每轮一份 round-N prompt |

---

## 0. 迭代脉络(从 v6.3 到 v7.6)

```
v6.3.0 → v6.3.1   外观微调,Warm 主题琥珀化
v6.3.1 → v6.8     Warm/Dev 二次调参(深琥珀 / 磷光绿)
v6.8   → v7.0     PM 提议:Warm/Dev 受众窄,换 Lavender/Cocoa
v7.0   → v7.3     Menu 重排:Skills 下沉,Theme 上一级
v7.3   → v7.5     Settings 中文化,Developer 默认可见
v7.5   → v7.6     Console 中文 + 6 体感按钮 + 6 剧本简化  ◀ 当前
```

---

## 1. v7.6 关键 UX 决策

### 1.1 主题方向:Lavender + Cocoa 替换 Warm + Dev

**问题**:
- Warm 偏中性温暖,与 Child 视觉重叠;
- Dev 极客绿色定位过窄,女性用户不爱。

**调研依据**(内测 80 人):
- 偏好女性化温柔色:**Lavender(柔紫)61%**;
- 偏好高级温暖色:**Cocoa(草莓可可)47%**;
- Warm 仅 28% 选,Dev 仅 12% 选。

**决策**:Warm → Lavender(柔紫),Dev → Cocoa(草莓可可)。Cocoa 同样是暗主题,弥补 Tech 之外的暗色需求,且色调更高级。

**色彩验证**:
- Tech(暗 / accent 青)— 默认科技感
- **Lavender(亮 / accent 紫)— 新增,温柔商务**
- Child(亮 / accent 珊瑚)— 圆润亲和
- **Cocoa(暗 / accent 草莓粉)— 替代,深可可暖**

### 1.2 信息架构:Skills 下沉,Theme 上一级

| 入口 | v6.3 频次 | 决策 |
|---|---|---|
| Theme 切换(原 Settings 子项) | 日均 1.2 次 | ★ 提升到 Menu 一级 |
| Skills(原 Menu 4 点位) | 日均 0.03 次 | 下沉到 Settings 子项 |

> 启动率差 40 倍,继续放一级浪费,做硬性置换。

### 1.3 Settings 全中文 + Developer 默认可见

**v6.3 痛点**:
- Settings 英文,老年用户不友好;
- Developer 5-tap 解锁被部分用户视为"炫技 bug";内测 17% 用户因为找不到 Console 提工单。

**v7.6 改动**:
- 5 组全部中文化:通用 / 显示与声音 / 拓展功能 / 系统 / **开发者选项**
- Developer 直接展示,组标题旁加「测试」徽章(`bg=accent text=bg`)
- Console tab 也全部中文:系统状态 / AI 引擎 / 显示调试 / 服务模拟

### 1.4 Console.System:3 英文 → 6 中文体感

**v6.3**:Shake / Flip / Cover(3 个,中性词,工程同学需要查文档对应到 IMU)
**v7.6**:前倾 / 后仰 / 左倾 / 右倾 / 摇晃 / 旋转(6 个,直接对应物理动作)

**收益**:
- 工程自测效率提升;
- 6 方向覆盖 IMU 全自由度,测试无盲区。

### 1.5 Console.Service:去顶部卡 + 中文 emoji

**v6.3**:顶部 3 大卡(Event Inject / Audio Loop / OTA Force)+ 6 剧本按钮
**v7.6**:**去掉顶部 3 大卡**,只保留 6 中文 emoji 剧本(🎤 语音唤醒 / 🤖 OTA模拟 / 📵 报错状态 / 📱 语音通话 / 🔋 低电量 / 🌡 过热)

**收益**:
- 大卡和剧本功能重叠 80%,删冗;
- 视觉更专注;
- 中文 + emoji 双语义,新人也能猜对意图。

### 1.6 Boot 动画统一

**v6.3**:每主题独立动画,Tech 闪屏 / Warm 渐亮 / Dev 矩阵雨 / Child 弹跳
**v7.6**:**统一揉眼起床**(无主题分支),仅 bg 取当前主题

**理由**:强化"机器人是生命体"心智;削弱主题的"换皮"感。机器人不应因换衣服而换灵魂。

### 1.7 StatusBar always 模式

**v6.3**:子页只继承 home 的状态栏模式(idle 时隐,critical 时显)
**v7.6**:**所有非 home 子页常驻**(Settings/Console/Model/Persona/Memory/Theme/Skills/Wifi)

**理由**:用户进入 Settings/Console 后同样需要电量 / Wi-Fi 感知,不该因切页失去信息。

### 1.8 DialogBubble 顶部居中

**新增**:顶部 85% 圆角卡,2 种类型 `text`(纯文本)/ `suggest`(带 ✓ ✗ 按钮)。

**触发场景**:
- 早安 / 无聊 / OTA / 错误 / 唤醒 / 建议

**视觉**:`bg=${panel}CC + border ${border}80 + box-shadow ${accent}33 0 0 12`,文字 11sp 居中。

### 1.9 底部 RGB 灯带场景化

把"状态"从屏幕内扩到屏幕外:

| 场景 | 灯带 |
|---|---|
| 平常 | `accent_hi` 4s 脉动 |
| 无聊 | **黄色** 4s 慢 |
| OTA | **紫色** 1.5s |
| 错误 | **红色** 0.5s 急 |
| 唤醒 | accent_hi 0.5s 高强 |
| 生气 | 红色 1.2s 中 |

---

## 2. v7.6 设计交付物

| 交付物 | 文件 | 状态 |
|---|---|---|
| 主设计简报 | `UI_AI_BRIEF_v7.6.md` | ✅ 已发 |
| Token 工程头 | `theme_tokens.h` | ✅ 已发 |
| 14 张 mock(主)+ 6 张辅 | `svg/*.svg / *.png` | ✅ 已发 |
| Round-3 迭代 Prompt | `DESIGNER_AI_PROMPT_round3.md` | ✅ 已发 |
| 工程实现规格 | `doc1_engineering_v7.6.md` | ✅ 本包 |
| 后台架构 | `doc2_backend_v2.0.md` | ✅ 本包 |
| 产品 PRD | `doc3_prd_v3.0.md` | ✅ 本包 |
| UX 迭代规划 | `doc4_ux_v3.0.md` | ✅ 本包(当前文件) |

---

## 3. 下一轮(round-3)迭代项

详见 `DESIGNER_AI_PROMPT_round3.md`,4 条必改:

| ID | 项 | 优先级 |
|---|---|---|
| B1 | 卡片透明度按"暗/亮主题"分流(`cardBg(theme)` helper) | P0 |
| B2 | ThemePicker 圆点描边在亮主题下加 drop-shadow,选中圆点 12→14 | P1 |
| B3 | Cocoa 主题「系统更新」alert 对比度增强(加粗 + `!`) | P1 |
| B4 | Console.Service 按钮补 touchstart/touchend(触屏支持) | P0 |

加 §A 三条自检 grep,确保:
- 不存在旧主题名(`warm/dev/steel/orange/blue/pink/green/purple/yellow/mono`)
- 不存在散写 Tailwind cyan 色阶
- 不存在 7 token 之外的 hex

---

## 4. UX 度量(v7.6 上线后追踪)

| 指标 | 当前(v6.3 灰度) | 目标(v7.6 全量后) |
|---|---|---|
| 主题切换次数 / 日 | 1.2 | ≥ 1.8 |
| 进入 Console 用户数 / 周 | 3% | 8%(降低门槛后) |
| 主题分布:Lavender 占比 | — | ≥ 25% |
| 主题分布:Cocoa 占比 | — | ≥ 20% |
| Skills 启动次数 / 日 | 0.03 | 不要求增长(主动降权) |
| 5-tap 文案投诉数 | 17 工单 / 月 | 0 |
| Console 中文化后工程自测平均时长 | 8 min | ≤ 5 min |

---

## 5. 风险

| 风险 | 影响 | 应对 |
|---|---|---|
| 老用户 OTA 后主题被回退 TECH | 体验回退感 | OTA 前云端持久化主题,OTA 后自动还原 |
| Cocoa 在 OLED 残影 | 暗色 + 长亮 | 提供「自动调暗」选项,30 min 无交互降亮度 50% |
| Lavender 在强光下对比度不足 | 白底紫字易眩 | 已校验 4.6 (AA),并加 darker text `#4C1D95` |
| Skills 下沉后用户找不到 | 投诉 | Settings → 拓展功能 内排第二位,加 badge "3 个已安装" 引流 |

---

## 6. 联动文档

- 产品决策:`doc3_prd_v3.0.md`
- 工程实施:`doc1_engineering_v7.6.md`
- 后端契约:`doc2_backend_v2.0.md`
- 设计基线:`UI_AI_BRIEF_v7.6.md`
- 14 mock:`svg/p*.svg|png`
- 下游迭代:`DESIGNER_AI_PROMPT_round3.md`

---

## 7. 验收 checklist(UX 侧)

- [ ] 4 主题命名:科技青 / 柔紫 / 温暖儿童 / 草莓可可,无 Warm / Dev 残留
- [ ] Menu 4 点位 = Palette 主题,Skills 沉到 Settings → 拓展功能
- [ ] Settings 5 组全中文,Developer 默认可见 + 「测试」徽章
- [ ] Console 4 tab 全中文,System = 6 体感中文按钮,Service = 6 中文 emoji 剧本
- [ ] Boot 动画统一揉眼,无主题分支
- [ ] StatusBar 在所有非 home 子页常驻
- [ ] DialogBubble 早安 / 无聊 / OTA / 错误 5 类场景文案与图示对齐
- [ ] 底部 RGB 灯带 6 种场景色相全部能触发(remote `cmd/scene_inject` 测试)
- [ ] 任何亮色主题(Lavender/Child)的可点击元素对比度 ≥ AA(4.5)
- [ ] 4 主题 ThemePicker mock 圆点颜色严格 `#22D3EE / #9333EA / #FF7F50 / #FB7185`
