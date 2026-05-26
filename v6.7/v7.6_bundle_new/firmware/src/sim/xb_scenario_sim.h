// xb_scenario_sim.h — 6 scenario triggers (Console "情景测试" tab).
#pragma once
#include "../widgets/xb_widgets.h"   // rgb_scene_t
#include "../core/xb_face.h"         // face_variant_t

typedef enum {
    SCN_VOICE_WAKE = 0, SCN_OTA,    SCN_ERROR,
    SCN_CALL,           SCN_LOW_BAT, SCN_OVERHEAT,
} scene_t;

typedef struct {
    scene_t        id;
    const char*    name_zh;
    face_variant_t face;
    rgb_scene_t    rgb;
    const char*    bubble_zh;
} scene_def_t;

extern const scene_def_t XB_SCENES[6];

void xb_scenario_fire(scene_t s);    // posts XB_EVT_SCENE_REQUEST + drives face/RGB/bubble
