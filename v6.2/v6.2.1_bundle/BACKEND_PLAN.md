# xiaoBan-Mini v6.2.1 Backend Architecture Plan

> Hardware: M5Stack CoreS3 (ESP32-S3, 16MB Flash, 8MB Quad PSRAM)
> Firmware: ESP-IDF 5.5.4 + LVGL 9.5.0 + esp_lvgl_port 2.4+
> Agents: claw_core + claw_memory + claw_event_router + claw_skill + claw_cap
> Backend: Single K8s cluster MVB (API Gateway + LLM Gateway + Session + OTA + Telemetry)

---

## 1. Partition Layout (16MB Flash)

```
# Name      Type  SubType   Offset     Size        Notes
nvs,        data, nvs,      0x9000,    0x6000,     24KB NVS (WiFi creds, settings)
phy_init,   data, phy,      0xf000,    0x1000,     4KB PHY calibration
factory,    app,  factory,  0x10000,   0x100000,    1MB factory rescue (immutable)
ota_0,      app,  ota_0,    ,          0x400000,    4MB active/standby A
ota_1,      app,  ota_1,    ,          0x400000,    4MB active/standby B
storage,    data, fat,      ,          0x400000,    4MB FATFS (logs, cache, NVS backup)
claw,       data, nvs,      ,          0x40000,     256KB claw_memory NVS partition
skills,     data, spiffs,   ,          0x200000,    2MB SPIFFS skill scripts (Step 3)
```

Rationale:
- A/B dual 4MB partitions on a 16MB flash with ota_0/1 sub-type enables native ESP-IDF rollback.
- factory (1MB) is a rescue image burned at manufacturing; never OTA'd.
- storage (4MB FATFS) holds persistent settings backup, crash logs, cached assets.
- claw (256KB NVS) is dedicated to `claw_memory` for structured fact/preference storage.
- skills (2MB SPIFFS) stores skill Lua scripts. If not doing Step 3, merge into storage (6MB).

## 2. OTA Architecture (5-Stage: Check / Download / Verify / Apply / Done)

### 2.1 Component: `components/ota_manager/`

New component wrapping `esp_https_ota` with lifecycle management.

```
ota_manager/
  ota_manager.c          # state machine + esp_https_ota wrapper
  include/ota_manager.h  # public API
  CMakeLists.txt
```

API surface:

```c
typedef enum {
    OTA_STAGE_CHECKING,     // fetching /v1/ota/manifest
    OTA_STAGE_DOWNLOADING,  // esp_https_ota download with resume
    OTA_STAGE_VERIFYING,    // SHA-256 + RSA-3072 signature check
    OTA_STAGE_APPLYING,     // writing to inactive ota_X + marking bootable
    OTA_STAGE_DONE,         // reboot countdown (5s)
    OTA_STAGE_FAILED,       // error with rollback
} ota_stage_t;

typedef void (*ota_progress_cb_t)(ota_stage_t stage, int pct, const char* msg, void* ctx);

esp_err_t ota_manager_init(const char* manifest_url, ota_progress_cb_t cb, void* ctx);
esp_err_t ota_manager_check(void);            // GET manifest -> 304 (no update) or 200
esp_err_t ota_manager_start_update(const char* firmware_url, const char* expected_sha256);
esp_err_t ota_manager_abort(void);
```

### 2.2 State Machine

```
  CHECKING                    DOWNLOADING               VERIFYING
     |                           |                          |
     v                           v                          v
  GET /v1/ota/manifest      esp_https_ota with         SHA-256 hash check
  ?sn=xxx&cur=v1.2.3        Range: bytes= offset        + RSA-3072 signature
     |                      (resume support)            against trusted pubkey
     |                           |                          |
  304 -> no update               |                    fail? -> FAILED
  200 -> url+sha256              v                          |
     |                      progress callback          pass? -> APPLYING
     v                      every 4KB chunk                 |
  start_update(url,sha256)       |                          v
                                 v                     esp_ota_set_boot_partition(
                             100% complete             esp_ota_get_next_update_partition())
                                 |                    -> mark valid, set boot
                                 v                          |
                              VERIFYING                    v
                                                        DONE (countdown 5s)
                                                           |
                                                           v
                                                     esp_restart()
```

### 2.3 Rollback Strategy

ESP-IDF built-in mechanism:

