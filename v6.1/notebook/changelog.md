## Iteration - v7.0 Major Upgrade: PRD v1.1 Settings & Logic
- Logic Changes: 修复了左右倾斜（Tilt Right/Left）映射为 thinking 和 surprised 的 Bug。按照 PRD 规范拆分重构了 `look_left` 和 `look_right` 状态，并准确绑定到开发者面板中的左倾与右倾事件。
- UI Changes: 全新实现了 `SettingsOverlay` 独立界面（代替原来简陋的直接跳 WiFi 扫码）。新增包含系统亮度、声音大小的平滑自定义滑块，并加入了语音切换、Wi-Fi连接、睡眠定时、系统更新、出厂重置等完善的 AI 伴侣机器设置清单列表。
- Routing Changes: 将径向菜单中的“设置”项路由切换至 `/?state=settings`，再由设置页内部跳转至具体的 `wifi_ap` 网络页面。
- Style Changes: 为设置页面内的 UI 控件精细化适配了三套主题体系下的色彩和发光阴影（Tech/Child/Dev）。