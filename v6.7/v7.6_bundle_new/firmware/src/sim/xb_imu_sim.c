// xb_imu_sim.c
#include "xb_imu_sim.h"
#include "../core/xb_event.h"

void xb_imu_sim_fire(imu_event_t e) {
    xb_event_post(XB_EVT_IMU, (void*)(intptr_t)e);
}

const char* xb_imu_name_zh(imu_event_t e) {
    switch (e) {
        case IMU_TILT_FWD:   return "前倾";
        case IMU_TILT_BACK:  return "后仰";
        case IMU_TILT_LEFT:  return "左倾";
        case IMU_TILT_RIGHT: return "右倾";
        case IMU_SHAKE:      return "摇晃";
        case IMU_ROTATE:     return "旋转";
    }
    return "?";
}