1. `esp_ota_set_boot_partition()` marks the newly written ota_X partition as bootable.
2. Bootloader increments `ota_seq` in otadata partition.
3. If app boots successfully, call `esp_ota_mark_app_valid_cancel_rollback()` in app_main early.
4. If app fails to boot 3 consecutive times (CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE), bootloader automatically reverts to the previous working partition.

Rollback configuration in sdkconfig:

```
CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y
CONFIG_BOOTLOADER_APP_ANTI_ROLLBACK=n       # allow downgrade (dev phase)
CONFIG_BOOTLOADER_NUM_PIN_APP=3             # 3 failed boots -> rollback
```

### 2.4 Critical Constraints

- **Write-flash stage**: shield all touch input and menu entry (rule 32); face = sleep (eyes closed); backlight forced ON.
- **No concurrent flash ops**: disable RGB LED animations, breathing, face carousel during download/verify/apply phases (avoid SPI/Flash contention).
- **Network resume**: `esp_https_ota` with `Range: bytes=` header supports resumption after power loss or network drop.
- **Manifest cache TTL**: cache the 304 response for 30 minutes to avoid polling per boot.

## 3. AI Chat Architecture (SSE Streaming + Expression Sync)

### 3.1 Data Flow

```
  User Input (page_chat.c)
       |
       v
  xb_event_post(XB_EVT_FACE_REQUEST, FACE_THINKING)   --> face_engine
       |
       v
  ui_bridge_submit_chat(user_text)
       |
       v
  claw_core_submit(&request)                            --> claw_core_task (Core 0)
       |
  claw_core starts Agent Loop:
    1. Collect system prompt + context (claw_core_context_provider_t chain)
    2. POST /v1/chat (SSE) to LLM Gateway
       |
       ├── SSE: data: {"delta":"你"}  --> ui_bridge_on_token(token)
       |       xb_event_post(XB_EVT_AI_STATE, AI_THINKING)
       |       xb_event_post(XB_EVT_FACE_REQUEST, FACE_TALKING) [on first delta]
       |       xb_event_post(XB_EVT_CHAT_DELTA, token)
       |       page_chat.c: append to active bubble
       |
       ├── SSE: data: {"tool_call":{"name":"weather.query","args":{...}}}
       |       --> claw_cap_call() executes tool
       |       --> result fed back to LLM for next turn
       |
       ├── SSE: data: [DONE]
       |       --> ui_bridge_on_done()
       |       xb_event_post(XB_EVT_CHAT_DONE, NULL)
       |       xb_event_post(XB_EVT_FACE_REQUEST, FACE_HAPPY_BLINK)
       |       page_chat.c: mark stream complete
       |
       └── SSE error / timeout
               --> ui_bridge_on_error()
               xb_event_post(XB_EVT_CHAT_ERROR, err_msg)
               xb_event_post(XB_EVT_FACE_REQUEST, FACE_SAD)
               page_chat.c: show inline error + retry button
```

### 3.2 claw_core Configuration for Device

```c
// claw_core acts as the SSE consumer on Core 0
claw_core_config_t ccfg = {
    .api_key             = device_jwt,               // device-signed JWT
    .backend_type        = "openai_compatible",
    .model               = "qwen2.5-7b",             // default; settable per settings
    .base_url            = "https://api.mirabot.io/v1",
    .max_tokens          = 1024,
    .timeout_ms          = 45000,                    // 45s total, 1.2s P95 first token
    .supports_tools      = true,
    .supports_vision     = false,                    // Phase 1 no vision
    .task_stack_size     = 12 * 1024,
    .task_priority       = 5,
    .task_core           = 0,                        // offload from LVGL Core 1
    .max_tool_iterations = 6,
    .request_queue_len   = 4,
    .response_queue_len  = 8,

    // Callbacks into device layer
    .persist_session     = claw_memory_persist_session_callback,
    .request_gate        = claw_memory_request_gate_callback,
    .on_request_start    = claw_memory_request_start_callback,
    .collect_stage_note  = claw_memory_stage_note_callback,
    .call_cap            = claw_cap_call_from_core,  // execute tools
};
```

### 3.3 UI Bridge (`components/ui_bridge/`)

The ui_bridge component is the glue between claw_core callbacks (on Core 0) and LVGL UI (on Core 1).

