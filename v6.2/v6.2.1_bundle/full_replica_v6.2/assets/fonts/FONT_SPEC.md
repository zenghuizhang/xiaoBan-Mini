# Font Spec

LVGL fonts to bundle (UTF-8, GBK overlap allowed):

| Name | Use | Glyphs |
|------|-----|--------|
| `xb_font_pingfang_12` | body, settings labels | ASCII + 4500 GB2312 |
| `xb_font_pingfang_13` | top-bar title | same |
| `xb_font_mono_10` | percentages, code, version | ASCII only |
| `xb_font_mono_20` | pairing code | digits 0-9 |
| `xb_font_mono_48` | pairing code (large) | digits 0-9 |

Generate via lv_font_conv:

```
lv_font_conv --font NotoSansSC-Regular.otf --size 12 --bpp 4 \
  --range 0x20-0x7F,0x4E00-0x9FA5 --format lvgl -o xb_font_pingfang_12.c
```
