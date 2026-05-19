# [PRD] 陪伴机器人 - UI 原型 v2.4

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

V2.4

</td>
      <td>

2026/03/28

</td>
      <td>

基于v2.3升级，优化扁平状态眼睛位置，完善昼夜模式与隐藏菜单

</td>
      <td>

产品经理

</td>
      <td>

-

</td>
    </tr>
  </tbody>
</table>

## 背景与收益

### 需求背景

- 陪伴机器人需要提供更自然、舒适的视觉交互体验。
- 在之前的版本（v2.3）中，扁平状态（如发呆、无聊）下的眼睛位置偏低，视觉重心不够平衡，容易产生“下坠”的视觉错觉。
- 需要进一步规范昼夜模式的视觉表现，并完善静默状态下的随机表情轮播与隐藏式菜单交互，以提升整体产品的生命感与易用性。

### 目标与收益

- **产品目标**：打造具有生命感的陪伴机器人 UI，提升用户在不同光照环境（昼夜模式）下的视觉舒适度；优化静默状态下的拟人化表现（随机表情轮播）；提供便捷且不打扰的隐藏式菜单。
- **数据目标**：提升用户单次互动时长 10%，降低夜间模式下的视觉疲劳反馈率。

## 产品方案概览

- **核心功能模块**：
  1. **昼夜模式切换**：支持白天模式与黑夜模式的无缝切换，适应不同环境光线。
  2. **静默随机表情轮播**：在 Idle（发呆/静默）状态下，机器人会自动在发呆、调皮、可爱、无聊等状态间随机轮播，增加生命感与趣味性。
  3. **隐藏式菜单**：常态下保持界面极简，无多余控件；点击屏幕即可唤起底部操作栏与功能菜单。

### 产品原型设计

<iframe src="https://69c7a33468040e0268057c82-prototype.inspire.bytedance.net/?from_source=prd-document"   title="Interactive prototype"></iframe>

## 视觉设计规范

- **黑夜模式（Dark Mode）**：采用纯黑背景（`bg-black`），面部表情使用暗色调青色发光效果（`bg-cyan-400` 配合发光阴影 `shadow-[0_0_15px_rgba(34,211,238,0.6)]`），营造科技感与夜间护眼体验。
- **白天模式（Light Mode）**：采用柔和低饱和度青色背景（`bg-cyan-50`），面部表情使用深青色（`bg-cyan-800`），去除发光阴影，确保在强光环境下清晰可见且不刺眼。
- **整体布局**：保持极简设计，核心表情居中展示；状态栏（连接状态、时间、电量）置于顶部，操作栏从底部弹出，互不干扰。

## 交互逻辑说明

- **扁平状态眼睛位置优化（核心交互）**：在发呆（Idle）、无聊（Bored）等扁平状态下，眼睛的 Y 轴坐标向上移动（调整至 `y: -15` 或更居中的位置），使其达到更佳的视觉平衡点，优化整体面部比例。
- **菜单唤起与隐藏**：点击屏幕任意非功能区域，可切换底部操作栏（BottomBar）的显示与隐藏，采用弹性动画（Spring）平滑过渡。
- **表情切换动画**：所有表情切换均采用弹性动画（stiffness: 300, damping: 20），确保形变过程自然流畅。特殊状态如眩晕（Dizzy）附加持续旋转动画，哭泣（Crying）附加眼泪滴落动画。

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

[P0] 黑夜模式-静默发呆

</td>
      <td>

