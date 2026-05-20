/* v5.0 MotionController (IMU, 对齐 MotionController.tsx) */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MOTION_NONE,
    MOTION_TILT_FORWARD,   // curious
    MOTION_TILT_BACKWARD,  // yawn
    MOTION_TILT_LEFT,      // look_around
    MOTION_TILT_RIGHT,     // look_around
    MOTION_SHAKE,          // dizzy
    MOTION_TAP,            // wink
} MotionAction;

void motion_init(void);
MotionAction motion_poll(void);  // 每帧调用, 返回检测到的动作

#ifdef __cplusplus
}
#endif
