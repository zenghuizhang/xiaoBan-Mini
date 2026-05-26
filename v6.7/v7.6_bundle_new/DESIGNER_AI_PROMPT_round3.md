# 投喂下游设计师 AI 的迭代 Prompt(v7.6 → round3)

> **使用方法**:把本文件**整段**复制进设计师 AI(或 React 编码 AI)的对话框。
> **前置条件**:对方已经持有 `陪伴机器人 UI 原型 v7.6` 代码库。
> 本 prompt 已自包含目标、约束、token、验收清单,无需再附其它资料。

---

## 你的任务

你已经交付了 `陪伴机器人 UI 原型 v7.6`。PM 完成走查,本轮没有大改方向,只做 **3 类对齐 + 1 类清理**,目标是把代码、设计 mock、工程 header 三方完全一致。提交:

1. 改后源码 zip(目录结构不变)
2. `CHANGELOG_round3.md`,列出每条修改对应文件 / 行号
3. 在 `RobotUI.tsx` 顶部追加注释 `// v7.6 round3 — alignment fixes per PM review`
4. **新增**:14 张 mock(命名 §C),每张 640×480 PNG-24,放在 `mocks/` 目录

---

## §0. 全局约束

1. **不要**改下列已对齐的部分:
   - Menu 6 扇区角度 / label / icon(对话/模型/主题/设置/人格/记忆)
   - StatusBar 4 槽顺序(右起 battery → wifi → ai_dot → plug)
   - 4 套主题名与 token(Tech / Lavender / Child / Cocoa)
   - Console 4 tab 名(系统状态 / AI引擎 / 显示调试 / 服务模拟)
   - face_engine variant 几何参数(`scale = 1.6`)
2. **不要**新增第三方依赖
3. **不要**改组件文件名,只改内部实现
4. 所有色值必须用 `src/theme/tokens.ts` 表里的 7 个 key,**禁止**散写十六进制
5. **不要**重新引入"5-tap 解锁 Developer"逻辑 — 已在 v20 下线

---

## §A. 主题 Token 单一真源(已对,但要校验)

`src/theme/tokens.ts` 已正确收敛到 4 套。本轮**只做自检**,不改源码,除非发现 §A.2 列举的违规:

### A.1 4 套 token 终版(贴在这里供你比对)

```ts
export const TECH_TOKENS     = { bg:'#000000', panel:'#0a1e28', accent:'#22D3EE',
                                 accent_hi:'#33C5FF', text:'#b4ebff', border:'#14506e', danger:'#F43F5E' } as const;
export const LAVENDER_TOKENS = { bg:'#FAF5FF', panel:'#FFFFFF', accent:'#9333EA',
                                 accent_hi:'#A855F7', text:'#4C1D95', border:'#E9D5FF', danger:'#EF4444' } as const;
export const CHILD_TOKENS    = { bg:'#FFF9E6', panel:'#FFFFFF', accent:'#FF7F50',
                                 accent_hi:'#FFAA78', text:'#c85a28', border:'#FFC8A0', danger:'#F43F5E' } as const;
export const COCOA_TOKENS    = { bg:'#2D1B0E', panel:'#3F2B20', accent:'#FB7185',
                                 accent_hi:'#FDA4AF', text:'#FEF3C7', border:'#573D2C', danger:'#EF4444' } as const;
```

### A.2 自检命令(改完跑一遍,期望都为空)

```bash
# 1. 不能出现已删除的旧主题名
grep -rE "theme === '(warm|dev|steel|orange|blue|pink|green|purple|yellow|mono)'" src/

# 2. 不能再散写 cyan-XXX 这类 Tailwind 色阶
grep -rE 'text-cyan-[0-9]|bg-cyan-[0-9]|border-cyan-[0-9]' src/

# 3. 不能直接写 7 个 token 之外的 hex(face / boot / RGB 灯带例外)
grep -rE '#[0-9A-Fa-f]{6}' src/components/ src/RobotUI.tsx | \
  grep -vE '#(000000|0a1e28|22D3EE|33C5FF|b4ebff|14506e|F43F5E|FAF5FF|FFFFFF|9333EA|A855F7|4C1D95|E9D5FF|EF4444|FFF9E6|FF7F50|FFAA78|c85a28|FFC8A0|2D1B0E|3F2B20|FB7185|FDA4AF|FEF3C7|573D2C|FACC15|A855F7)'
```

---

## §B. 必改 4 条

### B1. 卡片透明度按"暗/亮主题"分流(目前是 `theme === 'tech' ? 30% : 100%`,只判 tech 太死)

**BEFORE**:`backgroundColor: theme === 'tech' ? \`${t.panel}4D\` : t.panel`

**问题**:Cocoa 也是深色,同样需要 30% 透明卡片才能看到背景脸;但当前判 `'tech'` 字面量,Cocoa 走的是 100% panel,卡片在深色上"糊"成一块。

**AFTER**(在 `src/theme/tokens.ts` 末尾新增 helper):
```ts
export const DARK_THEMES: ThemeName[] = ['tech', 'cocoa'];
export const isLightTheme = (n: ThemeName) => !DARK_THEMES.includes(n);
export const cardBg = (n: ThemeName) => {
  const t = TOKENS_TABLE[n] || TOKENS_TABLE.tech;
  return isLightTheme(n) ? t.panel : `${t.panel}4D`;
};
```

