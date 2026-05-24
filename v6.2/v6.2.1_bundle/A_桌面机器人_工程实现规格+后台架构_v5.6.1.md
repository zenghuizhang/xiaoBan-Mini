# 桌面机器人 UI 工程实现规格 — Part 2 / ESP-IDF 5.5.4(LTS) + LVGL 9.5.0

> **本文档面向**:嵌入式工程师、AI 代码生成助手(Cursor / Claude Code / GPT)
> **目标**:基于 **ESP-IDF 5.5.4 LTS + LVGL 9.5.0 + esp_lvgl_port v2.4+**,在 M5Stack CoreS3 上完整实现 v6.1 原型,**含中文显示、径向菜单图标资源、ESP-Claw 边缘 Agent 集成评估**
> **配套设计稿**:Part 1 / 视觉与交互层
> **历史版本**:v5.1(IDF 5.2.1 基线)→ v5.2(对齐 v6.1)→ v5.3(附录 F 图标包)→ v5.4(附录 G/H IDF 5.5 适配)→ **v5.5(全文 sweep 至 IDF 5.5.4 + 附录 I ESP-Claw 集成)**

---

## 文档信息

| 项目 | 内容 |
|------|------|
| **文档版本** | **v5.6.1 / Part 2**(2026-05-23 同步全量复刻包 `full_replica_v6.2/`) |
| **更新日期** | 2026-05-23 |
| **目标硬件** | **M5Stack CoreS3**(ESP32-S3-WROOM-1-N16R8,16MB Flash + **8MB Quad PSRAM @ 80MHz**)|
| **工具链** | **ESP-IDF 5.5.4 LTS** / LVGL **9.5.0**(via Component Manager `lvgl/lvgl ^9.2`)/ **esp_lvgl_port** ≥ 2.4 |
| **编译器** | GCC 14(IDF 5.5 工具链自带) |
| **C 标准** | C11 + C++17(组件层 C,业务层可选 C++) |
| **字体方案** | lv_font_conv + 思源黑体 SourceHanSansCN(子集化) |
| **板级抽象** | 自研 `bsp_cores3` 组件;**亦可直接复用 ESP-Claw 的 `boards/m5stack/m5stack_cores3` YAML 声明式板级**(详见附录 I)|

> ⚠️ **若你看到的 git 历史里出现 ESP-IDF 5.2.1**:那是 v5.1 老底稿。**自 v5.4 起整个工程已经升级到 IDF 5.5.4 LTS**,本文不再支持 5.2.x 兼容路径,降级即视为不合规。

---

## ⚠️ LVGL 9 关键变更(从 v5.0 升级必读)

LVGL 9 对 8.x 是**破坏性升级**,以下 API 已变更,旧代码无法编译:

| 旧 API (LVGL 8.x) | 新 API (LVGL 9.x) |
|---|---|
| `lv_disp_t *` | `lv_display_t *` |
| `lv_disp_drv_t` + `lv_disp_drv_register()` | `lv_display_create()` + `lv_display_set_*()` 系列 |
| `lv_disp_draw_buf_t` + `lv_disp_draw_buf_init()` | `lv_display_set_buffers()` |
| `lv_scr_act()` | `lv_screen_active()` |
| `lv_anim_set_time()` | `lv_anim_set_duration()` |
| `lv_img_dsc_t` | `lv_image_dsc_t` |
| `lv_img_create()` | `lv_image_create()` |
| `lv_obj_clean()` 自动重建 | `lv_obj_remove_style_all()` + 手动构建 |
| flush_cb `lv_color_t *color_p` | flush_cb `uint8_t *px_map` |
| `LV_COLOR_DEPTH 16` | `LV_COLOR_FORMAT_RGB565` (新颜色枚举) |
| `lv_event_get_target()` | `lv_event_get_target_obj()` |

**ESP-IDF 5.5.4 注意点(取代旧 5.2 兼容路径)**:
- `esp_lcd` 组件 API 与 5.2 大致兼容;**新驱动接入统一用** `esp_lcd_new_panel_io_spi()` / `esp_lcd_new_panel_xxx()`,**禁止**再用 5.2 以前的 `esp_lcd_panel_io_create_*` 老接口名。
- I2C **强制**使用 `driver/i2c_master.h` 新版 API(`i2c_new_master_bus` / `i2c_master_bus_add_device`);5.5 起旧 `driver/i2c.h` 弃用,会有 deprecation warning。
- **必须**启用 SPIRAM(CoreS3 是 **Quad PSRAM @ 80MHz**,详见附录 G.3.5);320×240 RGB565 单帧约 153KB,双 buffer 走 PSRAM。
- LVGL 9 + esp_lvgl_port v2:**禁止**手写 `lv_display_create` / `lv_indev_create`,统一走 `lvgl_port_add_disp` / `lvgl_port_add_touch`(详见附录 G.3.2)。
- FreeRTOS v10.5 SMP 默认开启:LVGL task 由 `esp_lvgl_port` 内部 pin 到 Core 1,**禁止**自建 LVGL task。

---

## 一、项目结构

```
robot_ui/
├── CMakeLists.txt
├── sdkconfig.defaults
├── partitions.csv
├── dependencies.lock           # 由 idf.py 自动生成
├── main/
│   ├── CMakeLists.txt
│   ├── idf_component.yml      # 组件依赖声明
│   ├── main.c
│   └── app_config.h
├── components/
│   ├── bsp_cores3/            # M5Stack CoreS3 板级支持
│   │   ├── bsp_cores3.c
│   │   ├── bsp_display.c      # ILI9342C + esp_lvgl_port
│   │   ├── bsp_touch.c        # FT6336U
│   │   ├── bsp_imu.c          # BMI270
│   │   ├── bsp_pmu.c          # AXP2101
│   │   └── include/bsp_cores3.h
│   ├── lv_chinese_font/       # 中文字体组件 ★新增
│   │   ├── font_sourcehans_22.c   # lv_font_conv 生成
│   │   ├── font_sourcehans_14.c
│   │   └── include/lv_chinese_font.h
│   ├── ui_core/
│   │   ├── event_bus.c        # 事件总线
│   │   ├── breathing.c        # 呼吸引擎
│   │   ├── face_render.c      # 表情渲染(LVGL 9 API)
│   │   ├── dialog_bubble.c    # 对话气泡(中文支持)
│   │   ├── radial_menu.c      # 径向菜单
│   │   ├── strings_zh.h       # ★中文文本集中表
│   │   └── include/ui_core.h
│   ├── motion_detect/
│   │   ├── motion_detect.c
│   │   └── include/motion_detect.h
│   ├── memory_store/
│   │   ├── memory_store.c     # NVS 持久化
│   │   └── include/memory_store.h
│   └── feedback/
│       ├── haptic.c
│       ├── rgb_strip.c        # SK6812
│       └── include/feedback.h
└── tools/
    ├── chinese_chars.txt      # ★中文字符子集列表
    └── gen_chinese_font.sh    # ★字体生成脚本
```

---

## 二、组件依赖(idf_component.yml)

`main/idf_component.yml`:

```yaml
dependencies:
  idf: ">=5.5.0"               # 锁 5.5 LTS 系列,5.5.4 实测通过
  lvgl/lvgl: "^9.2"            # 9.2 起 ABI 稳定,9.5 实测通过
  espressif/esp_lvgl_port: "^2.4"
  espressif/esp_lcd_ili9341: "^2.0"   # ILI9341/ILI9342 兼容
  espressif/button: "^3.2"
  espressif/led_strip: "^2.5"          # SK6812
  # 可选:接入 ESP-Claw 边缘 Agent(详见附录 I)
  # espressif/esp_board_manager: "^0.5"
```

执行 `idf.py reconfigure` 自动拉取。**严禁**把 LVGL 源码直接 copy 进 `components/`(IDF 5.5 起淘汰),依赖管理一律走 Component Manager 2.x。

---

## 三、硬件引脚映射(M5Stack CoreS3)

| 子系统 | 接口 | 引脚 |
|---|---|---|
| **LCD ILI9342C** | SPI | MOSI=37, SCLK=36, CS=3, DC=35, RST=15, BL=N/A(PMU 控制) |
| **Touch FT6336U** | I2C | SDA=12, SCL=11, INT=21, ADDR=0x38 |
| **IMU BMI270** | I2C(同 Touch 总线) | ADDR=0x68 |
| **ALS LTR-553** | I2C | ADDR=0x23 |
| **PMU AXP2101** | I2C | ADDR=0x34 |
| **RGB SK6812** | RMT | GPIO=18, 数量=3 |
| **LRA Haptic** | DRV2605 / I2C | ADDR=0x5A |
| **MIC ES7210** | I2S | BCK=34, WS=33, DI=14 |
| **SPK AW88298** | I2S(共享) | DO=13 |

---

## 四、显示初始化(LVGL 9 + esp_lvgl_port)

> ⚠️ **本节代码示例为 IDF 5.2 时代基线**,IDF 5.5.4 下**必须按附录 G.3.2 的 `lvgl_port_*` 写法**(`ESP_LVGL_PORT_INIT_CONFIG` + `lvgl_port_add_disp` + `lvgl_port_add_touch`),不再手写 `lv_display_create` / `lv_indev_create`。本节保留是为了让你理解**底层在做什么**;实际工程**直接抄附录 G.3.2 即可**,代码量减半。

`components/bsp_cores3/bsp_display.c`(legacy reference 写法,IDF 5.2 时代):

```c
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lvgl_port.h"
#include "lvgl.h"

#define LCD_H_RES   320
#define LCD_V_RES   240
#define LCD_BIT_PER_PIXEL  16
#define LCD_HOST    SPI2_HOST

static lv_display_t *s_lvgl_disp = NULL;

esp_err_t bsp_display_start(void)
{
    /* 1. SPI 总线 */
    spi_bus_config_t buscfg = {
        .sclk_io_num = 36,
        .mosi_io_num = 37,
        .miso_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_H_RES * 80 * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

    /* 2. Panel IO */
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = 35,
        .cs_gpio_num = 3,
        .pclk_hz = 40 * 1000 * 1000,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST,
                                              &io_config, &io_handle));

    /* 3. ILI9342C 面板(用 ili9341 驱动兼容) */
    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = 15,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
        .bits_per_pixel = LCD_BIT_PER_PIXEL,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_ili9341(io_handle, &panel_config, &panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    /* 4. esp_lvgl_port(自动管理任务和锁) */
    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    ESP_ERROR_CHECK(lvgl_port_init(&lvgl_cfg));

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .buffer_size = LCD_H_RES * 40,
        .double_buffer = true,
        .hres = LCD_H_RES,
        .vres = LCD_V_RES,
        .monochrome = false,
        .rotation = { .swap_xy = false, .mirror_x = false, .mirror_y = false },
        .flags = {
            .buff_dma = false,
            .buff_spiram = true,    // ★ 必须启用 PSRAM
        },
    };
    s_lvgl_disp = lvgl_port_add_disp(&disp_cfg);
    return s_lvgl_disp ? ESP_OK : ESP_FAIL;
}

lv_display_t *bsp_display_get(void) { return s_lvgl_disp; }
```

**重要**:所有 LVGL API 调用必须包裹在 `lvgl_port_lock(0)` / `lvgl_port_unlock()` 之间,否则会与 LVGL 内部任务竞态。

---

## 五、★ 中文显示开发指南(v5.1 新增)

### 5.1 字体方案选型

| 方案 | 大小 | 字符量 | 优劣 |
|---|---|---|---|
| 系统字体(LVGL 内置 montserrat) | 小 | 仅拉丁 | ❌ 不支持中文 |
| 思源黑体 SourceHanSansCN | 16MB+ | 全集 | ❌ Flash 装不下 |
| **★ 思源黑体子集化(常用 1500 字)** | ~80KB(22pt 4bpp) | 常用字 | ✅ 推荐 |
| 文泉驿点阵 | 中 | 全集 | △ 颗粒感强 |

### 5.2 字符子集准备

`tools/chinese_chars.txt`(UTF-8,无 BOM,空白分隔无所谓):

```
你好早安晚再见点击双击长按摇晃倾斜唤醒睡眠待机休息深度浅
菜单设置主题表情对话扩展随机返回确定取消开关启用禁用模式
温馨提示电量低请充电连接成功失败网络断开重试加载请稍候完
今天天气真好充满能量陪我玩会儿无聊好累想睡觉饿了渴了开心
难过生气惊讶好奇害羞调皮可爱萌呆萌得意失落寂寞兴奋平静
小宝贝亲爱的朋友伙伴你真棒厉害加油不要哭别难过抱抱亲亲
版本电池信号WiFi蓝牙更新升级关于设备信息序列号制造商出厂
开发者工具体感测试场景模拟记忆数据日期时间星期一二三四五
六日年月号上下午分秒摄氏度毫秒像素亮度光照声音环境检测到
请勿打扰静音震动响铃模式自动手动调节范围最大最小默认参数
```

> 实际使用时根据 `strings_zh.h` 文本表批量提取所有出现的中文字符,保证全覆盖。建议总量控制在 1200–1800 字。

### 5.3 字体生成

`tools/gen_chinese_font.sh`:

```bash
#!/usr/bin/env bash
set -e
# 安装 lv_font_conv: npm i -g lv_font_conv@1.5.3
# 字体下载: https://github.com/adobe-fonts/source-han-sans

FONT_TTF=./SourceHanSansCN-Regular.otf
RANGE_LATIN="0x20-0x7F"
SUBSET=./tools/chinese_chars.txt

# 22pt 主字体(对话气泡)
lv_font_conv \
  --font ${FONT_TTF} -r ${RANGE_LATIN} \
  --font ${FONT_TTF} --symbols "$(cat ${SUBSET} | tr -d '[:space:]')" \
  --size 22 --bpp 4 --no-compress --format lvgl \
  --output components/lv_chinese_font/font_sourcehans_22.c

# 14pt 副字体(状态栏)
lv_font_conv \
  --font ${FONT_TTF} -r ${RANGE_LATIN} \
  --font ${FONT_TTF} --symbols "$(cat ${SUBSET} | tr -d '[:space:]')" \
  --size 14 --bpp 4 --no-compress --format lvgl \
  --output components/lv_chinese_font/font_sourcehans_14.c

echo "OK: Chinese fonts generated."
```

`components/lv_chinese_font/include/lv_chinese_font.h`:

```c
#pragma once
#include "lvgl.h"

LV_FONT_DECLARE(font_sourcehans_22);
LV_FONT_DECLARE(font_sourcehans_14);
```

`components/lv_chinese_font/CMakeLists.txt`:

```cmake
idf_component_register(
    SRCS "font_sourcehans_22.c" "font_sourcehans_14.c"
    INCLUDE_DIRS "include"
    REQUIRES lvgl
)
```

### 5.4 编译器 UTF-8 配置

`main/CMakeLists.txt` 末尾追加:

```cmake
target_compile_options(${COMPONENT_LIB} PRIVATE
    -finput-charset=UTF-8
    -fexec-charset=UTF-8
)
```

`sdkconfig.defaults`:

```
# 强制 UTF-8 字符串字面量
CONFIG_COMPILER_CXX_RTTI=n
CONFIG_LOG_DEFAULT_LEVEL_INFO=y

# LVGL 9 关键配置
CONFIG_LV_USE_PRIVATE_API=y
CONFIG_LV_FONT_DEFAULT_MONTSERRAT_14=y
CONFIG_LV_USE_FS_POSIX=y
CONFIG_LV_COLOR_DEPTH_16=y
CONFIG_LV_MEM_SIZE_KILOBYTES=64
CONFIG_LV_MEM_USE_STDLIB=y
CONFIG_LV_USE_LABEL=y
CONFIG_LV_LABEL_TEXT_SELECTION=n

# PSRAM 必需(★ CoreS3 是 Quad PSRAM,见附录 I.6)
CONFIG_SPIRAM=y
CONFIG_SPIRAM_MODE_QUAD=y
CONFIG_SPIRAM_SPEED_80M=y
CONFIG_SPIRAM_USE_MALLOC=y
CONFIG_SPIRAM_MALLOC_ALWAYSINTERNAL=16384

# 文件系统(可选,用于动态字体)
CONFIG_PARTITION_TABLE_CUSTOM=y
```

### 5.5 中文文本集中表

`components/ui_core/strings_zh.h`:

```c
#pragma once
/* ★ 所有用户可见中文字符串集中此处,便于子集化覆盖 */

#define STR_GREETING_MORNING    "早上好呀!今天也是充满能量的一天。"
#define STR_LONELY_3MIN         "好无聊哦,陪我玩一会吧..."
#define STR_REWARD              "你真棒!我们已经是好朋友啦~"
#define STR_ANGRY               "别这样对我嘛..."
#define STR_LOW_BATTERY         "电量不足,请充电"
#define STR_WIFI_CONNECTED      "网络连接成功"
#define STR_WIFI_FAILED         "连接失败,请重试"

#define STR_MENU_EXPRESSION     "表情"
#define STR_MENU_DIALOGUE       "对话"
#define STR_MENU_SETTINGS       "设置"
#define STR_MENU_THEME          "主题"
#define STR_MENU_EXTENSIONS     "扩展"
#define STR_MENU_RANDOM         "随机"

#define STR_DEV_TITLE           "开发者工具台"
#define STR_DEV_TAB_IMU         "体感测试"
#define STR_DEV_TAB_SCENARIO    "场景模拟"
#define STR_DEV_TAB_MEMORY      "记忆数据"
```

### 5.6 对话气泡(LVGL 9 + 中文)

`components/ui_core/dialog_bubble.c`:

```c
#include "lvgl.h"
#include "esp_lvgl_port.h"
#include "lv_chinese_font.h"
#include "ui_core.h"

static lv_obj_t *s_bubble = NULL;
static lv_obj_t *s_bubble_label = NULL;

void dialog_bubble_init(void)
{
    lvgl_port_lock(0);

    s_bubble = lv_obj_create(lv_screen_active());      // ★ LVGL 9
    lv_obj_remove_style_all(s_bubble);                 // ★ 替代 lv_obj_clean
    lv_obj_set_size(s_bubble, 280, 56);
    lv_obj_align(s_bubble, LV_ALIGN_TOP_MID, 0, 4);

    /* 毛玻璃背景 */
    lv_obj_set_style_bg_color(s_bubble, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(s_bubble, LV_OPA_60, 0);
    lv_obj_set_style_radius(s_bubble, 16, 0);
    lv_obj_set_style_border_width(s_bubble, 1, 0);
    lv_obj_set_style_border_color(s_bubble, lv_color_hex(0x22D3EE), 0);
    lv_obj_set_style_pad_all(s_bubble, 10, 0);
    lv_obj_add_flag(s_bubble, LV_OBJ_FLAG_HIDDEN);

    s_bubble_label = lv_label_create(s_bubble);
    lv_obj_set_style_text_font(s_bubble_label, &font_sourcehans_22, 0);  // ★ 中文字体
    lv_obj_set_style_text_color(s_bubble_label, lv_color_hex(0xFFFFFF), 0);
    lv_label_set_long_mode(s_bubble_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(s_bubble_label, 260);
    lv_obj_center(s_bubble_label);

    lvgl_port_unlock();
}

void dialog_bubble_show(const char *utf8_text, uint32_t hold_ms)
{
    lvgl_port_lock(0);

    lv_label_set_text(s_bubble_label, utf8_text);   // ★ UTF-8 直接传入
    lv_obj_remove_flag(s_bubble, LV_OBJ_FLAG_HIDDEN);

    /* 入场动画:y -10→0,scale 0.95→1,300ms spring */
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, s_bubble);
    lv_anim_set_values(&a, -10, 4);
    lv_anim_set_duration(&a, 300);     // ★ LVGL 9: set_duration
    lv_anim_set_path_cb(&a, lv_anim_path_overshoot);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_y);
    lv_anim_start(&a);

    /* 自动隐藏(可用 lv_timer 定时) */
    // 略:hold_ms 后调用 dialog_bubble_hide()

    lvgl_port_unlock();
}
```

### 5.7 中文显示验证

启动后调用:

```c
dialog_bubble_show(STR_GREETING_MORNING, 5000);
```

期望屏幕顶部显示完整中文气泡;若出现方块/问号:
1. 检查源文件是否 UTF-8 无 BOM 编码
2. 检查 `chinese_chars.txt` 是否覆盖该字
3. 检查编译器 `-fexec-charset=UTF-8`
4. 检查 `lv_obj_set_style_text_font` 是否传入了 `font_sourcehans_22`

---

## 六、事件总线

`components/ui_core/include/ui_core.h`(节选):

```c
typedef enum {
    EVT_NONE = 0,
    EVT_TAP, EVT_DOUBLE_TAP, EVT_SHAKE,
    EVT_TILT_FORWARD, EVT_TILT_BACK,
    EVT_TILT_LEFT, EVT_TILT_RIGHT,
    EVT_INVERTED, EVT_PICKED_UP,
    EVT_LUX_HIGH, EVT_LUX_LOW,
    EVT_NOISE_HIGH, EVT_QUIET,
    EVT_TIMEOUT_LONELY, EVT_MORNING,
    EVT_TOUCH_DOUBLE_CLICK,
    EVT_BREATH_LEVEL_CHANGED,
    EVT_MAX
} ui_event_t;

typedef struct {
    ui_event_t type;
    int32_t  payload_i;
    float    payload_f;
    uint32_t timestamp_ms;
} ui_event_msg_t;

esp_err_t event_bus_init(void);
esp_err_t event_bus_post(const ui_event_msg_t *msg, TickType_t to);
esp_err_t event_bus_recv(ui_event_msg_t *msg, TickType_t to);
```

