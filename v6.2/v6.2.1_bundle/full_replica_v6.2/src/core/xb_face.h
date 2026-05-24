// xb_face.h — face_engine bridge
#pragma once
#include "lvgl.h"

typedef enum {
    FACE_IDLE, FACE_BREATH, FACE_HAPPY, FACE_HAPPY_BLINK,
    FACE_TALKING, FACE_THINKING,
    FACE_SAD, FACE_ANGRY, FACE_DIZZY, FACE_CRYING,
    FACE_NAUGHTY, FACE_WINK, FACE_CURIOUS, FACE_SURPRISED,
    FACE_LOOK_LEFT, FACE_LOOK_RIGHT, FACE_LOOK_AROUND,
    FACE_YAWN, FACE_LOST, FACE_CELEBRATE, FACE_EXCITED,
    FACE_SLEEP_WAKE, FACE_DEEP_SLEEP, FACE_LIGHT_REST, FACE_ALERT,
    FACE_MENU,            // dimmed eyes when overlay shown
    FACE_MAX
} face_state_t;

void         xb_face_create(lv_obj_t* parent);
void         xb_face_set(face_state_t s);
face_state_t xb_face_get(void);
