// page_pomodoro.h — Pomodoro timer view page.
//
// Pure display view: the countdown is driven by pomodoro_tick() in the app main
// loop (keeps running even when this page is closed). This page only reads the
// engine accessors to render MM:SS / status / buttons.
#pragma once
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

lv_obj_t* page_pomodoro_create(lv_obj_t* parent);

#ifdef __cplusplus
}
#endif
