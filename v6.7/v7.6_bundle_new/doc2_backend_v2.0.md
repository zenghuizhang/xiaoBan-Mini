# 桌面陪伴机器人 后台架构设计 v2.0(配套 v7.6)

> 本文档定义云端 + 边缘 + 设备协同的全部后台能力,**配套工程实现规格 v7.6**。相对 v1.0(配 v5.6 工程)的关键升级在 §0。

## 文档信息

| 项 | 值 |
|---|---|
| 版本 | **v2.0** |
| 配套工程 | v7.6 |
| 上游消费者 | 设备(`claw_core@v1.0`)/ 小程序 / Mira Agent |
| 部署 | 火山引擎 VKE 集群(SG / DXB / US / CN 四区) |
| 状态 | 设计冻结,准备进入开发 |

---

## 0. v2.0 相对 v1.0 的关键变更

| 模块 | v1.0(v5.6 配套) | **v2.0(v7.6 配套)** |
|---|---|---|
| Theme 同步 | 仅本地 NVS | **云端 `device_pref` 表持久化**,跨端同步 |
| Persona | 不存在 | **新增 `persona_registry` + 用户级 active_persona** |
| Memory | 仅设备本地 sqlite | **设备主存 + 云端 `memory_shadow` 影子表**,异步对账,断网降级本地 |
| Skill 上架 | 7 类 | **6 类(去掉旧 `theme_pack`,因为主题已固化为 4 套)** |
| OTA 灰度 | 按 device_id 取模 | **按主题/人格交叉打 tag,可定向放量** |
| MCP Gateway | 单租户 | **多租户隔离 + per-tool quota** |
| 遥测维度 | 8 个 | **15 个**(新增 theme/persona/scene_count/lonely_trigger 等行为指标) |
| 区域合规 | 国内单区 | **4 区独立部署,数据不出域**(SG/DXB/US/CN) |
| LLM 路由 | 单云 | **Auto Router**(对应 ModelPicker 的 ⓘ Auto 选项),按时延/质量/成本权重路由 |

---

## 1. 整体架构

```
┌─ Device (CoreS3) ─────────────────┐    ┌─ Edge ─┐    ┌─ Cloud (VKE) ───────────────────────────┐
│ LVGL UI / face_engine / RGB strip │    │ CDN+WAF│    │ API GW                                  │
│ claw_core@v1.0                    │◄──►│ Anycast│◄──►│ ├─ LLM Proxy (Auto Router)              │
│ cap_mcp_server :8090              │    │ TLS    │    │ ├─ Skill Registry + S3                  │
│ cap_im_local (LLM stream)         │    └────────┘    │ ├─ Persona Registry                     │
│ xb_memory_store (sqlite)          │                  │ ├─ Memory Shadow Sync                   │
│ xb_theme NVS                      │                  │ ├─ OTA Server (双分区灰度+tag 定向)     │
└───────────────────────────────────┘                  │ ├─ Device Manager                       │
                                                       │ ├─ Telemetry / Logging                  │
                                                       │ ├─ MCP Gateway (多租户)                 │
                                                       │ └─ Auth (JWT + Device Cert)             │
                                                       │                                         │
                                                       │ Store: PG / Redis / S3 / ClickHouse     │
                                                       └─────────────────────────────────────────┘
```

---

## 2. 核心服务清单

### 2.1 LLM Proxy(Auto Router)

对应 ModelPicker 的 6 行(`Auto / GPT-4o / Claude 3.5 Sonnet / Doubao Pro / Qwen-2.5 1.5B / Phi-3 mini`):

- **本地模型**(Qwen-2.5 1.5B / Phi-3 mini)由设备 NPU 直接跑,不走云端;
- **云端 5 模型**由 LLM Proxy 统一鉴权、计费、限流、流式转发;
- **Auto Router 策略**:
  - 首字延迟权重 0.4 / 质量权重 0.4 / 成本权重 0.2;
  - 短问(< 20 字)优先 Doubao Pro;长上下文(> 4k tokens)优先 Claude 3.5;
  - 触发 `voice_wake` 场景时强制走最快通道。

### 2.2 Persona Registry(★ v2.0 新)

| 字段 | 类型 | 说明 |
|---|---|---|
| persona_id | text PK | "lyra" / "echo" / ... |
| name | text | "Lyra" |
| tagline | text | "温柔诗人" |
| emoji | text | "🎵" |
| system_prompt | text | 多语言模板(简中/英) |
| default_face | text | "happy" |
| created_at | timestamptz | |

