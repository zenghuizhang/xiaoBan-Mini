// xb_scenario_sim.c
#include "xb_scenario_sim.h"
#include "../core/xb_event.h"
#include "../core/xb_face.h"

const scene_def_t XB_SCENES[6] = {
    { SCN_VOICE_WAKE, "语音唤醒", FACE_ALERT,     XB_RGB_IDLE,     "嗯，我在。" },
    { SCN_OTA,        "OTA 升级", FACE_THINKING,  XB_RGB_OTA,      "正在下载更新…" },
    { SCN_ERROR,      "系统报错", FACE_LOST,      XB_RGB_ERROR,    "出错了，请稍后再试。" },
    { SCN_CALL,       "来电提醒", FACE_HAPPY,     XB_RGB_CALL,     "你有一个来电。" },
    { SCN_LOW_BAT,    "低电预警", FACE_CRYING,    XB_RGB_LOW_BAT,  "电池电量不足，请充电。" },
    { SCN_OVERHEAT,   "高温保护", FACE_DIZZY,     XB_RGB_OVERHEAT, "温度过高，正在降频。" },
};

void xb_scenario_fire(scene_t s) {
    if ((int)s < 0 || (int)s >= 6) return;
    const scene_def_t* d = &XB_SCENES[s];
    xb_face_set(d->face);
    xb_event_post(XB_EVT_RGB_SCENE,     (void*)(intptr_t)d->rgb);
    xb_event_post(XB_EVT_SCENE_REQUEST, (void*)(intptr_t)d->id);
}
