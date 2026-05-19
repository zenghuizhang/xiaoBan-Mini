#pragma once

#include <lvgl.h>
#include <esp_wifi.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * WiFi 连接状态
 */
typedef enum {
    WIFI_DISCONNECTED,       // 未连接
    WIFI_CONNECTING,         // 连接中
    WIFI_CONNECTED,          // 已连接
    WIFI_CONFIG_MODE,        // 配置模式 (SmartConfig)
    WIFI_CONFIG_AP_MODE,     // AP配网模式 (Web配网)
    WIFI_CONFIG_AP_CONNECTING // AP模式下正在连接WiFi
} WifiState;

/**
 * WiFi 配网 UI 步骤 (V3.9 - 对齐原型多步骤流程)
 */
typedef enum {
    WIFI_UI_STEP_AP,         // AP模式: 显示热点信息 + 二维码占位
    WIFI_UI_STEP_CONNECTING, // 连接中: 旋转动画 + 文字
    WIFI_UI_STEP_SUCCESS,    // 成功: checkmark + 完成按钮
    WIFI_UI_STEP_ERROR       // 失败: X图标 + 重试/返回
} WifiConfigUIStep;

/**
 * 初始化 WiFi
 */
void wifi_init(void);

/**
 * 启动 SmartConfig 配网模式
 * 注意: ESP-IDF 5.1 需要安装组件: idf.py add-dependency espressif/esp_smartconfig
 */
// void wifi_start_smartconfig(void);

/**
 * 停止 SmartConfig 配网模式
 */
// void wifi_stop_smartconfig(void);

/**
 * 启动 AP 配网模式 (默认配网方式)
 */
void wifi_start_ap_config(void);

/**
 * 停止 AP 配网模式
 */
void wifi_stop_ap_config(void);

/**
 * 获取当前 WiFi 状态
 */
WifiState wifi_get_state(void);

/**
 * 获取已连接的 WiFi SSID
 */
const char* wifi_get_ssid(void);

/**
 * 获取 WiFi 信号强度百分比
 */
int wifi_get_rssi_percent(void);

/**
 * 显示 WiFi 配置界面
 */
void wifi_show_config_ui(void);

/**
 * 隐藏 WiFi 配置界面
 */
void wifi_hide_config_ui(void);

/**
 * WiFi 状态图标 (用于状态栏)
 */
const char* wifi_get_status_icon(void);

#ifdef __cplusplus
}
#endif
