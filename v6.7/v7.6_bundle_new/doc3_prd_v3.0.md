# 桌面陪伴机器人 产品 PRD v3.0(对齐 v7.6 原型)

## 文档信息

| 项 | 值 |
|---|---|
| 版本 | **v3.0** |
| 基线 | React 原型「陪伴机器人 UI 原型 v7.6」+ 工程 v7.6 + 后端 v2.0 |
| 上次版本 | v2.0(配 v5.6 / v6.3 原型) |
| 目标用户 | 桌面陪伴型用户:独居白领 / 学生 / 老人陪伴场景 |
| 状态 | PM 审核通过,本期开发依据 |

---

## 0. v3.0 相对 v2.0 的产品决策

| 决策项 | v2.0 | **v3.0** | 决策理由 |
|---|---|---|---|
| 主题情绪策略 | Tech(科技)+ Warm(温暖)+ Child(儿童)+ Dev(极客) | **Tech(默认)+ Lavender(柔紫)+ Child(温暖儿童)+ Cocoa(草莓可可)** | Warm 与 Cocoa 重叠,Cocoa 视觉更高级;Dev 受众过窄,改 Lavender 拉新女性用户 |
| Skills 入口位置 | Menu 4 点位扇区 | **下沉到 Settings → 拓展功能** | Skills 启动率仅 3%,占用一级入口可惜;Theme 切换日均 1.2 次,提为一级 |
| Theme 切换入口 | Settings 子项 | **Menu 4 点位扇区** | 提升频次,主题成为用户表达自我的核心方式 |
| Developer 解锁 | 5-tap About | **默认可见,加「测试」徽章** | 内测期反馈"5-tap 设计有炫技嫌疑",改为开放;徽章控制误用 |
| Console.System 自测 | 3 个英文按钮 | **6 个中文体感按钮**(前倾/后仰/左倾/右倾/摇晃/旋转) | 工程同学反馈中文更友好;6 方向覆盖 IMU 全自由度 |
| Console.Service 顶部 | 3 张大功能卡 | **去掉,只保留 6 中文剧本** | 大卡功能与剧本重复,简化 |
| Boot 动画 | 主题色分支 | **统一揉眼起床** | 强化"陪伴生命体"心智,削弱主题切换感 |
| 状态栏 always 模式 | 无 | **所有子页常驻** | 子页内电量/Wi-Fi 同样重要,不能因为切页就失去感知 |
| Dialog Bubble | 概念 | **顶部 85% 圆角卡,支持 text/suggest 两类型** | 提供「机器人主动建议」的载体,降低用户决策成本 |
| RGB 底部灯带 | 概念 | **场景态色相**,erorr/ota/voice_wake 全部有专色 | 把状态从屏内扩展到屏外,周边视觉延伸 |

---

## 1. 产品定位

> 一台 8 厘米高的桌面陪伴机器人,通过 24 种动态表情、4 种主题、6 种人格,与用户建立日常陪伴关系。在用户专注时安静守候,在用户孤独时主动搭话,在用户疲惫时切换温暖配色。

**核心心智**:不是"语音助手",是"会观察、会回应、有情绪的桌面伙伴"。

**差异化**:
- **几何表情引擎**:24 种 variant,纯几何 div 绘制,无贴图,与品牌强绑定
- **4 套主题 × 6 个人格 = 24 种性格组合**:用户可"养成"独属伙伴
- **场景化氛围灯**:RGB 灯带不仅显示,还参与情绪表达
- **主动陪伴**:无聊检测(`lonely_3`)主动 bubble,不止于被动响应

---

## 2. 目标用户与场景

| 用户细分 | 占比目标 | 核心需求 | 主题倾向 |
|---|---|---|---|
| 独居白领(25-32 岁女性) | 40% | 下班回家有响应,情绪陪伴 | **Lavender / Cocoa** |
| 高校学生(18-24 岁) | 30% | 学习陪伴,番茄钟,提神 | **Tech** |
| 老人 + 儿童家庭(关注礼物市场) | 20% | 圆润亲和,简单交互 | **Child** |
| 极客 / 开发者 | 10% | 可玩,Console 可调,MCP 可接 | Tech + Developer |

