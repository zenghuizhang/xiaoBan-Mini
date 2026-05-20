/* v5.0 开机动画: Tech(cyan scanline) / Child(coral bounce) / Dev(terminal) */
#pragma once
#include <lvgl.h>
#include "theme_v3.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 播放开机动画, 3.5秒后自动回调 on_done */
void boot_anim_play(lv_obj_t *parent, void (*on_done)(void));

#ifdef __cplusplus
}
#endif
