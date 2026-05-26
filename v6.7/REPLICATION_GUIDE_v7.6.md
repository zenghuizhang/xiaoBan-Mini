# claw_xb v7.6 · 完全复刻指导手册

> **目标读者**:负责把本项目按设计稿 1:1 还原成可烧录固件的另一个 AI Agent。
> **目标精度**:像素级 / token 级 / 行为级三层完全对齐,任何偏差都可被本手册直接驳回。
> **基线**:M5Stack CoreS3 · ESP-IDF **5.5.4 LTS** · LVGL **9.5.0** · esp_lvgl_port **2.4+**
> **设计稿来源**:React 原型「陪伴机器人 UI 原型 v7.6」+ 本 bundle `svg_per_theme/` 共 52 张

---

## 0. 复刻验收"四个必过"

任何复刻产物提交前,必须自检以下四项,任何一项不过即视为不合格:

| # | 验收点 | 校验方法 |
|---|---|---|
| 1 | **4 主题背景色完全等同 token 表** | 截图取 (4,4) 像素 → 对照 §2.2 hex 值,误差为 0 |
| 2 | **face 颜色 = `accent_hi`(非 `accent`)** | 截图取眼部中心像素 → 对照 §2.2 表 `accent_hi` 列 |
| 3 | **6 扇区图标使用本手册 `icons_v7.6/` 高清 SVG 几何** | 不允许另出图标。每个图标的 lucide-react 母版名见 §6.1 |
| 4 | **暗主题(tech/cocoa)卡片半透,亮主题(lavender/child)不透明** | 截图取 menu 扇区按钮中心 → alpha = 0x4D 或 0xFF |

---

## 1. 硬件、工具链与依赖锁定

### 1.1 设备

- **MCU**:ESP32-S3-WROOM-1-N16R8(16 MB Flash / 8 MB Octal PSRAM)
- **设备**:M5Stack CoreS3 — 320×240 ST7789 + FT6336U 触摸 + AXP2101 PMU + MPU6886 IMU
- I2C:SDA=GPIO12 · SCL=GPIO11(板载固定)
- SPI/LCD 引脚:由 esp-bsp `m5stack_core_s3` 自动初始化

### 1.2 工具链 — 锁定版本

```bash
# ESP-IDF
git clone --branch v5.5.4 --depth 1 https://github.com/espressif/esp-idf.git
cd esp-idf && ./install.sh esp32s3 && . ./export.sh
```

| 依赖 | 版本 | 来源 |
|---|---|---|
| ESP-IDF | **v5.5.4 LTS** | git tag |
| LVGL | **9.5.0** | idf-component-manager: `lvgl/lvgl ~9.5.0` |
| esp_lvgl_port | ^2.4.0 | `espressif/esp_lvgl_port` |
| esp_sqlite (可选,记忆库) | ^1.0.0 | `espressif/esp_sqlite` |
| BSP | `m5stack/m5stack-core-s3` | 可选,推荐用 |

**严禁版本漂移**。LVGL 9.6 / IDF 5.6 上若干 API 已重命名(`lv_obj_get_event_target` → `lv_event_get_target_obj`)。

### 1.3 `menuconfig` 必改项

```
Component config → ESP PSRAM
  [*] Support for external SPI-connected RAM
  [Octal Mode PSRAM]
  Set RAM size: 8 MB
Component config → LVGL
  [*] Use the custom tick interface
  Color depth: 16
  Default refresh period: 16 ms
Component config → Bootloader
  Bootloader log verbosity: Warning
Partition Table → custom (csv): partitions_16M_ota.csv
```

---

## 2. Token 系统(不可改写的真相源)

### 2.1 结构定义(`assets/themes/theme_tokens.h`)

```c
typedef struct {
    lv_color_t bg, panel, accent, accent_dim, text, text_dim, border, danger, success;
    lv_color_t accent_hi;   // 给 face / RGB 灯带 / glow halo 用
    uint8_t    is_light;    // 0 = 暗主题, 1 = 亮主题
} theme_t;
```

### 2.2 4 主题 hex 值表(**唯一真相**)