然后把所有 `theme === 'tech' ? \`${t.panel}4D\` : t.panel` 全部替换为 `cardBg(theme as ThemeName)`。涉及文件:`Console.tsx` / `SettingsOverlay.tsx` / `PersonaGrid.tsx` / `ModelPicker.tsx` / `ThemePicker.tsx`。

### B2. ThemePicker 圆点描边在亮色主题下看不清

**BEFORE**:Lavender(白底)和 Child(米底)下,选中卡描边 `accent`(紫/橙)+ 圆点也是 `accent` — 没问题;但**未选中卡的 border `#E9D5FF / #FFC8A0`** 和**未选中圆点同色**,几乎隐形。

**AFTER**:未选中圆点 = `${dot} drop-shadow(0 0 4px ${dot}80)`(给一圈柔光),保留辨识度;选中卡的圆点尺寸 12 → **14** 强化反馈。

### B3. SettingsOverlay 的"系统更新"红色 alert 在 Cocoa 上对比度不足

**BEFORE**:`isAlert={true}` 用 `danger=#EF4444`,在 Cocoa bg `#2D1B0E` 上对比度 4.1(刚过 AA-large)

**AFTER**:对于暗色主题,alert 文字用 `${danger}` 但加 `font-weight: bold` + 右侧增加 `<span class="ml-1 text-[10px]">!</span>` 视觉提示。亮色主题保持原样。

### B4. Console.Service 按下态(`onMouseDown`)在触屏上无效

**BEFORE**:仅 `onMouseDown / onMouseUp / onMouseLeave`,触屏没 mouse 事件

**AFTER**:补 `onTouchStart` / `onTouchEnd` / `onTouchCancel`,逻辑同 mouse 三态。

---

## §C. 14 张 Mock 输出清单(本轮新增产出)

> 命名 `mocks/pNN_<page>.png`,**全部 640×480 PNG-24**。除 #M11 外主题统一 Tech。

| # | 文件名 | 说明 |
|---|---|---|
| M01 | `p01_boot.png`                       | Boot 揉眼定格:眼半睁 16,嘴打哈欠 28×36 |
| M02 | `p02_home_idle_clean.png`            | 全屏 idle face,无状态栏,底部 1.5px 灯带 |
| M03 | `p02_home_peek.png`                  | idle face + 22px 状态栏(plug/AI●/wifi/88%) |
| M04 | `p02_home_critical_lowbat.png`       | face 改 lost,状态栏 12% 红 + BatteryWarning |
| M05 | `p03_menu.png`                       | 6 扇区径向菜单 + 12 点对话 hover 态(label "对话") |
| M06 | `p06_model_picker.png`               | 6 行模型 + 底栏 hint |
| M07 | `p07_persona_grid.png`               | 3×2 卡 + Lyra 选中 |
| M08 | `p08_memory_browser.png`             | 6 行记忆 |
| M09 | `p08_memory_purge_modal.png`         | 同 #M08 + 底部 modal |
| M10 | `p09_settings.png`                   | 5 组,Developer 可见带"测试"徽章 |
| M11 | `p10_theme_picker_4themes.png`       | 2×2 主题对比,圆点 `#22D3EE / #9333EA / #FF7F50 / #FB7185` |
| M12 | `p11_console_ai.png`                 | 2×2 卡:运行日志/模型路由/记忆管理/技能调试 |
| M13 | `p11_console_system.png`             | 传感器卡 + 6 体感按钮(前倾/后仰/左倾/右倾/摇晃/旋转) |
| M14 | `p11_console_service.png`            | 6 中文 emoji 按钮 3×2 |

可选:`p_wifi_ap.png` / `p_dialog_bubble.png`(suggest 类带 ✓/✗)。

---

## §D. 验收 checklist(贴到 CHANGELOG_round3.md 末尾)

- [ ] B1 `cardBg(theme)` helper 已引入,5 个组件的 `theme === 'tech' ?` 已全部替换
- [ ] B2 ThemePicker 圆点带 drop-shadow,选中圆点 14px
- [ ] B3 Cocoa 主题下"系统更新"行有加粗 + `!` 标记
- [ ] B4 Service 按钮已加 touchstart/touchend/touchcancel
- [ ] §A 自检 3 条 grep 全部输出空
- [ ] 14 张 mock 全部 640×480 PNG-24,放在 `mocks/`
- [ ] #M11 4 圆点颜色严格 `#22D3EE / #9333EA / #FF7F50 / #FB7185`
- [ ] #M13 6 按钮文案严格 `前倾 / 后仰 / 左倾 / 右倾 / 摇晃 / 旋转`
- [ ] #M14 6 按钮文案严格 `🎤 语音唤醒 / 🤖 OTA模拟 / 📵 报错状态 / 📱 语音通话 / 🔋 低电量 / 🌡 过热`
- [ ] Menu **4 点位 = 主题(Palette)**,**不是** Skills
- [ ] 全文无 5-tap 文案 / 无 Warm / Dev 主题残留

---

## §E. 提交格式

```
deliverable_round3.zip
├── codebase/         (改动后的全部源码)
├── mocks/            (14 张 PNG)
├── CHANGELOG_round3.md
└── README.md         (沿用上一轮,补 "v7.6 round3" 节)
```

**完成判定**:§D 全部勾选 + zip 可 `npm run dev` 跑通 = 通过。
任何与本 prompt 冲突的"创新",请先列在 CHANGELOG 等 PM 决议,不要自由发挥。