底层使用 FreeRTOS Queue,长度 32。

---

## 七、呼吸引擎

`components/ui_core/breathing.c`:

```c
#include <math.h>
#include "ui_core.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef struct {
    uint32_t period_ms;
    float    amplitude;
    float    pupil_scale;
    float    rgb_min, rgb_peak;
} breath_param_t;

static const breath_param_t LEVELS[5] = {
    {8000, 0.05f, 0.60f, 0.10f, 0.20f},   // deep_sleep
    {6000, 0.08f, 0.75f, 0.20f, 0.35f},   // light_rest
    {4000, 0.10f, 0.80f, 0.30f, 0.60f},   // idle
    {2500, 0.15f, 0.85f, 0.50f, 0.80f},   // alert
    {1500, 0.18f, 0.90f, 0.60f, 0.90f},   // excited
};

static float ease_curve(float t01)
{
    if (t01 < 0.20f) {
        float k = t01 / 0.20f;
        return k * k * k;                   // cubic-bezier(0.42,0,1,1) 近似
    } else if (t01 < 0.80f) {
        return 0.20f + (t01 - 0.20f);
    } else {
        float k = (t01 - 0.80f) / 0.20f;
        return 0.80f + (1.0f - powf(1.0f - k, 3.0f)) * 0.20f;
    }
}

float breathing_eval(uint8_t level, uint32_t now_ms,
                     float *out_eye_scale, float *out_pupil_scale, float *out_rgb_alpha)
{
    if (level > 4) level = 4;
    const breath_param_t *p = &LEVELS[level];

    float t01 = (now_ms % p->period_ms) / (float)p->period_ms;
    float eased = ease_curve(t01);
    float wave = sinf(2.0f * (float)M_PI * eased - (float)M_PI / 2.0f);

    float base = 1.0f;
    if (out_eye_scale)   *out_eye_scale   = base + p->amplitude * wave;
    if (out_pupil_scale) *out_pupil_scale = p->pupil_scale + p->amplitude * 0.5f * wave;
    if (out_rgb_alpha)   *out_rgb_alpha   = p->rgb_min + (p->rgb_peak - p->rgb_min) * (0.5f + 0.5f * wave);

    return wave;
}
```

---

## 八、表情渲染(LVGL 9)

`components/ui_core/face_render.c`(节选):

```c
#include "lvgl.h"
#include "esp_lvgl_port.h"
#include "ui_core.h"

static lv_obj_t *s_eye_l = NULL, *s_eye_r = NULL, *s_mouth = NULL;

void face_init(void)
{
    lvgl_port_lock(0);

    lv_obj_t *scr = lv_screen_active();         // ★ LVGL 9
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), 0);

    s_eye_l = lv_obj_create(scr);
    lv_obj_remove_style_all(s_eye_l);
    lv_obj_set_size(s_eye_l, 32, 40);
    lv_obj_set_pos(s_eye_l, 100, 90);
    lv_obj_set_style_bg_color(s_eye_l, lv_color_hex(0x22D3EE), 0);
    lv_obj_set_style_bg_opa(s_eye_l, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(s_eye_l, 16, 0);

    s_eye_r = lv_obj_create(scr);
    lv_obj_remove_style_all(s_eye_r);
    lv_obj_set_size(s_eye_r, 32, 40);
    lv_obj_set_pos(s_eye_r, 188, 90);
    lv_obj_set_style_bg_color(s_eye_r, lv_color_hex(0x22D3EE), 0);
    lv_obj_set_style_radius(s_eye_r, 16, 0);

    s_mouth = lv_obj_create(scr);
    lv_obj_remove_style_all(s_mouth);
    lv_obj_set_size(s_mouth, 24, 4);
    lv_obj_set_pos(s_mouth, 148, 160);
    lv_obj_set_style_bg_color(s_mouth, lv_color_hex(0x22D3EE), 0);
    lv_obj_set_style_radius(s_mouth, 2, 0);

    lvgl_port_unlock();
}

/* 呼吸帧驱动:由 lv_timer 每 33ms 调用 */
void face_apply_breath(uint32_t now_ms, uint8_t level)
{
    float es, ps, ra;
    breathing_eval(level, now_ms, &es, &ps, &ra);

    lvgl_port_lock(0);
    lv_obj_set_style_transform_scale(s_eye_l, (int32_t)(256 * es), 0);   // LVGL 9 用 256=100%
    lv_obj_set_style_transform_scale(s_eye_r, (int32_t)(256 * es), 0);
    lvgl_port_unlock();
}

/* wink 单次动画(1500ms,repeat=0) */
void face_play_wink(void)
{
    lvgl_port_lock(0);

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, s_eye_l);
    lv_anim_set_values(&a, 40, 2);
    lv_anim_set_duration(&a, 400);          // ★ LVGL 9
    lv_anim_set_playback_duration(&a, 400);
    lv_anim_set_repeat_count(&a, 0);        // ★ 必须 0,单次
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_height);
    lv_anim_start(&a);

    lvgl_port_unlock();
}
```

---

## 九、运动检测

`components/motion_detect/motion_detect.c`:

```c
typedef struct {
    float ax, ay, az;       // m/s²
    float gx, gy, gz;       // dps
    uint32_t ts_ms;
} imu_sample_t;

/* 触发阈值 */
#define TAP_THRESHOLD_G        1.5f
#define SHAKE_THRESHOLD_G      2.5f
#define TILT_PITCH_DEG         15.0f
#define TILT_ROLL_DEG          15.0f
#define DEAD_ZONE_DEG          5.0f
#define DEBOUNCE_MS            300

void motion_detect_task(void *arg);
/*
  100Hz 采样 -> 滑窗 5 帧均值 -> 计算 pitch/roll ->
  状态机判定 idle/tilt/shake/tap -> event_bus_post
*/
```

事件映射(对应 v6.0 ScenarioOverlay 的 IMU Tab):

| 触发 | 阈值 | 表情 |
|---|---|---|
| 前倾 | pitch > 15° | curious |
| 后倾 | pitch < -15° | yawn |
| 左倾 | roll > 15° | thinking |
| 右倾 | roll < -15° | surprised |
| 摇晃 | abs(a) > 2.5g 且频率 > 3Hz | dizzy |
| 倒立 | gyro_z 持续反转 | naughty |
| 轻敲 | a_z 瞬时 > 1.5g | wink |

---

## 十、记忆系统(NVS)

`components/memory_store/memory_store.c`:

```c
typedef struct __attribute__((packed)) {
    uint32_t total_interactions;
    uint32_t total_minutes;
    uint16_t streak_days;
    uint16_t emotion_count[22];      // 22 种表情计数(对应 Part 1 §4.1)
    uint8_t  active_hours_bitmap[3]; // 24 小时位图
    uint8_t  preferred_theme;        // 0=tech 1=child 2=dev
    uint8_t  level;                  // 0=陌生 1=熟悉 2=亲密 3=朋友
    uint32_t last_interaction_ts;
} memory_data_t;

esp_err_t memory_load(memory_data_t *out);
esp_err_t memory_save(const memory_data_t *in);
esp_err_t memory_record_emotion(uint8_t emotion_id);
uint8_t   memory_calc_level(const memory_data_t *m);

/* 写入策略:
   - 内存中累积 30s 或 10 次变更触发一次 NVS commit
   - 防 Flash 写穿(NVS 单 key 寿命约 10 万次) */
```

NVS namespace = `"robot_mem"`,key = `"data"`,blob 写入。

---

## 十一、反馈系统

| 模块 | 实现 |
|---|---|
| **Haptic** | DRV2605 库调用预设 effect:click=1, double_click=10, error=27 |
| **RGB** | led_strip 组件,3 颗 SK6812;呼吸帧由 RTOS Task 100ms 更新 HSV |
| **Tone** | I2S DAC 输出正弦波 80ms;频率随事件:tap=600Hz, success=1200Hz |

`feedback.h`:

```c
void feedback_play_click(void);
void feedback_play_success(void);
void feedback_play_error(void);
void feedback_set_rgb(uint8_t theme, float alpha);
```

---

## 十二、构建与烧录

```bash
# 1. 设置目标
idf.py set-target esp32s3

# 2. 生成中文字体
bash tools/gen_chinese_font.sh

# 3. 配置(已通过 sdkconfig.defaults)
idf.py reconfigure

# 4. 编译
idf.py build

# 5. 烧录(假设端口 /dev/ttyACM0)
idf.py -p /dev/ttyACM0 flash monitor
```

预期 RAM 占用(开启 PSRAM):
- 双帧缓存 320×40×2B×2 = 51KB(放 PSRAM)
- LVGL heap 64KB(内部 SRAM)
- 中文字体 ~80KB(常驻 Flash,XIP)

---

## 十三、验收测试清单

| # | 测试项 | 预期 |
|---|---|---|
| 1 | 上电 | 3.5s 内进入 idle 呼吸 |
| 2 | 中文气泡显示 `STR_GREETING_MORNING` | 完整无方块 |
| 3 | 双击屏幕 | 径向菜单 6 按钮弹出,80ms 内完成 |
| 4 | 前倾 > 15° | 进入 curious 表情,2s 后回 idle |
| 5 | 摇晃 5 次 | 进入 angry,RGB 红色快闪 |
| 6 | 30 分钟无交互 | 切换到 lonely 表情 + 黄色慢闪 |
| 7 | 6:00–10:00 首次拿起 | 触发 sleep_wake + 早安气泡 |
| 8 | 切断电源后再上电 | streak_days 等记忆值保留 |
| 9 | 连续 8 小时呼吸 | 无内存泄漏(`heap_caps_get_free_size` 稳定 ±1KB) |
| 10 | wink 动画 | 仅播放 1 次,完成后自动回 idle |

---

## 十四、★ 给 AI 编码助手的硬性约束

**生成代码前必读**,否则一定编译失败:

1. **必须使用 LVGL 9 API**:`lv_display_t`(不是 `lv_disp_t`)、`lv_screen_active()`(不是 `lv_scr_act()`)、`lv_anim_set_duration()`(不是 `set_time()`)、`lv_image_dsc_t`(不是 `lv_img_dsc_t`)
2. **所有 LVGL 调用包裹在 `lvgl_port_lock(0)` / `lvgl_port_unlock()`**,禁止裸调用
3. **中文字符串必须 UTF-8 无 BOM**,源文件保存时确认编码
4. **中文 label 必须显式 `lv_obj_set_style_text_font(label, &font_sourcehans_22, 0)`**,否则使用默认 Montserrat 渲染为方块
5. **新增中文文本必须更新 `tools/chinese_chars.txt` 并重新生成字体**,否则该字渲染为方块
6. **wink/celebrate 等单次动画 `lv_anim_set_repeat_count(&a, 0)`**,绝对不能 `LV_ANIM_REPEAT_INFINITE`
7. **NVS 写入必须批量**,禁止每次事件都 commit
8. **PSRAM 必须启用**(`CONFIG_SPIRAM=y`),320×240 双缓存放不进 SRAM
9. **建议生成顺序**:bsp_display → lv_chinese_font → strings_zh.h → dialog_bubble → face_render → breathing → event_bus → motion_detect → memory_store → feedback → main

---

## 附录 A:main.c 骨架

```c
#include "esp_log.h"
#include "bsp_cores3.h"
#include "ui_core.h"
#include "strings_zh.h"

void app_main(void)
{
    ESP_ERROR_CHECK(bsp_cores3_init());
    ESP_ERROR_CHECK(bsp_display_start());

    event_bus_init();
    face_init();
    dialog_bubble_init();
    radial_menu_init();
    motion_detect_start();
    memory_load_or_create();

    /* 启动呼吸定时器 */
    lvgl_port_lock(0);
    lv_timer_create(breath_tick_cb, 33, NULL);
    lvgl_port_unlock();

    /* 早安问候(若时段匹配) */
    if (is_morning_first_pickup()) {
        face_play_sleep_wake();
        dialog_bubble_show(STR_GREETING_MORNING, 5000);
    }

    /* 主事件循环 */
    ui_event_msg_t msg;
    while (1) {
        if (event_bus_recv(&msg, portMAX_DELAY) == ESP_OK) {
            ui_dispatch(&msg);
        }
    }
}
```

---

## 附录 C:★ 径向菜单实现规格(双击屏幕弹出)

> 之前下游 AI agent 还原度低,本附录给出**像素级几何 + 三主题参考图 + 完整 LVGL 9 C 代码**,严格按此实现即可 1:1 复刻。

### C.1 参考图(三主题最终视觉效果)

