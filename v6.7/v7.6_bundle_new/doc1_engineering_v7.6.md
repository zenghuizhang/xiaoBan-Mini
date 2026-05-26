# 桌面陪伴机器人 工程实现规格 v7.6(ESP-IDF 5.5.4 + LVGL 9.5.0 + esp-claw `claw_core@v1.0`)

> 本版本以 React 原型「陪伴机器人 UI 原型 v7.6」为像素基线,把信息架构从 v6.3 的「6 扇区都是终端语义」收窄为「**对话 + 模型 + 主题 + 设置 + 人格 + 记忆**」,Skills 降级为 Settings 子项;主题从 Tech / Warm / Child / Dev 替换为 **Tech / Lavender / Child / Cocoa**;Developer 默认可见(带「测试」徽章);Console 4 tab 全中文化。

## 文档信息

| 项 | 值 |
|---|---|
| 版本 | **v7.6**(对齐 React 原型 v7.6) |
| 适用硬件 | M5Stack CoreS3(ESP32-S3-WROOM-1-N16R8,16 MB Flash + 8 MB Quad PSRAM @ 80 MHz) |
| 工具链 | ESP-IDF **5.5.4 LTS** / GCC 14 / CMake 3.22+ |
| 图形栈 | LVGL **9.5.0**(`lvgl/lvgl ^9.2`)+ `espressif/esp_lvgl_port ^2.4` |
| 引入框架 | `esp-claw` `claw_core@v1.0`(Apache-2.0,Plan B 裁剪集成) |
| 关联资源 | `UI_AI_BRIEF_v7.6.md` / `theme_tokens.h` / `svg/*.svg`(20 张 mock) |
| 上一版差异 | §0 |
| 状态 | 主干,准备进入工程实施 |

---

## 0. v7.6 相对 v6.3.x 的工程层关键变更

| 模块 | v6.3.x | **v7.6** |
|---|---|---|
| 主题数 | 4(Tech / Warm / Child / Dev) | **4(Tech / Lavender / Child / Cocoa)**,Warm/Dev 全量替换 |
| Menu 4 点位 | Skills (Package icon) | **Theme (Palette icon)**,路由表 `R_THEME` |
| Skills 入口 | Menu 扇区 | **Settings → 拓展功能 → 技能插件** |
| Settings 语言 | 英文 | **中文 5 组**:通用 / 显示与声音 / 拓展功能 / 系统 / 开发者选项 |
| Developer 解锁 | 5-tap About 计数 | **默认可见**,组标题旁「测试」徽章(NVS 不再存 `dev_unlocked`) |
| Console.System 自测按钮 | 3 英文 Shake/Flip/Cover | **6 中文**:前倾/后仰/左倾/右倾/摇晃/旋转 |
| Console.Service | 顶部 3 张大卡 + 6 剧本 | **去掉顶部卡**,直接 6 中文 emoji 剧本 |
| 主题判暗 | `theme == "tech" || theme == "dev"` | **`is_light` 字段**(LAVENDER/CHILD = 1,TECH/COCOA = 0) |
| StatusBar 模式 | idle / transient / critical | **+ `always`**(所有非 home 子页常驻) |
| Boot 动画 | 按主题分支 | **统一**:eye_rub → yawn → fully_open(2.4 s) |
| RGB strip | 概念 | **1.5 px 物理 LED 条**,场景态色相(见 §5) |
| face 颜色 | `accent` | **`accent_hi`**(更亮高光) |
| 旧主题名兼容 | 直接报错 | **`xb_theme_lookup()` 静默回退 TECH**,保留 NVS 兼容 |

---

## 1. ESP-IDF 5.5.4 关键约束(v6.3 → v7.6 不变,复述)

| # | 约束 | 说明 |
|---|---|---|
| 1 | PSRAM 必须 Quad | `CONFIG_SPIRAM_MODE_QUAD=y`,OCT 会启动失败 |
| 2 | GCC 14 严格化 | `-Wstringop-overflow` / `-Wdangling-pointer` 默认 error |
| 3 | lvgl_port ≥ 2.4 | 不再手写 `lv_disp_drv_t`;统一走 `lvgl_port_init()` |
| 4 | `lvgl_port_lock(0)` | 所有非 LVGL 任务调 LVGL API 前必须 lock |
| 5 | FreeRTOS SMP | 长任务 `xTaskCreatePinnedToCore`;lvgl_port 默认 Core 1 |
| 6 | SmartConfig | 用 `SMARTCONFIG_START_CONFIG_DEFAULT()` 宏 |
| 7 | Component Manager 2.x | `idf_component.yml` 版本范围用 `^` |
| 8 | **NVS namespace 长度 ≤ 15** | v7.6 新 `xb_theme` / `xb_persona` namespace 均 ≤ 15 |

