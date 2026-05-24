# v6.2.1 交付包变更说明

发布日期:2026-05-23

## 包内容

```
v6.2.1_bundle/
├── A_桌面机器人_工程实现规格+后台架构_v5.6.1.md   # 含新增附录 N(全量复刻参考)
├── B_陪伴机器人_产品PRD+UX迭代规划_v6.2.1.md      # 含新增 Part 3(条目对照)
├── AGENT_REPLICATION_PROMPT.md                     # ★ 投喂下游编码 Agent 的标准作业书
├── full_replica_v6.2/                              # 整树参考实现(可投喂下游 Agent)
│   └── docs/AGENT_REPLICATION_PROMPT.md            # 上述 prompt 的同位副本(就近查阅)
└── CHANGELOG_v6.2.1.md                             # 本文件
```

## 与 v5.6 / v6.2 的差异

### A 文档(v5.6 → v5.6.1)
- 新增**附录 N:全量复刻参考实现**(11 节)
  - N.1 包目录结构 + 与 §一 项目结构的命名映射
  - N.2 卡片样式锁定(修复"纯亮蓝填充"事故)
  - N.3 状态栏 4 槽锁定
  - N.4 事件 ID 锁定(10 个 `XB_EVT_*`)
  - N.5 路由约定
  - N.6 图标占位符策略 + 投产替换流程
  - N.7 命名映射表(本附录 ↔ §一 ↔ ESP-Claw)
  - N.8 主题数量从 7 → 10 的扩展说明
  - N.9 控制台栅格修复(2 列定档,修复 3 列截断)
  - N.10 AVG 动效目录(MI-01..MI-10)
  - N.11 规则补丁 35-38(在 §M 规则 34 之后追加)

### B 文档(v6.2 → v6.2.1)
- 新增**Part 3:复刻包对照映射**(6 节)
  - §9 页面 ↔ 复刻文件对照表(11 个页面全覆盖)
  - §10 微交互 MI-01..MI-10 ↔ AVG JSON
  - §11 主题数量说明(7 主题色 + 3 高对比变体)
  - §12 设计原则补丁兑现验证
  - §13 验收清单 §8 在复刻包上的回归路径
  - §14 与 A 文档附录 N 的关系
- 工程基线行从 `UI v5.6` 升级为 `UI v5.6.1`
- 头注新增 v6.2.1 变更说明

### `full_replica_v6.2/`(整树新增)
- 核心层:`xb_event` / `xb_theme` / `xb_face`
- 控件层:`xb_widgets`(card / button / statusbar / topbar / dot_loading / toast / error_inline)
- 11 页面:boot / home / menu / chat / wifi_ap / wifi_pair / ota / skills / skill_detail / settings / console
- 路由:栈式 navigator(深度 6),投产时替换为 `claw_navigator`
- 资源:81 SVG 图标 / 10 主题 token / 10 AVG 动效 JSON / 110 PNG mock
- 38 个 `lv_image_dsc_t` 占位(便于离线编译,投产前由 `lv_img_conv` 替换)
- CMakeLists.txt 可直接作为 ESP-IDF component drop-in
- 完整 docs/(README + INTEGRATION + COVERAGE)

## 兼容性

- 上游 `claw_core` / `claw_event_router` / `face_engine` 接口**无破坏性变更**
- A 文档原 §一~§十四正文**未改动**;附录 N 是**追加**,不是覆盖
- B 文档 Part 1 / Part 2 **未改动**;Part 3 是**追加**

## 走查路径

1. 工程师:读 A.附录 N → 把 `full_replica_v6.2/src/` 整树复制到 `components/ui_core/`,按 N.7 重命名 → 替换图标 stub → 接入 `claw_navigator`
2. 设计师:读 B.Part 3 §9 → 用 `mock_renders/*.png` 走查每个页面的设计实现一致性
3. QA:读 B.Part 3 §13 → 按表注入事件回归验收清单 §8 的 10 个用例
4. **下游编码 Agent**:读 `AGENT_REPLICATION_PROMPT.md` → 按 14 节执行,交付前必走 §11 的 24 条 Self-Audit 全过

## 投喂下游 Agent 的最小指令

把 `AGENT_REPLICATION_PROMPT.md` 作为 Cursor / Claude Code / Codex 的 **system prompt**,然后第一条 user message 写:

```
你已经拿到 v6.2.1_bundle 全量包。请先回答"已读 14 节,准备开工",
然后按 §7 顺序逐页面实现,每完成一个页面贴出 §11.D 的视觉走查截图。
```

Agent 必须先回"已读"再开始任何 tool 调用 — 这是规则触发信号。