| Tech 主题 | Child 主题 | Dev 主题 |
|---|---|---|
| ![Tech 径向菜单](https://p-mira-img-sign-sgnontt.byteintl.net/tos-mya-i-xobrcjvdq7/dbdfa44d71fd4c69b8022ea936dece01.jpeg~tplv-xobrcjvdq7-image-jpeg.jpeg?lk3s=3523e930&x-orig-authkey=miraorigin&x-orig-expires=1810980000&x-orig-sign=GFlUSfGKyta31Nvo9MqcY%2FjFVs0%3D) | ![Child 径向菜单](https://p-mira-img-sign-sgnontt.byteintl.net/tos-mya-i-xobrcjvdq7/0d4169fc3e7b478094671abbc0eaae0a.jpeg~tplv-xobrcjvdq7-image-jpeg.jpeg?lk3s=3523e930&x-orig-authkey=miraorigin&x-orig-expires=1810980000&x-orig-sign=lARXCAfjdOs15YzMvKPiOxF%2FKZs%3D) | ![Dev 径向菜单](https://p-mira-img-sign-sgnontt.byteintl.net/tos-mya-i-xobrcjvdq7/4fce894d7d42498bb1d0c5773bf12a10.jpeg~tplv-xobrcjvdq7-image-jpeg.jpeg?lk3s=3523e930&x-orig-authkey=miraorigin&x-orig-expires=1810980000&x-orig-sign=Xdgp4REFiCCdxqYy%2FT4y%2FM%2FJZ00%3D) |

### C.2 像素级几何参数(必须 1:1 实现)

| 项 | 值 | 说明 |
|---|---|---|
| 屏幕中心 | (160, 120) | 320×240 屏幕的几何中心 |
| 展开半径 | 70 px | 按钮中心到屏幕中心的距离 |
| 按钮尺寸 | 46×46 px | 圆形按钮 |
| 按钮圆角 | 23 px(即 width/2) | 完全圆形 |
| 按钮边框宽度 | 1 px | 主题主色 |
| 按钮外发光模糊 | 15 px(Tech/Dev)/ 12 px(Child) | LVGL 用 `shadow_width` |
| 按钮入场弹簧 | damping=14, stiffness=150 | 仅做近似:overshoot 路径 |
| 按钮入场延迟 | 50ms × index(顺时针) | 索引 0→5 |
| Hover 缩放 | 1.1× | 中心按钮 1.1 倍放大 |
| Hover 亮度 | +50% glow,边框加粗到 2px | 主题主色变亮版 |
| 中心标签字号 | 14pt(font_sourcehans_14) | 仅 hover 时显示 |
| 中心标签位置 | (160, 120) 居中 | drop-shadow 8px |
| 背景遮罩 | 80% 黑(Tech/Dev)/ 90% 米黄(Child) | + 4px 高斯模糊 |
| 收起触发 | 点击背景 / 2000ms 超时 | 反向 200ms 淡出 |

### C.3 六按钮布局(顺时针,index 0–5)

| index | angle | 极坐标 → 屏幕坐标(中心偏移) | 图标(lucide) | label | 触发动作 |
|---|---|---|---|---|---|
| 0 | -90° | (0, -70) → 屏幕 (160, 50) | smile | 表情 | `navigateTo("random")` |
| 1 | -30° | (+60.6, -35) → (220.6, 85) | message-circle | 对话 | `navigateTo("talking")` |
| 2 | +30° | (+60.6, +35) → (220.6, 155) | settings | 设置 | `navigateTo("wifi_ap")` |
| 3 | +90° | (0, +70) → (160, 190) | palette | 主题 | 切换 theme 循环 |
| 4 | +150° | (-60.6, +35) → (99.4, 155) | puzzle | 扩展 | `navigateTo("scenario_sim")` |
| 5 | +210° / -150° | (-60.6, -35) → (99.4, 85) | shuffle | 随机 | `navigateTo("random")` |

> 极坐标公式:`x = cos(angle·π/180) × 70`,`y = sin(angle·π/180) × 70`(屏幕坐标系 y 向下为正)。

### C.4 三主题样式 token(C 结构体)

`components/ui_core/radial_menu.c` 头部:

```c
#include "lvgl.h"
#include "esp_lvgl_port.h"
#include "lv_chinese_font.h"
#include "ui_core.h"
#include "strings_zh.h"
#include <math.h>

typedef struct {
    uint32_t bg_overlay;      /* 背景遮罩色 ARGB */
    lv_opa_t bg_opa;
    uint32_t btn_bg;          /* 按钮底色 */
    uint32_t btn_border;      /* 按钮边框/图标色(主色) */
    uint32_t btn_hover_bg;    /* hover 按钮底色 */
    uint32_t btn_hover_border;
    uint32_t glow_color;      /* 外发光色 */
    uint8_t  glow_width;      /* 外发光半径 */
    uint32_t label_color;     /* 中心标签色 */
} menu_theme_t;

static const menu_theme_t THEMES[3] = {
    /* Tech */
    { .bg_overlay=0x000000, .bg_opa=LV_OPA_80,
      .btn_bg=0x082F49, .btn_border=0x22D3EE,
      .btn_hover_bg=0x0E4D6B, .btn_hover_border=0x67E8F4,
      .glow_color=0x22D3EE, .glow_width=15,
      .label_color=0x67E8F4 },
    /* Child */
    { .bg_overlay=0xFFF9E6, .bg_opa=LV_OPA_90,
      .btn_bg=0xFFFFFF, .btn_border=0xFFB59A,
      .btn_hover_bg=0xFFE4D6, .btn_hover_border=0xFF7F50,
      .glow_color=0xFF7F50, .glow_width=12,
      .label_color=0xFF7F50 },
    /* Dev */
    { .bg_overlay=0x000000, .bg_opa=LV_OPA_80,
      .btn_bg=0x052E16, .btn_border=0x22C55E,
      .btn_hover_bg=0x14532D, .btn_hover_border=0x4ADE80,
      .glow_color=0x22C55E, .glow_width=12,
      .label_color=0x4ADE80 },
};
```

### C.5 六按钮定义表(C 数组)

```c
typedef struct {
    const char *label_utf8;      /* "表情" 等,需在 chinese_chars.txt 中 */
    const char *icon_symbol;     /* LVGL 内置符号 fallback */
    int16_t     angle_deg;
    void      (*on_select)(void);
} menu_item_t;

static void on_sel_expression(void) { event_bus_post_simple(EVT_GOTO_RANDOM); }
static void on_sel_dialogue  (void) { event_bus_post_simple(EVT_GOTO_TALKING); }
static void on_sel_settings  (void) { event_bus_post_simple(EVT_GOTO_WIFI_AP); }
static void on_sel_theme     (void) { event_bus_post_simple(EVT_THEME_NEXT); }
static void on_sel_extensions(void) { event_bus_post_simple(EVT_GOTO_SCENARIO); }
static void on_sel_random    (void) { event_bus_post_simple(EVT_GOTO_RANDOM); }

static const menu_item_t ITEMS[6] = {
    { "表情", LV_SYMBOL_EYE_OPEN,  -90, on_sel_expression },
    { "对话", LV_SYMBOL_BELL,      -30, on_sel_dialogue   },
    { "设置", LV_SYMBOL_SETTINGS,   30, on_sel_settings   },
    { "主题", LV_SYMBOL_IMAGE,      90, on_sel_theme      },
    { "扩展", LV_SYMBOL_LIST,      150, on_sel_extensions },
    { "随机", LV_SYMBOL_REFRESH,   210, on_sel_random     },
};
```

> ⚠️ **图标资源建议**:LVGL 内置符号风格与 lucide 不完全一致。**推荐**用 lv_img_conv 将 Part 1 §10 的 6 个 24×24 lucide SVG 转为 `lv_image_dsc_t`(命名 `icon_smile`, `icon_message`, `icon_settings`, `icon_palette`, `icon_puzzle`, `icon_shuffle`),并把上表 `icon_symbol` 替换为对应 `&icon_xxx`,用 `lv_image_create` 代替 `lv_label_create`。

### C.6 完整 LVGL 9 实现

```c
#define RADIUS_PX        70
#define BTN_SIZE_PX      46
#define MENU_HIDE_TIMEOUT_MS  2000

static lv_obj_t *s_menu_root = NULL;
static lv_obj_t *s_buttons[6] = {0};
static lv_obj_t *s_center_label = NULL;
static lv_timer_t *s_auto_hide_timer = NULL;
static uint8_t s_current_theme = 0;
static int8_t  s_hover_index = -1;

/* 1. 创建/销毁 */
static void radial_menu_close_cb(lv_event_t *e);
static void btn_pressed_cb(lv_event_t *e);
static void btn_hover_cb(lv_event_t *e);
static void auto_hide_cb(lv_timer_t *t);

void radial_menu_open(uint8_t theme)
{
    if (s_menu_root) return;
    if (theme > 2) theme = 0;
    s_current_theme = theme;
    const menu_theme_t *th = &THEMES[theme];

    lvgl_port_lock(0);

    /* 1.1 全屏遮罩层 */
    s_menu_root = lv_obj_create(lv_screen_active());
    lv_obj_remove_style_all(s_menu_root);
    lv_obj_set_size(s_menu_root, 320, 240);
    lv_obj_set_pos(s_menu_root, 0, 0);
    lv_obj_set_style_bg_color(s_menu_root, lv_color_hex(th->bg_overlay), 0);
    lv_obj_set_style_bg_opa(s_menu_root, th->bg_opa, 0);
    lv_obj_clear_flag(s_menu_root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_menu_root, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(s_menu_root, radial_menu_close_cb, LV_EVENT_CLICKED, NULL);

    /* 入场动画 (root 0→1 200ms) */
    lv_obj_set_style_opa(s_menu_root, LV_OPA_TRANSP, 0);
    lv_anim_t fa;
    lv_anim_init(&fa);
    lv_anim_set_var(&fa, s_menu_root);
    lv_anim_set_values(&fa, 0, 255);
    lv_anim_set_duration(&fa, 200);
    lv_anim_set_exec_cb(&fa, (lv_anim_exec_xcb_t)lv_obj_set_style_opa);
    lv_anim_start(&fa);

    /* 1.2 六按钮 */
    for (int i = 0; i < 6; i++) {
        lv_obj_t *btn = lv_btn_create(s_menu_root);
        lv_obj_remove_style_all(btn);
        lv_obj_set_size(btn, BTN_SIZE_PX, BTN_SIZE_PX);
        lv_obj_set_style_radius(btn, BTN_SIZE_PX / 2, 0);
        lv_obj_set_style_bg_color(btn, lv_color_hex(th->btn_bg), 0);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(btn, 1, 0);
        lv_obj_set_style_border_color(btn, lv_color_hex(th->btn_border), 0);
        lv_obj_set_style_shadow_width(btn, th->glow_width, 0);
        lv_obj_set_style_shadow_color(btn, lv_color_hex(th->glow_color), 0);
        lv_obj_set_style_shadow_opa(btn, LV_OPA_60, 0);
        lv_obj_set_style_shadow_spread(btn, 0, 0);

        /* hover 态额外样式(LV_STATE_PRESSED 替代 hover,触屏无 hover) */
        lv_obj_set_style_bg_color(btn, lv_color_hex(th->btn_hover_bg), LV_STATE_PRESSED);
        lv_obj_set_style_border_color(btn, lv_color_hex(th->btn_hover_border), LV_STATE_PRESSED);
        lv_obj_set_style_border_width(btn, 2, LV_STATE_PRESSED);
        lv_obj_set_style_transform_scale(btn, 282, LV_STATE_PRESSED); /* 256=1.0,282≈1.1 */

        /* 图标(label 占位,实际请替换为 lv_image_create + lv_image_dsc_t) */
        lv_obj_t *icon = lv_label_create(btn);
        lv_label_set_text(icon, ITEMS[i].icon_symbol);
        lv_obj_set_style_text_color(icon, lv_color_hex(th->btn_border), 0);
        lv_obj_set_style_text_font(icon, &lv_font_montserrat_14, 0);
        lv_obj_center(icon);

        /* 起始位置:屏幕中心,scale 0(入场动画从中心弹出到目标位置) */
        lv_obj_set_pos(btn, 160 - BTN_SIZE_PX/2, 120 - BTN_SIZE_PX/2);
        lv_obj_set_style_transform_scale(btn, 0, 0);

        /* 目标坐标 */
        float rad = ITEMS[i].angle_deg * (float)M_PI / 180.0f;
        int16_t tx = (int16_t)(160 + cosf(rad) * RADIUS_PX) - BTN_SIZE_PX/2;
        int16_t ty = (int16_t)(120 + sinf(rad) * RADIUS_PX) - BTN_SIZE_PX/2;

        /* 入场:x/y 平移 + scale 0→256,300ms,overshoot 模拟弹簧 */
        uint32_t delay = i * 50;

        lv_anim_t ax;
        lv_anim_init(&ax);
        lv_anim_set_var(&ax, btn);
        lv_anim_set_values(&ax, 160 - BTN_SIZE_PX/2, tx);
        lv_anim_set_duration(&ax, 300);
        lv_anim_set_delay(&ax, delay);
        lv_anim_set_path_cb(&ax, lv_anim_path_overshoot);
        lv_anim_set_exec_cb(&ax, (lv_anim_exec_xcb_t)lv_obj_set_x);
        lv_anim_start(&ax);

        lv_anim_t ay;
        lv_anim_init(&ay);
        lv_anim_set_var(&ay, btn);
        lv_anim_set_values(&ay, 120 - BTN_SIZE_PX/2, ty);
        lv_anim_set_duration(&ay, 300);
        lv_anim_set_delay(&ay, delay);
        lv_anim_set_path_cb(&ay, lv_anim_path_overshoot);
        lv_anim_set_exec_cb(&ay, (lv_anim_exec_xcb_t)lv_obj_set_y);
        lv_anim_start(&ay);

        lv_anim_t as;
        lv_anim_init(&as);
        lv_anim_set_var(&as, btn);
        lv_anim_set_values(&as, 0, 256);
        lv_anim_set_duration(&as, 300);
        lv_anim_set_delay(&as, delay);
        lv_anim_set_path_cb(&as, lv_anim_path_overshoot);
        lv_anim_set_exec_cb(&as,
            (lv_anim_exec_xcb_t)lv_obj_set_style_transform_scale_safe);
        lv_anim_start(&as);

        lv_obj_add_event_cb(btn, btn_pressed_cb, LV_EVENT_CLICKED, (void*)(intptr_t)i);
        lv_obj_add_event_cb(btn, btn_hover_cb,   LV_EVENT_PRESSED, (void*)(intptr_t)i);
        lv_obj_add_event_cb(btn, btn_hover_cb,   LV_EVENT_RELEASED, (void*)(intptr_t)-1);

        s_buttons[i] = btn;
    }

    /* 1.3 中心标签(初始隐藏) */
    s_center_label = lv_label_create(s_menu_root);
    lv_obj_set_style_text_font(s_center_label, &font_sourcehans_14, 0);
    lv_obj_set_style_text_color(s_center_label, lv_color_hex(th->label_color), 0);
    lv_label_set_text(s_center_label, "");
    lv_obj_align(s_center_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_flag(s_center_label, LV_OBJ_FLAG_HIDDEN);

    /* 1.4 2 秒自动收起 */
    s_auto_hide_timer = lv_timer_create(auto_hide_cb, MENU_HIDE_TIMEOUT_MS, NULL);
    lv_timer_set_repeat_count(s_auto_hide_timer, 1);

    /* 1.5 反馈 */
    feedback_play_click();

    lvgl_port_unlock();
}

/* 包装函数:LVGL 9 的 transform_scale 在 0 时会触发警告,做安全包装 */
static void lv_obj_set_style_transform_scale_safe(lv_obj_t *obj, int32_t v)
{
    if (v < 1) v = 1;
    lv_obj_set_style_transform_scale(obj, v, 0);
}

/* 2. hover 切换中心标签 */
static void btn_hover_cb(lv_event_t *e)
{
    intptr_t idx = (intptr_t)lv_event_get_user_data(e);
    if (idx < 0 || idx >= 6) {
        lv_obj_add_flag(s_center_label, LV_OBJ_FLAG_HIDDEN);
        s_hover_index = -1;
        return;
    }
    s_hover_index = (int8_t)idx;
    lv_label_set_text(s_center_label, ITEMS[idx].label_utf8);
    lv_obj_remove_flag(s_center_label, LV_OBJ_FLAG_HIDDEN);
    feedback_play_hover_tone(400 + idx * 50);
}

/* 3. 点击按钮 */
static void btn_pressed_cb(lv_event_t *e)
{
    intptr_t idx = (intptr_t)lv_event_get_user_data(e);
    if (idx < 0 || idx >= 6) return;

    /* 选中闪光:scale 1.2× + 80ms */
    lv_obj_t *btn = s_buttons[idx];
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, btn);
    lv_anim_set_values(&a, 256, 307); /* 1.2× */
    lv_anim_set_duration(&a, 80);
    lv_anim_set_playback_duration(&a, 80);
    lv_anim_set_repeat_count(&a, 0);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_style_transform_scale_safe);
    lv_anim_start(&a);

    feedback_play_success();
    ITEMS[idx].on_select();

    /* 200ms 后关闭菜单 */
    lv_timer_t *close_t = lv_timer_create((lv_timer_cb_t)radial_menu_close_internal,
                                          200, NULL);
    lv_timer_set_repeat_count(close_t, 1);
}

/* 4. 关闭(点击背景 / 超时) */
static void radial_menu_close_cb(lv_event_t *e)
{
    if (lv_event_get_target_obj(e) != s_menu_root) return;
    radial_menu_close_internal(NULL);
}
static void auto_hide_cb(lv_timer_t *t)
{
    (void)t;
    radial_menu_close_internal(NULL);
}

void radial_menu_close_internal(lv_timer_t *t)
{
    (void)t;
    if (!s_menu_root) return;
    lvgl_port_lock(0);

    /* 反向 200ms 淡出 */
    lv_anim_t fa;
    lv_anim_init(&fa);
    lv_anim_set_var(&fa, s_menu_root);
    lv_anim_set_values(&fa, 255, 0);
    lv_anim_set_duration(&fa, 200);
    lv_anim_set_exec_cb(&fa, (lv_anim_exec_xcb_t)lv_obj_set_style_opa);
    lv_anim_set_completed_cb(&fa, (lv_anim_completed_cb_t)lv_obj_del_async);
    lv_anim_start(&fa);

    if (s_auto_hide_timer) { lv_timer_del(s_auto_hide_timer); s_auto_hide_timer = NULL; }
    for (int i=0;i<6;i++) s_buttons[i] = NULL;
    s_center_label = NULL;
    s_menu_root = NULL;

    lvgl_port_unlock();
}
```

### C.7 双击触发接入

在 `main.c` / 触摸事件分发处:

```c
static uint32_t s_last_tap_ms = 0;
void on_screen_tap(void)
{
    uint32_t now = lv_tick_get();
    if (now - s_last_tap_ms < 350) {
        radial_menu_open(g_current_theme);
        s_last_tap_ms = 0;
    } else {
        s_last_tap_ms = now;
    }
}
```

### C.8 验收点(逐项核对参考图)

| 检查项 | 通过标准 |
|---|---|
| 六按钮位置 | 与图中 12/2/4/6/8/10 点钟方向完全一致,半径 70px |
| 按钮尺寸 | 46×46px 圆形,边框 1px 主色 |
| 按钮外发光 | 主色光晕,Tech/Dev 15px,Child 12px |
| 入场顺序 | 顺时针,index 0→5 每个延迟 50ms |
| 入场曲线 | overshoot(弹簧近似),300ms |
| Hover 态 | 1.1× 放大 + 边框 2px + 颜色变亮 |
| 中心标签 | 仅 hover 显示,中文 14pt,有发光阴影 |
| 背景遮罩 | Tech/Dev 80% 黑,Child 90% 米黄 |
| 点击背景关闭 | 200ms 淡出后销毁 |
| 2s 超时关闭 | 同上 |
| 选中反馈 | 1.2× 闪光 80ms + 触觉 + 音效 |

---

## 附录 D:★ 系统设置页实现规格(对齐 v6.1 原型)

> **本附录基于陪伴机器人 UI 原型 v6.1 的 `SettingsOverlay.tsx`**:径向菜单"设置"项现在跳转独立设置页(`state=settings`),不再直接跳 `wifi_ap`。

### D.1 三主题参考图

| Tech | Child | Dev |
|---|---|---|
| ![Tech 设置](https://p-mira-img-sign-sgnontt.byteintl.net/tos-mya-i-xobrcjvdq7/1acebde2758f42129e9998f43211c62d.jpeg~tplv-xobrcjvdq7-image-jpeg.jpeg?lk3s=3523e930&x-orig-authkey=miraorigin&x-orig-expires=1810983600&x-orig-sign=JcmqiiW9zinQajGXtCG29eo6OrM%3D) | ![Child 设置](https://p-mira-img-sign-sgnontt.byteintl.net/tos-mya-i-xobrcjvdq7/fbb8935fac404c2e947c1ad8d117e9ff.jpeg~tplv-xobrcjvdq7-image-jpeg.jpeg?lk3s=3523e930&x-orig-authkey=miraorigin&x-orig-expires=1810983600&x-orig-sign=VDqw7WJgJ0kt9WmFL%2BWA36jyx8s%3D) | ![Dev 设置](https://p-mira-img-sign-sgnontt.byteintl.net/tos-mya-i-xobrcjvdq7/d3e603d0546c47f7bd8485da49187853.jpeg~tplv-xobrcjvdq7-image-jpeg.jpeg?lk3s=3523e930&x-orig-authkey=miraorigin&x-orig-expires=1810983600&x-orig-sign=oMFan2jAT6V%2F2T14RlQlu8w%2BR2g%3D) |

### D.2 页面布局

```
┌─────────────────────────────────────┐ 0
│ [<]      系统设置                    │ 40   头部栏
├─────────────────────────────────────┤
│ 🔊 ████████░░░  70%                  │      Section 1
│ ☀  █████████░  80%                   │      滑块控件
├─────────────────────────────────────┤
│ 📶 Wi-Fi 网络     未连接       >    │
│ 🎙 语音设定       默认         >    │      Section 2
│ ⏰ 睡眠定时       30 分钟      >    │      行动项
├─────────────────────────────────────┤
│ 👤 账号绑定       已绑定 User01 >   │
│ 🔄 检查更新       当前 v6.0    >    │      Section 3
│ ⚠  出厂重置                    >    │      (red)
└─────────────────────────────────────┘ 240
```

### D.3 控件规格

| 控件 | 数值 | LVGL 9 对应 |
|---|---|---|
| 头部栏高度 | 40 px | `lv_obj_t *header` |
| 返回按钮 | 28×28 圆形,左上 6px 内距 | `lv_btn_create` |
| 标题字号 | 12pt(font_sourcehans_14) | `lv_label_create` |
| 滑块行高 | 28 px,内距 8 px | `lv_slider_create` |
| 滑块轨道高 | 6 px,圆角 3 px | `lv_obj_set_style_height` |
| 滑块滑块尺寸 | 0(纯填充条,无握把) | `LV_PART_KNOB` 透明 |
| 行动项行高 | 32 px,圆角 6 px,边框 1 px | `lv_btn_create` |
| 行动项左图标 | 14 px | `lv_label_create + 符号` |
| 行动项右指示 | ChevronRight 12 px | `LV_SYMBOL_RIGHT` |
| 项间距 | Section 内 6 px,Section 间 12 px | `lv_obj_set_style_pad_*` |

### D.4 数据模型(NVS 持久化)

`components/settings_store/settings_store.h`:

```c
typedef struct __attribute__((packed)) {
    uint8_t  volume;                /* 0-100 */
    uint8_t  brightness;            /* 0-100 */
    uint8_t  voice_id;              /* 0=默认,1-N 预设音色 */
    uint16_t sleep_timeout_min;     /* 0=永不,1-1440 */
    char     bound_account[32];     /* "User01"/"" */
    char     current_version[16];   /* "v6.0" */
    uint8_t  theme;                 /* 0=tech 1=child 2=dev,本字段已有则覆盖 */
    uint8_t  reserved[8];
} settings_data_t;

esp_err_t settings_load(settings_data_t *out);
esp_err_t settings_save(const settings_data_t *in);

/* 单字段快捷设置(内部累积 1s 后批量 commit,防 Flash 写穿) */
void settings_set_volume(uint8_t v);
void settings_set_brightness(uint8_t v);
void settings_set_sleep_timeout(uint16_t min);
```

NVS namespace = `"robot_set"`,key = `"data"`,blob 写入。

### D.5 副作用绑定

设置值变更必须触发对应硬件动作:

| 设置项 | 副作用 | 实现位置 |
|---|---|---|
| volume | I2S DAC 增益 + RGB 反馈音音量 | `audio_set_volume(v)` 在 feedback.c |
| brightness | AXP2101 背光 PWM | `bsp_pmu_set_backlight(v)` |
| voice_id | TTS/预录音色切换 | `tts_set_voice(id)` |
| sleep_timeout_min | 进入 `deep_sleep` 的超时阈值 | breathing 状态机参数 |
| 出厂重置 | NVS 全擦除 + 重启 | `nvs_flash_erase` + `esp_restart` |

### D.6 完整 LVGL 9 C 实现(片段)

```c
/* components/ui_core/settings_overlay.c */
#include "lvgl.h"
#include "esp_lvgl_port.h"
#include "lv_chinese_font.h"
#include "settings_store.h"
#include "ui_core.h"
#include "strings_zh.h"

static lv_obj_t *s_settings_root = NULL;
static settings_data_t s_cur;

static void on_slider_volume(lv_event_t *e)
{
    lv_obj_t *s = lv_event_get_target_obj(e);
    int32_t v = lv_slider_get_value(s);
    s_cur.volume = (uint8_t)v;
    settings_set_volume(s_cur.volume);
}

static void on_slider_brightness(lv_event_t *e)
{
    lv_obj_t *s = lv_event_get_target_obj(e);
    int32_t v = lv_slider_get_value(s);
    s_cur.brightness = (uint8_t)v;
    settings_set_brightness(s_cur.brightness);
}

static void on_action_wifi(lv_event_t *e)      { event_bus_post_simple(EVT_GOTO_WIFI_AP); }
static void on_action_voice(lv_event_t *e)     { event_bus_post_simple(EVT_GOTO_VOICE_PICK); }
static void on_action_sleep(lv_event_t *e)     { event_bus_post_simple(EVT_GOTO_SLEEP_PICK); }
static void on_action_account(lv_event_t *e)   { event_bus_post_simple(EVT_GOTO_ACCOUNT); }
static void on_action_update(lv_event_t *e)    { event_bus_post_simple(EVT_GOTO_OTA); }
static void on_action_reset(lv_event_t *e)     { event_bus_post_simple(EVT_FACTORY_RESET_CONFIRM); }
static void on_back(lv_event_t *e)             { settings_overlay_close(); event_bus_post_simple(EVT_GOTO_MENU); }

/* 构建一个 ActionItem 行 */
static lv_obj_t *build_action_item(lv_obj_t *parent, const char *icon_sym,
                                   const char *label_utf8, const char *value_utf8,
                                   lv_event_cb_t cb, bool is_alert)
{
    const menu_theme_t *th = &THEMES[s_cur.theme];
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_remove_style_all(btn);
    lv_obj_set_size(btn, LV_PCT(100), 32);
    lv_obj_set_style_radius(btn, 6, 0);
    lv_obj_set_style_border_width(btn, 1, 0);
    lv_obj_set_style_pad_left(btn, 8, 0);
    lv_obj_set_style_pad_right(btn, 8, 0);

    if (is_alert) {
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x2D0B14), 0);
        lv_obj_set_style_border_color(btn, lv_color_hex(0xF43F5E), 0);
    } else {
        lv_obj_set_style_bg_color(btn, lv_color_hex(th->btn_bg), 0);
        lv_obj_set_style_border_color(btn, lv_color_hex(th->btn_border), 0);
        lv_obj_set_style_border_opa(btn, LV_OPA_40, 0);
    }

    /* 左侧图标 */
    lv_obj_t *icon = lv_label_create(btn);
    lv_label_set_text(icon, icon_sym);
    lv_obj_set_style_text_color(icon,
        lv_color_hex(is_alert ? 0xF43F5E : th->btn_border), 0);
    lv_obj_align(icon, LV_ALIGN_LEFT_MID, 0, 0);

    /* 标签 */
    lv_obj_t *label = lv_label_create(btn);
    lv_obj_set_style_text_font(label, &font_sourcehans_14, 0);
    lv_label_set_text(label, label_utf8);
    lv_obj_set_style_text_color(label,
        lv_color_hex(is_alert ? 0xF43F5E : th->btn_border), 0);
    lv_obj_align_to(label, icon, LV_ALIGN_OUT_RIGHT_MID, 8, 0);

    /* 右侧 sub-value */
    if (value_utf8 && value_utf8[0]) {
        lv_obj_t *val = lv_label_create(btn);
        lv_obj_set_style_text_font(val, &font_sourcehans_14, 0);
        lv_label_set_text(val, value_utf8);
        lv_obj_set_style_text_color(val,
            lv_color_hex(is_alert ? 0x9F1239 : th->btn_border), 0);
        lv_obj_set_style_text_opa(val, LV_OPA_60, 0);
        lv_obj_align(val, LV_ALIGN_RIGHT_MID, -16, 0);
    }

    /* 右侧 chevron */
    lv_obj_t *arrow = lv_label_create(btn);
    lv_label_set_text(arrow, LV_SYMBOL_RIGHT);
    lv_obj_set_style_text_color(arrow,
        lv_color_hex(is_alert ? 0xF43F5E : th->btn_border), 0);
    lv_obj_set_style_text_opa(arrow, LV_OPA_60, 0);
    lv_obj_align(arrow, LV_ALIGN_RIGHT_MID, 0, 0);

    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, NULL);
    return btn;
}

/* 构建滑块行 */
static lv_obj_t *build_slider_row(lv_obj_t *parent, const char *icon_sym,
                                  int32_t value, lv_event_cb_t cb)
{
    const menu_theme_t *th = &THEMES[s_cur.theme];
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, LV_PCT(100), 28);
    lv_obj_set_style_radius(row, 6, 0);
    lv_obj_set_style_bg_color(row, lv_color_hex(th->btn_bg), 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_40, 0);
    lv_obj_set_style_border_width(row, 1, 0);
    lv_obj_set_style_border_color(row, lv_color_hex(th->btn_border), 0);
    lv_obj_set_style_border_opa(row, LV_OPA_40, 0);
    lv_obj_set_style_pad_all(row, 6, 0);

    lv_obj_t *icon = lv_label_create(row);
    lv_label_set_text(icon, icon_sym);
    lv_obj_set_style_text_color(icon, lv_color_hex(th->btn_border), 0);
    lv_obj_align(icon, LV_ALIGN_LEFT_MID, 0, 0);

    lv_obj_t *slider = lv_slider_create(row);
    lv_obj_set_size(slider, 180, 6);
    lv_obj_align_to(slider, icon, LV_ALIGN_OUT_RIGHT_MID, 8, 0);
    lv_slider_set_range(slider, 0, 100);
    lv_slider_set_value(slider, value, LV_ANIM_OFF);
    /* 主题色填充 */
    lv_obj_set_style_bg_color(slider, lv_color_hex(th->bg_overlay), LV_PART_MAIN);
    lv_obj_set_style_bg_color(slider, lv_color_hex(th->btn_border), LV_PART_INDICATOR);
    /* 隐藏 KNOB(无握把,纯填充条) */
    lv_obj_set_style_bg_opa(slider, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_pad_all(slider, 0, LV_PART_KNOB);
    lv_obj_add_event_cb(slider, cb, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t *pct = lv_label_create(row);
    char buf[8]; snprintf(buf, sizeof(buf), "%ld%%", (long)value);
    lv_label_set_text(pct, buf);
    lv_obj_set_style_text_color(pct, lv_color_hex(th->btn_border), 0);
    lv_obj_set_style_text_opa(pct, LV_OPA_60, 0);
    lv_obj_align(pct, LV_ALIGN_RIGHT_MID, 0, 0);

    return row;
}

void settings_overlay_open(void)
{
    if (s_settings_root) return;
    settings_load(&s_cur);

    const menu_theme_t *th = &THEMES[s_cur.theme];
    lvgl_port_lock(0);

    s_settings_root = lv_obj_create(lv_screen_active());
    lv_obj_remove_style_all(s_settings_root);
    lv_obj_set_size(s_settings_root, 320, 240);
    lv_obj_set_style_bg_color(s_settings_root, lv_color_hex(th->bg_overlay), 0);
    lv_obj_set_style_bg_opa(s_settings_root, LV_OPA_COVER, 0);

    /* 头部 */
    lv_obj_t *header = lv_obj_create(s_settings_root);
    lv_obj_remove_style_all(header);
    lv_obj_set_size(header, 320, 40);
    lv_obj_t *back = lv_btn_create(header);
    lv_obj_set_size(back, 28, 28);
    lv_obj_set_style_radius(back, 14, 0);
    lv_obj_align(back, LV_ALIGN_LEFT_MID, 8, 0);
    lv_obj_add_event_cb(back, on_back, LV_EVENT_CLICKED, NULL);
    lv_obj_t *bi = lv_label_create(back);
    lv_label_set_text(bi, LV_SYMBOL_LEFT);
    lv_obj_center(bi);

    lv_obj_t *title = lv_label_create(header);
    lv_obj_set_style_text_font(title, &font_sourcehans_14, 0);
    lv_label_set_text(title, "系统设置");
    lv_obj_set_style_text_color(title, lv_color_hex(th->btn_border), 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);

    /* 滚动列表 */
    lv_obj_t *list = lv_obj_create(s_settings_root);
    lv_obj_remove_style_all(list);
    lv_obj_set_size(list, 320, 200);
    lv_obj_set_pos(list, 0, 40);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(list, 8, 0);
    lv_obj_set_style_pad_row(list, 6, 0);
    lv_obj_set_scroll_dir(list, LV_DIR_VER);

    /* Section 1: sliders */
    build_slider_row(list, LV_SYMBOL_VOLUME_MAX, s_cur.volume,     on_slider_volume);
    build_slider_row(list, LV_SYMBOL_IMAGE,      s_cur.brightness, on_slider_brightness);

    /* Section 2: configurations */
    build_action_item(list, LV_SYMBOL_WIFI,  "Wi-Fi 网络", "未连接",   on_action_wifi,    false);
    build_action_item(list, LV_SYMBOL_AUDIO, "语音设定",   "默认",     on_action_voice,   false);
    build_action_item(list, LV_SYMBOL_LOOP,  "睡眠定时",   "30 分钟",  on_action_sleep,   false);

    /* Section 3: system */
    build_action_item(list, LV_SYMBOL_OK,        "账号绑定", s_cur.bound_account[0]? s_cur.bound_account : "未绑定", on_action_account, false);
    build_action_item(list, LV_SYMBOL_REFRESH,   "检查更新", s_cur.current_version,                                   on_action_update,  false);
    build_action_item(list, LV_SYMBOL_WARNING,   "出厂重置", "",                                                       on_action_reset,   true);

    /* 入场动画 opacity + scale 0.95→1 */
    lv_obj_set_style_opa(s_settings_root, LV_OPA_TRANSP, 0);
    lv_obj_set_style_transform_scale(s_settings_root, 245, 0);   /* 0.95 */
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, s_settings_root);
    lv_anim_set_values(&a, 0, 255);
    lv_anim_set_duration(&a, 200);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_style_opa);
    lv_anim_start(&a);
    lv_anim_set_values(&a, 245, 256);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_style_transform_scale_safe);
    lv_anim_start(&a);

    lvgl_port_unlock();
}

void settings_overlay_close(void)
{
    if (!s_settings_root) return;
    settings_save(&s_cur);
    lvgl_port_lock(0);
    lv_obj_del_async(s_settings_root);
    s_settings_root = NULL;
    lvgl_port_unlock();
}
```

### D.7 路由变更(v6.0 → v6.1)

| 路由 | v6.0 | v6.1 |
|---|---|---|
| 径向菜单"设置"点击后 | `state=wifi_ap` | `state=settings` |
| 设置页 Wi-Fi 项点击后 | — | `state=wifi_ap` |
| 设置页返回 | — | `state=menu` |

工程上对应 `event_bus.h` 新增事件:

```c
EVT_GOTO_SETTINGS,       // 径向菜单"设置"→ settings_overlay_open()
EVT_GOTO_WIFI_AP,        // 设置页"Wi-Fi"→ wifi_setup_open(WIFI_AP)
EVT_GOTO_VOICE_PICK,     // 语音设定子页(待实现)
EVT_GOTO_SLEEP_PICK,     // 睡眠定时选择(待实现)
EVT_GOTO_ACCOUNT,        // 账号绑定页(待实现)
EVT_GOTO_OTA,            // OTA 检查更新(待实现)
EVT_FACTORY_RESET_CONFIRM, // 弹出二次确认对话框
```

### D.8 出厂重置流程

```
点击"出厂重置" → 弹出 confirm dialog "将清除所有设置和记忆,确定?"
   ├── [取消] → 关闭 dialog
   └── [确定] → nvs_flash_erase() + esp_restart()
```

confirm dialog 用 `lv_msgbox_create`(LVGL 9 新 API 同名,签名变化:`lv_msgbox_create(NULL)`)。

---

## 附录 E:★ v6.1 vs v5.x 文档差异 & 功能缺口清单

### E.1 表情清单更新(Part 1 §4.1 需同步)

**v6.1 拆分**:`look_around` → `look_left` + `look_right`(原 v5.0 文档合并为一项)

| 状态 | 触发 | 视觉 |
|---|---|---|
| **look_left** | IMU `roll > +15°`(左倾) | 双眼整体 x: -20px,固定 1.5s |
| **look_right** | IMU `roll < -15°`(右倾) | 双眼整体 x: +20px,固定 1.5s |

**v6.0 → v6.1 bug 修复**:左倾原误映射为 `thinking`、右倾原误映射为 `surprised`,v6.1 已纠正。Part 2 §9 运动检测表需同步:

```diff
- | 左倾 | roll > 15° | thinking |
- | 右倾 | roll < -15° | surprised |
+ | 左倾 | roll > 15° | look_left   |
+ | 右倾 | roll < -15° | look_right  |
```

`thinking` / `surprised` 改为由对话/AI 思考逻辑主动触发,不再由 IMU 直接绑定。

### E.2 v6.1 已实现 vs 缺失功能矩阵

| # | 功能 | 原型 v6.1 | v5.2 文档 | 实物固件需做 |
|---|---|---|---|---|
| 1 | 22 表情静帧 | ✅ Face.tsx | ✅ §8 | LVGL 9 重写完整 22 套 |
| 2 | 5 级呼吸 | ✅ | ✅ §7 | ✓ |
| 3 | 径向菜单 | ✅ | ✅ 附录 C | ✓ |
| 4 | **系统设置页** | ✅ NEW | ✅ 附录 D(本次新增) | ✓ |
| 5 | Wi-Fi 配网 | ✅ 4 屏 | ⚠️ 仅事件 | **缺**:实物需写 SmartConfig/AP+QR 配网完整流程 |
| 6 | 中文气泡 | ✅ | ✅ §5 | ✓ |
| 7 | RGB 灯条 | ✅ 全状态 | ⚠️ §11 仅接口 | **缺**:HSV 渐变曲线表 + RMT 时序 |
| 8 | NVS 记忆 | ✅ Mock | ✅ §10 schema | ✓ |
| 9 | IMU 触发 | ✅ Mock | ✅ §9 | **缺**:真实 BMI270 寄存器配置 + 滤波 |
| 10 | 反馈音/震动 | ✅ Web Audio | ✅ §11 | **缺**:I2S sine 合成 + DRV2605 effect 映射表 |
| 11 | 开机动画 | ✅ 3 主题 | ⚠️ Part1 §8 仅描述 | **缺**:LVGL 9 动画时序与资源准备 |
| 12 | 主题热切换 | ✅ 径向菜单"主题" | ⚠️ 仅 token 表 | **缺**:运行期 `style_token_apply_all()` 统一调用入口 |
| 13 | suggest 类型对话气泡 | ✅ 含按钮+timeout | ⚠️ §5.6 仅 text | **缺**:`dialog_bubble_show_suggest(text, on_yes, on_timeout, ms)` API |
| 14 | random 表情自动巡演 | ✅ 3s 切一次 | ❌ | **缺**:`face_random_start(interval_ms)` |
| 15 | **语音设定子页** | ❌ TODO | ❌ | **缺**:音色列表 + 试听 |
| 16 | **睡眠定时子页** | ❌ TODO | ❌ | **缺**:5/15/30/60/never 选择 |
| 17 | **账号绑定页** | ❌ TODO | ❌ | **缺**:扫码绑定 + 显示头像 |
| 18 | **OTA 升级** | ❌ TODO | ❌ | **缺**:ESP-IDF `esp_https_ota` + 进度气泡 + 回滚 |
| 19 | **出厂重置二次确认 dialog** | ❌ TODO | ❌ | **缺**:`lv_msgbox` 实现 |
| 20 | 网络断开后台逻辑 | ❌ | ❌ | **缺**:Wi-Fi 状态机 + 重连退避策略 |
| 21 | 时间同步(SNTP) | ❌ | ❌ | **缺**:`esp_sntp_*`,早安触发依赖准确时间 |
| 22 | 电量管理 | ❌ | ❌ | **缺**:AXP2101 读 SOC + 低电气泡告警(20% / 10%) |
| 23 | 远程升级 / 配置中心 | ❌ | ❌ | **缺**:MQTT 或 HTTP 长轮询(可选) |
| 24 | 多语言切换 | ❌ | ❌ | **缺**:`strings_zh.h` / `strings_en.h` 切换机制 |
| 25 | 可访问性 / 高对比度 | ❌ | ❌ | 优先级低 |

### E.3 必须补齐的 P0 工程实现

按优先级排序,**下游 RD 必须先做这些才能让设备真正可用**:

1. **Wi-Fi 完整配网链路**(对应 #5)
   - 启动检测 NVS 中是否有 Wi-Fi 凭证
   - 无 → 进入 SoftAP + 静态 QR(QR 内容固定显示热点名/密码,扫码后用户连热点访问 192.168.4.1 网页提交 ssid/psk)
   - 有 → STA 模式连接,失败 3 次回退 AP
   - 事件桥:`EVT_WIFI_CONNECTING` / `EVT_WIFI_SUCCESS` / `EVT_WIFI_ERROR` 驱动 wifi_setup_overlay 切屏

2. **AXP2101 电量上报 + 低电告警**(#22)
   - 100ms 轮询 SOC
   - 跨越 20% 阈值 → `dialog_bubble_show("电量较低,请连接充电...", 5000)` + RGB 黄色慢闪
   - 跨越 10% → `dialog_bubble_show("快没电了!", 5000)` + 红色快闪 + 强制 deep_sleep

3. **SNTP 时间同步**(#21)
   - Wi-Fi 连上后立即 `esp_sntp_init` + `pool.ntp.org`
   - 时区固定 `CST-8`
   - 同步成功后才允许"早安问候"触发

4. **OTA 流程**(#18)
   - `esp_https_ota_begin` + 进度回调 → 顶部气泡显示百分比
   - 失败回滚到上一个 partition
   - 完成后倒数 3s `esp_restart`

5. **出厂重置 confirm dialog**(#19)
   - `lv_msgbox_create` 居中弹出,确定/取消两按钮
   - 确定 → `nvs_flash_erase()` → `esp_restart()`

### E.4 v6.1 推荐组件结构(增量)

在原 §1 项目结构基础上新增:

```
components/
├── settings_store/         ★ NEW
│   ├── settings_store.c
│   └── include/settings_store.h
├── wifi_manager/           ★ NEW
│   ├── wifi_manager.c      // SmartConfig + SoftAP fallback
│   ├── http_provision.c    // 192.168.4.1 配网网页
│   └── include/wifi_manager.h
├── ota_manager/            ★ NEW
│   ├── ota_manager.c       // esp_https_ota wrapper
│   └── include/ota_manager.h
├── power_monitor/          ★ NEW
│   ├── power_monitor.c     // AXP2101 SOC 轮询
│   └── include/power_monitor.h
└── ui_core/
    ├── settings_overlay.c  ★ NEW(本附录 D.6)
    ├── msgbox.c            ★ NEW(出厂重置确认)
    └── dialog_bubble.c     // 扩展 show_suggest API
```

### E.5 给 AI 编码助手的增量约束(v5.2)

在原 §14 九条约束之外追加:

10. **设置项变更必须触发副作用**(参见 D.5),不允许只改 NVS 不改硬件
11. **NVS 写入必须用 `settings_set_xxx` 接口**,内部已做 1s 防抖,直接调 `settings_save` 会导致每次拖动滑块都写 Flash
12. **`lv_msgbox` 是模态**,显示期间禁用底层触摸事件,需用 `lv_obj_add_flag(parent, LV_OBJ_FLAG_CLICKABLE_DESCENDANTS)` 配合
13. **Wi-Fi 状态变化 → `event_bus_post`**,绝不能在 Wi-Fi 事件 handler 里直接调用 LVGL API(不同任务上下文)
14. **OTA 期间禁止 RGB / 呼吸动画**,以免占用 SPI/Flash 带宽导致 OTA 失败
15. **`look_left` / `look_right` 是 v6.1 独立表情**(不是 `look_around`),IMU 左/右倾直接映射这两个 state,不要映射到 `thinking` / `surprised`

---

## 附录 B:中文显示常见问题

| 现象 | 原因 | 解决 |
|---|---|---|
| 全部中文显示为 □ | 未设置 text_font | `lv_obj_set_style_text_font(label, &font_sourcehans_22, 0)` |
| 部分字符显示为 □ | 字符未在子集中 | 加入 `chinese_chars.txt` 重新生成 |
| 中文显示为乱码 | 源文件非 UTF-8 | 用 VSCode 重新另存为 UTF-8 无 BOM |
| 编译报 `\xe4\xbd...` warning | 未加 `-fexec-charset=UTF-8` | 在 CMakeLists 添加编译选项 |
| Flash 编译失败 size overflow | 字体过大 | 减小子集 / 降低 bpp 至 2 / 减小 size |
| 中文渲染慢卡顿 | bpp=8 过大 | 改用 bpp=4 或 bpp=2 |

---

---

## 附录 F:★ 径向菜单 6 扇区图标资源包(icons_pack)

> **写在最前**:此前下游 Agent 用 `lv_canvas` "看图复刻" 6 扇区图标,还原度极差。本附录提供**预编译 LVGL 9 `lv_image_dsc_t` 数组**作为唯一权威产物——下游只需 `#include` 后调用 `lv_image_create / lv_image_set_src`,**不再做任何"看图画"的工作**。

### F.1 资源包内容

```
icons_pack/                              # 作为 ESP-IDF 组件直接放进 components/
├── build_icons.py                       # 重生产脚本(SVG → PNG → C 数组)
├── CMakeLists.txt                       # idf_component_register
├── README.md
├── include/
│   └── menu_icons.h                     # 6 个 LV_IMG_DECLARE + menu_icons[]
├── svg/                                 # lucide-react 原始 SVG(24×24,stroke="currentColor")
│   └── icon_{smile,message,settings,palette,puzzle,shuffle}.svg
├── png/                                 # 32×32 RGBA8888 校对图(白描边)
│   └── icon_*.png
└── c/
    ├── icon_smile.c    (≈26KB,4096B 像素 + lv_image_dsc_t)
    ├── icon_message.c
    ├── icon_settings.c
    ├── icon_palette.c
    ├── icon_puzzle.c
    ├── icon_shuffle.c
    └── menu_icons.c                     # 指针表实现
```

### F.2 图标 ↔ 扇区映射(与 v6.1 `MenuOverlay.tsx` 完全一致)

| Sector | C 符号            | lucide 图标   | 语义        | LVGL 引用            |
| ------ | ----------------- | ------------- | ----------- | -------------------- |
| 0      | `icon_smile`      | Smile         | 表情/互动   | `&icon_smile`        |
| 1      | `icon_message`    | MessageCircle | 对话        | `&icon_message`      |
| 2      | `icon_settings`   | Settings      | 设置        | `&icon_settings`     |
| 3      | `icon_palette`    | Palette       | 主题        | `&icon_palette`      |
| 4      | `icon_puzzle`     | Puzzle        | 扩展/插件   | `&icon_puzzle`       |
| 5      | `icon_shuffle`    | Shuffle       | 随机/切换   | `&icon_shuffle`      |

> 顺时针,起始扇区 12 点钟方向(与 Appendix C 几何一致)。

### F.3 技术规格

| 项                    | 值                                                                     |
| --------------------- | ---------------------------------------------------------------------- |
| 目标平台              | **ESP-IDF 5.5.4 LTS** + LVGL 9.5.0,M5Stack CoreS3(ESP32-S3-WROOM-1-N16R8)|
| 颜色格式              | `LV_COLOR_FORMAT_ARGB8888`(4 B/px,内存字节序 **B,G,R,A**)            |
| 尺寸                  | 32 × 32 px                                                             |
| `stride`              | 128 字节                                                               |
| `data_size`           | 4096 字节                                                              |
| 笔画策略              | **白色描边 + alpha 编码笔画形状**;运行时通过 `image_recolor` 上主题色 |
| 单图二进制大小        | ≈ 4 KB(.rodata)                                                       |
| 6 图合计              | ≈ 24 KB(CoreS3 16 MB Flash 完全可承受)                                |
| 图源                  | lucide-react 官方 SVG path                                             |

### F.4 用法(替换 §C.6 的占位 label)

> Appendix C.6 原占位写法是 `lv_label_create(btn) + lv_label_set_text(LV_SYMBOL_xxx)`。**v5.3 起强制改为下面写法**。

```c
#include "menu_icons.h"     // 来自 icons_pack 组件

static void radial_menu_build_buttons(lv_obj_t *menu_root,
                                      const ui_theme_t *theme)
{
    /* 几何参数:R=104, 6 扇区, 起始角 -90° */
    const float R = 104.0f;
    const float step_deg = 60.0f;
    const float start_deg = -90.0f;

    for (int i = 0; i < MENU_ICON_COUNT; i++) {
        /* 1) 按钮容器(半透圆形,背景色由主题决定) */
        lv_obj_t *btn = lv_obj_create(menu_root);
        lv_obj_remove_style_all(btn);
        lv_obj_set_size(btn, 64, 64);
        lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(btn, lv_color_hex(theme->btn_bg), 0);
        lv_obj_set_style_bg_opa(btn, LV_OPA_70, 0);
        lv_obj_set_style_border_color(btn, lv_color_hex(theme->btn_border), 0);
        lv_obj_set_style_border_width(btn, 2, 0);
        lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);

        /* 2) 极坐标 → 屏幕坐标(屏幕中心 160,120) */
        float a = (start_deg + step_deg * i) * (float)M_PI / 180.0f;
        int cx = 160 + (int)(R * cosf(a)) - 32;
        int cy = 120 + (int)(R * sinf(a)) - 32;
        lv_obj_set_pos(btn, cx, cy);

        /* 3) 图标 —— 直接用预生成的 lv_image_dsc_t */
        lv_obj_t *img = lv_image_create(btn);
        lv_image_set_src(img, menu_icons[i]);
        lv_obj_center(img);

        /* 4) 主题色重着色(白描边 → 主题色),无需为每个主题再生资源 */
        lv_obj_set_style_image_recolor(img, lv_color_hex(theme->icon_color), 0);
        lv_obj_set_style_image_recolor_opa(img, LV_OPA_COVER, 0);

        /* 5) 事件:把 sector index 编进 user_data */
        lv_obj_set_user_data(btn, (void*)(intptr_t)i);
        lv_obj_add_event_cb(btn, on_radial_btn_click, LV_EVENT_CLICKED, NULL);
    }
}
```

### F.5 ESP-IDF 集成步骤(给下游 Agent 的 SOP)

1. 把 `icons_pack/` 整个目录拷贝到工程 `components/icons_pack/`;
2. 主组件(通常是 `main/CMakeLists.txt`)无需改动—— ESP-IDF 自动发现 components;
3. 在用到的源文件 `#include "menu_icons.h"`;
4. **不要**手工改 `c/icon_*.c`,这些文件由脚本生成;
5. 如需替换图标:改 `svg/icon_xxx.svg` → 跑 `python3 build_icons.py` → 提交。

### F.6 重生产流程

```bash
pip install --user --break-system-packages cairosvg pillow

# 替换 svg/icon_xxx.svg(保持 24×24 viewBox + stroke="currentColor")
python3 icons_pack/build_icons.py
# -> 自动覆盖 png/、c/ 下对应文件,CMake 增量重编。
```

### F.7 决策记录:为什么不让 Agent 用 lv_canvas 画

| 方案                                  | 还原度 | 可重现 | 体积   | 决策             |
| ------------------------------------- | ------ | ------ | ------ | ---------------- |
| Agent 看 PNG 用 `lv_canvas` 描线      | 低     | 否     | 高     | ✗ 已淘汰         |
| Agent 用 `LV_SYMBOL_xxx` 占位         | 极低   | 是     | 低     | ✗ 仅过渡         |
| **预生产 `lv_image_dsc_t`(本附录)** | **100%** | **是** | **低** | **✓ 强制采用**   |
| 运行时 `lv_svg` 渲染                  | 100%   | 是     | 高     | ✗ LVGL9 SVG 依赖太重 |

### F.8 给 AI 编码助手的增量约束(v5.3)

在原 §E.5 第 15 条之后追加:

16. **径向菜单图标必须**用 `lv_image_create + lv_image_set_src(menu_icons[i])`;**禁止**再用 `LV_SYMBOL_xxx` 标签或 `lv_canvas` 描线复刻;
17. **图标着色必须**用 `lv_obj_set_style_image_recolor` + `image_recolor_opa = LV_OPA_COVER`;**禁止**为每个主题各生成一份图标资源;
18. **`icons_pack` 是只读资产**—— `c/icon_*.c` 由 `build_icons.py` 生成,任何手工编辑会在下次重生产时被覆盖;需要改设计先改 `svg/`。

---

---

## 附录 G:★ ESP-IDF 5.5.4(LTS)适配规则

> **背景**:本规格书最初锚定 ESP-IDF **5.2.1**;实际工程已升级到 **5.5.4(LTS,2025-08 系列补丁版)**。
> **优先级**:本附录 G 与规格书 §附录 A、§14 R3 冲突时,**以本附录为准**。其他附录(C/D/E/F)继续生效。
> **结论先行**:
> - LVGL 9.5.0 仍兼容,`icons_pack`(ARGB8888 / `lv_image_dsc_t`)**零改动**继续用。
> - 真正要改的是 **驱动初始化方式**(从手写改为 `esp_lvgl_port`)与 **依赖声明**(走 Component Manager 2.x)。

### G.1 5.2.1 → 5.5.4 工程影响一览

| 维度 | 5.2.1 | 5.5.4 | 工程动作 |
|---|---|---|---|
| 工具链 | GCC 13 | **GCC 14**(Xtensa/RISC-V) | 注意 `-Wenum-int-mismatch`、`-Wcalloc-transposed-args` |
| CMake 最低版 | 3.16 | **3.22** | 自定义 CMake 用现代 `target_*` 写法 |
| Component Manager | 1.x | **2.x**,`idf_component.yml` 强 schema | LVGL/esp_lvgl_port 走 managed component |
| NVS | 经典 API | 强类型 `nvs_handle_t` + Encryption v2 | 老接口仍兼容,新代码可选 C++ wrapper |
| Wi-Fi | 老 API | 新增 802.11ax(S3 仍 Wi-Fi 4) | SmartConfig 字段微调,用 DEFAULT 宏 |
| LVGL 集成 | 手写 `lv_disp_drv_t` | **官方推荐 `espressif/esp_lvgl_port` v2.x** | 驱动注册改成 `lvgl_port_*` 一行起 |
| FreeRTOS | v10.4 | **v10.5(SMP 默认开启)** | 业务任务显式 pin 到 Core 0,Core 1 留给 lvgl_port |
| PSRAM 配置 | `CONFIG_SPIRAM_*` | 部分项更名 `CONFIG_ESP32S3_SPIRAM_SUPPORT` 等 | 不能照搬旧 sdkconfig,重跑 menuconfig |
| mbedTLS | 3.4 | **3.6 LTS** | 弃用宏会 warning,不影响逻辑 |
| boot log | — | 多行(SoC/PSRAM 自检) | 串口 grep 规则更新 |

### G.2 与 6 个 Step 的对应改动

| Step | 影响 | 必改点 |
|---|---|---|
| **Step 0 脚手架** | Component Manager 2.x | 新增 `main/idf_component.yml`,声明 `lvgl/lvgl: "^9.2"` 与 `espressif/esp_lvgl_port: "^2.4"` |
| **Step 1 显示/触摸** | LVGL 集成方式变 | 不再手写 `lv_disp_drv_t`,改用 `esp_lvgl_port_init` + `lvgl_port_add_disp` + `lvgl_port_add_touch` |
| Step 2 RobotFace | 无影响 | — |
| Step 3 径向菜单 | 无影响(icons_pack 兼容) | — |
| Step 4 设置 | NVS Encryption v2 可选 | P0 不开启 |
| **Step 5 Wi-Fi 配网** | SmartConfig 字段微调 | 用 `SMARTCONFIG_START_CONFIG_DEFAULT()` 宏避免漏字段 |

### G.3 强制规则(覆盖原 §14 R3)

#### G.3.1 依赖管理走 Component Manager 2.x

`main/idf_component.yml` **必须存在**:

```yaml
dependencies:
  idf: ">=5.5.0"
  lvgl/lvgl: "^9.2"
  espressif/esp_lvgl_port: "^2.4"
```

- **禁止**把 LVGL 源码 copy 进 `components/`(老做法,5.5 起淘汰)。
- `icons_pack/` 作为 **本地 component** 保留(不上 registry)。
- `idf.py reconfigure` 后,build 日志里 `lvgl/lvgl` 与 `espressif/esp_lvgl_port` **必须是 registry 拉取**,出现 `local` 视为不合格。

#### G.3.2 LVGL 初始化用 esp_lvgl_port,不再手写驱动注册

> **附录 A 中的 `lv_disp_drv_t` / `lv_indev_drv_t` 手工注册写法在 5.5.4 下作废**。

```c
#include "esp_lvgl_port.h"

const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
ESP_ERROR_CHECK(lvgl_port_init(&lvgl_cfg));

lvgl_port_display_cfg_t disp_cfg = {
    .io_handle      = io_handle,
    .panel_handle   = panel_handle,
    .buffer_size    = 320 * 240,
    .double_buffer  = true,
    .hres           = 320,
    .vres           = 240,
    .color_format   = LV_COLOR_FORMAT_RGB565,
    .flags = { .buff_dma = true, .swap_bytes = true },
};
lv_display_t *disp = lvgl_port_add_disp(&disp_cfg);

lvgl_port_touch_cfg_t touch_cfg = { .disp = disp, .handle = tp_handle };
lv_indev_t *indev = lvgl_port_add_touch(&touch_cfg);
```

- `lvgl_port` 内部已起 LVGL task **并 pin 到 Core 1**——**禁止**自建 LVGL task。
- 任何 LVGL API 调用必须包在 `lvgl_port_lock(0)` / `lvgl_port_unlock()` 之间。

#### G.3.3 LVGL 并发(取代原 §14 R3)

```c
/* 业务任务里调 LVGL */
if (lvgl_port_lock(0)) {     // 0 = 永久等待
    lv_label_set_text(my_label, "新文本");
    lv_obj_set_x(my_btn, 100);
    lvgl_port_unlock();
}
```

- Wi-Fi/IMU/触摸 ISR/handler → `event_bus_post()` → 业务任务拿锁后调 LVGL;**仍然禁止**在 ISR / Wi-Fi handler 里直调 LVGL。
- 锁等待时间 0 表示永久等待;中断上下文不可调,改用 `xTimerPendFunctionCallFromISR`。

#### G.3.4 FreeRTOS SMP 任务亲和性

| 任务 | 建议 Core | 原因 |
|---|---|---|
| `lvgl_port` 内部 task | **Core 1**(由 port 决定,勿改) | 渲染密集,独占降低抖动 |
| Wi-Fi / network | Core 0(IDF 默认) | 与协议栈同核 |
| event_bus 消费 / 业务 | Core 0 或 `tskNO_AFFINITY` | 让出 Core 1 |
| IMU 读取 / 传感器轮询 | Core 0 | 与触摸轮询同核避免抢锁 |

```c
xTaskCreatePinnedToCore(event_bus_task, "event_bus", 4096, NULL, 5, NULL, 0);
```

#### G.3.5 sdkconfig 关键项(5.5 命名)

```
CONFIG_ESP32S3_SPIRAM_SUPPORT=y
CONFIG_SPIRAM_MODE_OCT=y               # ❌ 已作废,CoreS3 是 Quad PSRAM,见附录 I.6
# 正确写法 ↓
# CONFIG_SPIRAM_MODE_QUAD=y
CONFIG_SPIRAM_SPEED_80M=y
CONFIG_FREERTOS_HZ=1000
CONFIG_FREERTOS_UNICORE=n              # SMP 开启
CONFIG_LV_COLOR_DEPTH_16=y             # 屏幕 RGB565
CONFIG_LV_USE_DRAW_SW=y
CONFIG_COMPILER_OPTIMIZATION_PERF=y
CONFIG_PARTITION_TABLE_CUSTOM=y
```

> **不要**照搬 5.2 的 sdkconfig——5.5 的 menuconfig 路径已改,直接重生成。

#### G.3.6 SmartConfig 用 DEFAULT 宏(避免 5.5 新增字段被漏)

```c
smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT();
ESP_ERROR_CHECK(esp_smartconfig_start(&cfg));
```

#### G.3.7 GCC 14 警告策略

- **禁止** `-Werror` 全开。
- 第三方 component 的 warning(LVGL/esp_lvgl_port 自身)用 `target_compile_options(<lib> PRIVATE -Wno-xxx)` 局部豁免。
- **禁止**改 LVGL 源码消除 warning。

### G.4 给 AI 编码助手的增量约束(v5.3 → v5.4)

在原 §E.5、§F.8 之后追加:

19. **必须** ESP-IDF ≥ 5.5.0;`idf.py --version` 不是 5.5.x 系列时立即停下报告,**禁止**降级。
20. **必须**用 `esp_lvgl_port` 初始化显示与触摸;**禁止**在 5.5 工程里手写 `lv_disp_drv_t`/`lv_indev_drv_t` 注册流程。
21. **必须**在 LVGL API 调用前后做 `lvgl_port_lock(0)` / `lvgl_port_unlock()`;**禁止**自建 LVGL task。
22. **必须**在 `main/idf_component.yml` 声明 `lvgl/lvgl ^9.2` 与 `espressif/esp_lvgl_port ^2.4`;**禁止**把 LVGL 源码直接 copy 进 `components/`。
23. SMP 任务**必须**显式选核:渲染让位 Core 1,业务/Wi-Fi/传感器走 Core 0 或 `tskNO_AFFINITY`。

---

## 附录 H:★ 下游 Agent 投喂 Prompt(IDF 5.5.4 版)

> **用法**:把本附录 H 的 markdown 整段复制给下游开发 Agent(Cursor / Claude Code / Copilot Workspace 等),作为对话首轮 system prompt。配合 4 份资料(原型 zip / 本规格书 / v6.1 分析报告 / icons_pack zip)同时投喂。

````markdown
# 角色
你是嵌入式 GUI 工程师,负责把 React 原型 v6.1 落地到 M5Stack CoreS3。
技术栈固定:**ESP-IDF 5.5.4(LTS)+ LVGL 9.5.0 + FreeRTOS v10.5(SMP)**,屏幕 320×240 IPS 触摸。
不要质疑技术栈,不要建议换框架/换库/降级 IDF。

# 输入资料(共 4 份,按下列用途使用,不要混淆)

| # | 文件 | 用途 | 权限 |
|---|------|------|------|
| 1 | `prototype_v6.1.zip` | React+Tailwind 原型,**交互/视觉的唯一事实来源** | 只读参考 |
| 2 | `桌面机器人UI_v5.1_Part2_工程实现规格_ESPIDF_LVGL9.md`(v5.4,含附录 A–H) | **唯一规则书** | 只读规则 |
| 3 | `陪伴机器人UI_v6.1_原型分析与差异报告.md` | 背景/缺口分析 | 只读背景,冲突时以 #2 为准 |
| 4 | `icons_pack.zip` | **预编译 LVGL9 6 扇区菜单图标** | 整目录放进 `components/icons_pack/`,内部禁改 |

> 规格书 #2 的附录 A 写于 IDF 5.2.1。**附录 G 的 IDF 5.5.4 适配规则优先级最高**(覆盖附录 A 驱动初始化部分);其余附录(C/D/E/F)继续生效。

# IDF 5.5.4 适配规则(★ 与原始规格书的差异,先读这段)

## A1 依赖管理走 Component Manager 2.x
`main/idf_component.yml` 必须存在:
```yaml
dependencies:
  idf: ">=5.5.0"
  lvgl/lvgl: "^9.2"
  espressif/esp_lvgl_port: "^2.4"
```
- 不要把 LVGL 源码 copy 进 `components/`(5.5 起淘汰)
- `icons_pack` 作为本地 component 保留

## A2 LVGL 初始化走 esp_lvgl_port
原规格书附录 A 的手写 `lv_disp_drv_t` 写法**作废**:
```c
const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
ESP_ERROR_CHECK(lvgl_port_init(&lvgl_cfg));
lv_display_t *disp = lvgl_port_add_disp(&disp_cfg);
lv_indev_t   *indev = lvgl_port_add_touch(&touch_cfg);
```
- lvgl_port 已建好 LVGL task 并 pin Core 1,**不要**自建
- 任何 LVGL API 必须 `lvgl_port_lock(0)` / `lvgl_port_unlock()` 配对

## A3 FreeRTOS SMP
- 业务任务用 `xTaskCreatePinnedToCore`,pin Core 0 或 `tskNO_AFFINITY`
- Core 1 留给 lvgl_port

## A4 sdkconfig 关键项(5.5 命名,CoreS3 用 **Quad** PSRAM,见附录 I.6)
```
CONFIG_ESP32S3_SPIRAM_SUPPORT=y
CONFIG_SPIRAM_MODE_QUAD=y
CONFIG_SPIRAM_SPEED_80M=y
CONFIG_SPIRAM_XIP_FROM_PSRAM=n
CONFIG_FREERTOS_TASK_CREATE_ALLOW_EXT_MEM=n
CONFIG_FREERTOS_HZ=1000
CONFIG_LV_COLOR_DEPTH_16=y
CONFIG_COMPILER_OPTIMIZATION_PERF=y
```
不要照搬 5.2 的 sdkconfig。**禁止**使用 `CONFIG_SPIRAM_MODE_OCT`(CoreS3 硬件不支持)。

## A5 SmartConfig 用 DEFAULT 宏
```c
smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT();
ESP_ERROR_CHECK(esp_smartconfig_start(&cfg));
```

## A6 GCC 14 警告
- 不要 `-Werror` 全开
- 第三方 component warning 用 `target_compile_options(... -Wno-xxx)` 局部豁免
- 不要改 LVGL 源码

# 硬性规则(违反 = 不合格)

## R1 图标(★ 上一个 Agent 在这里翻车)
- 6 扇区图标必须 `lv_image_create + lv_image_set_src(menu_icons[i])`
- 禁止 `LV_SYMBOL_xxx` 占位
- 禁止 `lv_canvas / lv_line / lv_arc` 描线
- 禁止改 `icons_pack/c/icon_*.c`
- 主题换色只用:
  ```c
  lv_obj_set_style_image_recolor(img, lv_color_hex(theme->icon_color), 0);
  lv_obj_set_style_image_recolor_opa(img, LV_OPA_COVER, 0);
  ```
- 顺序:`0=smile, 1=message, 2=settings, 3=palette, 4=puzzle, 5=shuffle`

## R2 中文字体
- `font_sourcehans_22.c`(子集化,见附录 B)
- 中文 label 必须设 text_font
- UTF-8 无 BOM;CMake 加 `-fexec-charset=UTF-8`

## R3 LVGL 并发(★ 5.5 改了)
- LVGL API 必须 `lvgl_port_lock(0)` / `lvgl_port_unlock()`
- 不自建 LVGL task
- ISR/Wi-Fi handler 不直调 LVGL,走 event_bus

## R4 NVS / 设置
- 走 `settings_set_xxx`(1s 防抖),禁止直调 `settings_save`
- 设置变更必须触发副作用(附录 D.5)

## R5 表情
- `look_left` / `look_right` 直接映射 IMU 左右倾,不复用 `look_around` / `thinking`

## R6 不要做的事
- 不起 web/MQTT/HTTP server
- 不 mock Wi-Fi/触摸/IMU
- 不写 README 之外的文档
- 不改 `icons_pack/`
- **不降级 IDF**

# 工作流(每步停下来等 review)

## Step 0:工程脚手架
- `idf.py --version` 必须是 5.5.x
- `idf.py create-project robot_ui --target esp32s3`
- `icons_pack/` → `components/icons_pack/`
- 写 `main/idf_component.yml`(A1)
- `idf.py reconfigure` 拉依赖
- **交付**:目录树 + build 日志(贴 lvgl/lvgl 与 esp_lvgl_port 的版本号,且来源是 registry)

## Step 1:显示/触摸驱动 + 主循环
- 用 `esp_lvgl_port`(A2)
- 跑通中文 "你好"
- **交付**:可编译工程 + 串口日志(含 `lvgl_port_init` 输出)

## Step 2:RobotFace 8 表情
- 待机 ≤15 fps,交互态 30 fps
- LVGL 调用全部走 lvgl_port 锁
- **交付**:`face_engine.c` + 切换录屏

## Step 3:径向菜单(用 icons_pack)
- 几何 R=104, 6 扇区, 起始角 -90°, 步长 60°
- 双击屏弹出 / 单击外部收起 / 200ms 缩放
- 图标严格按 R1,抄附录 F.4
- **交付**:`radial_menu.c` + 截图(对得上 contact_sheet 6 个图标)

## Step 4:SettingsOverlay
- 8 项设置(附录 D),setter 触发副作用
- **交付**:`settings_overlay.c` + NVS 读写日志

## Step 5:Wi-Fi 配网(P0)
- SmartConfig + AP 兜底,用 `SMARTCONFIG_START_CONFIG_DEFAULT()` 宏(A5)
- 状态走 event_bus
- **交付**:配网完整链路录屏

# 每步交付格式
```
## Step X 完成
- 新增/修改文件:components/xxx/xxx.c (+213 -0)
- 编译:✅ / ❌(贴关键错误)
- 设备验证:✅ / ⏸ 待 review
- 偏离规格:无 / [明确列出]
- 阻塞:无 / [一句话]
```

# 启动指令(做完 4 步停下,等我说"开始 Step 0")
1. `idf.py --version`,贴输出。**不是 5.5.x 立即停下报告**。
2. 解压 4 份资料并 `ls`,确认能读到:
   - `MenuOverlay.tsx`(资料 1)
   - 规格书"附录 F"与"附录 G"(资料 2)
   - `LV_IMG_DECLARE(icon_smile)`(资料 4)
   - `menu_icons[6]`(资料 4)
3. 用 6 句话复述 R1–R6;再用 6 句话复述 A1–A6(自查理解,不准抄)
4. 列出 Step 0 计划目录树 + `main/idf_component.yml` 草稿(还不写业务代码)

**未完成上述 4 步前,禁止写任何业务代码。**
````

### H.1 Review 关卡(给规格 owner 用)

| 关卡 | 触发点 | 不通过表现 | 动作 |
|---|---|---|---|
| 闸 1 | 启动指令 step 3 复述 A2 | 提到"自己 `xTaskCreate` LVGL task" | 立即打断重做(5.5 最大坑) |
| 闸 2 | Step 0 交付 | `lvgl/lvgl` 或 `esp_lvgl_port` 来源是 `local` | 退回,要求改用 Component Manager |
| 闸 3 | Step 1 交付 | 还在手写 `lv_disp_drv_t` 注册 | 退回,改用 `lvgl_port_*` |
| 闸 4 | Step 3 交付 | 图标渲染与 `contact_sheet.png` 不一致 | 说明没用 icons_pack,退回 |
| 闸 5 | Step 5 交付 | Wi-Fi handler 里直调 LVGL | 退回,违反 R3 |

---

---

## 附录 I:★ ESP-Claw 集成评估(乐鑫 Edge AI Agent 框架)

> **背景**:乐鑫官方 2026 开源的 **ESP-Claw**(<https://github.com/espressif/esp-claw>,Apache-2.0)是面向 IoT 设备的边缘 Agent 运行时,**官方已包含 `boards/m5stack/m5stack_cores3` 板级支持**——与本规格目标硬件 100% 匹配。
> **本附录目的**:评估能否引入、与现有 v6.1 设计如何共存、给出推荐的集成方案。

### I.1 ESP-Claw 是什么(一句话)

> **不是 UI 框架,是 Agent Runtime**——把 IM 聊天、LLM 调度、Lua 动态加载、MCP 协议、结构化记忆、技能商店全塞进 ESP32,与本规格的 LVGL UI 层**互补**而非替代。

### I.2 仓库骨架(实测,非二手资料)

| 子模块 | 价值 | 与本规格关系 |
|---|---|---|
| `application/edge_agent/boards/m5stack/m5stack_cores3/` | YAML 声明式板级(AXP2101/AW88298/ES7210/ILI9341/触摸全配好)+ `partitions_16MB.csv`(ota×2 4M + emote 3M + storage 4M)+ `sdkconfig.defaults.board`(Quad PSRAM 80M / QIO Flash 80M) | ★ 直接替代或借鉴 `bsp_cores3` |
| `components/claw_modules/claw_core/` | Agent Loop 引擎、session/context provider | ★ 对话扇区(Sector 1 Message)的后端 |
| `components/claw_modules/claw_memory/` | 结构化记忆持久化 | 替代 `memory_store` |
| `components/claw_modules/claw_event_router/` | 事件→Agent 触发路由 | 与 `event_bus` 互补 |
| `components/claw_modules/claw_skill/` + `cap_skill_mgr` + `cap_lua` | 技能注册/Lua 动态加载 | ★ Puzzle 扇区(Sector 4 Extensions)落地 |
| `components/claw_capabilities/cap_im_local/` | 本地 IM(WebUI 聊天) | 对话能力 |
| `components/claw_capabilities/cap_mcp_server/` | 设备成为 MCP server,被其他 Agent 调用 | 跨设备协同(可选) |
| `components/claw_capabilities/cap_mcp_client/` | 调用外部 MCP 工具 | 联网能力(可选) |
| `components/common/wifi_manager/` + `captive_dns/` + `http_server` | STA+AP 双模 + Captive Portal + Web 配网 | ★ **直接补齐 §E.3 P0 Wi-Fi 配网缺口** |
| `components/common/settings/` + FATFS 4M + RAMFS | NVS/FATFS/RAMFS 三层存储 | 替代或扩展 `memory_store` |
| `components/common/emote/` | 表情引擎,基于 `esp_emote_gfx`(**非 LVGL**) | ⚠️ **与本规格 face_engine 冲突**,详见 I.4 |
| `components/common/display_arbiter/` | 显示资源仲裁(LVGL ↔ emote 互斥占用) | 多渲染管线共存时有用 |
| `components/lua_modules/`(35 个) | i2c/gpio/uart/adc/touch/imu/camera/lvgl 等 Lua 封装 | 让"非工程师"能写设备技能 |

### I.3 与本规格的契合度

| 维度 | 本规格 v5.5 | ESP-Claw | 评估 |
|---|---|---|---|
| 硬件 | M5Stack CoreS3 | 官方 `boards/m5stack/m5stack_cores3` | ✅ 完美命中 |
| IDF 版本 | 5.5.4 LTS | 跟主线,实测兼容 5.5.x | ✅ |
| LVGL | 9.5 + icons_pack | **未用 LVGL**,用 `esp_emote_gfx` | ⚠️ 渲染栈分歧,需 `display_arbiter` |
| Wi-Fi 配网 | P0 缺口(§E.3) | 完整 STA+AP+Captive Portal+Web UI | ✅ 直接拿来用 |
| AI 对话 | v6.1 Sector 1 仅图标,无后端 | `claw_core` + `cap_im_local` 全套 | ✅ 最大价值点 |
| MCP | 未规划 | client/server 双向 | ✅ 跨设备协同白捡 |
| OTA | 未规划 | 16MB 分区表 ota×2 各 4M 就绪 | ✅ 白捡 |
| 表情 | face_engine 自研 8 态 | emote 模块自带表情(乐鑫风格) | ⚠️ 见 I.4 |
| 径向菜单/设置页 | v6.1 主交互 | **完全没有** | ➖ 互补 |
| License | (本工程) | Apache-2.0 | ✅ 可商用 |

### I.4 ★ ESP-Claw `emote` vs 本规格 `face_engine` 详细对比

> **决策关键节点**——表情模块是 esp-claw 里唯一与本规格设计撞车的地方。

#### I.4.1 渲染管线本质区别

| 维度 | 本规格 face_engine | ESP-Claw emote |
|---|---|---|
| 渲染库 | LVGL 9.5(`lv_canvas` / `lv_image` / `lv_anim`) | `esp_emote_gfx`(乐鑫私有 component,Apache-2.0) |
| 资源格式 | 自制(SVG→C 数组 或 帧序列 PNG→C) | 内置 `assets_local/`(emoji 风格 spiffs 3MB) |
| flush 路径 | `esp_lvgl_port` → LVGL → `lv_display_flush_cb` → LCD | `gfx_*` API 直接调 `esp_lcd_panel_draw_bitmap`,**绕开 LVGL** |
| 屏幕仲裁 | LVGL 独占 | `display_arbiter` 让 emote 与 LVGL **抢屏**,owner 切换驱动重绘 |
| Flash 占用 | 自行控制(KB 级) | 3MB SPIFFS 起步(`emote` 分区) |
| PSRAM 占用 | 取决于帧数 | 实测约 1.5–2MB |
| 可二次开发 | 全 LVGL 生态,任意改 | 风格受限,只能换 assets 不能改渲染逻辑 |
| 与 AI 状态联动 | 需自己写 thinking/replying 状态机 | `emote_set_event_msg()` 直接接 `claw_core` 状态 |
| 与 LVGL 共存 | N/A(同栈) | 需仲裁,有切屏闪烁风险 |

#### I.4.2 优缺点对比矩阵

| | face_engine | emote |
|---|---|---|
| **优点** | ① 设计自主,8 表情+`look_left/right` 是产品定义的味道<br>② 全屏统一 LVGL 栈,菜单/设置/气泡可直接叠加<br>③ 每像素可调,出问题能 debug<br>④ 资源可精算到 KB 级 | ① `emote_start()` 一行起,网络状态自动反馈,P0 价值<br>② 乐鑫维护,有 component manager 持续更新<br>③ 与 `claw_core` 对话状态直连(thinking/replying)<br>④ 专门优化的逐帧渲染,延迟低、抖动小<br>⑤ `display_arbiter` 本身可单独复用 |
| **缺点** | ① 表情资产需自己美术工作量<br>② LVGL task 在 30fps 时压力大<br>③ AI 联动需自写状态机<br>④ 大量逐帧动画暂未优化 | ① 视觉风格无法定制,与产品 IP 难匹配<br>② 黑盒,出问题难调<br>③ 占 3MB Flash + 1.5-2MB PSRAM 起步<br>④ 与 LVGL 走仲裁,可能出现切屏闪烁<br>⑤ 强绑 `esp_emote_gfx`,后续升级跟乐鑫节奏 |

#### I.4.3 判断准则

| 产品定位 | 建议 |
|---|---|
| 有自己 IP 的桌面机器人(本项目正在做的事) | ✅ **保留 face_engine,不引入 emote** |
| 通用 AI 设备 demo,要快出货 | 用 emote,省 2–4 周美术+工程 |
| 折中 | `display_arbiter` 单独拎过来用(为未来扩屏留口),表情仍走 face_engine |

> **本规格默认选项:保留 face_engine,放弃 emote。** 理由:v6.1 原型的表情设计与产品定位绑定,emote 的 emoji 风格资产风格不符;且本工程已规划好资源生成流水线(同 icons_pack 套路),美术成本可控。

### I.5 三种集成方案(挑一)

#### 方案 A:全量集成
把 `application/edge_agent` 整套搬过来,你的 LVGL UI 作为 component 叠在上面。
- **收益**:对话/配网/OTA/技能/MCP 全到位,工期压缩 60–70%
- **代价**:核心路径深度耦合 esp-claw,跟它升级节奏走
- **适用**:产品定位 = "AI 桌面伴侣",不只是会动的屏幕

#### 方案 B:剥离式集成 ★(推荐)
只拿 ESP-Claw 的**"基础设施层"**,你的 LVGL UI 主权不动。

| 拿过来 | 不拿 |
|---|---|
| ✅ `boards/m5stack/m5stack_cores3/`(板级 YAML) | ❌ `emote/`(I.4 决定) |
| ✅ `wifi_manager/` + `captive_dns/` + `http_server`(直接补 P0 配网) | ❌ `cap_im_wechat`(非需求) |
| ✅ `settings/` + FATFS 挂载方式 | ❌ `cap_im_platform`(非需求) |
| ✅ `claw_core/` + `cap_im_local/` + `claw_session_mgr`(接 Sector 1 对话) | ❌ `cap_web_search`(可选) |
| ✅ `cap_mcp_server`(可选,Sector 4 Puzzle 外延) | |
| ✅ `cap_skill_mgr` + `cap_lua`(可选,Sector 4 Puzzle 做技能商店) | |
| ✅ `partitions_16MB.csv`(直接采用,见 I.6) | |

#### 方案 C:零集成,只借鉴
读源码,把 `wifi_manager` 状态机、`board_devices.yaml` 声明式板级、Lua 模块封装思路抄到自己工程,**不引入任何 component**。
- **收益**:零依赖,完全自主
- **代价**:对话/MCP/技能从零写,工期 +3–6 个月
- **适用**:对 esp-claw 演进节奏不放心,或团队有强反 NIH 政策

### I.6 关键校正:Quad PSRAM 而非 Octal(重要)

> **CoreS3 实际是 Quad PSRAM**(N16R8 中的 R8 是 8MB Quad,不是 Octal)。本规格附录 G.3.5 早期写法是错的,本节修正:

```
CONFIG_ESP32S3_SPIRAM_SUPPORT=y
CONFIG_SPIRAM=y
CONFIG_SPIRAM_MODE_QUAD=y               # ★ CoreS3 是 Quad,不是 Octal
CONFIG_SPIRAM_SPEED_80M=y
CONFIG_SPIRAM_XIP_FROM_PSRAM=n          # Quad PSRAM 不支持 XIP
CONFIG_SPIRAM_FETCH_INSTRUCTIONS=n
CONFIG_SPIRAM_RODATA=n
CONFIG_FREERTOS_TASK_CREATE_ALLOW_EXT_MEM=n   # Quad 不能在 cache 关闭时访问栈
CONFIG_ESPTOOLPY_FLASHMODE_QIO=y
CONFIG_ESPTOOLPY_FLASHFREQ_80M=y
CONFIG_ESPTOOLPY_FLASHSIZE_16MB=y
CONFIG_FREERTOS_HZ=1000
CONFIG_LV_COLOR_DEPTH_16=y
CONFIG_COMPILER_OPTIMIZATION_PERF=y
```

> 验证依据:`esp-claw/application/edge_agent/boards/m5stack/m5stack_cores3/sdkconfig.defaults.board` 明确写 `CONFIG_SPIRAM_MODE_QUAD=y`。**附录 G.3.5 中 "CONFIG_SPIRAM_MODE_OCT=y" 的写法作废,以本节为准。**

### I.7 方案 B 落地步骤(可直接执行)

**Step 0** — 拓扑变更
```
robot_ui/
├── components/
│   ├── icons_pack/                  (现有)
│   ├── ui_face/                     (face_engine,保留)
│   ├── ui_radial_menu/              (Step 3 产出)
│   ├── ui_settings/                 (Step 4 产出)
│   ├── ui_chat/                     ★ 新:LVGL 对话页 widget
│   ├── claw_core/                   ← 从 esp-claw 拷
│   ├── claw_cap/                    ← 从 esp-claw 拷
│   ├── claw_memory/                 ← 从 esp-claw 拷
│   ├── claw_event_router/           ← 从 esp-claw 拷
│   ├── cap_session_mgr/             ← 从 esp-claw 拷
│   ├── cap_im_local/                ← 从 esp-claw 拷
│   ├── cap_skill_mgr/               ← 可选
│   ├── cap_mcp_server/              ← 可选
│   ├── wifi_manager/                ← 从 esp-claw 拷(补 P0 配网)
│   ├── captive_dns/                 ← 配套
│   ├── settings/                    ← 从 esp-claw 拷
│   └── m5stack_cores3_board/        ← 从 esp-claw/boards/ 拷,改路径
```

**Step 1** — Wi-Fi 配网(直接补 §E.3 P0,最高 ROI)
搬 `wifi_manager` + `captive_dns` + `http_server`,配网走 Web,不占 LVGL。**工期 1–2 天,几乎零代码。**

**Step 2** — 对话能力上 Sector 1 Message
- `ui_chat/` 写 LVGL 对话页(气泡列表 + 输入栏)
- 接 `claw_core` 请求/响应回调:用户敲字 → `claw_core_submit_request` → 回调里 `lvgl_port_lock(0)` 后追加气泡(遵守附录 G.3.3)
- LLM endpoint 在 `app_config.h` 配
- **工期 3–5 天**

**Step 3(可选)** — Sector 4 Puzzle 做技能商店
启用 `cap_skill_mgr` + `cap_lua`,Puzzle 扇区列出本机已装 Lua 技能,点击执行。**工期 2–3 天**

**Step 4(可选)** — MCP server
启 `cap_mcp_server`,飞书侧 Mira 或其他 Agent 通过 MCP 调用设备能力(读 IMU、播表情、调亮度)。**工期 2 天**

**Step 5** — 不引入 emote,保留 face_engine
**不要**把 `components/common/emote/` 加进 robot_ui。若未来想换,通过 `display_arbiter` 注册 owner 平滑切换;此模块可独立拿过来。

### I.8 风险与必须避开的坑

| 风险 | 表现 | 规避 |
|---|---|---|
| 渲染栈双轨 | LVGL 与 emote_gfx 抢屏 → 撕裂 | 用 `display_arbiter`,或干脆只装一个(见 I.4) |
| **PSRAM 模式分歧** | 旧写法用 Octal,实际 CoreS3 是 Quad → 启动 crash | 按 I.6 修正,**附录 G.3.5 已作废** |
| 依赖膨胀 | 全量集成后 build 时间 2min → 6min+ | 用方案 B 只挑必要 cap |
| upstream 漂移 | esp-claw 还在快速迭代(2026 年的代码) | 锁版本到 commit,半年 review 一次升级 |
| License 合规 | Apache-2.0 需保留 NOTICE | 拷贝 components 时连 `SPDX-FileCopyrightText` 头一起保留 |
| `esp_board_manager` 学习成本 | 板级从手写驱动 → YAML 声明式 | 收益是减负,200 行 init 变 1 个 yaml |

### I.9 给 AI 编码助手的增量约束(v5.4 → v5.5)

在原 §E.5、§F.8、§G.4 之后追加:

24. **若引入 ESP-Claw**:`emote` 模块**禁止**与本工程 face_engine 并存——二选一,默认保留 face_engine。
25. **PSRAM 配置**:**必须**用 `CONFIG_SPIRAM_MODE_QUAD=y`,**禁止**使用 Octal 模式(CoreS3 硬件不支持)。
26. **claw_core 与 LVGL 交互**:claw_core 回调运行在独立任务上下文,**必须**在回调内 `lvgl_port_lock(0)` 后再操作 UI(遵守 G.3.3)。
27. **License 头**:从 esp-claw 拷贝的任何 `.c/.h` 文件**必须**保留 `SPDX-FileCopyrightText: 2026 Espressif Systems` 头部,不得删除。
28. **CoreS3 sdkconfig 基线**:**必须**先合并 `esp-claw/application/edge_agent/boards/m5stack/m5stack_cores3/sdkconfig.defaults.board` 再叠加业务配置,**禁止**从零撸 sdkconfig。

---

---

## 附录 J:方案 B 实施详解(v5.6 新增)

> 把附录 I.7 的 5 个 Step 展开为代码级落地说明。覆盖 **Wi-Fi 配网 / 对话 / 技能商店 / MCP server / 不引入 emote** 的具体源码、组件清单、`sdkconfig` 增量、`idf_component.yml`、与 v5.1 UI 层的绑定点。

### J.1 顶层组件拓扑(目标态)

```
robot_ui (main 工程)
├── components/
│   ├── ui_core/                # v5.1 既有:LVGL widgets / 主题系统
│   ├── face_engine/            # v5.1 既有:8 表情状态机(emote 不并存,见规则 24)
│   ├── icons_pack/             # 附录 F:6 扇区图标
│   ├── claw_core/              # ★ 从 esp-claw 拷入,Agent Loop 引擎
│   ├── claw_memory/            # ★ 拷入,结构化会话记忆
│   ├── claw_event_router/      # ★ 拷入,事件总线
│   ├── cap_im_platform/        # ★ 拷入,LLM/IM 接入(对话能力)
│   ├── cap_mcp_client/         # ★ 拷入,MCP 客户端
│   ├── cap_mcp_server/         # ☆ Step 4 可选
│   ├── cap_skill_mgr/          # ☆ Step 3 可选,技能注册/调度
│   ├── cap_lua/                # ☆ Step 3 可选,Lua 沙箱
│   ├── wifi_manager/           # ★ 拷入,STA+AP 双模 + Captive Portal
│   ├── captive_dns/            # ★ 拷入,DNS 劫持(配网)
│   ├── http_reuse/             # ★ 拷入,HTTP 复用/SSE
│   ├── settings/               # ★ 拷入,NVS 配置门面
│   └── (esp_board_manager via managed component)
└── main/
    ├── main.c                  # 启动序列(见 J.3)
    ├── ui_bridge.c             # ★ 新增:claw_core ↔ UI 的胶水
    └── idf_component.yml       # 见 J.2
```

**关键决策**:`emote/` **不拷**(规则 24);`display_arbiter` 只在未来需要 emote 时才引入;`cap_im_local`(端侧小模型)**暂不**集成,**Step 2 直接走平台侧 LLM**。

### J.2 `main/idf_component.yml`(增量)

```yaml
dependencies:
  idf:
    version: ">=5.5.4"

  # 既有(v5.5)
  lvgl/lvgl: "^9.5.0"
  espressif/esp_lvgl_port: "^2.4.0"

  # ★ 新增:板级声明式管理(消除手写 board init)
  espressif/esp_board_manager: "^0.5.10"

  # ★ 新增:LLM/Agent 运行时(以 commit 锁版)
  espressif/claw_core:
    version: "*"
    git: "https://github.com/espressif/esp-claw.git"
    path: "components/claw_modules/claw_core"
    rules:
      - if: "target == esp32s3"

  espressif/claw_memory:
    version: "*"
    git: "https://github.com/espressif/esp-claw.git"
    path: "components/claw_modules/claw_memory"

  espressif/claw_event_router:
    version: "*"
    git: "https://github.com/espressif/esp-claw.git"
    path: "components/claw_modules/claw_event_router"

  espressif/cap_im_platform:
    version: "*"
    git: "https://github.com/espressif/esp-claw.git"
    path: "components/claw_modules/cap_im_platform"

  espressif/cap_mcp_client:
    version: "*"
    git: "https://github.com/espressif/esp-claw.git"
    path: "components/claw_modules/cap_mcp_client"

  # ☆ Step 3/4 可选,不勾就注释掉
  # espressif/cap_skill_mgr: { git: "...", path: "components/claw_modules/cap_skill_mgr" }
  # espressif/cap_lua:       { git: "...", path: "components/claw_modules/cap_lua" }
  # espressif/cap_mcp_server:{ git: "...", path: "components/claw_modules/cap_mcp_server" }
```

> 真实工程建议用 `idf.py add-dependency` 后手工把 `version:` 改成具体 commit hash,半年一次升级 review(规则 28 衍生)。

### J.3 启动序列(`main.c`)

```c
/* main.c — robot_ui v5.6 启动序列 */
#include "nvs_flash.h"
#include "esp_lvgl_port.h"
#include "esp_board_manager.h"
#include "wifi_manager.h"
#include "captive_dns.h"
#include "settings.h"
#include "claw_core.h"
#include "claw_memory.h"
#include "claw_event_router.h"
#include "ui_core.h"          // v5.1
#include "face_engine.h"      // v5.1
#include "ui_bridge.h"        // 新增

void app_main(void)
{
    /* 0. NVS + 设置门面 */
    ESP_ERROR_CHECK(nvs_flash_init());
    settings_init();

    /* 1. 板级硬件:LCD/touch/AXP2101/aw88298/es7210 一把梭(YAML 驱动) */
    ESP_ERROR_CHECK(esp_board_manager_init());

    /* 2. LVGL port(附录 G.3.2)+ UI 根 + face_engine */
    lvgl_port_cfg_t lvcfg = ESP_LVGL_PORT_INIT_CONFIG();
    lvcfg.task_affinity = 1;          /* SMP:固定到 Core1,见规则 22 */
    lvcfg.task_priority = 4;
    lvcfg.task_stack    = 8 * 1024;
    ESP_ERROR_CHECK(lvgl_port_init(&lvcfg));

    /* 把 board_manager 暴露的 panel/touch 注册到 lvgl_port */
    ui_core_attach_display();         /* 内部 lvgl_port_add_disp/touch */
    face_engine_start();              /* 起背景 6 帧情绪状态机 */

    /* 3. 网络:启动 wifi_manager,优先 STA;失败回落 AP+Captive */
    wifi_manager_cfg_t wcfg = {
        .ap_ssid_prefix = "MiraBot-",
        .ap_password    = "",          /* 开放 AP,跳 Captive */
        .sta_retry_max  = 3,
        .on_event       = ui_bridge_on_wifi_event,   /* J.4 */
    };
    wifi_manager_start(&wcfg);
    captive_dns_start();

    /* 4. Agent Loop(对话核心) */
    claw_memory_init(&(claw_memory_cfg_t){
        .backend = CLAW_MEM_NVS_PARTITION, .ns = "claw"
    });
    claw_event_router_init();

    claw_core_cfg_t ccfg = {
        .max_turns       = 16,
        .context_provider= ui_bridge_context_provider,   /* J.5 注入设备态 */
        .on_token        = ui_bridge_on_token,           /* 流式回调 */
        .on_done         = ui_bridge_on_done,
        .on_tool_call    = ui_bridge_on_tool,
    };
    claw_core_init(&ccfg);

    /* 5. UI 桥接:6 扇区菜单注册 sector 回调 */
    ui_bridge_register_menu();
}
```

### J.4 Wi-Fi 配网 UX 与状态机(对应 Step 1)

| 阶段 | UI 表现(face_engine + LVGL) | 后台动作 |
|---|---|---|
| 上电 STA 重连 | face = `idle`,屏顶 icon: Wi-Fi 灰 | `wifi_manager` 尝试 NVS 中保存的 SSID,3 次失败回落 |
| 配网模式 | face = `look_left`,Toast「请连接 MiraBot-XXXX」 | AP 开,`captive_dns` 把所有 DNS 解析到 192.168.4.1 |
| 用户连入 AP | face = `smile`,屏显二维码(LVGL `lv_qrcode`) | HTTP 服务返回配网页面 |
| 提交凭据 | face = `thinking`(眨眼) | `wifi_manager_provision(ssid,pwd)` → 切 STA,失败回 AP |
| 联网成功 | face = `happy_blink` 1.5s → 回 `idle` | `claw_core_post_event("net.online")`,Sector 1 解锁 |

**胶水实现**(节选 `ui_bridge.c`):

```c
void ui_bridge_on_wifi_event(wifi_manager_event_t ev, void *data) {
    lvgl_port_lock(0);
    switch (ev) {
    case WMGR_AP_STARTED:
        face_engine_set(FACE_LOOK_LEFT);
        ui_core_toast("请连接 Wi-Fi:%s", (const char *)data);
        ui_core_show_qrcode("http://192.168.4.1");
        break;
    case WMGR_STA_CONNECTED:
        face_engine_set(FACE_HAPPY_BLINK);
        ui_core_hide_qrcode();
        claw_event_router_post("net.online", NULL, 0);
        break;
    case WMGR_STA_FAILED:
        face_engine_set(FACE_SAD);
        ui_core_toast("联网失败,重试中...");
        break;
    }
    lvgl_port_unlock();
}
```

### J.5 对话(Sector 1 / Step 2)

**UI**:点击 Sector 1 → 进入「对话页」全屏 widget;LVGL 组件:

```
┌──────────────────────────────┐
│  ← 返回           主题:🟧    │  ← 顶栏
├──────────────────────────────┤
│  [Bot] 你好,我在听呢       │
│              [Me] 帮我设个闹钟│  ← lv_list 反向滚动
│  [Bot] 几点呢?              │
│  …                            │
├──────────────────────────────┤
│ [🎤]  ▢ 输入...        [发送] │  ← 底部输入条
└──────────────────────────────┘
```

**绑定**(节选):

```c
/* sector 1 按下时打开 Dialog 页 */
static void on_sector_msg_clicked(lv_event_t *e) {
    ui_dialog_open();                /* 创建/复用 dialog 页 */
}

/* 发送按钮 */
static void on_send_clicked(lv_event_t *e) {
    const char *txt = lv_textarea_get_text(input_ta);
    ui_dialog_append_bubble(BUBBLE_ME, txt);
    lv_textarea_set_text(input_ta, "");

    /* 投递到 claw_core(非阻塞,流式回调走 ui_bridge_on_token) */
    claw_core_request_t req = {
        .role = "user",
        .content = txt,
        .session_id = ui_dialog_session_id(),
    };
    claw_core_send(&req);
    face_engine_set(FACE_THINKING);
}

/* claw_core 流式 token 回调(运行在 claw_core 任务) */
void ui_bridge_on_token(const char *delta, void *ud) {
    lvgl_port_lock(0);
    ui_dialog_append_or_extend_bot(delta);
    lvgl_port_unlock();
}

void ui_bridge_on_done(claw_core_result_t r, void *ud) {
    lvgl_port_lock(0);
    face_engine_set(r.ok ? FACE_HAPPY_BLINK : FACE_SAD);
    lvgl_port_unlock();
}
```

**Context Provider**(把设备态注入 LLM 上下文):

```c
int ui_bridge_context_provider(char *buf, size_t cap) {
    return snprintf(buf, cap,
        "{\"device\":\"M5Stack CoreS3\","
        "\"battery\":%d,\"theme\":\"%s\","
        "\"location\":\"%s\",\"time\":\"%s\"}",
        settings_get_int("battery", 100),
        settings_get_str("theme", "orange"),
        settings_get_str("location", "unknown"),
        ui_core_now_iso8601());
}
```

### J.6 技能商店(Sector 4 / Step 3,可选)

**SKILL.md 规范**(摘自 esp-claw `claw-skill-spec.md`):

```yaml
---
name: weather_today
description: 查询某城市今日天气
runtime: lua
trigger:
  intents: ["问天气", "weather"]
  slots:
    city: { type: string, required: true }
permissions: ["net.http"]
---
function on_invoke(ctx, slots)
  local r = http.get("https://wttr.in/" .. slots.city .. "?format=j1")
  return { text = "今天 " .. slots.city .. " " .. r.current.weatherDesc }
end
```

**Sector 4 页面**:

| 区域 | 控件 | 数据源 |
|---|---|---|
| 顶部「已安装」 | `lv_list` 复选行 | `cap_skill_mgr_list(SKILL_INSTALLED)` |
| 中部「商店」 | `lv_list` 卡片(图标+名+一句话) | 后台 `GET /v1/skills?board=cores3` |
| 底部「同步」 | `lv_btn` | 触发增量拉取 |

调用约束:`cap_skill_mgr` 跑在独立任务,UI 拉列表要走 `cap_skill_mgr_list_async(cb)`,不得在 LVGL 任务里同步等。

### J.7 MCP server(Step 4,可选)

把本机能力(face / battery / IMU / LED)注册成 MCP tools 让远端调用:

```c
cap_mcp_server_cfg_t mcfg = { .port = 8080, .auth_token = settings_get_str("mcp_token", "") };
cap_mcp_server_start(&mcfg);

cap_mcp_server_register_tool("face.set", "切换表情", face_set_schema, on_mcp_face_set);
cap_mcp_server_register_tool("imu.read", "读 IMU", imu_read_schema, on_mcp_imu_read);
```

**UI 提示**:Settings 页新增「允许远端调用」开关(默认关);开启时屏顶持续显示 🔌 图标(LVGL 主题色)。

### J.8 `sdkconfig` 增量(在 G.3.5 修正基础上叠加)

```
# CoreS3 基线已通过 esp-claw sdkconfig.defaults.board 合并(规则 28)
CONFIG_SPIRAM_MODE_QUAD=y
CONFIG_SPIRAM_SPEED_80M=y
CONFIG_ESPTOOLPY_FLASHSIZE_16MB=y

# 分区:沿用 esp-claw 的 16MB 模板
CONFIG_PARTITION_TABLE_CUSTOM=y
CONFIG_PARTITION_TABLE_CUSTOM_FILENAME="partitions_16MB.csv"

# HTTP/SSE 流式接收
CONFIG_ESP_HTTP_CLIENT_ENABLE_HTTPS=y
CONFIG_MBEDTLS_DYNAMIC_BUFFER=y

# claw_core 任务栈
CONFIG_CLAW_CORE_TASK_STACK=12288
CONFIG_CLAW_CORE_TASK_PRIO=5
CONFIG_CLAW_CORE_TASK_AFFINITY=0   # ★ 与 LVGL Core1 错开
```

### J.9 分区表(`partitions_16MB.csv`,与 esp-claw 对齐)

```
# Name,    Type, SubType,  Offset,   Size,   Flags
nvs,       data, nvs,      0x9000,   0x6000,
phy_init,  data, phy,      0xf000,   0x1000,
factory,   app,  factory,  0x10000,  0x100000,    # 1MB(可选,出厂烧录用)
ota_0,     app,  ota_0,    ,         0x400000,    # 4MB
ota_1,     app,  ota_1,    ,         0x400000,    # 4MB
storage,   data, fat,      ,         0x400000,    # 4MB FATFS(设置/日志/缓存)
claw,      data, nvs,      ,         0x40000,     # 256KB claw_memory
skills,    data, spiffs,   ,         0x200000,    # 2MB 技能脚本(Step 3 才用)
```

> 不放 emote(规则 24)。skills 分区若不启用 Step 3 可去掉,改成 storage 扩到 6MB。

### J.10 任务/核心分配(SMP)

| 任务 | Core | 优先级 | 栈 | 说明 |
|---|---|---|---|---|
| `lvgl_port_task` | **1** | 4 | 8K | UI 刷新,见规则 22 |
| `face_engine_task` | 1 | 4 | 4K | 与 LVGL 同核,免锁 |
| `claw_core_task` | **0** | 5 | 12K | LLM/工具调度,IO 重 |
| `wifi_manager_task` | 0 | 6 | 4K | 网络事件 |
| `http_reuse 工作池` | 0 | 5 | 6K×N | SSE/REST |
| `cap_skill_mgr` | 0 | 4 | 6K | Lua 沙箱执行 |

跨核回调必走 `lvgl_port_lock(0)`(规则 26)。

### J.11 与既有 §四章源码的迁移点

§四(IDF 5.2 时代的手写 driver_init / xTaskCreate / lv_disp_drv_register)在方案 B 下**整章作废为参考**,实际新代码遵循:

1. 屏/触摸/电源:**删除手写驱动**,改 `esp_board_manager_init()`(J.3 第 1 步)。
2. LVGL 初始化:**删除** `lv_init` + `lv_disp_drv_t` 三件套,改 `lvgl_port_init`(G.3.2)。
3. 表情状态机:**保留** `face_engine`,无需改动。
4. 主菜单事件:沿用 v5.1 widget,只把扇区 1/4 的回调从 stub 改为 J.5/J.6 的 binding。

---

## 附录 K:后台架构(v5.6 新增)

> 设备端跑 claw_core / MCP 是"前台";配套的"后台"服务承接 LLM 推理、会话持久化、技能分发、固件分发、配网协助、可观测性。本附录给出**最小可用后台(MVB)**+ **演进路线**。

### K.1 顶层架构(文字版)

```
                     ┌──────────────────────────┐
                     │       前端控制台          │  (Web,Vue/React)
                     │   设备管理 / 技能审核     │
                     └──────────┬───────────────┘
                                │ HTTPS
   ┌────────────────────────────┴────────────────────────────┐
   │                  API Gateway (Kong / APISIX)             │
   │  鉴权 / 限流 / 路由 / TLS 终止 / 多租户隔离              │
   └─┬─────────┬─────────┬─────────┬─────────┬───────────────┘
     │         │         │         │         │
     ▼         ▼         ▼         ▼         ▼
 ┌────────┐┌────────┐┌────────┐┌────────┐┌─────────────┐
 │ LLM    ││ Session││ Skill  ││ OTA    ││ Provisioning│
 │ Gateway││ Store  ││ Store  ││ Service││ Helper      │
 └───┬────┘└────────┘└────────┘└────────┘└─────────────┘
     │                                          ▲
     ▼                                          │
 ┌─────────────────────┐                ┌──────────────┐
 │ Model Backends      │                │  MCP Registry │
 │  OpenAI / Claude /  │                │  设备发现 / ACL│
 │  Qwen / 内网 vLLM   │                └──────────────┘
 └─────────────────────┘
                        ┌─────────────────────────────────┐
                        │     Telemetry / Observability    │
                        │  ClickHouse + Grafana + Loki     │
                        └─────────────────────────────────┘
```

### K.2 服务清单

| 服务 | 职责 | 技术栈建议 | 数据库 | MVB? |
|---|---|---|---|---|
| **API Gateway** | 统一入口、TLS、租户鉴权、限流 | APISIX / Kong | — | ✅ |
| **LLM Gateway** | 厂商抽象、SSE 流式转发、模型路由、Token 计量、合规过滤 | Go / Rust | Redis(限流) | ✅ |
| **Session Store** | 会话与记忆持久化(对齐 claw_memory schema) | Node/Python | PostgreSQL + pgvector | ✅ |
| **Device Registry** | 设备身份、租户绑定、影子状态 | Go | PostgreSQL | ✅ |
| **OTA Service** | 固件版本、灰度、回滚、签名 | Go | S3/OSS + Postgres | ✅ |
| **Skill Store** | SKILL.md 审核、版本、分发、签名 | Python(FastAPI) | Postgres + S3 | △ Step 3 起用 |
| **MCP Registry** | 设备暴露的 MCP server 发现 / ACL | Go | Postgres + Redis | △ Step 4 起用 |
| **Provisioning Helper** | 云配对(QR / 6 位码)、Wi-Fi 凭据下发 | Go | Postgres + Redis | △ 可选 |
| **Auth** | 用户登录、JWT、租户、角色 | Keycloak 或自研 | Postgres | ✅ |
| **Telemetry** | 设备心跳 / 崩溃 / 对话采样 / 业务指标 | OTLP → ClickHouse | ClickHouse + Loki | ✅ |

> **MVB(最小可用后台)**:API Gateway + LLM Gateway + Session Store + Device Registry + OTA + Auth + Telemetry。**Step 1+2 上线即可用**;Step 3/4 再加 Skill / MCP / Provisioning。

### K.3 LLM Gateway 详设

**请求范式**(设备 ↔ Gateway):

```
POST /v1/chat
Authorization: Bearer <device_jwt>
X-Mira-Session: <session_id>
X-Mira-Device: <device_sn>
Content-Type: application/json

{
  "messages": [{"role":"user","content":"今天上海天气"}],
  "context":  {"device":"M5Stack CoreS3","battery":78,"theme":"orange"},
  "stream":   true,
  "tools":    [{"name":"weather.query","schema":{...}}]
}

→ text/event-stream
data: {"delta":"今天"}
data: {"delta":"上海"}
data: {"tool_call":{"name":"weather.query","args":{"city":"shanghai"}}}
data: {"delta":"...多云,28°C"}
data: [DONE]
```

**路由策略**:

| 场景 | 默认模型 | 兜底 |
|---|---|---|
| 短问答(<200 tok) | 内网 Qwen2.5-7B(vLLM) | OpenAI gpt-4o-mini |
| 长上下文 / 工具调用 | Claude Sonnet | OpenAI gpt-4o |
| 紧急降级 | 缓存回放 | 固定话术 |

**合规与安全**:出向脱敏(IMEI/手机号正则);入向反注入(检测 `<system-reminder>` 等保留 token);全链路记 trace_id;Token 计量入 ClickHouse 按租户/设备聚合。

### K.4 Session Store 与 claw_memory 对齐

设备端 `claw_memory` 是端侧短期记忆(NVS,~256KB);后台 Session Store 是长期记忆。

**schema**(PostgreSQL):

```sql
CREATE TABLE sessions (
  session_id   UUID PRIMARY KEY,
  device_sn    TEXT NOT NULL,
  tenant_id    TEXT NOT NULL,
  created_at   TIMESTAMPTZ DEFAULT now(),
  last_active  TIMESTAMPTZ,
  summary      TEXT
);

CREATE TABLE turns (
  turn_id      BIGSERIAL PRIMARY KEY,
  session_id   UUID REFERENCES sessions,
  role         TEXT,        -- user / assistant / tool
  content      TEXT,
  tool_name    TEXT,
  tool_args    JSONB,
  tokens_in    INT,
  tokens_out   INT,
  created_at   TIMESTAMPTZ DEFAULT now()
);

CREATE TABLE memories (
  mem_id       BIGSERIAL PRIMARY KEY,
  device_sn    TEXT,
  kind         TEXT,        -- fact / preference / event
  payload      JSONB,
  embedding    VECTOR(768), -- pgvector
  weight       REAL DEFAULT 1.0,
  created_at   TIMESTAMPTZ DEFAULT now()
);

CREATE INDEX ON memories USING ivfflat (embedding vector_cosine_ops);
```

**同步策略**:设备每完成一轮 → 后台 `POST /v1/turns`;设备启动时 `GET /v1/session/last?device=...` 拉取摘要回灌 claw_memory。

### K.5 OTA Service

| 项 | 设计 |
|---|---|
| 协议 | `esp_https_ota` + ESP-Claw 内置 OTA 客户端 |
| 通道 | `stable / beta / dev`,设备 SN 维度灰度 |
| 签名 | RSA-3072 + `esp_secure_boot_v2`,后台保私钥 |
| 回滚 | A/B 双分区(分区表 ota_0/1,J.9);连续 3 次启动失败自动回滚 |
| 元数据 | 版本、变更说明、最低硬件 rev、CRC、大小、URL |
| 灰度 API | `POST /v1/ota/rollout {channel, percent, device_filter}` |
| 设备拉取 | `GET /v1/ota/manifest?sn=...&cur=v1.2.3` → 返回 url/sha256 或 304 |

**安全要求**:URL 30 分钟短期签名;固件包 SHA-256 入 manifest 二次校验。

### K.6 Skill Store(Step 3 起用)

| 流程 | 端点 |
|---|---|
| 开发者上传 | `POST /v1/skills` (multipart: SKILL.md + lua 源 + 图标) |
| 审核 | 后台控制台人审 + Lua 静态扫描(危险 API 黑名单) |
| 发布 | `POST /v1/skills/{id}/publish {channel, board}` |
| 设备拉清单 | `GET /v1/skills?board=cores3&installed=...` |
| 设备拉包 | `GET /v1/skills/{id}/pkg` (tar.gz,RSA 签名) |

**沙箱约束**:Lua 默认无 `io/os`;`net.http` 白名单域;CPU 时片 100ms;内存 64KB。后台审核器同标准。

### K.7 MCP Registry(Step 4 起用)

把设备暴露成 MCP server 后,Registry 解决"飞书/外部 Agent 怎么发现这台设备":

```
POST /v1/mcp/register   { device_sn, endpoint, tools[], auth_pubkey }
GET  /v1/mcp/discover?owner=<user>  → 返回该用户名下设备的 MCP 端点列表
POST /v1/mcp/acl        { device_sn, allow_callers: [...] }
```

设备上线后周期性心跳保活;离线 60s 标记不可达。

### K.8 配网 Helper(可选,云配对)

替代 Captive Portal 的更顺手流程:

1. 设备开 AP 模式,屏幕显示 6 位云码(从 `/v1/provision/claim` 取);
2. App 扫码或输入 6 位码;
3. App 向后台 `POST /v1/provision/bind {code, ssid, pwd_enc}`;
4. 设备轮询 `GET /v1/provision/cred?code=...`,拿到密文凭据(设备公钥加密);
5. 设备落 NVS,切 STA。

好处:无需用户切 Wi-Fi 连 MiraBot-XXXX。

### K.9 Telemetry / 可观测性

| 类目 | 字段 | 采样 |
|---|---|---|
| 设备心跳 | sn, fw, rssi, batt, free_heap, uptime | 60s |
| 对话采样 | session_id, model, token_in/out, latency_ms, tool_calls | 100% 元数据 / 1% 内容 |
| 崩溃 | core dump url, fw, ts | 100% |
| 业务事件 | sector_click, theme_change, skill_run | 100% |

通道:设备 OTLP/HTTP → Collector → ClickHouse(指标/事件) + Loki(日志) + S3(core dump)。Grafana 看板按租户/版本/型号切片。

### K.10 多租户与安全

- **租户隔离**:`tenant_id` 贯穿所有表;Gateway 层在 JWT 校验后注入,RLS(Row Level Security)兜底。
- **设备身份**:出厂烧录 ECDSA 设备私钥(eFuse 保护);首次上线走双向 TLS 取设备 JWT。
- **配额**:LLM Token / OTA 带宽 / 技能数,租户级软限 + 设备级硬限。
- **审计**:所有 admin 操作入 audit_log(append-only);敏感操作二次确认。

### K.11 部署形态

| 规模 | 形态 |
|---|---|
| **MVB** | 单 K8s 集群,1 个 namespace;Postgres + Redis + Clickhouse 各 1 实例(StatefulSet) |
| **生产** | 3 region(SG/US/CN),设备就近接入;LLM Gateway 跟随 region;Session Store 跨 region 异步复制 |
| **离线/私有化** | docker-compose 一键起;LLM 默认指向 vLLM 自建 |

### K.12 后台 API 速查(MVB)

| 方法 | 路径 | 用途 |
|---|---|---|
| POST | `/v1/auth/device` | 双向 TLS → 设备 JWT |
| POST | `/v1/chat` (SSE) | LLM 流式对话 |
| POST | `/v1/turns` | 会话轮次落库 |
| GET  | `/v1/session/last` | 拉取上次会话摘要 |
| GET  | `/v1/ota/manifest` | 检查升级 |
| POST | `/v1/telemetry` | 上报心跳/事件 |
| GET  | `/v1/skills` | 拉技能清单(Step 3) |
| POST | `/v1/mcp/register` | 注册 MCP(Step 4) |
| POST | `/v1/provision/bind` | 云配对(可选) |

---

## 附录 L:UX 迭代规划(v5.6 新增)

> 方案 B 落地后,设备从"纯本地表情玩具"升级为"联网 Agent 终端"。UX 必须给新能力相称的入口、状态与可恢复路径。

### L.1 新增/调整的页面清单

| # | 页面 | 入口 | 状态 | 优先级 |
|---|---|---|---|---|
| 1 | 配网页(AP + Captive) | 开机未配网自动进入 | 新增 | P0 |
| 2 | 配网页(云配对 6 位码版) | Settings → 重新配网 | 新增可选 | P1 |
| 3 | 对话页(Sector 1) | 径向菜单 → Message | 由占位升级为功能页 | P0 |
| 4 | OTA 升级页 | Settings → 检查更新 / 后台推送 | 新增 | P0 |
| 5 | 技能列表页(Sector 4) | 径向菜单 → Puzzle | 由占位升级 | P1 |
| 6 | 技能详情/确权页 | 技能列表 → 安装 | 新增 | P1 |
| 7 | MCP 远端调用提示 | 顶栏图标 + Toast | 新增 | P2 |
| 8 | 隐私/合规面板 | Settings → 隐私 | 新增 | P0(合规) |

### L.2 对话页(P0)详细 spec

| 元素 | 规格 |
|---|---|
| 顶栏 | 高 36px,左:返回箭头(28×28);右:模型徽标(小圆点+模型名,主题色) |
| 消息列表 | `lv_list` 反向追加;气泡:Me 主题色填充右贴,Bot 灰底左贴;头像 24×24 |
| 气泡间距 | 8px vertical;最大宽 84% 屏宽;长按复制(LVGL `LV_EVENT_LONG_PRESSED`) |
| 思考态 | Bot 气泡占位 + 3 点 dot loading(轮播 200ms) |
| 输入条 | 高 56px;左:🎤 切换语音;中:`lv_textarea`;右:发送(disable 当空) |
| 错误态 | 红色 inline error + 重试按钮;face = SAD 1.5s |
| 流式渲染 | `delta` 直接 `append_text`,不重排;>500 字才分页 |
| 中断 | 思考态点屏其他位置弹「停止生成」浮按 |
| 与表情联动 | 用户发送 → THINKING;首 delta 到达 → TALKING;DONE → HAPPY_BLINK 0.8s → IDLE;失败 → SAD |

### L.3 配网页(P0)详细 spec

**AP + Captive 版**:

```
┌──────────────────────────────────────┐
│            [Wi-Fi 灰图]               │
│         请用手机连接                  │
│      ┌───────────────────┐           │
│      │   MiraBot-A1B2    │           │  ← 设备 SN 后 4 位
│      └───────────────────┘           │
│                                       │
│        [二维码 192×192]               │  ← 扫码或手动输 192.168.4.1
│                                       │
│       连接后浏览器自动打开            │
└──────────────────────────────────────┘
```

face 表现:`look_left`(等待)→ 检测到用户连入 AP 切 `smile`。

**云配对版**:

```
┌──────────────────────────────────────┐
│        请在 App 输入云配对码          │
│                                       │
│              ╭───────╮                │
│              │ 8 4 2 │                │
│              │ 6 7 9 │                │  ← 大号 6 位码,5min 失效
│              ╰───────╯                │
│        剩余 04:32 · 重新生成          │
└──────────────────────────────────────┘
```

### L.4 OTA 升级页(P0)

| 阶段 | UI | face |
|---|---|---|
| 检测中 | spinner + "检查更新中..." | idle |
| 有更新 | 卡片显示版本+变更说明+「立即升级 / 稍后」 | look_right |
| 下载中 | 进度条 + "下载中 32%" + "可断电,自动续传" | thinking(眨眼频率↓) |
| 写 flash | "正在安装,请勿断电" + 警告色 | sleep(闭眼) |
| 完成 | "已升级到 v1.3.0" + 倒数 5s 重启 | happy_blink |
| 失败 | "升级失败,已回滚" + 重试 + 上报 | sad |

约束:整页期间屏蔽径向菜单进入;写 flash 阶段强制保活背光。

### L.5 技能列表页(P1)

```
┌──────────────────────────────────────┐
│  ← 技能          [搜索]    [刷新]    │
├──────────────────────────────────────┤
│  已安装(3)                            │
│  ┌──────────────────────────────┐    │
│  │ ☑ 🌤  weather_today    [启用]│    │
│  │ ☑ ⏰  alarm            [启用]│    │
│  │ ☐ 🎵  music_search     [启用]│    │
│  └──────────────────────────────┘    │
│                                       │
│  商店推荐                              │
│  ┌──────────────────────────────┐    │
│  │ 🦜 鹦鹉学舌            [安装]│    │
│  │ 📚 番茄钟              [安装]│    │
│  └──────────────────────────────┘    │
└──────────────────────────────────────┘
```

**确权页**(点「安装」):展示权限清单(net.http / mic / face),用户必须明确勾选「我已了解并同意」才能 install。

### L.6 设置页新增项

| 分组 | 项 | 控件 |
|---|---|---|
| 网络 | Wi-Fi 信息 | 信息行 |
| 网络 | 重新配网 | 跳配网页 |
| AI | 模型偏好 | 选择(自动/Qwen/Claude/OpenAI) |
| AI | 历史记录 | 跳查看,可清空 |
| AI | 个性化提示词 | 文本编辑(最多 200 字) |
| 技能 | 技能管理 | 跳 L.5 |
| 远端 | 允许 MCP 调用 | 开关(默认关) |
| 远端 | 已授权调用方 | 列表 |
| 系统 | 检查更新 | 跳 L.4 |
| 系统 | 自动升级 | 开关 |
| 隐私 | 对话数据云端保留 | 开关(默认关,关时仅本地) |
| 隐私 | 麦克风指示灯 | 开关(默认开,合规要求) |
| 关于 | 版本/SN/法律 | 信息行 |

### L.7 全局状态与微交互

| 状态 | 顶栏图标 | face 联动 |
|---|---|---|
| 离线 | ⚫ Wi-Fi 红 | 1s 一次 `look_left`/`look_right` 巡视 |
| 在线 idle | 🟢 Wi-Fi 主题色 | idle 呼吸眨眼 |
| AI 思考中 | 🔄 旋转 dot | thinking(眨眼频率↑) |
| AI 说话中 | 🟧 主题色脉冲 | talking(嘴部循环动画) |
| 远端 MCP 接入 | 🔌 主题色 | 接入瞬间 `look_right` 0.5s |
| 升级中 | ⬇ 进度色 | sleep |
| 低电 | 🔋 红 | sad(每 30s 闪一次) |

### L.8 可达性与防误触

- 所有目标点击区 ≥ 44×44px;
- 文本最小 14px;高对比模式(主题切「高对比」)所有色差 ≥ 4.5:1;
- 配网 / OTA / 安装技能页**禁用径向菜单手势**,只能通过明确按钮退出;
- 任何不可逆操作(清空历史、卸载技能、回滚固件)二次确认。

### L.9 与 v6.1 原型差异点

| v6.1 原型 | v5.6 落地 | 说明 |
|---|---|---|
| Sector 1 占位 | 对话页完整功能 | L.2 |
| Sector 4 占位 | 技能商店 | L.5 |
| 无配网页 | AP + 云配对双版本 | L.3 |
| 无 OTA 页 | 5 阶段流程 | L.4 |
| 设置仅主题切换 | 11 项,分 5 组 | L.6 |
| 状态仅 Wi-Fi/电池 | 7 类联动 | L.7 |

### L.10 迭代节奏建议

| 迭代 | 内容 | 工期 |
|---|---|---|
| Sprint 1(2w) | L.3 配网 + L.4 OTA + L.7 状态联动 | 解锁联网与可升级 |
| Sprint 2(2w) | L.2 对话页 + 隐私设置 | 核心 AI 体验 |
| Sprint 3(2w) | L.5/L.6 技能与设置 | 生态雏形 |
| Sprint 4(1w) | L.8 可达性走查 + 微交互打磨 | 上线前打磨 |

---

## 附录 M:Agent 编码规则补丁(v5.5 → v5.6)

在原 §I.9(规则 24~28)之后追加:

29. **方案 B 启动序列**:`main.c` 必须按 J.3 的 5 步顺序(NVS → board_manager → lvgl_port → wifi/claw_core/event_router → ui_bridge),**不得调换**。
30. **claw_core 上下文注入**:必须实现 `ui_bridge_context_provider`(J.5),把 device/battery/theme/location/time 5 字段以 JSON 返回。
31. **对话页流式回调**:`on_token` 必须用 `ui_dialog_append_or_extend_bot` 增量追加,**禁止**整段重排或频繁 `lv_label_set_text`(性能)。
32. **OTA 安装阶段**:必须屏蔽所有用户交互入口,且**禁止**调用 `face_engine_set` 之外的 face API(避免动画抢资源)。
33. **隐私默认**:对话云端保留默认**关**;首次开启必须弹合规说明 modal 并记录用户同意时间(写 settings + 上报)。
34. **MCP 接入**:`cap_mcp_server` 默认**不启用**;启用后必须在屏顶持续显示 🔌 图标(L.7);ACL 默认拒绝,白名单方式放行。

---

**Part 2 文档结束 / v5.6.1**
**配套**:Part 1 视觉与交互层(无变更);`icons_pack/`(附录 F);IDF 5.5.4 适配(附录 G);Agent 投喂 Prompt(附录 H);ESP-Claw 集成评估(附录 I);**方案 B 实施详解(附录 J)**;**后台架构(附录 K)**;**UX 迭代规划(附录 L)**;**Agent 规则补丁 29-34(附录 M)**;**全量复刻参考实现(附录 N)**

---

## 附录 N:全量复刻参考实现 `full_replica_v6.2/`(v5.6.1 新增)

> 触发原因:多份 Agent 实现的真机版本与设计稿严重偏离(图标缺失、卡片纯亮蓝填充、3 列网格文字截断 `…tap=w`)。本附录给出**经过对齐的、可整树投喂下游 Agent 的参考实现包**,锁定命名、配色、卡片样式、事件 ID、AVG 时间线、图标占位符。**任何偏离本附录的实现视为不合规**,QA 走查时与 §13 验收清单一并校验。

### N.1 包目录结构(对齐 §一,扁平化重组)

> 本附录使用 `xb_*` 前缀(xiaobao,内部代号),与 §一 中 `ui_core/` 命名等价;迁移到 §一 项目结构时,把 `src/core/` → `components/ui_core/`,`src/widgets/` → `components/ui_core/widgets/`,`src/pages/` → `components/ui_core/pages/`。命名映射详见 §N.7。

```
full_replica_v6.2/
├── CMakeLists.txt                    # ESP-IDF 组件描述(REQUIRES lvgl + esp_lvgl_port)
├── README.md
├── build_assets.py                   # 一键再生:81 SVG / 10 主题 / 10 AVG / 110 PNG mock
├── assets/
│   ├── icons/                        # 81 lucide 风格 SVG 源(单色,#000,可 image_recolor)
│   ├── themes/theme_tokens.h         # 10 个 theme_t 静态结构体
│   ├── anims/                        # MI-01 ~ MI-10 时间线 JSON(L.4 微交互)
│   └── fonts/                        # 字体子集 manifest(对齐 §五 lv_chinese_font)
├── mock_renders/                     # 110 张高保真 PNG(10 主题 × 11 页)
├── src/
│   ├── main.c                        # xb_app_start():event_init → theme_set → router_init
│   ├── core/
│   │   ├── xb_event.{h,c}            # 等价 components/ui_core/event_bus.c
│   │   ├── xb_theme.{h,c}            # 主题运行时 + EVT_THEME_CHANGED 广播
│   │   └── xb_face.{h,c}             # face_engine 桥(26 状态 enum)
│   ├── widgets/
│   │   └── xb_widgets.{h,c}          # xb_card / xb_button / xb_statusbar /
│   │                                 # xb_topbar / xb_dot_loading / xb_toast / xb_error_inline
│   ├── pages/
│   │   ├── xb_pages.h                # 页面入口 + 路由声明
│   │   ├── xb_router.c               # 栈式 navigator(深度 6)
│   │   ├── page_boot.c               # 3500ms 开机时间线
│   │   ├── page_home.c               # 首页(face 背景 + 状态条)
│   │   ├── page_menu.c               # 6 扇区径向菜单(对齐附录 C)
│   │   ├── page_chat.c               # P0 对话页(对齐 L.2 / B.3.1)
│   │   ├── page_wifi_ap.c            # AP+Captive(L.3 / B.3.2.1)
│   │   ├── page_wifi_pair.c          # 6 位云码(L.3 / B.3.2.2)
│   │   ├── page_ota.c                # 5 阶段 OTA(L.4 / B.3.3)
│   │   ├── page_skills.c             # 已安装 / 商店 双 tab(L.5 / B.3.4.1)
│   │   ├── page_skill_detail.c      # 权限确权页(B.3.4.2)
│   │   ├── page_settings.c           # 11 项 5 分组(附录 D / B.3.5)
│   │   └── page_console.c            # 开发者控制台 3 tab(2 列栅格,修复 3 列截断)
│   └── assets/
│       └── xb_icons_stub.c           # 38 个 lv_image_dsc_t 占位(16×16 ARGB8888 全透明)
└── docs/
    ├── INTEGRATION.md                # 嵌入既有 IDF 工程步骤
    └── COVERAGE.md                   # 与本附录 + B 全文条目逐条对照表
```

### N.2 卡片样式锁定(修复"纯亮蓝填充"事故)

任何卡片(包括 chat 气泡、设置 row、技能 card、控制台 row)**必须**走 `xb_card()`,样式硬编码为:

```c
// src/widgets/xb_widgets.c
lv_obj_set_style_radius(o, 4, 0);
lv_obj_set_style_bg_color(o, th->panel, 0);
lv_obj_set_style_bg_opa (o, LV_OPA_30, 0);   // ★ 30% 透明度,不是纯色
lv_obj_set_style_border_color(o, th->border, 0);
lv_obj_set_style_border_width(o, 1, 0);
lv_obj_set_style_pad_all(o, 6, 0);
```

**禁止**直接 `lv_obj_set_style_bg_color(card, accent, 0)`(那会得到上次真机那种"蓝糖纸"效果);只有"主动按钮"(`xb_button`)和"用户自己说的气泡"(`page_chat.c::add_bubble(is_user=true)`)允许 `bg_opa = COVER`。

### N.3 状态栏 4 槽锁定(对齐 L.7 / B.3.6)

```
slots from right to left:  [battery] [wifi] [ai_dot] [plug=mcp]
```

```c
// src/widgets/xb_widgets.c::xb_statusbar_create
const lv_image_dsc_t* slots[] = { &ic_plug, &ic_ai_dot, &ic_wifi, &ic_battery };
// flex_align END -> 实际从右到左排列
```

`ic_ai_dot` 默认 hide;思考态 = 旋转;说话态 = 主题色脉冲(MI-08)。`ic_plug` 仅在 `cap_mcp_server` enabled 后挂出。**任何新加状态必须先升 §L.7 + 本节**,不允许私自加槽。

### N.4 事件 ID 锁定(`xb_event.h`)

| ID | Payload 类型 | 来源 | 订阅者 |
|---|---|---|---|
| `XB_EVT_THEME_CHANGED` | `theme_t*` | `xb_theme_set()` | 所有 page(re-read `xb_theme_get()`) |
| `XB_EVT_WIFI_STATE` | `int 0/1/2` | `claw_core` net mgr | statusbar / page_wifi_ap |
| `XB_EVT_BATTERY` | `int %` | bsp_pmu(AXP2101) | statusbar |
| `XB_EVT_AI_STATE` | `enum {idle,thinking,talking}` | `ui_bridge::on_token` | statusbar / face |
| `XB_EVT_MCP_ACTIVE` | `bool` | `cap_mcp_server` | statusbar |
| `XB_EVT_OTA_PROGRESS` | `int 0..100` | `claw_core::ota_task` | page_ota |
| `XB_EVT_FACE_REQUEST` | `face_state_t` | 任意业务 | xb_face |
| `XB_EVT_CHAT_DELTA` | `const char*` | `claw_core::on_token` | page_chat |
| `XB_EVT_CHAT_DONE`  | `NULL` | 同上 | page_chat |
| `XB_EVT_CHAT_ERROR` | `const char*` | 同上 | page_chat |

下游接 ESP-Claw 时:`xb_event_post()` 由 `ui_bridge` 转发自 `claw_event_router`;事件名前缀 `XB_EVT_*` 与 J.7 中的 `claw_evt_*` **一一对应**。

### N.5 路由约定

`src/pages/xb_router.c` 是**临时栈式 navigator**(深度 6),仅供本复刻包独立运行用;接入真机时**替换**为 `claw_navigator`(规则 28),保持 `xb_page_id_t` 枚举与 navigator route 表对应。

### N.6 图标占位符策略(`src/assets/xb_icons_stub.c`)

为让本包**离线即可编译**,38 个 `lv_image_dsc_t` 全部生成为 16×16 全透明 ARGB8888 stub。投产前必须执行:

```bash
# 1. SVG → PNG(任意分辨率,推荐 24×24 或 32×32)
inkscape assets/icons/*.svg --export-type=png -o build/png/

# 2. PNG → C array
lv_img_conv -f true_color_alpha -cf rgb565a8 \
    --output components/ui_core/assets/ic_*.c build/png/*.png

# 3. 从 CMakeLists.txt 中删掉 src/assets/xb_icons_stub.c
```

**禁止**带 stub 出厂(规则补丁 35,见下文)。

### N.7 命名映射(本附录 ↔ §一 项目结构 ↔ ESP-Claw 公约)

| 复刻包 | §一 推荐位置 | ESP-Claw 对应 |
|---|---|---|
| `src/core/xb_event.{h,c}` | `components/ui_core/event_bus.c` | `claw_event_router`(部分) |
| `src/core/xb_theme.{h,c}` | `components/ui_core/theme.c` | — (设备本地) |
| `src/core/xb_face.{h,c}`  | `components/ui_core/face_render.c` | face_engine(规则 24) |
| `src/widgets/xb_widgets.{h,c}` | `components/ui_core/widgets/` | — |
| `src/pages/page_*.c` | `components/ui_core/pages/` | claw_navigator routes |
| `src/pages/xb_router.c` | 替换为 `claw_navigator` 调用 | claw_navigator |
| `src/main.c::xb_app_start` | `main/main.c::app_main` 内调用 | 走 J.3 启动序列 |

### N.8 主题数量从 7 → 10 的扩展说明

B.5 规定 7 主题色,本复刻包再加 3 个**高对比度衍生**(mono-hc / blue-hc / green-hc),用途仅限"设置 → 显示 → 高对比模式"开启时。**主题色仍是 7 个**,符合 PRD;高对比是布尔开关下的**配色变体**(token 级,不是新主题)。

### N.9 控制台栅格修复(2 列定档)

事故现场是 3 列 × 文字截断("…tap=w")。本附录强制:

```c
// src/pages/page_console.c::make_script
lv_obj_set_size(card, 148, 56);  // 320 - 6*2(pad) - 6(gap) = 308 / 2 ≈ 148
lv_obj_set_style_pad_gap(grid, 6, 0);
```

**任何**密度更高的栅格必须先做 5 字符以上 label 长字符串走查(用「rebuild_imu_calibration」做基线),通过才允许提密。

### N.10 AVG 动效目录(`assets/anims/MI-01..MI-10.json`)

对齐 L.4 / B.4 表:

| ID | 文件 | 目标 widget |
|---|---|---|
| MI-01 | `mi01_dot_loading.json` | `xb_dot_loading_create` |
| MI-02 | `mi02_bubble_pop.json` | `page_chat.c::add_bubble` |
| MI-03 | `mi03_toast.json` | `xb_toast` |
| MI-04 | `mi04_button_press.json` | `xb_button` 自带 |
| MI-05 | `mi05_sector_hover.json` | `page_menu.c` 6 sector card |
| MI-06 | `mi06_progress_strip.json` | `page_ota.c::lv_bar` indicator |
| MI-07 | `mi07_qr_shimmer.json` | `page_wifi_ap.c::AP_IDLE` 二维码 |
| MI-08 | `mi08_face_blink.json` | `xb_face` apply() |
| MI-09 | `mi09_mcp_pulse.json` | statusbar `ic_plug` |
| MI-10 | `mi10_error_shake.json` | `xb_error_inline` |

JSON 字段:`{ "id", "duration", "easing", "keyframes": [{t, prop, value}] }`,字段名与 LVGL `lv_anim_t` setter 保持 1:1 映射,可被 `tools/avg_to_lv_anim.py`(待补)直接转出 C 代码。

### N.11 规则补丁 35-38(在 §M 规则 34 之后追加)

35. **不得带占位图标投产**:`xb_icons_stub.c` 不得进 release 构建;CMakeLists.txt 必须用 `if(NOT CONFIG_RELEASE)` 包裹;CI 检查 `git grep XB_ICON_BLANK_16` 必须为空。
36. **卡片样式只能用 `xb_card()`**:任何 `lv_obj_set_style_bg_color(...accent...)` 直接对内容卡用 = code review block。
37. **状态栏槽位扩展**:新加状态必须先改 §N.3 + L.7,且 statusbar 总宽不超过 4 个 16×16 槽。超过须改为可滚动条或合并图标。
38. **AVG 时间线必须落 JSON**:任何新增微动效先在 `assets/anims/` 提交 JSON,再写 C 代码;PR 走查若发现"裸 lv_anim_t"未对应 JSON,打回。

---