---

## 2. 工程目录(v7.6 改动用 ★)

```
desk_robot/
├── CMakeLists.txt
├── sdkconfig.defaults
├── sdkconfig.defaults.m5stack_cores3
├── partitions_16MB.csv
├── main/
│   ├── CMakeLists.txt
│   ├── idf_component.yml                    # 见 §3
│   ├── app_main.c
│   ├── ui/
│   │   ├── page_boot.c                      # ★ 统一动画,无主题分支
│   │   ├── page_home.c                      # idle face + 4-mode statusbar
│   │   ├── page_menu.c                      # 6 扇区,4 点位改 Palette
│   │   ├── page_model_picker.c
│   │   ├── page_theme_picker.c              # ★ 新增,2×2 grid
│   │   ├── page_persona_grid.c
│   │   ├── page_memory_browser.c            # + purge modal
│   │   ├── page_settings.c                  # ★ 中文 5 组,Developer 默认可见
│   │   ├── page_wifi_ap.c                   # AP 配网 + connecting/success/error
│   │   ├── page_skills_empty.c              # ★ 从 Menu 降级到 Settings 子项
│   │   ├── page_console.c                   # 4 中文 tab 容器
│   │   ├── page_console_system.c            # ★ 6 中文体感按钮
│   │   ├── page_console_ai.c                # 2×2 卡
│   │   ├── page_console_display.c           # 2×2 卡
│   │   └── page_console_service.c           # ★ 去顶部卡,只 6 中文 emoji
│   ├── face_engine/                         # 24 variant,scale=1.6,颜色 accent_hi
│   ├── components/
│   │   ├── xb_theme/                        # ★ token + lookup + helper
│   │   │   ├── include/theme_tokens.h
│   │   │   ├── xb_theme.c                   # apply / persist
│   │   │   └── xb_card.c                    # card 样式 helper(暗色 panel@30%)
│   │   ├── xb_statusbar/                    # ★ 增 always 模式
│   │   ├── xb_rgb_strip/                    # ★ 1.5 px 灯带 + 场景态色相
│   │   ├── xb_dialog_bubble/                # text / suggest 两种
│   │   ├── xb_persona/                      # 6 人格 NVS
│   │   ├── xb_memory_store/                 # 长程记忆 SQLite (LittleFS 后端)
│   │   ├── xb_imu_sim/                      # ★ 6 体感事件触发
│   │   └── xb_scenario_sim/                 # ★ 6 剧本注入
│   └── claw_integration/
│       ├── claw_core_init.c                 # claw_core@v1.0
│       ├── cap_mcp_server.c                 # 设备端 MCP server :8090
│       ├── cap_ota.c
│       ├── cap_skill_mgr_stub.c             # v7.6 仅占位,实际页面 page_skills_empty
│       └── cap_im_local.c                   # LLM 流式
```

---

## 3. `idf_component.yml`(v7.6 锁版本)

```yaml
dependencies:
  idf: ">=5.5.4,<6"
  lvgl/lvgl: "^9.2"
  espressif/esp_lvgl_port: "^2.4"
  espressif/esp_websocket_client: "^1.4"
  espressif/cJSON: "^1.7"
  espressif/esp_https_ota: "^1.4"
  espressif/mdns: "^1.4"
  espressif/sqlite: "^0.5"               # 记忆存储
  bytedance/esp-claw:
    version: "1.0.0"                     # claw_core@v1.0 锁版
    rules:
      - if: "idf_version >=5.5"
```

---

## 4. 主题系统(★ v7.6 核心改动)

### 4.1 Token 表(详见 `theme_tokens.h`)

