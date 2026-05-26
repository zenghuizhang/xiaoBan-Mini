// xb_face.h — face engine bridge.
// v7.6: 24 variants (see assets/face/), color = theme->accent_hi (NOT accent).
// Geometric only — 2 eye rects + 1 mouth rect/path, scale 1.6.
#pragma once
#include "lvgl.h"

typedef enum {
    FACE_IDLE = 0,    FACE_BREATH,
    FACE_HAPPY,       FACE_TALKING,
    FACE_LOOK_LEFT,   FACE_LOOK_RIGHT,
    FACE_CURIOUS,     FACE_THINKING,
    FACE_ALERT,       FACE_LOST,
    FACE_ANGRY,       FACE_DIZZY,
    FACE_WINK,        FACE_CELEBRATE,
    FACE_EXCITED,     FACE_CRYING,
    FACE_NAUGHTY,     FACE_SLEEP_WAKE,
    FACE_YAWN,        FACE_SURPRISED,
    FACE_DEEP_SLEEP,
    FACE_MENU,        FACE_BOOT_MID,    // boot intermediate (half-open)
    FACE_BOOT_OPEN,
    FACE_VARIANT_MAX
} face_variant_t;

// Lifecycle
lv_obj_t* xb_face_create(lv_obj_t* parent);     // create container; theme-aware
void      xb_face_destroy(void);                // tear down
void      xb_face_set(face_variant_t v);        // switch variant (with 150ms tween)
face_variant_t xb_face_get(void);
void      xb_face_redraw(void);                 // re-paint after theme change