| Token | tech(默认 暗) | lavender(亮 紫) | child(亮 暖) | cocoa(暗 玫瑰) |
|---|---|---|---|---|
| **bg** | `#000000` | `#FAF5FF` | `#FFF9E6` | `#2D1B0E` |
| **panel** | `#0A1E28` | `#FFFFFF` | `#FFFFFF` | `#3F2B20` |
| **accent** | `#22D3EE` | `#9333EA` | `#FF7F50` | `#FB7185` |
| **accent_hi** ★face | `#33C5FF` | `#A855F7` | `#FFAA78` | `#FDA4AF` |
| **accent_dim** | `#0F5A78` | `#A855F7` | `#FFAA78` | `#FDA4AF` |
| **text** | `#B4EBFF` | `#4C1D95` | `#C85A28` | `#FEF3C7` |
| **text_dim** | `#6EAAC8` | `#7C3AED` | `#D28C5A` | `#D9C1A0` |
| **border** | `#14506E` | `#E9D5FF` | `#FFC8A0` | `#573D2C` |
| **danger** | `#F43F5E` | `#EF4444` | `#F43F5E` | `#EF4444` |
| **success** | `#22C55E` | `#22C55E` | `#22C55E` | `#22C55E` |
| **is_light** | `0` | `1` | `1` | `0` |
| **卡片 alpha** | `0x4D` (30%) | `0xFF` (100%) | `0xFF` (100%) | `0x4D` (30%) |

### 2.3 卡片透明度规则(全局唯一)

```c
lv_opa_t xb_card_panel_opa(void) {
    return xb_theme_get()->is_light ? LV_OPA_COVER : (lv_opa_t)0x4D;
}
```

**所有** card / button / bubble / modal 的 `bg_opa` 必须调用此函数,**禁止**写死 `LV_OPA_COVER` 或硬编码 alpha。

### 2.4 旧主题名 OTA 静默回退

`xb_theme_lookup()` 对 `warm` / `dev` / `orange` / `blue` / `pink` / `green` / `purple` / `yellow` / `mono` 等旧名一律返回 `&THEME_TECH`,不抛错。这是 v7.6 → v7.x OTA 平滑切换的硬要求。

---

## 3. 复刻 4 主题的常见错误(其他 AI 已犯过)

| 错误现象 | 根因 | 纠正 |
|---|---|---|
| Child 主题画成"米黄 + 黑字"难看 | 把 `text` 当成 `#000`,实际是 `#C85A28`(焦糖橘) | 严格用表 |
| Lavender 卡片看上去和背景"糊在一起" | 把 panel 当 `#FAF5FF`(实际是纯白 `#FFFFFF`) | 亮主题 panel = 纯白 |
| Cocoa 字体不可读 | 把 `text` 当 `text_dim` | text=`#FEF3C7` 是奶油黄,字非常亮 |
| 4 主题 face 颜色都画成 cyan | 用了 `accent` 而不是 `accent_hi` | 任何 face 元素必须 `accent_hi` |
| 暗主题卡片完全不透明 | 没走 `xb_card_panel_opa()` | 强制走该函数 |
| 6 扇区图标各主题里几何不一样 | 用了主题色填充的 emoji | 几何用 `icons_v7.6/`,只换 `stroke` |

---

## 4. 文件结构(必须照搬)

```
components/xb_ui/
├── CMakeLists.txt
├── idf_component.yml
├── assets/
│   ├── themes/theme_tokens.h
│   └── icons/          (81 SVG, lucide-react 母版)
├── src/
│   ├── main.c
│   ├── core/    xb_event{.h,.c}  xb_theme{.h,.c}  xb_face{.h,.c}
│   ├── widgets/ xb_widgets{.h,.c}
│   ├── sim/     xb_persona  xb_memory_store  xb_imu_sim  xb_scenario_sim
│   ├── pages/   xb_pages.h xb_router.c + 12 page_*.c
│   └── assets/  xb_icons_stub.c
└── docs/        INTEGRATION.md  COVERAGE.md
```

任何文件名 / 路径变更 = 不通过。

---

## 5. 启动序列(`main.c::app_main`)

**严格顺序**,任何颠倒都会 NVS 读不到主题或 face 拿不到色:

