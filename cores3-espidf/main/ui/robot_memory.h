/* v5.0 记忆系统 (NVS, 对齐 useMemory.ts) */
#pragma once
#include "theme_v3.h"

#ifdef __cplusplus
extern "C" {
#endif

void memory_init(void);

void memory_save_theme(ThemeV3 theme);
ThemeV3 memory_load_theme(void);
void memory_save_last_date(void);
bool memory_should_morning_greet(void);  // 6-10点 + 今天未问候 → true
void memory_record_interaction(void);
int  memory_get_interaction_count(void);

#ifdef __cplusplus
}
#endif