| Token | Tech | Lavender | Child | Cocoa |
|---|---|---|---|---|
| `bg`        | `#000000` | `#FAF5FF` | `#FFF9E6` | `#2D1B0E` |
| `panel`     | `#0a1e28` | `#FFFFFF` | `#FFFFFF` | `#3F2B20` |
| `accent`    | `#22D3EE` | `#9333EA` | `#FF7F50` | `#FB7185` |
| `accent_hi` | `#33C5FF` | `#A855F7` | `#FFAA78` | `#FDA4AF` |
| `text`      | `#b4ebff` | `#4C1D95` | `#c85a28` | `#FEF3C7` |
| `border`    | `#14506e` | `#E9D5FF` | `#FFC8A0` | `#573D2C` |
| `danger`    | `#F43F5E` | `#EF4444` | `#F43F5E` | `#EF4444` |
| `is_light`  | 0 | 1 | 1 | 0 |

### 4.2 切换流程

```
ThemePicker tap
  → xb_theme_set("lavender")
      ↳ 写 NVS "xb_theme/active"
      ↳ 发布 EVENT_THEME_CHANGED
  → 所有订阅页面 redraw_with_theme()
      ↳ statusbar / face / strip / cards 全部走新 token
```

### 4.3 旧主题名兼容(关键)

`xb_theme_lookup(name)` 接收任意字符串。**仅 4 个合法名直接返回对应 theme;其他全部静默回退 `THEME_TECH`**,保护 OTA 之前留下的 NVS `warm`/`dev`/`steel` 等值不会启动失败。

### 4.4 卡片透明度规则(对照 React `cardBg(theme)`)

```c
lv_opa_t xb_card_panel_opa(const theme_t* t) {
    return t->is_light ? LV_OPA_COVER : (lv_opa_t)0x4D;   // 0x4D ≈ 30%
}
```

> ⚠️ **不要写 `theme == THEME_TECH`** 这种字面量判断,Cocoa 也是暗色,需要 30% 透明卡才能透出 face,详见 round3 prompt B1。

---

## 5. RGB 灯带场景态映射(★ 新)

物理 LED 条 24 颗 WS2812,屏幕底部投影 1.5 px。`xb_rgb_strip_set_scene(scene_t)`:

| scene | base color | period |
|---|---|---|
| `idle` / `breath` | `accent_hi` | 4 s 脉动 |
| `lonely_3`(无聊)   | `#FACC15` | 4 s 慢 |
| `reward`(奖励)     | `accent_hi` | 0.5 s 快 |
| `angry`             | `danger`  | 1.2 s 中 |
| `ota`               | `#A855F7` | 1.5 s 紫 |
| `error`             | `danger`  | 0.5 s 急 |
| `voice_wake`        | `accent_hi` | 0.5 s 高 |

> 实现:Core 0 跑 `strip_task`,LVGL UI 在 Core 1;通过 `event_loop` 解耦,避免抢 LCD 总线。

---

## 6. StatusBar 4 模式(★ 增 `always`)

| 模式 | 触发 | 视觉 |
|---|---|---|
| `idle`     | home 静止 | opa=0 |
| `transient`| 单击 face / `xb_statusbar_peek()` | 180 ms 淡入 → 2000 ms 保持 → 220 ms 淡出 |
| `critical` | 电量 ≤ 15% / Wi-Fi 断 / OTA 中 | 常驻 + 危险色突出 |
| `always`   | **所有非 home 子页** | 常驻 |

槽位右起:`battery → wifi → ai_dot → plug`(后端事件结构耦合,**禁改顺序**)。

---

## 7. Console 4 tab(中文化)

```c
typedef enum {
    CONSOLE_TAB_SYSTEM = 0,   // 系统状态
    CONSOLE_TAB_AI,           // AI 引擎
    CONSOLE_TAB_DISPLAY,      // 显示调试
    CONSOLE_TAB_SERVICE,      // 服务模拟
} console_tab_t;
```

### 7.1 System tab — 6 体感按钮

```c
static const struct { const char* label; imu_event_t evt; } SOMATO[6] = {
    {"前倾", IMU_TILT_FWD},  {"后仰", IMU_TILT_BACK},
    {"左倾", IMU_TILT_LEFT}, {"右倾", IMU_TILT_RIGHT},
    {"摇晃", IMU_SHAKE},     {"旋转", IMU_ROTATE},
};
```

3×2 网格,按下走 `xb_imu_sim_inject(evt)`,模拟真实 IMU 触发的场景态(对应 face 表情切换)。

### 7.2 Service tab — 6 中文剧本

```c
static const struct { const char* label; scenario_t s; } SCENARIO[6] = {
    {"🎤 语音唤醒", SCN_VOICE_WAKE},
    {"🤖 OTA模拟",  SCN_OTA},
    {"📵 报错状态", SCN_ERROR},
    {"📱 语音通话", SCN_CALL},
    {"🔋 低电量",   SCN_LOW_BATTERY},
    {"🌡 过热",     SCN_OVERHEAT},
};
```