```c
// All UI bridge callbacks MUST acquire lvgl_port_lock(0) before touching LVGL objects,
// then unlock after. This is rule 26 (cross-core LVGL access).

void ui_bridge_on_token(const claw_core_request_t* req, const char* token, void* ctx) {
    lvgl_port_lock(0);
    static bool first_token = true;
    if (first_token) {
        xb_event_post(XB_EVT_AI_STATE, (void*)AI_STATE_TALKING);
        first_token = false;
    }
    xb_event_post(XB_EVT_CHAT_DELTA, (void*)token);
    lvgl_port_unlock();
}

void ui_bridge_on_done(const claw_core_request_t* req, void* ctx) {
    lvgl_port_lock(0);
    xb_event_post(XB_EVT_CHAT_DONE, NULL);
    xb_event_post(XB_EVT_AI_STATE, (void*)AI_STATE_IDLE);
    lvgl_port_unlock();
}
```

### 3.4 Context Provider (Device Metadata)

Every chat request carries device context injected via `claw_core_context_provider_t`:

```c
esp_err_t device_context_provider(const claw_core_request_t* req,
                                   claw_core_context_t* out, void* ctx) {
    // Injects: battery%, current theme, uptime, WiFi RSSI, locale, timezone
    // Output is a JSON string appended to the system prompt
    char buf[512];
    snprintf(buf, sizeof(buf),
        "{\"device\":\"M5Stack CoreS3\",\"battery\":%d,\"theme\":\"%s\","
        "\"rssi\":%d,\"free_heap\":%d,\"locale\":\"zh-CN\"}",
        bsp_pmu_get_battery_pct(), theme_get_name(),
        wifi_get_rssi(), esp_get_free_heap_size());
    out->kind = CLAW_CORE_CONTEXT_KIND_SYSTEM_PROMPT;
    out->content = strdup(buf);
    return ESP_OK;
}
```

### 3.5 Expression Timing Contract

```
t=0ms     User taps send
           face: IDLE -> THINKING (eyes narrow, blink rate x1.5, 200ms ease)
t=300ms   Thinking dots (MI-01) appear in chat bubble
t=~1200ms First SSE delta arrives
           face: THINKING -> TALKING (mouth cycle 400ms loop)
           thinking dots replaced by first text fragment
t=DONE    SSE stream ends
           face: TALKING -> HAPPY_BLINK (800ms) -> IDLE
t=ERROR   SSE fails / timeout
           face: TALKING -> SAD (2000ms) -> IDLE
           inline error bubble with retry button
t=STOP    User taps "stop generating"
           cancel SSE within 500ms, face -> IDLE
```

## 4. Skills Store Architecture

### 4.1 Component: `components/skill_manager/`

New component bridging the Skills UI with claw_skill and claw_memory.

```
skill_manager/
  skill_manager.c
  include/skill_manager.h
  CMakeLists.txt
```

API surface:

```c
typedef struct {
    char id[64];
    char name[64];
    char summary[256];
    char version[16];
    char author[64];
    size_t size_bytes;
    bool installed;
    bool enabled;
    uint32_t updated_at;
} skill_info_t;

esp_err_t skill_manager_init(void);
esp_err_t skill_manager_sync_catalog(const char* catalog_url);  // GET /v1/skills?board=cores3
esp_err_t skill_manager_list_catalog(skill_info_t** out_list, size_t* out_count);
esp_err_t skill_manager_list_installed(skill_info_t** out_list, size_t* out_count);
esp_err_t skill_manager_install(const char* skill_id, const char* session_id);
esp_err_t skill_manager_uninstall(const char* skill_id, const char* session_id);
esp_err_t skill_manager_enable(const char* skill_id, const char* session_id);
esp_err_t skill_manager_disable(const char* skill_id, const char* session_id);
esp_err_t skill_manager_get_detail(const char* skill_id, skill_info_t* out_info);
```

### 4.2 Install Flow

```
  page_skills.c: user taps "Install" on a skill
       |
       v
  page_skill_detail.c: show permissions + checkbox
       |
       v (user checks "I understand and agree")
  skill_manager_install(skill_id, session_id)
       |
       ├── 1. GET /v1/skills/{id}/pkg        (tar.gz signed)
       ├── 2. Verify RSA signature
       ├── 3. Extract SKILL.md + Lua source to /skills/{id}/
       ├── 4. claw_skill_reload_registry()     (re-index skills dir)
       ├── 5. claw_skill_activate_for_session(session_id, skill_id)
       └── 6. Return success; page_skills refreshes list
```

