// xb_face.c — minimal LVGL placeholder of the face_engine.
// On the real device this delegates to claw_emote replacement (see Appendix J of v5.6 spec).
#include "xb_face.h"
#include "xb_theme.h"

static lv_obj_t *g_left_eye, *g_right_eye, *g_mouth;
static face_state_t g_state = FACE_IDLE;

static void apply(face_state_t s) {
    const theme_t* th = xb_theme_get();
    lv_color_t c = th->accent;
    lv_obj_set_style_bg_color(g_left_eye,  c, 0);
    lv_obj_set_style_bg_color(g_right_eye, c, 0);
    lv_obj_set_style_bg_color(g_mouth,     c, 0);
    int eye_h, eye_w, mouth_w, mouth_h;
    switch (s) {
        case FACE_HAPPY: case FACE_HAPPY_BLINK: case FACE_CELEBRATE:
            eye_h = 12; eye_w = 38; mouth_w = 50; mouth_h = 24; break;
        case FACE_TALKING:
            eye_h = 38; eye_w = 32; mouth_w = 22; mouth_h = 18; break;
        case FACE_THINKING:
            eye_h = 30; eye_w = 28; mouth_w = 14; mouth_h = 6; break;
        case FACE_SAD: case FACE_LOST:
            eye_h = 22; eye_w = 32; mouth_w = 18; mouth_h = 8; break;
        case FACE_ANGRY:
            eye_h = 16; eye_w = 32; mouth_w = 22; mouth_h = 8; break;
        case FACE_MENU:
            eye_h = 20; eye_w = 20; mouth_w = 10; mouth_h = 4; break;
        case FACE_DEEP_SLEEP: case FACE_LIGHT_REST:
            eye_h = 8; eye_w = 32; mouth_w = 16; mouth_h = 4; break;
        default:
            eye_h = 38; eye_w = 32; mouth_w = 22; mouth_h = 6;
    }
    lv_obj_set_size(g_left_eye,  eye_w, eye_h);
    lv_obj_set_size(g_right_eye, eye_w, eye_h);
    lv_obj_set_size(g_mouth,     mouth_w, mouth_h);
}

void xb_face_create(lv_obj_t* parent) {
    g_left_eye  = lv_obj_create(parent);
    g_right_eye = lv_obj_create(parent);
    g_mouth     = lv_obj_create(parent);
    lv_obj_t* parts[3] = { g_left_eye, g_right_eye, g_mouth };
    for (int i = 0; i < 3; ++i) {
        lv_obj_t* o = parts[i];
        lv_obj_set_style_radius(o, 16, 0);
        lv_obj_set_style_border_width(o, 0, 0);
        lv_obj_remove_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    }
    lv_obj_align(g_left_eye,  LV_ALIGN_CENTER, -32, -10);
    lv_obj_align(g_right_eye, LV_ALIGN_CENTER,  32, -10);
    lv_obj_align(g_mouth,     LV_ALIGN_CENTER,   0,  30);
    apply(FACE_IDLE);
}

void xb_face_set(face_state_t s) { g_state = s; if (g_left_eye) apply(s); }
face_state_t xb_face_get(void) { return g_state; }
