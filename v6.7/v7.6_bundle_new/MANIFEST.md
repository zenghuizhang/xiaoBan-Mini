# v7.6 完整交付包

> 基线:React 原型「陪伴机器人 UI 原型 v7.6」
> 输出:2026-05-26(增补:复刻指导手册 + 6 扇区独立高清图 + 每页 ×4 主题截图)
> 适用:M5Stack CoreS3 + ESP-IDF 5.5.4 + LVGL 9.5.0 + esp-claw `claw_core@v1.0`

## 0. ★ 本次重点新增(给"另一个 AI"用)

| 文件 / 目录 | 作用 |
|---|---|
| **`REPLICATION_GUIDE_v7.6.md`** | 完全复刻指导手册 — 17 章,4 个必过验收 + 4 主题 hex 严锁 + 6 扇区几何写死 + 启动序列 + 严禁清单 |
| **`build_icons_v7.6.py`** | 6 扇区图标生成脚本(纯 Python,无依赖) |
| **`icons_v7.6/<theme>/menu_0*_<theme>.svg`** | 6 扇区图标 × 5 色组(tech/lavender/child/cocoa/mono),**30 张独立高清 SVG**,256×256,可直接喂图像 AI |
| **`icons_v7.6/OVERVIEW_menu_icons_<theme>.svg`** | 5 张 3×2 总览图(960×720),一眼对照 |
| **`build_pages_per_theme_v7.6.py`** | 每页 ×4 主题 SVG 生成脚本 |
| **`svg_per_theme/<theme>/p*.svg`** | **52 张** 每页 ×4 主题完整截图,补齐 Child / Lavender / Cocoa 缺图 |

## 1. 文档(7 份)

| 文件 | 用途 | 给谁 |
|---|---|---|
| `UI_AI_BRIEF_v7.6.md`         | 主设计简报:画布 / token / IA / 14 mock 任务 / 不变项白名单 / PM checklist | UI 设计 AI / PM |
| `theme_tokens.h`              | LVGL C 头,4 套主题 + `is_light` + `xb_card_panel_opa` + 旧主题名兼容 | ESP-Claw 工程 |
| `DESIGNER_AI_PROMPT_round3.md`| 下游迭代 Prompt:B1~B4 必改 + §A 自检 + 14 mock 输出清单 | 设计师 AI / 编码 AI |
| **`doc1_engineering_v7.6.md`**| ★ 工程实现规格:目录 / 依赖 / 主题系统 / RGB / StatusBar / Console / Memory / Persona / Boot / 性能 | 固件 owner |
| **`doc2_backend_v2.0.md`**    | ★ 后台架构:LLM Auto Router / Persona Registry / Memory Shadow / OTA tag 灰度 / Telemetry 15 维 / 安全合规 | 后端 owner |
| **`doc3_prd_v3.0.md`**        | ★ 产品 PRD:v3.0 决策表 / 定位 / 用户场景 / IA / 功能定义 / 北极星 / P0~P2 | PM |
| **`doc4_ux_v3.0.md`**         | ★ UX 迭代规划:v6.3 → v7.6 脉络 / 9 项关键 UX 决策 / round-3 / 度量 / 风险 | UX 设计 |

★ = 本次根据 v7.6 新设计**全新撰写**,对齐工程 / 后台 / 产品 / UX 四方。

## 2. 设计稿(20 张,svg/ 目录,svg + png 各 1 份)

| # | 文件名(.svg + .png) | 说明 |
|---|---|---|
| M01 | `p01_boot`                       | Boot 揉眼定格 |
| M02 | `p02_home_idle_clean`            | 全屏 idle face |
| M03 | `p02_home_peek`                  | + 状态栏 transient |
| M04 | `p02_home_critical_lowbat`       | 低电 critical |
| M05 | `p03_menu`                       | 6 扇区 + 12 点 hover |
| M06 | `p06_model_picker`               | 模型选择 6 行 |
| M07 | `p07_persona_grid`               | 3×2 人格 + Lyra 选中 |
| M08 | `p08_memory_browser`             | 6 行记忆 |
| M09 | `p08_memory_purge_modal`         | + 清空 modal |
| M10 | `p09_settings`                   | 5 组中文,Developer 默认可见 |
| M11 | `p10_theme_picker_4themes`       | 2×2 主题对比 |
| M12 | `p11_console_ai`                 | AI 引擎 4 卡 |
| M13 | `p11_console_system`             | 6 体感按钮 |
| M14 | `p11_console_service`            | 6 场景按钮 |
| 辅 | `p11_console_display`            | 显示调试 4 卡 |
| 辅 | `p_wifi_ap`                      | Wi-Fi AP 配网 |
| 辅 | `p_theme_preview_{tech,lavender,child,cocoa}` | 4 主题独立缩略 |