### 4.3 Enabled/Disabled Toggle

```c
// When user toggles a skill in page_skills:
//   Enable  -> claw_skill_activate_for_session(session_id, skill_id)
//   Disable -> claw_skill_deactivate_for_session(session_id, skill_id)
//
// The claw_skill context provider injects active skill SKILL.md content
// into the system prompt so the LLM knows what tools are available.
// Capability groups (cap_groups) from each active skill are registered
// via claw_cap_set_session_llm_visible_groups() so the LLM can call them.
```

### 4.4 Skill Sandbox

- Lua runtime via `cap_lua`; no `io`/`os` libraries.
- `net.http` whitelist only (approved API domains in skill manifest).
- CPU time slice: 100ms max per execution.
- Memory: 64KB Lua heap per skill.
- Static scanning on backend before publish (dangerous API blacklist).

### 4.5 Storage Layout

```
/skills/                         (SPIFFS, 2MB)
  registry.json                  (index: all installed skill IDs + versions)
  weather_today/
    SKILL.md                     (skill description for LLM prompt)
    weather_today.lua            (Lua source)
    icon.png                     (32x32)
  alarm/
    SKILL.md
    alarm.lua
  pomodoro/
    SKILL.md
    pomodoro.lua
```

## 5. Event Flow Architecture

### 5.1 Event Bus (xb_event / claw_event_router layered)

```
┌──────────────────────────────────────────────────────────────┐
│                    claw_event_router (Core 0)                 │
│  Routes between caps: IM inbound -> agent -> outbound reply   │
│  Owns: rules.json, cap routing, session routing               │
└────────────────────┬─────────────────────────────────────────┘
                     │
                     ▼
┌──────────────────────────────────────────────────────────────┐
│                    xb_event (Core 1, LVGL thread-safe)        │
│  UI-internal bus: pages subscribe, publishers push             │
│  Owns: theme, face, chat_delta, ota_progress, wifi_state      │
└──────────────────────────────────────────────────────────────┘
```

### 5.2 Defined Events (xb_event_id_t)

| Event | Publisher | Data Source | Subscribers |
|---|---|---|---|
| `XB_EVT_THEME_CHANGED` | `xb_theme_set()` | NVS `settings/theme` | All pages (re-apply styles) |
| `XB_EVT_WIFI_STATE` | `wifi_event_handler` | esp-netif `IP_EVENT_*` | statusbar, wifi pages |
| `XB_EVT_BATTERY` | `bsp_pmu_task` (2Hz) | AXP2101 register | statusbar |
| `XB_EVT_AI_STATE` | `ui_bridge::on_token/done` | claw_core SSE | statusbar (ai_dot animation) |
| `XB_EVT_MCP_ACTIVE` | `cap_mcp_server::on_session` | MCP connection state | statusbar (plug icon) |
| `XB_EVT_OTA_PROGRESS` | `ota_manager::progress_cb` | esp_https_ota callback | page_ota |
| `XB_EVT_FACE_REQUEST` | any page (throttled 100ms) | pages requesting expression | xb_face (debounced bridge to face_engine) |
| `XB_EVT_CHAT_DELTA` | `ui_bridge::on_token` | claw_core SSE delta | page_chat |
| `XB_EVT_CHAT_DONE` | `ui_bridge::on_done` | claw_core completion | page_chat, xb_face |
| `XB_EVT_CHAT_ERROR` | `ui_bridge::on_error` | claw_core error/timeout | page_chat, xb_face |

### 5.3 Cross-Core Synchronization

```
Core 0 (IO-heavy)                          Core 1 (UI)
─────────────────                          ────────────
claw_core_task                             lvgl_port_task (prio 4)
  ├─ SSE recv                               ├─ LVGL render loop
  ├─ tool execution (claw_cap_call)          ├─ face_engine_task (prio 4, same core)
  └─ ui_bridge callbacks                     │   └─ breathing + expression animation
       │                                     │
       ├── lvgl_port_lock(0)  ────────────> ├── xb_event_post() in UI context
       ├── xb_event_post(...)               ├── page subscribers react
       └── lvgl_port_unlock() <─────────────└── update LVGL objects in-place

wifi_manager_task (Core 0, prio 6)         bsp_pmu_task (Core 0, prio 3)
  ├─ STA/AP state machine                    ├─ 2Hz battery poll
  └─ callback -> lvgl_port_lock               └─ xb_event_post(XB_EVT_BATTERY, pct)
       -> xb_event_post(WIFI_STATE)
```