```
1. nvs_flash_init() / erase+init on NO_FREE_PAGES
2. xb_event_init()
3. xb_theme_init()        // 读 NVS  xb_theme/active,默认 "tech"
4. xb_persona_init()      // 读 NVS  xb_persona/active,默认 "lyra"
5. xb_memory_init()       // 打开 /littlefs/memory.db,失败则 RAM fallback
6. bsp_init()             // 你的板级:lvgl_port_init + 注册 ST7789 + 注册 FT6336U
7. lv_screen_active() 上挂背景色 = xb_theme_get()->bg
8. xb_router_init(scr)    // 进入 PAGE_BOOT,4 步揉眼 → PAGE_HOME
```

---

## 6. 6 扇区菜单(高优先级 - 之前复刻最差的部分)

### 6.1 图标资产 — 直接复用 `icons_v7.6/`

| 扇区 # | 角度 (deg) | 中文 | lucide-react 母版 | 高清 SVG 路径 |
|---|---|---|---|---|
| 0 | -90° (上) | 对话 | `MessageCircle` | `icons_v7.6/<theme>/menu_01_chat_<theme>.svg` |
| 1 | -30° (右上) | 模型 | `Cpu` | `icons_v7.6/<theme>/menu_02_model_<theme>.svg` |
| 2 | +30° (右下,**4 点位**) | 主题 | `Palette` | `icons_v7.6/<theme>/menu_03_theme_<theme>.svg` |
| 3 | +90° (下) | 设置 | `Settings` | `icons_v7.6/<theme>/menu_04_settings_<theme>.svg` |
| 4 | +150° (左下) | 人格 | `User` | `icons_v7.6/<theme>/menu_05_persona_<theme>.svg` |
| 5 | +210° (左上) | 记忆 | `BookOpen` | `icons_v7.6/<theme>/menu_06_memory_<theme>.svg` |

> **`<theme>` ∈ {tech, lavender, child, cocoa, mono}**。Mono 是 256×256 透明背景纯白线,可直接 `tools/svg_to_lvgl.py` 转 LVGL bin。
> 还有 5 张 9-cell 总览图 `OVERVIEW_menu_icons_<theme>.svg`,可直接喂给图像 AI 当 reference 输入。

### 6.2 几何 — 必须精确

| 参数 | 值 |
|---|---|
| 圆盘中心 | `(cx=160, cy=130)` |
| 扇区按钮半径 | `r=78` |
| 按钮直径 | `48`(`radius=24`) |
| 角度阵列 | `[-90, -30, 30, 90, 150, 210]`(度,顺时针,0° 指右) |
| 按钮 `bg_opa` | `xb_card_panel_opa()` |
| 按钮 border | `accent`,宽 1 |
| 图标颜色 | `accent_hi`,顶部 4px |
| 标签颜色 | `text`,底部 4px |
| 中央 face | `FACE_MENU`(28×28 眼,gap 52,微张嘴) |

### 6.3 写死的 LVGL 调用(可直接 copy)

```c
for (int i = 0; i < 6; ++i) {
    float rad = SECTORS[i].angle_deg * 3.14159265f / 180.0f;
    int x = 160 + (int)(cosf(rad) * 78) - 24;
    int y = 130 + (int)(sinf(rad) * 78) - 24;

    lv_obj_t* btn = lv_btn_create(root);
    lv_obj_set_size(btn, 48, 48);
    lv_obj_set_pos(btn, x, y);
    lv_obj_set_style_radius(btn, 24, 0);
    lv_obj_set_style_bg_color(btn, th->panel, 0);
    lv_obj_set_style_bg_opa(btn, xb_card_panel_opa(), 0);  /* ★ */
    lv_obj_set_style_border_color(btn, th->accent, 0);
    lv_obj_set_style_border_width(btn, 1, 0);
    /* icon + label 见 page_menu.c */
}
```

---

## 7. Face 引擎(24 变体,核心心智模型)

### 7.1 不变量

- 颜色 = `theme_t::accent_hi` — **从不**用 `accent`
- 缩放 = **1.6×**(已计入 `G[]` 表)
- 形状:**2 矩形眼 + 1 矩形嘴**,矩形圆角 = 高/2
- Halo:`shadow_width=20`,`shadow_color=accent_hi`,`shadow_opa=0xB3`(70%)
- 嘴 halo:`shadow_width=18`
- 中心 = `(160, 130)`
- 眨眼:`blink_eligible=true` 的变体随机 3-5s 触发,150ms 闭合再开

