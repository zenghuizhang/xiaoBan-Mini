// xb_imu_sim.h — 6 somatosensory IMU events.
// On real hardware xb_imu (driving the MPU6886 on CoreS3) fires these.
// In the simulator the Console "体感测试" tab fires them via xb_imu_sim_fire().
#pragma once

typedef enum {
    IMU_TILT_FWD = 0, IMU_TILT_BACK,
    IMU_TILT_LEFT,    IMU_TILT_RIGHT,
    IMU_SHAKE,        IMU_ROTATE,
} imu_event_t;

void xb_imu_sim_fire(imu_event_t e);   // posts XB_EVT_IMU
const char* xb_imu_name_zh(imu_event_t e);
