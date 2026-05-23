# 桌面机器人 UI 工程实现规格 v5.1 — Part 2 / ESP-IDF 5.2.1 + LVGL 9.5.0

> **本文档面向**:嵌入式工程师、AI 代码生成助手(Cursor / Claude Code / GPT)
> **目标**:基于 **ESP-IDF 5.2.1 + LVGL 9.5.0**,在 M5Stack CoreS3 上完整实现 v5.0 视觉与交互设计,**含中文显示开发指南**
> **配套设计稿**:Part 1 / 视觉与交互层

---

## 文档信息

| 项目 | 内容 |
|------|------|
| **文档版本** | v5.2 / Part 2(对齐 v6.1 原型:新增系统设置页、表情拆分、功能缺口分析) |
| **更新日期** | 2026-05-22 |
| **目标硬件** | M5Stack CoreS3 (ESP32-S3-WROOM-1-N16R8,16MB Flash + 8MB PSRAM) |
| **工具链** | ESP-IDF **5.2.1** / LVGL **9.5.0** / esp_lvgl_port **2.4+** |
| **C 标准** | C11 + C++17(组件层 C,业务层可选 C++) |
| **字体方案** | lv_font_conv + 思源黑体 SourceHanSansCN(子集化) |

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

**ESP-IDF 5.2.1 注意点**:
- `esp_lcd` 组件 API 稳定;但 `esp_lcd_panel_io_create_*` 接口建议使用 `esp_lcd_new_panel_io_spi()` 形式
- I2C 推荐使用新版 `i2c_master_*` API(`driver/i2c_master.h`),但兼容旧 `driver/i2c.h`
- 必须启用 SPIRAM 才能驱动 320×240 全帧缓存

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
  idf: ">=5.2.1,<5.3.0"
  lvgl/lvgl: "~9.5.0"
  espressif/esp_lvgl_port: "~2.4.0"
  espressif/esp_lcd_ili9341: "~2.0.0"   # ILI9342C 兼容
  espressif/button: "~3.2.0"
  espressif/led_strip: "~2.5.0"          # SK6812
```

执行 `idf.py reconfigure` 自动拉取。

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

`components/bsp_cores3/bsp_display.c`:

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

# PSRAM 必需
CONFIG_SPIRAM=y
CONFIG_SPIRAM_MODE_OCT=y
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
| 目标平台              | ESP-IDF 5.2.1 + LVGL 9.5.0,M5Stack CoreS3(ESP32-S3-WROOM-1-N16R8)    |
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

**Part 2 文档结束 / v5.3**
**配套**:Part 1 视觉与交互层(无变更);`icons_pack/` 资源包(本附录 F)