REST 接口:
- `GET /v1/personas` — 列出所有 6 个;
- `POST /v1/devices/{id}/persona` `{persona_id}` — 设置 active;
- `GET /v1/devices/{id}/persona` — 当前 active。

### 2.3 Memory Shadow(★ v2.0 新)

设备每次写本地 memory 时,异步推送到云端 `memory_shadow`:

```sql
CREATE TABLE memory_shadow (
  id           BIGSERIAL PRIMARY KEY,
  device_id    UUID NOT NULL,
  client_id    BIGINT NOT NULL,        -- 设备端 sqlite rowid
  kind         TEXT NOT NULL,          -- chat/settings/skills/usage/net
  content      TEXT NOT NULL,
  created_at   TIMESTAMPTZ NOT NULL,
  uploaded_at  TIMESTAMPTZ DEFAULT now(),
  UNIQUE(device_id, client_id)
);
```

清空(Memory Browser purge modal)→ 设备本地软删 + 异步推送 tombstone → 云端打 `deleted_at`,保留 30 天审计窗口后物理删。

### 2.4 OTA Server(★ tag 定向)

新增 `device_tag`:`theme=lavender` / `persona=lyra` / `region=SG` / `firmware=7.6.x`。
灰度策略示例:

```yaml
release: 7.7.0-beta
target:
  - theme: ["lavender", "child"]   # 先对亮色主题用户灰度
  - persona: ["lyra", "pico"]      # 对温柔类人格灰度
  - region: ["SG"]
ramp:
  - 24h: 5%
  - 48h: 20%
  - 7d:  100%
```

### 2.5 Telemetry(★ 15 维)

| 维度 | 例 | 用途 |
|---|---|---|
| device_id | uuid | 反查 |
| firmware | "7.6.0" | 版本分布 |
| theme | "cocoa" | **新**:验证 Cocoa 渗透率 |
| persona | "lyra" | **新**:验证人格偏好 |
| region | "SG" | 合规 |
| scene_count.lonely_3 | 12 | **新**:每天被检测为「无聊」的次数 |
| scene_count.voice_wake | 47 | **新**:唤醒次数 |
| memory_count | 87 | 记忆条数 |
| battery_drain_per_h | 8.3 | 续航 |
| boot_count | 4 | |
| ota_attempt_count | 1 | |
| llm_first_token_p95 | 320 ms | **新** |
| llm_provider_share | {gpt4o:0.4,claude:0.3,doubao:0.3} | **新** |
| crash_signature | "lvgl_port_lock_timeout" | 故障 |
| somato_test_count | 6 | **新**:开发者控制台体感按钮使用度 |

事件流:Device → MQTT(EMQX)→ Kafka → ClickHouse / Grafana。

---

## 3. 数据流:典型场景

### 3.1 用户切主题 Tech → Cocoa

```
ThemePicker tap "草莓可可"
  ↓
设备:xb_theme_set("cocoa") → NVS 写 → EVENT_THEME_CHANGED
  ↓
设备:cap_mcp_server 通过 POST /v1/devices/{id}/pref 上报 {theme:"cocoa"}
  ↓
Device Manager:UPDATE device_pref SET theme='cocoa' WHERE device_id=...
  ↓
Telemetry:emit theme_changed event → ClickHouse
  ↓
小程序端实时刷新:WebSocket push {device_id, theme:"cocoa"}
```

### 3.2 LLM 对话(Auto Router)

```
用户按 Menu 12 点 → 进对话页 → 输入语音
  ↓
设备:cap_im_local 把 ASR 结果 + 当前 persona.system_prompt + 最近 3 条 memory 打包
  ↓
HTTP /v1/chat/completions  (provider=auto)
  ↓
LLM Proxy Router:< 20 字 → Doubao Pro
  ↓
SSE 流式回设备 → face 切 talking variant → strip accent_hi 4 s 脉动
  ↓
对话结束 → 异步 POST /v1/memory `kind=chat,content="..."` → memory_shadow
```

### 3.3 OTA(按 theme/persona 定向)

```
OTA Server 发布 7.7.0-beta,target=theme:lavender
  ↓
Device heartbeat:POST /v1/heartbeat { theme: "lavender" }  → 命中
  ↓
返回 update_available + url + sha256
  ↓
设备:cap_ota 拉取 → 双分区写 → 校验
  ↓
进度回写 → statusbar critical(RGB 紫 1.5 s 脉动)+ "OTA 系统更新中..." bubble
  ↓
完成 → reboot → 启用新分区
```

