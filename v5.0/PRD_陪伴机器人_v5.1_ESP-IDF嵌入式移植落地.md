# [PRD] 陪伴机器人 v5.1 - ESP-IDF 嵌入式移植落地

## 版本信息

<table>
 <thead>
 <tr>
 <th>版本号</th>
 <th>修订时间</th>
 <th>修订内容</th>
 <th>负责人</th>
 <th>备注</th>
 </tr>
 </thead>
 <tbody>
 <tr>
 <td>

V5.1

</td>
 <td>

2026/05/19

</td>
 <td>

基于 Web 原型 v5.1 生成 ESP-IDF 嵌入式开发技术 PRD，包含硬件映射、算法、动效与传感器融合逻辑

</td>
 <td>

PRD Agent

</td>
 <td>

覆盖 M5Stack CoreS3 平台

</td>
 </tr>
 </tbody>
</table>

## 背景与收益

### 需求背景

- 当前陪伴机器人已有基于 React + Framer Motion 的 v5.1 高保真 Web 原型，实现了 5 级呼吸系统、20 种表情、径向菜单、环境感知面板以及基于 Web API 的体感、音效、震动反馈。
- 为实现硬件产品化，需要将 Web 原型移植到 M5Stack CoreS3（基于 ESP32-S3）硬件平台。
- Web 前端的技术栈（React/Framer Motion）无法直接运行在 MCU 上，必须转化为基于 ESP-IDF 和 LVGL 的 C/C++ 实现，明确动效数学模型、硬件外设（MPU6886、I2S、PWM、NVS）的底层系统映射方案。

### 目标与收益

- **目标**：为固件开发 Agent 或嵌入式工程师提供清晰的技术蓝图，确保 Web 原型的交互意图（特别是眼神微动、体感交互、动效过渡）在 ESP-IDF 平台上以极高还原度重现。
- **技术收益**：统一软硬件接口定义，降低从 UI 原型到嵌入式固件转化过程中的理解损耗，保障产品最终的流畅度（30FPS+ 渲染）与低延迟体感响应。

## 产品方案概览

- 需求概览：本次需求核心是将 v5.1 原型向嵌入式端完整映射。硬件基于 M5Stack CoreS3，配备 320x240 LCD 屏幕，软件栈基于 ESP-IDF + LVGL v8/v9。
- 关键映射流程：
 - **图形渲染**：基于 LVGL 的 Canvas 绘制或基础 `lv_obj` 配合圆角遮罩绘制机器人面部（眼睛、瞳孔、嘴巴），利用 LVGL Timer 替代 React 状态和 Framer Motion 动画，实现基于时间的平滑渲染过渡。
 - **传感器交互**：通过 I2C 每 10-20ms 读取 MPU6886 6轴姿态数据，解算 Pitch/Roll 实现倾斜反馈，解算加速度峰值识别剧烈摇晃与设备敲击。
 - **多模态反馈**：利用 ESP32-S3 的 I2S 驱动扬声器输出不同频率的 Sine 波提示音效；利用 PWM 驱动外接震动马达实现不同节拍的触觉反馈。
 - **状态持久化**：利用 NVS 或 SPIFFS 记录用户交互数据与偏好，实现开机读取个性化记忆。

### 产品原型设计

<iframe src="https://6a0c6fe94042e4025524e599-prototype.inspire.bytedance.net/?from_source=prd-document" title="Interactive prototype"></iframe>

## 需求详情

### 核心场景图文交互表

<table>
 <thead>
 <tr>
 <th>场景名称</th>
 <th>页面设计</th>
 <th>交互步骤</th>
 <th>状态与异常</th>
 <th>事件/埋点</th>
 </tr>
 </thead>
 <tbody>
 <tr>
 <td>

[P0] 基础表情与瞳孔微动（Idle）

</td>
 <td>