### 7.2 24 变体精确表 — 见 `firmware/src/core/xb_face.c` 的 `G[FACE_VARIANT_MAX]`

不要重排顺序,不要改字段名,**任何变体的 (eye_w, eye_h, gap, dy, mouth_w, mouth_h, mouth_dy, mouth_radius) 都是有意调过的**。

### 7.3 主题切换时

订阅 `XB_EVT_THEME_CHANGED` → 调 `apply_geom(g_current)`。这是已写好的 `on_theme()` 回调,**不要自己写**。

---

## 8. StatusBar 4 模式(v7.6 新增 ALWAYS)

| Mode | 何时使用 | 行为 |
|---|---|---|
| `IDLE` | Home 默认 | 只显示左上角 1 个 dot |
| `TRANSIENT` | 用户手势 peek | 完整一行,**2.4s 自动收回** |
| `CRITICAL` | 低电 / 断网 | 完整一行 + 颜色变 `danger` |
| `ALWAYS` | settings / wifi / chat | 完整一行,永不消失 |

切模式只能调 `xb_statusbar_set_mode(mode)` 或 post `XB_EVT_STATUSBAR_MODE`。**禁止直接操作 statusbar 子节点**。

---

## 9. RGB 灯带场景色映射

| `rgb_scene_t` | 触发场景 | 色 |
|---|---|---|
| `XB_RGB_IDLE` | 默认 | `accent_hi`(随主题) |
| `XB_RGB_LONELY` | 5 分钟无交互 | `#FACC15` |
| `XB_RGB_OTA` | OTA 进度可见 | `#A855F7` |
| `XB_RGB_ERROR` | 异常 | `#EF4444` |
| `XB_RGB_CALL` | 来电 | `#22C55E` |
| `XB_RGB_LOW_BAT` | < 15 % | `#F97316` |
| `XB_RGB_OVERHEAT` | > 70 °C | `#DC2626` |

LVGL 上是一条 320×2 px 顶部条,带 8px 投影 80% opa。

---

## 10. Settings 5 中文组(顺序、徽章必须一致)

| # | 组名 | 徽章 | 行 | 跳转 |
|---|---|---|---|---|
| 1 | 通用 | — | 主题 | PAGE_THEME_PICKER |
| 2 | 网络 | — | WiFi 配网 | PAGE_WIFI_AP |
| 3 | 模型 | — | 默认模型 | PAGE_MODEL_PICKER |
| 4 | 关于 | — | 技能(即将上线) | PAGE_SKILLS_EMPTY |
| 5 | **开发者** | **`[测试]`** | 控制台 | PAGE_CONSOLE |

⚠ **开发者默认可见**(v6.3 是 5-tap 才开),徽章 `[测试]` 标在组标题右侧。

---

## 11. 控制台 4 中文 Tab

| Tab | 内容 |
|---|---|
| 系统 | 6 体感按钮(前倾/后仰/左倾/右倾/摇晃/旋转)+ 6 情景(语音唤醒/OTA/报错/来电/低电/高温) |
| AI | 3 态切换(空闲/思考/说话) + 弹示例气泡 |
| 显示 | 下一表情 / 下一主题 / 状态栏模式 cycle |
| 服务 | WiFi 状态循环 / 电量 -20% / MCP 切换 / OTA +20% |

按钮统一 `xb_button`,**不要混用** `lv_btn_create` 直造。

---

## 12. 事件总线 21 主题(只能订阅,不能改)

```
XB_EVT_THEME_CHANGED      WIFI_STATE     BATTERY        PLUG
AI_STATE                  MCP_ACTIVE     OTA_PROGRESS   FACE_REQUEST
SCENE_REQUEST             BUBBLE         RGB_SCENE      STATUSBAR_MODE
STATUSBAR_PEEK            PERSONA_CHANGED  MODEL_CHANGED  MEMORY_CHANGED
IMU                       CHAT_DELTA     CHAT_DONE      CHAT_ERROR
                          (XB_EVT_MAX = 哨兵,勿订阅)
```

`MAX_SUBS = 12` 已够 v7.6 全部页面。新增订阅前先想清楚是不是该用现成事件。