![黑夜模式-静默发呆](https://cdn-tos-cn.bytedance.net/obj/tiktok-web-ai-cn/296c8f570702e09c6f36c011583491dd52975e3530f258db4db0620c7c90b905.png)

纯黑背景，青色发光眼睛与嘴巴。眼睛位置已向上微调至视觉中心。

</td>
      <td>

1. [P0] 默认进入黑夜模式的 Idle 状态。
2. [P0] 停留超过一定时间（4-7秒），自动触发随机表情轮播（发呆、调皮、可爱、无聊）。
3. [P0] 随机触发眨眼动画（间隔3-5秒）。

</td>
      <td>

- 眼睛高度 56px，宽度 44px，圆角 22px，Y轴偏移 -15px。
- 嘴巴宽度 32px，高度 6px。

</td>
      <td>

- robot_state_idle_shown

</td>
    </tr>
    <tr>
      <td>

[P0] 白天模式-静默发呆

</td>
      <td>

![白天模式-静默发呆](https://cdn-tos-cn.bytedance.net/obj/tiktok-web-ai-cn/1b4f668c11d0e2e61be99a87f4489f5c5df6eada9e37995667976dd49e4eb7d1.png)

柔和青色背景，深青色无发光表情。

</td>
      <td>

1. [P0] 用户在底部操作栏点击"主题"按钮切换至白天模式。
2. [P0] 界面背景与表情颜色平滑过渡。

</td>
      <td>

- 状态栏文字与图标同步切换为深色。
- 底部操作栏背景切换为浅色半透明。

</td>
      <td>

- robot_theme_light_switched

</td>
    </tr>
    <tr>
      <td>

[P0] 隐藏式菜单唤起

</td>
      <td>

![隐藏式菜单唤起](https://cdn-tos-cn.bytedance.net/obj/tiktok-web-ai-cn/3c9eb60a5e8a9d7a9a60e68a89e6540be85bee4ae2f05936ae838d326c27b0c6.png)

底部弹出操作栏，包含菜单、主题、对话、开心、眩晕等按钮。

</td>
      <td>

1. [P0] 用户点击屏幕任意空白区域。
2. [P0] 底部操作栏从下向上滑出。
3. [P0] 再次点击屏幕空白区域，操作栏向下滑动隐藏。

</td>
      <td>

- 操作栏支持横向滚动（隐藏滚动条）。
- 选中状态的按钮高亮显示。

</td>
      <td>

- robot_bottom_bar_shown

</td>
    </tr>
    <tr>
      <td>

[P0] 功能菜单浮层

</td>
      <td>

![功能菜单浮层](https://cdn-tos-cn.bytedance.net/obj/tiktok-web-ai-cn/b395d07110068f861df00d4d38d016e4e67c95c8e997bb6e597aa3818c731461.png)

半透明遮罩，展示系统设置、音量调节、闹钟提醒等列表。

</td>
      <td>

1. [P0] 用户点击底部操作栏的"菜单"按钮。
2. [P0] 机器人表情变为缩小变暗的 menu 状态。
3. [P0] 弹出功能菜单浮层。
4. [P0] 点击右上角"X"或浮层外区域关闭菜单，恢复 Idle 状态。

</td>
      <td>

- 浮层带有背景模糊效果。
- [TBD:细节待定] 菜单项的具体点击跳转逻辑待后续版本定义。

</td>
      <td>

- robot_menu_overlay_shown

</td>
    </tr>
  </tbody>
</table>

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

robot_state_idle_shown

</td>
      <td>

进入静默发呆状态时触发

</td>
      <td>

theme (enum): dark, light

</td>
      <td>

状态曝光量

</td>
    </tr>
    <tr>
      <td>

robot_theme_light_switched

</td>
      <td>

切换至白天模式时触发

</td>
      <td>

source (enum): bottom_bar

</td>
      <td>

主题切换率

</td>
    </tr>
    <tr>
      <td>

robot_bottom_bar_shown

</td>
      <td>

底部操作栏唤起时触发

</td>
      <td>

-

</td>
      <td>

菜单唤起率

</td>
    </tr>
    <tr>
      <td>

robot_menu_overlay_shown

</td>
      <td>

功能菜单浮层展示时触发

</td>
      <td>

-

</td>
      <td>

功能菜单点击率

</td>
    </tr>
  </tbody>
</table>

### 指标口径与计算

- 菜单唤起率 = robot_bottom_bar_shown UV / 整体活跃 UV
- 主题切换率 = robot_theme_light_switched UV / 整体活跃 UV

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

昼夜模式切换

</td>
      <td>

唤起操作栏 → 点击主题按钮

</td>
      <td>

背景色、表情颜色、状态栏颜色平滑切换，无闪烁

</td>
      <td>

颜色与发光效果符合视觉规范

</td>
    </tr>
    <tr>
      <td>

静默表情轮播

</td>
      <td>

保持 Idle 状态 10 秒以上

</td>
      <td>

自动在发呆、调皮、可爱等状态间切换，并伴随眨眼

</td>
      <td>

动画流畅，无卡顿

</td>
    </tr>
    <tr>
      <td>

扁平状态视觉中心

</td>
      <td>

观察 Idle、Bored 状态

</td>
      <td>

眼睛位置偏上（y: -15），视觉重心居中

</td>
      <td>

无“下坠”视觉错觉

</td>
    </tr>
    <tr>
      <td>

隐藏菜单交互

</td>
      <td>

点击屏幕空白处 → 点击菜单按钮 → 点击关闭

</td>
      <td>

操作栏正常升降，功能浮层正常弹出与关闭，表情状态同步变化

</td>
      <td>

交互顺畅，状态恢复正确

</td>
    </tr>
  </tbody>
</table>