### 5.4 Page Lifecycle Event Pattern

Every page subscribes on create and **must** clean up on delete:

```c
lv_obj_t* page_chat_create(lv_obj_t* parent) {
    chat_ctx_t* ctx = lv_malloc(sizeof(chat_ctx_t));

    // Subscribe with LV_EVENT_DELETE cleanup
    xb_event_subscribe(XB_EVT_CHAT_DELTA, on_chat_delta, ctx);
    xb_event_subscribe(XB_EVT_CHAT_DONE,  on_chat_done,  ctx);
    xb_event_subscribe(XB_EVT_CHAT_ERROR, on_chat_error, ctx);
    xb_event_subscribe(XB_EVT_THEME_CHANGED, on_theme, ctx);

    lv_obj_add_event_cb(page_root, on_delete, LV_EVENT_DELETE, ctx);
    return page_root;
}

static void on_delete(lv_event_t* e) {
    chat_ctx_t* ctx = lv_event_get_user_data(e);
    xb_event_unsubscribe_all(ctx);   // clean up subscriptions
    lv_free(ctx);
}
```

### 5.5 Face Engine Bridge (xb_face)

`xb_face` is the **single authorized** bridge to face_engine. All pages must go through it, never call face_engine directly.

```c
// xb_event_subscribe(XB_EVT_FACE_REQUEST, ...)
// Debounce: if two requests arrive within 100ms, only the later one executes.
// Priority: TALKING > THINKING > HAPPY > SAD > IDLE > others

void xb_face_on_request(xb_event_id_t id, void* payload, void* user) {
    face_state_t requested = (face_state_t)(intptr_t)payload;
    // Priority filter
    if (g_current_face == FACE_TALKING && requested != FACE_TALKING && requested != FACE_HAPPY_BLINK)
        return;  // don't interrupt talking animation
    if (g_face_timer) lv_timer_del(g_face_timer);
    face_engine_set(requested);
    if (requested == FACE_TALKING || requested == FACE_HAPPY_BLINK || requested == FACE_SAD) {
        g_face_timer = lv_timer_create(return_to_idle, duration_for(requested), NULL);
    }
}
```

## 6. Task / Core Allocation

| Task | Core | Priority | Stack | Notes |
|---|---|---|---|---|
| `lvgl_port_task` | 1 | 4 | 8KB | UI rendering, esp_lvgl_port managed |
| `face_engine_task` | 1 | 4 | 4KB | Breathing + expression, same core as LVGL (no lock needed) |
| `claw_core_task` | 0 | 5 | 12KB | LLM dispatch, SSE streaming, tool orchestration |
| `wifi_manager_task` | 0 | 6 | 4KB | STA/AP state machine, event callbacks |
| `http_reuse_pool` | 0 | 5 | 6KB x N | Shared HTTP client pool for REST/SSE |
| `cap_lua_executor` | 0 | 4 | 6KB | Lua sandbox for skill execution |
| `bsp_pmu_task` | 0 | 3 | 2KB | 2Hz battery/temp polling from AXP2101 |
| `ota_manager_task` | 0 | 5 | 8KB | OTA download/verify state machine (created on demand) |

## 7. Startup Sequence (Rule 29)

```c
void app_main(void) {
    // 1. NVS + settings
    ESP_ERROR_CHECK(nvs_flash_init());
    settings_init();
    app_config_load(&g_config);    // theme, wifi, brightness, volume

    // 2. Hardware init (board_manager or bsp_cores3)
    bsp_display_init();            // lvgl_port_add_disp + lvgl_port_add_touch (Core 1)
    bsp_pmu_init();                // AXP2101
    bsp_imu_init();                // BMI270 (optional)

    // 3. LVGL + face_engine
    lvgl_port_init(&lv_cfg);       // fixed to Core 1, priority 4
    face_engine_start();           // background expression state machine (Core 1)

    // 4. Network: STA first, fall back to AP+Captive
    wifi_manager_start(&wcfg);
    captive_dns_start();

    // 5. Agent infrastructure (Core 0)
    claw_memory_init(&mem_cfg);
    claw_event_router_init(&er_cfg);
    claw_core_init(&core_cfg);
    claw_core_start();
    claw_event_router_start();

    // 6. UI bridge: wire claw_core callbacks -> xb_event
    ui_bridge_init();
    register_context_providers();  // device state injection
    register_completion_observers(); // on_done -> event

    // 7. Boot UI
    xb_event_init();
    xb_theme_set(saved_theme);
    xb_router_init(lv_screen_active());  // PAGE_BOOT -> PAGE_HOME

    // 8. Self-test: mark OTA valid (only if booted successfully)
    esp_ota_mark_app_valid_cancel_rollback();

    // 9. Main loop
    while (1) {
        lv_timer_handler();        // LVGL tick
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}
```

