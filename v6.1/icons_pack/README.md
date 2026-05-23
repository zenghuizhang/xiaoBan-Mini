# 径向菜单图标资源包（icons_pack）

> v6.1 原型 6 扇区菜单图标的工程级复刻产物。**下游 Agent / 工程师直接 `#include` 使用即可，无需再"看图复刻"。**

## 1. 内容物

```
icons_pack/
├── build_icons.py            # 重生产脚本（SVG -> PNG -> C 数组）
├── CMakeLists.txt            # ESP-IDF 组件清单
├── include/
│   └── menu_icons.h          # 6 个 LV_IMG_DECLARE + menu_icons[] 指针表
├── svg/                      # 原始 lucide-react SVG（24x24 viewBox，stroke="currentColor"）
│   ├── icon_smile.svg
│   ├── icon_message.svg
│   ├── icon_settings.svg
│   ├── icon_palette.svg
│   ├── icon_puzzle.svg
│   └── icon_shuffle.svg
├── png/                      # 32x32 RGBA8888 白描边参考图（人工校对用）
│   └── icon_*.png × 6
└── c/                        # LVGL 9 lv_image_dsc_t（编译进固件，~26KB/文件）
    ├── icon_*.c × 6
    └── menu_icons.c          # 指针表实现
```

## 2. 图标 — 扇区映射（与 v6.1 `MenuOverlay.tsx` 完全一致）

| Sector | 符号名（C）       | lucide 图标   | 语义             |
| ------ | ----------------- | ------------- | ---------------- |
| 0      | `icon_smile`      | Smile         | 表情 / 互动      |
| 1      | `icon_message`    | MessageCircle | 对话             |
| 2      | `icon_settings`   | Settings      | 设置             |
| 3      | `icon_palette`    | Palette       | 主题             |
| 4      | `icon_puzzle`     | Puzzle        | 扩展 / 插件      |
| 5      | `icon_shuffle`    | Shuffle       | 随机 / 切换      |

## 3. 技术规格

| 项                  | 值                                                            |
| ------------------- | ------------------------------------------------------------- |
| 目标平台            | ESP-IDF 5.2.1 + LVGL 9.5.0（M5Stack CoreS3, ESP32-S3）        |
| 图像格式            | `LV_COLOR_FORMAT_ARGB8888`（4 B/px, 内存布局 B,G,R,A）        |
| 尺寸                | 32 × 32 px                                                    |
| `stride`            | 128 字节                                                      |
| `data_size`         | 4096 字节                                                     |
| 着色策略            | 白描边 + alpha 编码笔画 → 运行时 `image_recolor` 上主题色     |
| 单图编译后体积      | ≈ 4 KB（.rodata）                                             |
| 6 图总占用          | ≈ 24 KB（CoreS3 16 MB Flash 完全可承受）                      |

## 4. 调用示例（替换 Appendix C.6 的占位 label）

```c
#include "menu_icons.h"

/* 在径向菜单创建 6 个按钮时，每个按钮内放一个 lv_image */
for (int i = 0; i < 6; i++) {
    lv_obj_t *btn = lv_obj_create(menu_root);
    /* ... 按之前 Appendix C 的几何计算 set_pos / set_size ... */

    lv_obj_t *img = lv_image_create(btn);
    lv_image_set_src(img, menu_icons[i]);     /* 0..5 → 6 个图标 */
    lv_obj_center(img);

    /* 主题色重着色（白描边变主题色），无需为每个主题再生成资源 */
    lv_obj_set_style_image_recolor(img, lv_color_hex(theme->icon_color), 0);
    lv_obj_set_style_image_recolor_opa(img, LV_OPA_COVER, 0);
}
```

## 5. 重生产流程（如需替换图标）

```bash
# 1) 依赖
pip install --user --break-system-packages cairosvg pillow

# 2) 替换 svg/icon_xxx.svg（保持 stroke="currentColor" 约定）

# 3) 重跑
python3 icons_pack/build_icons.py
# -> 覆盖 png/、c/ 下对应文件；CMake 自动重编。
```

## 6. 设计决策（为什么不让 Agent "看图画"）

| 方案                              | 还原度 | 可重现 | 体积 | 决策     |
| --------------------------------- | ------ | ------ | ---- | -------- |
| Agent 看 PNG 用 lv_canvas 画线    | 低     | 否     | 高   | ✗ 已淘汰 |
| Agent 用 LV_SYMBOL_xxx 占位       | 极低   | 是     | 低   | ✗ 仅过渡 |
| **预生产 lv_image_dsc_t（本包）** | **100%** | **是** | **低** | **✓ 采用** |
| 运行时 lv_svg 渲染                | 100%   | 是     | 高   | ✗ LVGL9 SVG 模块依赖大 |
