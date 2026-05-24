// AUTO-GENERATED theme_tokens.h — DO NOT EDIT BY HAND
// 7 main themes + high-contrast (mono).
#pragma once
#include "lvgl.h"

typedef struct {
    lv_color_t bg, panel, accent, accent_dim, text, text_dim, border, danger, success;
} theme_t;

static const theme_t THEME_TECH = {
    lv_color_make(  0,  0,  0),
    lv_color_make( 10, 30, 40),
    lv_color_make( 51,197,255),
    lv_color_make( 15, 90,120),
    lv_color_make(180,235,255),
    lv_color_make(110,170,200),
    lv_color_make( 20, 80,110),
    lv_color_make(244, 63, 94),
    lv_color_make( 34,197, 94),
};
static const theme_t THEME_CHILD = {
    lv_color_make(255,249,230),
    lv_color_make(255,255,255),
    lv_color_make(255,127, 80),
    lv_color_make(255,170,120),
    lv_color_make(200, 90, 40),
    lv_color_make(210,140, 90),
    lv_color_make(255,200,160),
    lv_color_make(244, 63, 94),
    lv_color_make( 34,197, 94),
};
static const theme_t THEME_DEV = {
    lv_color_make(  0,  0,  0),
    lv_color_make( 10, 30, 15),
    lv_color_make( 34,197, 94),
    lv_color_make( 15, 90, 40),
    lv_color_make(180,255,200),
    lv_color_make(110,200,140),
    lv_color_make( 20, 80, 30),
    lv_color_make(244, 63, 94),
    lv_color_make(120,200, 80),
};
static const theme_t THEME_ORANGE = {
    lv_color_make( 24, 16,  8),
    lv_color_make( 40, 28, 20),
    lv_color_make(255,140, 66),
    lv_color_make(120, 70, 30),
    lv_color_make(255,228,200),
    lv_color_make(200,160,120),
    lv_color_make( 80, 50, 30),
    lv_color_make(244, 63, 94),
    lv_color_make( 34,197, 94),
};
static const theme_t THEME_BLUE = {
    lv_color_make(  8, 14, 24),
    lv_color_make( 20, 30, 50),
    lv_color_make( 74,144,226),
    lv_color_make( 30, 70,140),
    lv_color_make(220,232,255),
    lv_color_make(140,170,210),
    lv_color_make( 40, 70,120),
    lv_color_make(244, 63, 94),
    lv_color_make( 34,197, 94),
};
static const theme_t THEME_PINK = {
    lv_color_make( 24, 14, 20),
    lv_color_make( 50, 28, 40),
    lv_color_make(255,107,157),
    lv_color_make(140, 60, 90),
    lv_color_make(255,220,235),
    lv_color_make(210,160,185),
    lv_color_make( 90, 50, 70),
    lv_color_make(244, 63, 94),
    lv_color_make( 34,197, 94),
};
static const theme_t THEME_GREEN = {
    lv_color_make(  8, 20, 12),
    lv_color_make( 20, 40, 24),
    lv_color_make( 91,184, 92),
    lv_color_make( 40, 90, 50),
    lv_color_make(220,245,222),
    lv_color_make(150,200,160),
    lv_color_make( 40, 80, 50),
    lv_color_make(244, 63, 94),
    lv_color_make( 91,184, 92),
};
static const theme_t THEME_PURPLE = {
    lv_color_make( 18, 10, 28),
    lv_color_make( 36, 22, 56),
    lv_color_make(155, 89,182),
    lv_color_make( 80, 45,100),
    lv_color_make(232,220,250),
    lv_color_make(180,160,210),
    lv_color_make( 60, 40, 90),
    lv_color_make(244, 63, 94),
    lv_color_make( 34,197, 94),
};
static const theme_t THEME_YELLOW = {
    lv_color_make( 30, 26,  8),
    lv_color_make( 50, 44, 20),
    lv_color_make(241,196, 15),
    lv_color_make(130,100, 10),
    lv_color_make(255,248,210),
    lv_color_make(210,190,120),
    lv_color_make(100, 80, 20),
    lv_color_make(244, 63, 94),
    lv_color_make( 34,197, 94),
};
static const theme_t THEME_MONO = {
    lv_color_make(  0,  0,  0),
    lv_color_make( 40, 40, 40),
    lv_color_make(255,255,255),
    lv_color_make(180,180,180),
    lv_color_make(255,255,255),
    lv_color_make(200,200,200),
    lv_color_make(120,120,120),
    lv_color_make(255, 90, 90),
    lv_color_make(120,255,120),
};

// usage: switch by theme name string from settings
static inline const theme_t* xb_theme_lookup(const char* name) {
    if(!name) return &THEME_TECH;
    if(strcmp(name,"tech")==0) return &THEME_TECH;
    if(strcmp(name,"child")==0) return &THEME_CHILD;
    if(strcmp(name,"dev")==0) return &THEME_DEV;
    if(strcmp(name,"orange")==0) return &THEME_ORANGE;
    if(strcmp(name,"blue")==0) return &THEME_BLUE;
    if(strcmp(name,"pink")==0) return &THEME_PINK;
    if(strcmp(name,"green")==0) return &THEME_GREEN;
    if(strcmp(name,"purple")==0) return &THEME_PURPLE;
    if(strcmp(name,"yellow")==0) return &THEME_YELLOW;
    if(strcmp(name,"mono")==0) return &THEME_MONO;
    return &THEME_TECH;
}