## 8. New Files / Components to Create

### 8.1 New ESP-IDF Components

```
cores3-espidf/components/
  ├── ota_manager/                    # NEW  OTA state machine + esp_https_ota
  │   ├── CMakeLists.txt
  │   ├── ota_manager.c
  │   └── include/ota_manager.h
  │
  ├── ui_bridge/                      # NEW  claw_core <-> xb_event glue
  │   ├── CMakeLists.txt
  │   ├── ui_bridge.c                 #   callbacks + context provider
  │   └── include/ui_bridge.h
  │
  ├── skill_manager/                  # NEW  skill install/enable/disable wrappers
  │   ├── CMakeLists.txt
  │   ├── skill_manager.c
  │   └── include/skill_manager.h
  │
  ├── ui_core/                        # REFACTOR  (absorb full_replica_v6.2/src/)
  │   ├── CMakeLists.txt              #            updated
  │   ├── core/                       # NEW subdir
  │   │   ├── xb_event.c / xb_event.h
  │   │   ├── xb_theme.c / xb_theme.h
  │   │   └── xb_face.c / xb_face.h
  │   ├── widgets/                    # NEW subdir
  │   │   └── xb_widgets.c / xb_widgets.h
  │   ├── pages/                      # NEW subdir (11 pages)
  │   │   ├── xb_pages.h
  │   │   ├── page_boot.c
  │   │   ├── page_home.c
  │   │   ├── page_menu.c
  │   │   ├── page_chat.c
  │   │   ├── page_wifi_ap.c
  │   │   ├── page_wifi_pair.c
  │   │   ├── page_ota.c
  │   │   ├── page_skills.c
  │   │   ├── page_skill_detail.c
  │   │   ├── page_settings.c
  │   │   └── page_console.c
  │   └── assets/                     # NEW subdir
  │       └── ic_*.c                  # 81 lv_img_conv generated icons
  │
  └── (existing components unchanged)
      ├── bsp_cores3/
      ├── wifi_manager/               # already migrated from esp-claw
      ├── captive_dns/                # already migrated
      ├── claw_core/                  # already migrated
      ├── claw_cap/                   # already migrated
      ├── claw_event_router/          # already migrated
      ├── claw_memory/                # already migrated
      ├── claw_skill/                 # already migrated
      ├── claw_ramfs/                 # already migrated
      ├── cap_im_platform/            # already migrated
      ├── cap_mcp_server/             # already migrated
      ├── cap_mcp_client/             # already migrated
      ├── settings/                   # already migrated
      └── http_reuse/                 # already migrated
```

### 8.2 New Main-Level Files

```
cores3-espidf/main/
  ├── app_main.cpp                    # REFACTOR  (use rule 29 startup sequence)
  ├── ui/
  │   ├── expressions.cpp             # KEEP      (face_engine, per rule 24)
  │   ├── theme_v3.h/cpp              # REPLACE   (with xb_theme bridge)
  │   └── ... (other files merged or replaced)
  └── CMakeLists.txt                  # UPDATE    (add new component deps)
```

### 8.3 New Config Files

```
cores3-espidf/
  ├── partitions_16MB.csv             # NEW  OTA A/B partition table
  ├── sdkconfig.defaults              # UPDATE  (add rollback, OTA, SSE configs)
  └── tools/
      └── regen_icons.sh              # NEW  SVG -> PNG -> lv_img_conv pipeline
```

### 8.4 New sdkconfig.defaults Entries

