// xb_theme.h — runtime theme switch
#pragma once
#include "lvgl.h"
#include "../../assets/themes/theme_tokens.h"

const theme_t* xb_theme_get(void);
void           xb_theme_set(const char* name); // broadcasts EVT_THEME_CHANGED