![Idle状态](https://cdn-tos-cn.bytedance.net/obj/tiktok-web-ai-cn/ca19c2acdc8102611f7c974ce47ae4c388080cea91b69faea6af685a70bde73d.png)

屏幕 320x240，面部元素（左右眼、瞳孔、嘴部）基于中心 `(160,120)` 相对布局。瞳孔存在随机漂移注视感。

</td>
 <td>

1. [P0] 系统处于 Idle 状态时，瞳孔在 X 轴 ±5px、Y 轴 ±3px 范围内随机连续移动。
2. [P0] 引入 Perlin 噪声叠加实现 8 秒周期的自然生物游走感。
3. [P0] 凌晨（6:00-10:00）初次进入 Idle 将触发"早安问候"，状态切换至 Morning。

</td>
 <td>

- [P0] 要求保障 30FPS 以上渲染帧率，杜绝闪烁和画面撕裂。
- [P1] 瞳孔移动受限幅函数保护，不可超出眼眶设定边界。
- [TBD:边界情况] 开机后长时间未连接 SNTP 时，凌晨时间判断逻辑是否回退或暂缓执行待确认。

</td>
 <td>

- face_state_changed

</td>
 </tr>
 <tr>
 <td>

[P0] 径向菜单交互（Radial Menu）

</td>
 <td>

![径向菜单](https://cdn-tos-cn.bytedance.net/obj/tiktok-web-ai-cn/bb9ef3e8f085e77de3f8cb82cd20e5f48f3244db064e5dc8300afb7dce0ba01c.png)

双击屏幕呼出径向菜单，6 个选项按环形排布（半径 70px）。包含阻尼弹簧效果展开。

</td>
 <td>

1. [P0] 触摸屏检测到双击（间隔时间 < 300ms），呼出中心点辐射菜单。
2. [P0] 同步触发 600Hz、80ms 的展开音效。
3. [P0] 菜单项从中心带阻尼弹动（overshoot）向外展开，伴随 Alpha 淡入。
4. [P0] 用户点击某一选项后，播放 1200Hz 确认音效与短促震动。
5. [P0] 菜单关闭时，平滑缩小至中心消失（v5.1 新增渐出过渡）。

</td>
 <td>

- 边缘触摸精度不足时，需增大菜单项逻辑响应热区范围。
- 菜单展示态阻断其他屏幕触控及轻量级体感手势响应。

</td>
 <td>

- menu_open
- menu_select

</td>
 </tr>
 <tr>
 <td>

[P0] 物理体感交互响应（IMU）

</td>
 <td>

（动作反馈无特定可见面板，表现为瞬间表情切换响应）
利用 MPU6886 加速度计/陀螺仪，将真实物理动作映射为交互意图。

</td>
 <td>

1. [P0] 设备向前倾斜（Pitch > 15°）：触发 Curious (好奇) 表情。
2. [P0] 设备剧烈摇晃（加速度幅度 > 2.5g）：触发 Dizzy (眩晕)，伴随强震动模式。
3. [P0] 顶部轻敲外壳（Z轴突变冲击 > 1.5g）：触发单次 Wink (眨眼) 动画。
4. [P0] 向左/右倾斜（Roll > 15° 或 < -15°）：触发 Look_around (左顾右盼)。

</td>
 <td>

- [P0] 触发防抖：v5.1 设置统一拦截窗 500ms（短于 500ms 内的新阈值不触发动作跳变）。
- [TBD:数据依赖] I2C 读取 MPU6886 数据的轮询频率（建议 10-20ms），需视 ESP32-S3 主线程负载情况调整。

</td>
 <td>

- imu_action_triggered

</td>
 </tr>
 </tbody>
</table>

### 文案与多语言表

<table>
 <thead>
 <tr>
 <th>场景名称</th>
 <th>文案 key</th>
 <th>原文案</th>
 <th>翻译文案</th>
 </tr>
 </thead>
 <tbody>
 <tr>
 <td>

[P0] 基础表情与瞳孔微动（Idle）

</td>
 <td>

morning_greeting_text

</td>
 <td>

"早上好呀！今天也是充满能量的一天。"

</td>
 <td>

"Good morning! Full of energy today."

</td>
 </tr>
 <tr>
 <td>

[P0] 径向菜单交互（Radial Menu）

</td>
 <td>

menu_item_theme

</td>
 <td>

"主题"

</td>
 <td>

"Theme"

</td>
 </tr>
 <tr>
 <td>

[P0] 径向菜单交互（Radial Menu）

</td>
 <td>

menu_item_expression

</td>
 <td>

"表情"

</td>
 <td>

"Expressions"

</td>
 </tr>
 <tr>
 <td>

[P0] 径向菜单交互（Radial Menu）

</td>
 <td>

menu_item_dialogue

</td>
 <td>

"对话"

</td>
 <td>

"Dialogue"

</td>
 </tr>
 <tr>
 <td>

[P0] 径向菜单交互（Radial Menu）

</td>
 <td>

menu_item_random

</td>
 <td>

"随机"

</td>
 <td>

"Random"

</td>
 </tr>
 <tr>
 <td>

[P0] 径向菜单交互（Radial Menu）

</td>
 <td>

menu_item_settings

</td>
 <td>

"设置"

</td>
 <td>

"Settings"

</td>
 </tr>
 </tbody>
</table>

### 硬件平台适配与 LVGL 映射

1. **图形组件规划**：放弃 DOM 节点的层级概念，统一在 `lv_scr_act()` 下创建眼睛、嘴巴的对象（推荐使用带有 `radius` 样式的 `lv_obj` 或者完全用 `lv_canvas` 手绘缓冲区）。
2. **主题配色与发光**：
 - 科技风格（Tech）：底色 `0x000000`，面部主色 `0x22D3EE`。LVGL 中可借助 `lv_style_set_shadow_width` 与 `lv_style_set_shadow_color` 实现边缘柔和发光效果。
 - 儿童风格（Child）：底色 `0xFFF9E6`，面部主色 `0xFF7F50`。
3. **Wink 单次动画修复**：Web 端的 `repeat: 0` 在 LVGL 中的实现需显式为动画器设置 `lv_anim_set_repeat_count(&a, 1)`，完成后通过 `ready_cb` 回调清理状态，防止出现闭眼卡死或无限循环。

### 动效数学化（React 转 C 逻辑）

1. **呼吸曲线驱动 (Modified Sine)**：
 - 呼吸循环不再依靠 Framer Motion 的 Keyframes。使用一个 10-20ms 执行一次的 LVGL Timer 或 FreeRTOS Task 不断累加时间变量 `t`。
 - 闲置状态的缩放比例计算公式参考：`scale = 1.0f + 0.05f * sinf(2.0f * PI * t / duration_sec)`。
2. **弹簧阻尼模型 (Spring)**：
 - 径向菜单或眼球瞬间弹跳可利用 LVGL 自带曲线 `lv_anim_path_overshoot` 或 `lv_anim_path_bounce` 拟合，通过调整 `lv_anim_set_time`（设定时长，如 400ms）近似出 UI 原型中 Spring(damping:14, stiffness:150) 的弹性观感。

### 传感器融合与算法实现 (MPU6886 & Perlin)

1. **姿态解算算法**：
 - 读取 3 轴加速度（$A_x, A_y, A_z$），执行低通滤波后：
 - $Pitch = atan2(A_y, \sqrt{A_x^2 + A_z^2}) \times \frac{180.0}{\pi}$
 - $Roll = atan2(-A_x, A_z) \times \frac{180.0}{\pi}$
 - 冲击检测使用瞬间差分：$\Delta Z = |A_z - 1.0g|$，当 $\Delta Z > 1.5g$ 触发轻敲动作。
2. **瞳孔漂移的类 Perlin 噪声算法 (降维方案建议)**：
 - 受 MCU 数学库性能约束，不建议完整生成 2D 梯度噪声，可采用非公倍频率的 Sine/Cosine 叠加模拟连续平滑游走：
 ```c
 // C伪代码：8秒周期，随时间线前推生成(X, Y)偏移
 float t = (float)lv_tick_get() / 1000.0f;
 float x_drift = sinf(t * 2 * PI / 8.0f) * 3.0f + cosf(t * 2 * PI / 4.3f) * 2.0f;
 float y_drift = cosf(t * 2 * PI / 7.0f) * 2.0f + sinf(t * 2 * PI / 3.1f) * 1.0f;
 // 将计算得出的微量 x_drift 与 y_drift 叠加至瞳孔中心坐标
 ```

### 多模态驱动逻辑 (I2S音效与 PWM震动)

1. **音效合成 (Web Audio API 转向 ESP-IDF I2S)**：
 - 直接在内存中按给定频率计算正弦波样本，然后通过 DMA 推流给 I2S 外设驱动外接扬声器。
 - 菜单展开音：频率 600Hz，时长 80ms。
 - 菜单选中确认：频率 1200Hz，时长 80ms。
 - 交互需附加快速指数衰减包络线（Exponential Decay）消除波形截断的"啪"声（Anti-pop）。
2. **触觉反馈 (Vibration API 转向 PWM)**：
 - 利用 MCPWM / LEDC 外设控制马达振次。
 - 双击设备：振动序列 `[50ms ON -> 30ms OFF -> 50ms ON]`。
 - 菜单选择：振动序列 `[30ms ON]`。
 - 眩晕态强震动：振动序列 `[100ms ON -> 50ms OFF -> 100ms ON]`。

### 记忆系统与持久化存储 (NVS)

1. **存储架构映射**：Web 的 localStorage 转化为 ESP-IDF 的 NVS (Non-Volatile Storage) 键值对系统。
2. **核心业务体结构定义**：
 ```c
 typedef struct {
 uint32_t total_interactions;
 uint16_t consecutive_days;
 char last_active_date[12]; // e.g. "2026-05-19"
 } robot_memory_t;
 ```
3. **开机问候逻辑**：使用 SNTP 抓取网络时间，初始化读取 NVS 发现系统处于 `06:00-10:00` 且 `last_active_date` ≠ 当日，注入 Morning 场景拦截器，完成后将当天日期回写 NVS。

## 依赖与前置工作

> 此为预留板块，示例：
>
> - 硬件前置核验：确保 M5Stack CoreS3 的 I2S 引脚映射、I2C 端口号（SDA/SCL）正确定义并可用。
> - 环境依赖：需配置基于 ESP-IDF v5.x 的开发环境。
> - 外部依赖库：LVGL（v8 或更高版本）核心组件引入，配置好显存双缓冲机制防止画面撕裂。

## 灰度与时间线

> 此为预留板块，示例：
>
> - 第一阶段：完成 UI 静态图元绘制与 MPU6886 传感器链路打通测试（预计排期 3 天）。
> - 第二阶段：实现 LVGL 复杂动效绑定（呼吸/瞳孔移动）、体感交互动作拦截器植入（预计排期 4 天）。
> - 第三阶段：接通 I2S 音响提示与震动马达反馈手感调优，引入 NVS 储存持久化方案（预计排期 3 天）。
> - 最终验收：合板集成，进行连续不宕机的内存泄漏检测与帧率监控分析。

## 指标统计

### 埋点设计

<table>
 <thead>
 <tr>
 <th>事件 key</th>
 <th>触发时机</th>
 <th>params（事件特有参数）</th>
 <th>指标归属</th>
 </tr>
 </thead>
 <tbody>
 <tr>
 <td>

face_state_changed

</td>
 <td>

表情或运行状态发生切换时

</td>
 <td>

from_state (string): 之前状态；to_state (string): 目标状态；trigger_source (enum): 切换诱因(imu、touch、timeout)

</td>
 <td>

状态分布覆盖率

</td>
 </tr>
 <tr>
 <td>

menu_open

</td>
 <td>

用户双击呼出径向菜单

</td>
 <td>

theme (string): 唤起时的系统主题

</td>
 <td>

菜单唤起率

</td>
 </tr>
 <tr>
 <td>

menu_select

</td>
 <td>

用户点击选中径向菜单某一项

</td>
 <td>

action_item (string): 具体菜单项id

</td>
 <td>

菜单点击转化率

</td>
 </tr>
 <tr>
 <td>

imu_action_triggered

</td>
 <td>

体感交互幅度破限，成功触发反馈时

</td>
 <td>

action_type (enum): 枚举值 (curious, yawn, dizzy, wink, look_around)；value (number): 触发时峰值数据

</td>
 <td>

体感交互率

</td>
 </tr>
 </tbody>
</table>

### 指标口径与计算

- 体感交互率 = `imu_action_triggered` 触发 UV / 总体开机活跃设备数 UV。
- 菜单点击转化率 = `menu_select` 触发 PV / `menu_open` 曝光 PV。
- 硬件健康度指标监控：重点关注加速度计上报极值错误、零位失效等脏数据发生率，用以动态校准软件防抖窗口参数。

## 验收标准与测试要点

### 验收用例表

<table>
 <thead>
 <tr>
 <th>用例名称</th>
 <th>步骤</th>
 <th>期望结果</th>
 <th>通过条件</th>
 </tr>
 </thead>
 <tbody>
 <tr>
 <td>

径向菜单展开与选中

</td>
 <td>

在 Idle 态连续双击屏幕 → 展现菜单并展开 → 点击表情项

</td>
 <td>

菜单辐射状外发散展开伴随 600Hz 提示音；选中操作时伴发 1200Hz 短音与轻微震感，之后菜单平滑缩回退出，机器人表情响应改变

</td>
 <td>

动效 30FPS 不掉帧；音波平滑无明显切断爆音

</td>
 </tr>
 <tr>
 <td>

剧烈摇晃体感回馈

</td>
 <td>

抓起设备剧烈摇晃（模拟三轴矢量加速度 > 2.5g）

</td>
 <td>

触发三段式强震动 [100, 50, 100]，机器人面部立刻切为旋转圈圈的眩晕态 (Dizzy)

</td>
 <td>

500ms 抖动屏蔽生效；动作停下后需依规退回 Idle

</td>
 </tr>
 <tr>
 <td>

头部敲击动作触发

</td>
 <td>

用指节由上而下敲击设备外壳（模拟 Z 轴激变 > 1.5g）

</td>
 <td>

触发 Wink (眨眼) 动画；且仅闭眼然后睁开复位

</td>
 <td>

必须保证且只能播放一次，验证原型 v5.1 单次执行补丁是否落实

</td>
 </tr>
 <tr>
 <td>

早安问候与状态流转

</td>
 <td>

设备通过 NVS 清理模拟长时未唤醒状态；设定系统时间至早上 08:00 并切入 Idle

</td>
 <td>

强制推流 Morning 表情状态并展现时间/温度问候 UI

</td>
 <td>

SNTP 时间拉取正常；NVS 隔日判定生效，重复不触发

</td>
 </tr>
 <tr>
 <td>

闲时类生命感游走

</td>
 <td>

无任何操作静置设备于桌面长达 10 秒以上

</td>
 <td>

系统保活在 Idle 态，眼睛中瞳孔跟随类 Perlin 路径平滑不定向游走

</td>
 <td>

瞳孔偏移不发生硬切或坐标超界穿模

</td>
 </tr>
 </tbody>
</table>
