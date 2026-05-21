# 桌面机器人 UI 工程实现规格 v5.1 — Part 2 / ESP-IDF 5.2.1 + LVGL 9.5.0

> **本文档面向**:嵌入式工程师、AI 代码生成助手(Cursor / Claude Code / GPT)
> **目标**:基于 **ESP-IDF 5.2.1 + LVGL 9.5.0**,在 M5Stack CoreS3 上完整实现 v5.0 视觉与交互设计,**含中文显示开发指南**
> **配套设计稿**:Part 1 / 视觉与交互层

---

## 文档信息

| 项目 | 内容 |
|------|------|
| **文档版本** | v5.1 / Part 2 |
| **更新日期** | 2026-05-20 |
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

**Part 2 文档结束 / v5.1**
**配套**:Part 1 视觉与交互层(无变更)