```
# OTA
CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y
CONFIG_BOOTLOADER_APP_ANTI_ROLLBACK=n
CONFIG_BOOTLOADER_NUM_PIN_APP=3
CONFIG_PARTITION_TABLE_CUSTOM=y
CONFIG_PARTITION_TABLE_CUSTOM_FILENAME="partitions_16MB.csv"

# HTTP/SSE streaming
CONFIG_ESP_HTTP_CLIENT_ENABLE_HTTPS=y
CONFIG_ESP_HTTP_CLIENT_ENABLE_DIGEST_AUTH=n
CONFIG_MBEDTLS_DYNAMIC_BUFFER=y
CONFIG_MBEDTLS_SSL_IN_CONTENT_LEN=16384
CONFIG_MBEDTLS_SSL_OUT_CONTENT_LEN=4096

# FreeRTOS SMP - task affinities
CONFIG_FREERTOS_UNICORE=n
CONFIG_FREERTOS_NUMBER_OF_CORES=2

# PSRAM (Quad, 80MHz - mandatory)
CONFIG_SPIRAM=y
CONFIG_SPIRAM_MODE_QUAD=y
CONFIG_SPIRAM_SPEED_80M=y
CONFIG_SPIRAM_USE_MALLOC=y
CONFIG_SPIRAM_MALLOC_ALWAYSINTERNAL=16384

# claw_core task
CONFIG_CLAW_CORE_TASK_STACK=12288
CONFIG_CLAW_CORE_TASK_PRIO=5
CONFIG_CLAW_CORE_TASK_AFFINITY=0

# SPIFFS for skills
CONFIG_SPIFFS_MAX_PARTITIONS=3
```

## 9. Backend API Integration Points

| Method | Endpoint | Device Role | Notes |
|---|---|---|---|
| POST | `/v1/auth/device` | mTLS -> device JWT | Called once at first boot |
| POST | `/v1/chat` | Client (SSE subscriber) | Streaming LLM inference |
| POST | `/v1/turns` | Client | Upload session turn batch |
| GET | `/v1/session/last` | Client | Fetch last session summary |
| GET | `/v1/ota/manifest?sn=...&cur=v...` | Client | Check for firmware update |
| GET | `/v1/ota/firmware?url=...` | Client | Download firmware binary |
| POST | `/v1/telemetry` | Client | Heartbeat (60s) + crash + events |
| GET | `/v1/skills?board=cores3` | Client | Pull skill catalog |
| GET | `/v1/skills/{id}/pkg` | Client | Download skill package |
| POST | `/v1/mcp/register` | Client (if MCP enabled) | Register device as MCP server |
| POST | `/v1/provision/bind` | Client (if cloud pairing) | 6-digit code provisioning |

## 10. Implementation Phases (Sprint Alignment)

| Sprint | Backend Deliverables | Dependencies |
|---|---|---|
| **S1 (2w)** | ota_manager component + partitions_16MB.csv + page_ota + page_wifi_ap + page_wifi_pair + statusbar 4-slot widget | wifi_manager, captive_dns (ready) |
| **S2 (2w)** | ui_bridge component + page_chat SSE streaming + face expression sync + XB_EVT_CHAT_* events | claw_core, claw_memory (ready) |
| **S3 (2w)** | skill_manager component + page_skills + page_skill_detail + 5 starter skills | claw_skill, claw_cap, SPIFFS skills partition |
| **S4 (1w)** | cap_mcp_server integration + privacy UX + page_settings full 11-item | cap_mcp_server, settings |
| **S5 (1w)** | Theme hot-swap polish + 10 AVG micro-animations + multi-language + QA regression | All components |

## 11. Key Risks and Mitigations

| Risk | Mitigation |
|---|---|
| SSE connection drops during chat | HTTP keep-alive pool (http_reuse); auto-retry with exponential backoff; show reconnect prompt in UI |
| OTA write fails mid-flash (power loss) | A/B dual partition; resume from Range: bytes offset; bootloader 3-fail automatic rollback |
| Skill Lua sandbox escape | Backend static scan (dangerous API blacklist); CPU timeslice 100ms; memory cap 64KB; whitelist net.http domains |
| PSRAM exhaustion during SSE + LVGL | 8MB Quad PSRAM is sufficient (LVGL 153KB x2 + claw_core 12KB + http buffers 16KB); monitor free_heap at startup |
| Core 0/Core 1 race on LVGL objects | Enforce lvgl_port_lock(0)/unlock() in ALL Core 0 callbacks; never call LVGL API without lock (rule 26) |
| Manifest polling overwhelms OTA backend | Cache 304 for 30 min; stagger device poll windows by SN hash |