## 3. 工具

| 文件 | 用途 |
|---|---|
| `build_svgs.py` | Python 一键重生 20 张 SVG,改 token/文案后 `python3 build_svgs.py` |

## 3.5 可落地固件 (firmware/, ★ 本次新增)

完整 ESP-IDF 5.5.4 + LVGL 9.5.0 组件,即拉即编。整体作为 `components/xb_ui/` 投入项目即可。

```
firmware/
├── CMakeLists.txt              # idf_component_register, 20 个 SRCS + REQUIRES lvgl/esp_lvgl_port/nvs_flash
├── idf_component.yml           # idf-component-manager 依赖清单
├── assets/
│   ├── themes/theme_tokens.h   # 4 主题 + xb_theme_lookup + xb_card_panel_opa
│   └── icons/                  # 81 个 SVG icon(待 tools/svg_to_lvgl.py 转换)
├── src/
│   ├── main.c                  # app_main: nvs/event/theme/persona/memory/bsp/router 启动序列
│   ├── core/
│   │   ├── xb_event.{h,c}      # 21 主题事件 pub/sub,MAX_SUBS=12,带 unsubscribe
│   │   ├── xb_theme.{h,c}      # NVS 持久化 + 旧主题名静默回退 + xb_card_panel_opa
│   │   └── xb_face.{h,c}       # 24 变体几何 face 引擎,color=accent_hi,20px halo @0xB3,150ms 眨眼
│   ├── widgets/
│   │   └── xb_widgets.{h,c}    # xb_card/button/topbar/statusbar(4 模式)/dialog_bubble/rgb_strip/dot_loading/toast/error_inline/modal
│   ├── sim/
│   │   ├── xb_persona.{h,c}    # 6 人格(lyra/echo/nova/sage/pico/doc), NVS 持久化
│   │   ├── xb_memory_store.{h,c}  # sqlite 记忆库 + RAM fallback
│   │   ├── xb_imu_sim.{h,c}    # 6 体感事件(前倾/后仰/左倾/右倾/摇晃/旋转)
│   │   └── xb_scenario_sim.{h,c}  # 6 场景(语音唤醒/OTA/报错/来电/低电/高温)
│   ├── pages/
│   │   ├── xb_pages.h          # PAGE_* enum + ctor 声明 + router 接口
│   │   ├── xb_router.c         # 8 层 page stack,LV_LAYOUT 切换
│   │   ├── page_boot.c         # 4 步揉眼动画 → HOME
│   │   ├── page_home.c         # idle face + IDLE 状态栏 + RGB strip
│   │   ├── page_menu.c         # 6 扇区(对话/模型/主题/设置/人格/记忆)
│   │   ├── page_chat.c         # 气泡 + dot loader,CHAT_DELTA/DONE/ERROR
│   │   ├── page_model_picker.c # 6 行 Auto/GPT-4o/Claude/Doubao/Qwen/Phi
│   │   ├── page_theme_picker.c # 2×2 主题
│   │   ├── page_persona_grid.c # 3×2 人格
│   │   ├── page_memory_browser.c # 列表 + 清空 modal
│   │   ├── page_settings.c     # 5 中文分组,Developer 默认可见 + 测试 徽章
│   │   ├── page_wifi_ap.c      # 配网 QR + 3 态状态
│   │   ├── page_skills_empty.c # v7.7 占位
│   │   └── page_console.c      # 4 tab(系统/AI/显示/服务)+ 6 体感 + 6 场景 + 主题/状态栏循环
│   └── assets/xb_icons_stub.c  # icon 注册表占位
└── docs/
    ├── INTEGRATION.md          # 集成指南 + bsp + NVS + Card opacity rule
    └── COVERAGE.md             # React ↔ LVGL 对照覆盖表
```

