/* v6.2 NVS Memory Store */
#pragma once
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void memory_init(void);
void memory_save_theme(int theme);
int  memory_load_theme(void);
void memory_save_last_date(void);
bool memory_should_morning_greet(void);
void memory_record_interaction(void);
int  memory_get_interaction_count(void);

#ifdef __cplusplus
}
#endif