---

## 4. 接口契约(摘录,完整 OpenAPI 见仓库)

### 4.1 设备 → 云

| Endpoint | Method | 说明 |
|---|---|---|
| `/v1/heartbeat`              | POST | 每 30 s,含 fw / theme / persona / battery |
| `/v1/chat/completions`       | POST(SSE) | LLM,流式 |
| `/v1/memory`                 | POST | 写一条记忆(异步到 shadow) |
| `/v1/devices/{id}/pref`      | POST | 更新主题/人格/亮度/音量 |
| `/v1/telemetry/event`        | POST | 行为打点 |
| `/v1/ota/check`              | GET  | 拉 OTA 元 |
| `/v1/skills/list`            | GET  | Skill 商店列表(v7.6 不直接消费,留给后续 Settings.Skills 启用时) |

### 4.2 云 → 设备(WebSocket / MQTT)

| Topic | Payload | 说明 |
|---|---|---|
| `cmd/scene_inject`     | `{scene:"voice_wake"}` | 远程触发剧本(运营测试用) |
| `cmd/theme_force`      | `{theme:"cocoa"}` | 强制切主题(灰度试验) |
| `cmd/ota_now`          | `{url, sha256}` | 立即升级 |
| `cmd/persona_force`    | `{persona:"lyra"}` | 强制切人格(A/B) |

---

## 5. 安全 & 合规

| 项 | 说明 |
|---|---|
| 设备身份 | ECC P-256 设备证书,首次激活时由 Device Manager 签发,存 eFuse |
| 传输 | TLS 1.3,Pin 服务端 SPKI |
| LLM 内容审计 | Proxy 侧统一接入字节内容审核 SDK,命中关键词后降级到 cap_im_local 本地模型 |
| 用户数据归属 | 每个 region 数据不出域;memory_shadow 仅同区 |
| 删除权 | Settings → 系统 → 清除偏好数据 → 触发设备 + 云端双侧删 |
| 录音 | v7.6 不上云,仅 ASR 转文字本地处理(隐私第一) |
| 日志脱敏 | 上传 telemetry 前过 dlp_sdk,姓名/电话/email 全部 hash |

---

## 6. 容量规划(单区 SG,首年)

| 指标 | 估值 |
|---|---|
| 设备规模 | 10 万台 |
| QPS(LLM) | 平均 80,峰值 320 |
| MQTT 长连接 | 10 万 |
| memory_shadow 增量 | 80 GB/年 |
| Telemetry | 400 GB/年(ClickHouse 压缩后 60 GB) |
| OTA 带宽峰值 | 1.5 Gbps(灰度首小时) |

VKE 节点:控制面 3 × 4c8g,LLM Proxy 6 × 8c16g,Telemetry 4 × 4c8g。月成本约 ¥1.2 万。

---

## 7. 监控 & 告警

| 告警 | 阈值 | 接收人 |
|---|---|---|
| LLM 首字延迟 P95 > 800 ms | 5 min 持续 | LLM owner |
| `crash_signature` 同 device 24 h 内 ≥ 3 | 立刻 | 固件 owner |
| OTA 失败率 > 5% | 30 min | 固件 owner |
| MCP gateway 5xx > 1% | 15 min | 后端 owner |
| theme=cocoa 异常退出率 比 tech 高 200% | 1 h | UX + 固件 |
| scene_count.lonely_3 中位 > 20/天 | 1 d | 产品(可能用户太孤独,需要主动对话) |

---

## 8. 验收 checklist(后端侧)

- [ ] 4 区域独立部署,跨区调用 ≤ 0(VPC 隔离)
- [ ] 设备能成功上报 `theme=lavender/cocoa`,小程序可实时观测
- [ ] Auto Router 在弱网下能 200 ms 内 fallback Phi-3 mini 本地
- [ ] memory_shadow 断网 1 h 后恢复,设备 → 云对账无丢
- [ ] OTA 按 `theme=lavender` tag 灰度,Cocoa 用户不会收到包
- [ ] Telemetry 15 维度全部入 ClickHouse,Grafana dashboard 有现成图
- [ ] dlp_sdk 兜底 telemetry,姓名/电话被 hash
- [ ] persona 切换 → cap_im_local 下一轮请求 system_prompt 已变