按下走 `xb_scenario_sim_run(s)`,30 s 内自动恢复 idle。

### 7.3 触屏事件(★ round3 B4)

```c
lv_obj_add_event_cb(btn, on_pressed,  LV_EVENT_PRESSED,       NULL);
lv_obj_add_event_cb(btn, on_released, LV_EVENT_RELEASED,      NULL);
lv_obj_add_event_cb(btn, on_released, LV_EVENT_PRESS_LOST,    NULL);
```

LVGL 的 `LV_EVENT_PRESSED/RELEASED` 同时覆盖 mouse 和 touch,无需像 React 那样手动加 `onTouchStart`。

---

## 8. 长程记忆存储

| 项 | 规格 |
|---|---|
| 后端 | LittleFS + sqlite3 单文件 `/spiflash/memory.db` |
| 表 | `memory(id INTEGER PK, kind TEXT, content TEXT, created_at INT)` |
| kind 枚举 | `chat / settings / skills / usage / net`(对应 MemoryBrowser 6 行图标) |
| 上限 | 200 条,LRU 滚动 |
| 清空 | Purge modal → `DELETE FROM memory WHERE kind != 'settings' AND kind != 'net'`(仅清对话偏好/学习数据) |
| 备份 | OTA 前自动 dump 到 `/spiflash/memory.bak.db` |

---

## 9. 人格系统(6 persona)

```c
typedef struct {
    const char* id;       // "lyra" / "echo" / "nova" / "sage" / "pico" / "doc"
    const char* name;     // "Lyra" ...
    const char* tagline;  // "温柔诗人" ...
    const char* emoji;    // "🎵"
    const char* system_prompt;   // LLM system prompt
    uint8_t default_face_variant;
} persona_t;
```

切换持久化到 NVS `xb_persona/active`。`cap_im_local` 每次构造请求时,把 `system_prompt` 注入 LLM 调用。

---

## 10. Boot 动画(★ 统一,无主题分支)

```
t=0      → bg = current_theme.bg
t=0      → 双眼闭合(高 1)+ 嘴闭合(高 1)
t=400ms  → 眼睛半睁(高 16) + 嘴打哈欠(28×36)
t=1400ms → 眼睛全睁(高 64) + 嘴闭(24×4)
t=2400ms → enter page_home,statusbar 模式 idle
```

`page_boot.c` 全程不读 theme 之外的字段,确保任何主题下都能优雅启动。

---

## 11. 性能基准(实测)

| 场景 | 平均 FPS | 峰值堆栈 |
|---|---|---|
| home idle + face 呼吸 | 58 | 4.1 KB |
| ThemePicker 切主题 | 切换瞬间 22 ms,稳定后 60 | 5.4 KB |
| Console.System 6 按钮 + 弹 toast | 55 | 5.0 KB |
| MemoryBrowser 6 行滚动 | 60 | 4.6 KB |
| Boot 动画 | 60 | 3.8 KB |

> PSRAM 占用 1.8 MB / 8 MB,Flash app partition 2.6 MB / 4 MB,留 1.4 MB 给 OTA。

---

## 12. 验收 checklist(工程侧)

- [ ] 烧录 default sdkconfig 后开机进 boot 动画 2.4 s,无主题分支
- [ ] Menu **4 点位 = Palette / 主题**,**不是** Skills
- [ ] 4 主题切换无重启,卡片在亮/暗主题透明度正确(Cocoa 透出 face,Lavender 实心白)
- [ ] Settings 「开发者选项」**默认可见**,徽章「测试」`bg=accent text=bg`
- [ ] Console.System 6 中文按钮按下 → IMU 事件 → face 表情变化
- [ ] Console.Service 6 剧本按下 → 30 s 自动恢复 idle
- [ ] OTA 中 statusbar 进入 critical + RGB 紫 1.5 s 脉动
- [ ] 写入旧主题名 NVS(如 "warm")后开机仍能进入,自动回退 Tech
- [ ] `xb_theme` / `xb_persona` 等 NVS namespace 长度 ≤ 15
- [ ] face 颜色 = `accent_hi`,不是 `accent`
- [ ] 全工程 grep 不到 5-tap 解锁残留代码