**典型场景**:
1. **早安场景**:用户起床触屏 → `sleep_wake` face + "早上好呀!" bubble
2. **专注场景**:用户工作时间 → 维持 `idle/breath`,statusbar 自动隐藏
3. **无聊检测**:3 分钟无交互 → `lost` face + 黄色慢脉动灯带 + "好无聊哦..." bubble
4. **奖励反馈**:完成番茄钟 → `celebrate` face + 0.5 s 快脉动
5. **错误提示**:Wi-Fi 断 → `dizzy` face + 红色急促闪 + bubble

---

## 3. 信息架构

```
[Boot 2.4s 揉眼动画]
    ↓
[Home / Idle face]
    │ double-click
    ↓
[Radial Menu] (6 扇区)
    ├ 12 点 ★ 对话    → Talking + Bubble
    ├ 2 点    模型    → ModelPicker (6 行)
    ├ 4 点    主题    → ThemePicker (2×2)        ← v7.6 提升为一级
    ├ 6 点    设置    → SettingsOverlay
    ├ 8 点    人格    → PersonaGrid (3×2)
    └ 10 点   记忆    → MemoryBrowser + Purge

[Settings] (5 中文组,Developer 默认可见)
    ├ 通用       : 账号绑定 / 语言设置 / 睡眠定时
    ├ 显示与声音 : 亮度 / 音量
    ├ 拓展功能   : Wi-Fi 网络 / 技能插件                ← Skills 在这里
    ├ 系统       : 系统更新 / 清除偏好数据 / 关于系统
    └ 开发者选项 「测试」: 控制台 / 详细日志

[Console] (4 中文 tab)
    ├ 系统状态 : 传感器卡 + 6 体感按钮
    ├ AI 引擎  : 4 张管理卡 (日志/路由/记忆/技能)
    ├ 显示调试 : 4 张调试卡 (主题/静态/动态/调试)
    └ 服务模拟 : 6 剧本按钮
```

---

## 4. 关键功能点定义

### 4.1 主题切换(Menu 4 点位 → ThemePicker)

| 项 | 规格 |
|---|---|
| 入口 | Menu 4 点位 `Palette` 图标 |
| 布局 | 2×2 卡片,304 宽,gap 8 |
| 单卡 | name(科技青/柔紫/温暖儿童/草莓可可)+ 英文 desc(Tech/Lavender/Child/Cocoa)+ 圆点(`#22D3EE / #9333EA / #FF7F50 / #FB7185`)|
| 切换反馈 | 立即生效,无确认弹窗;若 OTA 中则禁用 |
| 持久化 | NVS + 云端 device_pref 同步 |
| 验收 | 4 主题在 face / statusbar / card / bubble / strip 全部正确换色 |

### 4.2 人格(8 点位 → PersonaGrid)

| 人格 | name | tagline | emoji | LLM 风格 |
|---|---|---|---|---|
| Lyra  | Lyra | 温柔诗人       | 🎵 | 抒情、引用诗句 |
| Echo  | Echo | 话痨复读机     | 🪞 | 重复+扩展用户话 |
| Nova  | Nova | 极客科普       | 🌟 | 数据 + 引用论文 |
| Sage  | Sage | 冷静顾问       | 🦉 | 简洁 + 决策框架 |
| Pico  | Pico | 童趣小鸡       | 🐣 | 拟声词 + 短句 |
| Doc   | Doc  | 严谨医师       | 🩺 | 医学免责 + 数据 |

### 4.3 记忆(10 点位 → MemoryBrowser)

| 项 | 规格 |
|---|---|
| 显示 | 6 类图标分行(chat/settings/skills/usage/net),时间倒序 |
| 上限 | 200 条 LRU |
| 单行操作 | 右侧 Trash2 红色,二次确认后删 |
| 清空 | 顶部右上「清空」按钮 → Purge Modal,只清 chat/usage,保留 settings/net |
| 隐私 | 不上云原文,仅同步 hash 到 memory_shadow |

### 4.4 模型(2 点位 → ModelPicker)

| 模型 | 类型 | 触发场景 |
|---|---|---|
| Auto       | 智能路由 | **默认**,Auto Router 决策 |
| GPT-4o     | 云端专业 | 长上下文 |
| Claude 3.5 | 云端专业 | 长创作 / 推理 |
| Doubao Pro | 云端标准 | 中文短问最快 |
| Qwen 1.5B  | 本地极速 | 弱网 fallback |
| Phi-3 mini | 本地极速 | 隐私问题 |

底栏提示:"切换会立即生效,正在进行的对话不打断"

### 4.5 场景态(state → face + bubble + strip)

