# 陪伴机器人 v6.2 交付包

> 生成时间:2026-05-23
> 硬件基线:M5Stack CoreS3(ESP32-S3-WROOM-1-N16R8 / 16MB Flash / 8MB Quad PSRAM)
> 工程基线:ESP-IDF 5.5.4 LTS + LVGL 9.5.0 + esp_lvgl_port 2.4 + ESP-Claw(方案 B)

## 交付清单

| 文件 | 说明 |
|---|---|
| `A_桌面机器人_工程实现规格+后台架构_v5.6.md` | 工程实现规格 + 后台架构(章节独立);附录 F~M 完整 |
| `B_陪伴机器人_产品PRD+UX迭代规划_v6.2.md` | 产品 PRD + UX 迭代(Part 1 / Part 2 章节独立) |
| `icons_pack.zip` | 6 扇区径向菜单图标资源包(LVGL 9 ARGB8888) |

## 飞书云文档(同源)

- 文档 A:https://bytedance.larkoffice.com/docx/L9OedSAnHozvzlxmaYdcGFb5nIf
- 文档 B:https://bytedance.larkoffice.com/docx/Ztyfd5JntoLDwixzOm5cykkZnVg

## 文档 A 章节地图

- §一 ~ §六:工程主体
- 附录 F:icons_pack 资源包说明
- 附录 G:ESP-IDF 5.5.4 适配规则(规则 19~23)
- 附录 H:下游 Agent 投喂 Prompt
- 附录 I:ESP-Claw 集成评估(选定方案 B)
- 附录 J:方案 B 实施详解(组件拓扑 / main.c / 配网 / 对话 / 技能 / MCP / sdkconfig / 分区表 / SMP)
- 附录 K:**后台架构 v1.0**(顶层架构 / 服务清单 / LLM Gateway / Session Store / OTA / Skill Store / MCP Registry / 配网 Helper / Telemetry / 安全 / 部署 / API 速查)
- 附录 L:UX 索引(详见文档 B Part 2)
- 附录 M:Agent 编码规则补丁(规则 24~34)

## 文档 B 章节地图

- Part 1 产品 PRD:版本说明 / 北极星指标 / P0~P2 新功能 / 变更项 / 用户流程图 / 技术依赖 / 验收清单 / 上线节奏 / 风险
- Part 2 UX 迭代:设计原则补丁 / IA / 对话页 / 配网双方案 / OTA / 技能列表与确权 / 设置 / 全局状态联动 / 微交互资源 / 主题与可达性 / 走查 / 阶段化迭代路线