---

## 13. 人格 / 记忆 / IMU / 场景

### 13.1 6 人格(顺序必须一致 → 影响 grid 显示)

| id | 中文 | tagline | face_color(React 预览用) |
|---|---|---|---|
| lyra | Lyra | 温柔陪伴 · 慢声细语 | #A855F7 |
| echo | Echo | 回声助手 · 高效专注 | #22D3EE |
| nova | Nova | 活泼能量 · 阳光积极 | #F97316 |
| sage | Sage | 知识导师 · 沉稳博学 | #34D399 |
| pico | Pico | 童趣小友 · 软萌可爱 | #FB7185 |
| doc | Doc | 工程伙伴 · 严谨理性 | #94A3B8 |

NVS 命名空间 `xb_persona`,key `active`,默认 `"lyra"`。

### 13.2 记忆库 schema

```sql
CREATE TABLE memory (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  title TEXT, snippet TEXT, ts INTEGER, persona TEXT);
```

无 sqlite 时 fall back 到 RAM ring(容量 64,最老淘汰)。

---

## 14. 边到边像素栅格(LVGL 坐标系)

| 元素 | x | y | w | h |
|---|---|---|---|---|
| 屏幕 | 0 | 0 | 320 | 240 |
| RGB 条 | 0 | 0 | 320 | 2 |
| 状态栏 | 0 | 0 | 320 | 16 |
| 顶栏 | 0 | 0 | 320 | 28 |
| 顶栏返回键 | 4 | 2 | 28 | 22 |
| 主卡区域 | 12 | 32 | 296 | 196 |
| Menu face 中心 | 160 | 130 | — | — |
| Menu 扇区半径 | — | — | 78 | — |
| Menu 按钮 | -24 偏移 | -24 偏移 | 48 | 48 |
| Toast | 居中底部 | -24 (from bottom) | 220 | 32 |

---

## 15. 复刻交付清单(Agent 提交时必带)

- [ ] `git diff` 仅在 `components/xb_ui/` 范围内
- [ ] `idf.py build` 通过,无 warning 升 error
- [ ] 在模拟器或真机跑出 4 主题各 1 张全屏截图,对应 §2.2 hex
- [ ] 6 扇区菜单实物 vs `svg_per_theme/<theme>/p03_menu_<theme>.svg`,逐扇区比对
- [ ] 24 face 变体能 cycle 一遍(走 Console.显示 → 下一表情)
- [ ] Settings 顺序 = §10
- [ ] OTA 注入老主题名(如 `"warm"`)后,设备启动落回 Tech 而非崩溃
- [ ] `tools/coverage_diff.py` 输出 0 项缺失(若有该工具)

---

## 16. 参考 SVG 资产索引

| 路径 | 用途 |
|---|---|
| `svg_per_theme/<theme>/p*.svg` | **52 张** 每页 ×4 主题完整截图 |
| `icons_v7.6/<theme>/menu_0*_<theme>.svg` | 6 扇区图标 × 5 色 = 30 张独立高清(可直接喂图像 AI) |
| `icons_v7.6/OVERVIEW_menu_icons_<theme>.svg` | 5 张 3×2 总览图 |
| `svg/p*.svg` | 早期 20 张 Tech 主题 mock(保留) |
| `firmware/assets/icons/*.svg` | 81 个 lucide 母版,用于 `tools/svg_to_lvgl.py` |

---

## 17. 严禁清单

| ✗ 禁止 | 原因 |
|---|---|
| 任何主题色硬编码 | 必须走 `theme_t` |
| face 用 `accent` | 设计强制 `accent_hi` |
| 6 扇区图标自创 | 用本 bundle 高清 SVG |
| Skills 入口加回菜单 | v7.6 已降级到 Settings > 关于 |
| Console tab 英文 | 必须中文 |
| Developer 入口隐藏 | 默认可见 + `[测试]` 徽章 |
| 改 24 face 变体几何 | 全部已调过 |
| 升级 LVGL 到 9.6+ / IDF 5.6+ | API 已变,不在锁定范围 |

---

**End of Manual** — 任何模糊点都先回到这份文档,再回到 `theme_tokens.h`,再回到 `svg_per_theme/` 截图。三者之间不允许有冲突。