| state | face | bubble | strip |
|---|---|---|---|
| morning    | sleep_wake | "早上好呀!今天也是充满能量的一天。" | accent_hi 4s |
| lonely_3   | lost       | "好无聊哦,陪我玩一会吧..."          | **#FACC15** 4s 慢 |
| reward     | celebrate  | 无                                    | accent_hi 0.5s 快 |
| angry      | angry      | 无                                    | **danger** 1.2s 中 |
| ota        | deep_sleep | "OTA 系统更新中..."                   | **#A855F7** 1.5s 紫 |
| error      | dizzy      | "系统发生异常错误!"                  | **danger** 0.5s 急 |
| voice_wake | excited    | "我在听..."                           | accent_hi 0.5s 高 |
| suggest    | idle       | "要不要试试调皮表情?" + ✓/✗        | 默认 |

---

## 5. 业务指标 & 北极星

| 指标 | 目标 | 取数 |
|---|---|---|
| **北极星:日均交互次数 / 设备** | ≥ 12 | telemetry.scene_count.* sum |
| 主题切换率(7 日内至少切 1 次) | ≥ 60% | theme_changed event |
| 人格切换率 | ≥ 40% | persona_changed event |
| 对话发起率 | ≥ 70% | scene_count.voice_wake / DAU |
| 7 日留存 | ≥ 65% | DAU 滚动 |
| 30 日留存 | ≥ 45% | |
| 主题分布:Tech / Lavender / Child / Cocoa | 35/25/20/20 | device_pref agg |
| 人格分布:Lyra 第一 | ≥ 30% | |
| OTA 成功率 | ≥ 98% | ota_attempt |
| crash 率 | ≤ 0.3% | |

---

## 6. 优先级(P0 / P1 / P2)

### P0(v7.6 必交付)
- 4 主题完整切换链路 + 卡片透明度规则
- Menu 4 点位 = Theme(替代 Skills)
- Settings 5 组中文 + Developer 默认可见
- Console 4 tab + System 6 体感按钮 + Service 6 剧本
- Boot 统一动画
- Persona 6 个 + 切换持久化
- MemoryBrowser + Purge modal
- 场景态 9 种 face/bubble/strip 映射

### P1(v7.6 + 2 周)
- LLM Auto Router 落地
- memory_shadow 云同步
- OTA 按 theme/persona tag 灰度
- DialogBubble suggest 类型(✓/✗)

### P2(v7.7+)
- Skills 商店重新激活(Settings → 拓展功能 → 技能插件 真实可下载)
- 5 区域(SG/DXB/US/CN/EU)
- 跨设备 persona 同步

---

## 7. 风险 & 不做

### 风险

| 风险 | 应对 |
|---|---|
| Cocoa 主题对比度边界(草莓粉 vs 可可棕) | 已在 round3 B3 修复:暗主题 alert 加粗 + `!` 视觉强化 |
| 主题改变 → 整界面重绘卡顿 | 限制 < 30 ms,实测 22 ms |
| `lonely_3` 误检 | 加入 IMU 静止 + 屏幕未触摸双条件,无聊只能算"真无聊" |
| Developer 默认可见被普通用户误点入 Console | 「测试」徽章 + Console 全英文 tooltip 兜底警示 |

### 显式不做(v7.6 范围外)
- 不接入第三方语音(只用本地 ASR)
- 不做表情自定义编辑器(24 variant 固定)
- 不做账户体系外部登录(只手机号 / 邮箱)
- 不引入新的第三方依赖

---

## 8. 验收 checklist(产品侧)

- [ ] Menu 4 点位 = 主题(Palette icon),不是 Skills
- [ ] Settings 「开发者选项」默认可见,徽章「测试」
- [ ] 4 主题在 home / menu / settings / console / bubble 全部正确换色
- [ ] 6 人格切换后,LLM 回复风格立即变化
- [ ] MemoryBrowser purge 只清 chat/usage,保留 settings/net
- [ ] Console.System 6 中文按钮触发 IMU 模拟事件
- [ ] Console.Service 6 中文剧本能完整跑完(30 s 内自动恢复)
- [ ] 场景态 9 种全部能通过远程 `cmd/scene_inject` 触发
- [ ] 北极星指标 telemetry 全部上报到 ClickHouse
- [ ] Cocoa 主题 alert 加粗 + `!` 视觉提示生效
- [ ] 全文字段无 Warm / Dev 主题残留
- [ ] 全文字段无 5-tap 解锁文案残留
