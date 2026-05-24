#pragma once
#include "lvgl.h"
#ifdef __cplusplus
extern "C" {
#endif

lv_obj_t* page_chat_create(lv_obj_t* parent);
void page_chat_add_message(const char* text, bool is_user);
void page_chat_set_thinking(bool thinking);

#ifdef __cplusplus
}
#endif