总计 **24 个 C/H 源文件 + 81 SVG + 2 文档**,约 1700 行 C。

| 验证项 | 状态 |
|---|---|
| 24 face 变体几何 | ✅ G[FACE_VARIANT_MAX] 表内 |
| face color=accent_hi | ✅ apply_geom 中 `th->accent_hi` |
| 4 主题 + is_light | ✅ theme_tokens.h |
| 卡片透明度规则 | ✅ xb_card_panel_opa 全局复用 |
| 6 扇区 Theme 在 4 点位 | ✅ SECTORS[2].angle_deg=30 |
| StatusBar 4 模式 | ✅ XB_STATUSBAR_{IDLE/TRANSIENT/CRITICAL/ALWAYS} |
| Console 4 中文 tab | ✅ 系统/AI/显示/服务 |
| Console 系统 6 体感 + 6 场景 | ✅ build_system_tab |
| Settings 5 组 + Developer 默认显示 | ✅ GROUPS[5], 测试 badge |
| 旧主题名 OTA 回退 | ✅ xb_theme_lookup → THEME_TECH |
| 编译路径 | ESP-IDF `idf.py build` |


## 4. 主题 token 终版

| Token | Tech(默认) | Lavender(柔紫) | Child(温暖儿童) | Cocoa(草莓可可) |
|---|---|---|---|---|
| bg        | #000000 | #FAF5FF | #FFF9E6 | #2D1B0E |
| panel     | #0a1e28 | #FFFFFF | #FFFFFF | #3F2B20 |
| accent    | **#22D3EE** | **#9333EA** | **#FF7F50** | **#FB7185** |
| accent_hi | #33C5FF | #A855F7 | #FFAA78 | #FDA4AF |
| text      | #b4ebff | #4C1D95 | #c85a28 | #FEF3C7 |
| border    | #14506e | #E9D5FF | #FFC8A0 | #573D2C |
| danger    | #F43F5E | #EF4444 | #F43F5E | #EF4444 |
| is_light  | 0 | 1 | 1 | 0 |

## 5. v7.6 vs v6.3.x 体系变更速查

| 模块 | v6.3.x | v7.6 |
|---|---|---|
| 主题集 | Tech / Warm / Child / Dev | **Tech / Lavender / Child / Cocoa** |
| Menu 4 点位 | Skills (Package) | **Theme (Palette)** |
| Skills 入口 | Menu 扇区 | **Settings → 拓展功能 → 技能插件** |
| Settings 语言 | 英文 | **中文 5 组** |
| Developer 入口 | 5-tap About | **默认可见,「测试」徽章** |
| Console.System 按钮 | 3 英文 | **6 中文体感** |
| Console.Service 顶 | 3 大卡 + 6 剧本 | **去顶卡,只 6 中文 emoji** |
| StatusBar 模式 | 3 个 | **4 个(+ always)** |
| Boot 动画 | 主题分支 | **统一揉眼** |
| RGB 灯带 | 概念 | **场景态色相** |
| face 颜色 | accent | **accent_hi** |
| 旧主题名 | 报错 | **静默回退 TECH** |

## 6. 下一步动作

1. **PM**:看 `doc3_prd_v3.0.md` § 6 优先级,确认 P0/P1 切分
2. **UX**:把 `DESIGNER_AI_PROMPT_round3.md` 投喂下游设计师 AI 完成 round-3
3. **固件**:照 `doc1_engineering_v7.6.md` § 2 重构目录,§ 4 落地主题系统
4. **后端**:按 `doc2_backend_v2.0.md` § 2 拉起 Persona Registry / Memory Shadow / Auto Router
5. **设计**:14 张主 mock 即像素基线,分歧以 SVG 几何为准

## 7. 联合验收 checklist 入口

每份文档末尾都有自己的 checklist:
- `doc1` 工程(11 项)/ `doc2` 后端(8 项)/ `doc3` 产品(12 项)/ `doc4` UX(10 项)
- `UI_AI_BRIEF_v7.6.md` § 9 PM checklist(9 项)
- `DESIGNER_AI_PROMPT_round3.md` § D round-3 checklist(11 项)

合计 61 个 checkpoint。
