#include "wifi_config.h"
#include "theme_v3.h"
#include "expressions.h"
#include "qrcode.h"
#include "screenshot.h"
// 设备端配网UI用emoji就够了，中文主要在Web配网页面
// #include "font_chinese.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <esp_event.h>
// ESP-IDF 5.1 SmartConfig 需要单独安装组件: idf.py add-dependency espressif/esp_smartconfig
// #include <esp_smartconfig.h>
#include <esp_http_server.h>
#include <string.h>
#include <nvs_flash.h>
#include <lwip/sockets.h>
#include <lwip/netdb.h>

static const char *TAG = "WIFI";

// ========== DNS 劫持服务器 (Captive Portal 的关键!) ==========
// 将所有 DNS 查询解析为 192.168.4.1 → 手机自动弹出配网页面
static TaskHandle_t s_dns_task = NULL;

static void _dns_server_task(void *pvParameters)
{
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) { ESP_LOGE(TAG, "DNS socket fail"); vTaskDelete(NULL); return; }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(53);
    bind(sock, (struct sockaddr *)&addr, sizeof(addr));

    ESP_LOGI(TAG, "DNS 劫持服务器启动 (所有域名 → 192.168.4.1)");

    while (1) {
        uint8_t rx_buf[512];
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int len = recvfrom(sock, rx_buf, sizeof(rx_buf), 0,
                          (struct sockaddr *)&client_addr, &client_len);
        if (len < 12) continue; // 太小，跳过

        // 构造 DNS 响应: 将查询 ID 和标志位原样复制，添加回答
        uint8_t tx_buf[512];
        memcpy(tx_buf, rx_buf, len);      // 复制查询包
        tx_buf[2] = 0x81;                 // QR=1(响应), RA=1(递归可用)
        tx_buf[3] = 0x80;                 // RCODE=0
        tx_buf[6] = rx_buf[6];            // QDCOUNT: 复制 byte 6
        tx_buf[7] = rx_buf[7];            // QDCOUNT: 复制 byte 7
        tx_buf[6] = 0; tx_buf[7] = 1;     // ANCOUNT = 1
        // 添加回答: NAME pointer(0xC00C), TYPE=A(0x0001), CLASS=IN(0x0001), TTL=300, RDLENGTH=4
        int ans_pos = len;
        tx_buf[ans_pos++] = 0xC0; tx_buf[ans_pos++] = 0x0C; // 指向查询中的域名
        tx_buf[ans_pos++] = 0x00; tx_buf[ans_pos++] = 0x01; // TYPE A
        tx_buf[ans_pos++] = 0x00; tx_buf[ans_pos++] = 0x01; // CLASS IN
        tx_buf[ans_pos++] = 0x00; tx_buf[ans_pos++] = 0x00;
        tx_buf[ans_pos++] = 0x01; tx_buf[ans_pos++] = 0x2C; // TTL 300
        tx_buf[ans_pos++] = 0x00; tx_buf[ans_pos++] = 0x04; // RDLENGTH 4
        tx_buf[ans_pos++] = 192; tx_buf[ans_pos++] = 168;   // 192.168
        tx_buf[ans_pos++] = 4;   tx_buf[ans_pos++] = 1;     // 4.1

        sendto(sock, tx_buf, ans_pos, 0, (struct sockaddr *)&client_addr, client_len);
    }
}

static void _dns_server_start(void)
{
    xTaskCreate(_dns_server_task, "dns_hijack", 4096, NULL, 2, &s_dns_task);
}

static void _dns_server_stop(void)
{
    if (s_dns_task) { vTaskDelete(s_dns_task); s_dns_task = NULL; }
}

// ========== 全局状态 ==========
static WifiState s_wifi_state = WIFI_DISCONNECTED;
static EventGroupHandle_t s_wifi_event_group;
static char s_connected_ssid[33] = {0};
static int s_rssi = -100;
static lv_obj_t *s_wifi_ui = NULL;
static lv_obj_t *s_status_label = NULL;
static TaskHandle_t s_ap_task_handle = NULL;
static httpd_handle_t s_http_server = NULL;
static esp_netif_t *s_ap_netif = NULL;
static char s_connecting_ssid[33] = {0};
static WifiConfigUIStep s_ui_step = WIFI_UI_STEP_AP;
// 线程安全: LVGL 操作必须延迟到主线程
static volatile bool s_pending_wifi_ui = false;
static volatile int  s_pending_wifi_step = 0;
static volatile bool s_pending_qr_close = false;
static volatile bool s_pending_scr_start = false;

// 事件组位定义
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_SMARTCONFIG_DONE_BIT BIT1
#define WIFI_AP_PROVISIONING_DONE_BIT BIT2

// AP配网配置
#define AP_SSID "ESP32-S3-Box-Config"
#define AP_PASSWORD ""  // 无密码
#define AP_TIMEOUT_MS (5 * 60 * 1000)  // 5分钟超时


// ========== Web配网页面HTML ==========
static const char PROVISIONING_HTML[] =
"<!DOCTYPE html><html><head>"
"<meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1'>"
"<title>WiFi Setup</title>"
"<style>"
"*{margin:0;padding:0;box-sizing:border-box}"
"body{font-family:-apple-system,sans-serif;background:#1a1a2e;color:#eee;min-height:100vh;display:flex;align-items:center;justify-content:center;padding:16px}"
".card{background:#16213e;border-radius:12px;padding:24px;width:100%;max-width:380px;box-shadow:0 8px 32px rgba(0,0,0,.4)}"
"h2{text-align:center;margin-bottom:4px;color:#e94560}"
".sub{text-align:center;color:#888;font-size:13px;margin-bottom:20px}"
"label{display:block;margin-bottom:4px;color:#aaa;font-size:13px}"
"input{width:100%;padding:12px;margin-bottom:14px;border:2px solid #333;border-radius:8px;font-size:16px;background:#0f3460;color:#fff}"
"input:focus{outline:none;border-color:#e94560}"
"button{width:100%;padding:14px;background:#e94560;color:#fff;border:none;border-radius:8px;font-size:16px;font-weight:600;cursor:pointer;margin-top:4px}"
"button:active{background:#c23152}"
"button:disabled{opacity:.6}"
".ok{text-align:center;color:#4caf50;padding:20px;display:none}"
".ok.show{display:block}"
".err{color:#f44336;padding:10px;background:#3e1a1a;border-radius:8px;text-align:center;margin-top:12px;display:none}"
".err.show{display:block}"
"</style></head><body>"
"<div class='card'>"
"<h2>WiFi Setup</h2>"
"<p class='sub'>Enter your WiFi credentials</p>"
"<label>WiFi Name (SSID)</label>"
"<input id='ssid' placeholder='Your WiFi name'>"
"<label>Password</label>"
"<input id='pwd' type='password' placeholder='WiFi password'>"
"<button id='btn' onclick='go()'>Connect</button>"
"<div id='err' class='err'></div>"
"<div id='ok' class='ok'><h3>Done!</h3><p>Connecting to WiFi...</p></div>"
"</div>"
"<script>"
"async function go(){"
"var s=document.getElementById('ssid').value.trim();"
"var p=document.getElementById('pwd').value;"
"if(!s){alert('Enter WiFi name');return}"
"var btn=document.getElementById('btn');btn.disabled=true;btn.textContent='Connecting...';"
"try{"
"var r=await fetch('/api/connect',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({ssid:s,password:p})});"
"var j=await r.json();"
"if(j.success){"
"document.getElementById('ok').classList.add('show');"
"}else{"
"var e=document.getElementById('err');e.textContent=j.message||'Failed';e.classList.add('show');"
"btn.disabled=false;btn.textContent='Connect';"
"}"
"}catch(ex){"
"var e=document.getElementById('err');e.textContent='Network error';e.classList.add('show');"
"btn.disabled=false;btn.textContent='Connect';"
"}"
"}"
"</script></body></html>";

// ========== HTTP 请求处理 ==========
static esp_err_t _http_root_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, PROVISIONING_HTML, strlen(PROVISIONING_HTML));
    return ESP_OK;
}

static esp_err_t _http_scan_handler(httpd_req_t *req)
{
    // AP模式下WiFi扫描会导致崩溃,返回空列表让用户手动输入
    const char *json = "[]";
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json, strlen(json));
    ESP_LOGI(TAG, "WiFi扫描: 返回空列表(手动输入模式)");
    return ESP_OK;
}

static esp_err_t _http_connect_handler(httpd_req_t *req)
{
    char buf[256] = {0};
    int ret, remaining = req->content_len;

    if (remaining >= sizeof(buf)) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    ret = httpd_req_recv(req, buf, remaining);
    if (ret <= 0) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    buf[ret] = '\0';

    // 简单解析JSON
    char ssid[33] = {0};
    char password[65] = {0};

    // 提取ssid
    char *ssid_start = strstr(buf, "\"ssid\":\"");
    if (ssid_start) {
        ssid_start += 8;
        char *ssid_end = strchr(ssid_start, '"');
        if (ssid_end) {
            int len = ssid_end - ssid_start;
            if (len > 0 && len < 33) {
                memcpy(ssid, ssid_start, len);
                ssid[len] = '\0';
            }
        }
    }

    // 提取password
    char *pwd_start = strstr(buf, "\"password\":\"");
    if (pwd_start) {
        pwd_start += 12;
        char *pwd_end = strchr(pwd_start, '"');
        if (pwd_end) {
            int len = pwd_end - pwd_start;
            if (len >= 0 && len < 65) {
                memcpy(password, pwd_start, len);
                password[len] = '\0';
            }
        }
    }

    ESP_LOGI(TAG, "收到配网请求: SSID=%s", ssid);

    if (ssid[0] == '\0') {
        const char *resp = "{\"success\":false,\"message\":\"SSID不能为空\"}";
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, resp, strlen(resp));
        return ESP_OK;
    }

    // 保存到临时变量用于连接
    strncpy(s_connecting_ssid, ssid, sizeof(s_connecting_ssid) - 1);
    s_connecting_ssid[sizeof(s_connecting_ssid) - 1] = '\0';

    // 先发 HTTP 响应 (AP 还在, 手机能收到)
    const char *resp = "{\"success\":true,\"message\":\"正在连接WiFi\"}";
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp, strlen(resp));

    // 延迟到主线程: 关闭 QR + 更新 WiFi 页面到连接中
    s_pending_qr_close = true;
    s_pending_wifi_step = 1;  // connecting
    s_pending_wifi_ui = true;
    s_wifi_state = WIFI_CONFIG_AP_CONNECTING;

    // 保存凭据用于连接
    strncpy(s_connecting_ssid, ssid, sizeof(s_connecting_ssid) - 1);
    s_connecting_ssid[sizeof(s_connecting_ssid) - 1] = '\0';

    // 通知配网任务
    xEventGroupSetBits(s_wifi_event_group, WIFI_AP_PROVISIONING_DONE_BIT);

    // 延迟切换: 等 HTTP 响应发完再关 AP
    vTaskDelay(pdMS_TO_TICKS(500));

    // 切换到 STA 模式连接目标 WiFi
    wifi_config_t wifi_config = {0};
    memcpy(wifi_config.sta.ssid, ssid, strlen(ssid));
    memcpy(wifi_config.sta.password, password, strlen(password));
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_connect();

    return ESP_OK;
}

// Captive Portal 重定向处理
// Captive Portal 检测: 返回 204 No Content (告诉手机"无需登录")
// 这样手机不会强制弹出配网页，用户可以自由访问 /screen.bmp
static esp_err_t _http_captive_no_content(httpd_req_t *req)
{
    httpd_resp_set_status(req, "204 No Content");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

// 配网入口: 302 到主页 (手动打开浏览器时触发)
static esp_err_t _http_captive_handler(httpd_req_t *req)
{
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "http://192.168.4.1/");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

// 404 → 配网主页
static esp_err_t _http_404_handler(httpd_req_t *req, httpd_err_code_t err)
{
    return _http_captive_handler(req);
}

static httpd_handle_t _start_webserver(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_open_sockets = 7;
    config.lru_purge_enable = true;

    httpd_handle_t server = NULL;
    if (httpd_start(&server, &config) == ESP_OK) {
        // 配网页面
        httpd_uri_t root_uri = {
            .uri = "/",
            .method = HTTP_GET,
            .handler = _http_root_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &root_uri);

        // WiFi扫描API
        httpd_uri_t scan_uri = {
            .uri = "/api/scan",
            .method = HTTP_GET,
            .handler = _http_scan_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &scan_uri);

        // 连接API
        httpd_uri_t connect_uri = {
            .uri = "/api/connect",
            .method = HTTP_POST,
            .handler = _http_connect_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &connect_uri);

        // Captive Portal 检测: 返回 204, 手机不弹窗, 用户可手动打开配网页或 /screen.bmp
        httpd_uri_t captive_uris[] = {
            {.uri = "/generate_204", .method = HTTP_GET, .handler = _http_captive_no_content, .user_ctx = NULL},
            {.uri = "/gen_204", .method = HTTP_GET, .handler = _http_captive_no_content, .user_ctx = NULL},
            {.uri = "/hotspot-detect.html", .method = HTTP_GET, .handler = _http_captive_no_content, .user_ctx = NULL},
            {.uri = "/ncsi.txt", .method = HTTP_GET, .handler = _http_captive_no_content, .user_ctx = NULL},
            {.uri = "/redirect", .method = HTTP_GET, .handler = _http_captive_no_content, .user_ctx = NULL},
        };

        for (size_t i = 0; i < sizeof(captive_uris)/sizeof(captive_uris[0]); i++) {
            httpd_register_uri_handler(server, &captive_uris[i]);
        }

        // Screenshot endpoint
        screenshot_register(server);

        // 404 → 302 redirect (catch-all for captive portal)
        httpd_register_err_handler(server, HTTPD_404_NOT_FOUND, _http_404_handler);
        ESP_LOGI(TAG, "✓ Web服务器 + DNS劫持 + CaptivePortal + Screenshot 已就绪");
        return server;
    }

    ESP_LOGE(TAG, "✗ Web服务器启动失败");
    return NULL;
}

static void _stop_webserver(httpd_handle_t server)
{
    if (server) {
        httpd_stop(server);
        ESP_LOGI(TAG, "✓ Web服务器已停止");
    }
}

// ========== AP配网任务 ==========
static void _ap_config_task(void *pvParameters)
{
    ESP_LOGI(TAG, "🚀 启动 AP 配网模式");

    // 先停掉当前 WiFi，再切到 APSTA 模式
    esp_wifi_stop();
    vTaskDelay(pdMS_TO_TICKS(200));

    // 创建AP接口
    s_ap_netif = esp_netif_create_default_wifi_ap();

    // 配置AP
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = AP_SSID,
            .ssid_len = strlen(AP_SSID),
            .channel = 1,
            .authmode = WIFI_AUTH_OPEN,
            .max_connection = 5,
            .beacon_interval = 100,
        },
    };

    if (strlen(AP_PASSWORD) > 0) {
        wifi_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
        memcpy(wifi_config.ap.password, AP_PASSWORD, strlen(AP_PASSWORD));
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    // 启动 DNS 劫持 + Web服务器
    _dns_server_start();
    s_http_server = _start_webserver();

    ESP_LOGI(TAG, "📡 热点已启动: %s", AP_SSID);
    ESP_LOGI(TAG, "🌐 DNS劫持+配网页面: http://192.168.4.1");

    // 等待配网完成或超时
    EventBits_t bits = xEventGroupWaitBits(
        s_wifi_event_group,
        WIFI_AP_PROVISIONING_DONE_BIT | WIFI_CONNECTED_BIT,
        pdFALSE, pdFALSE,
        pdMS_TO_TICKS(AP_TIMEOUT_MS)
    );

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "✅ WiFi 连接成功，配网完成");
    } else if (bits & WIFI_AP_PROVISIONING_DONE_BIT) {
        // 等待实际连接完成
        xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdFALSE, pdMS_TO_TICKS(15000));
    } else {
        ESP_LOGI(TAG, "⏰ 配网超时，自动退出");
    }

    // 清理资源
    _dns_server_stop();
    _stop_webserver(s_http_server);
    s_http_server = NULL;

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_netif_destroy(s_ap_netif);
    s_ap_netif = NULL;

    s_wifi_state = (wifi_get_state() == WIFI_CONNECTED) ? WIFI_CONNECTED : WIFI_DISCONNECTED;

    // 3秒后关闭UI
    vTaskDelay(pdMS_TO_TICKS(3000));
    wifi_hide_config_ui();

    s_ap_task_handle = NULL;
    vTaskDelete(NULL);
}

// ========== WiFi 事件处理 ==========
static void _wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                ESP_LOGI(TAG, "WiFi STA 启动");
                esp_wifi_connect();
                break;

            case WIFI_EVENT_STA_CONNECTED: {
                wifi_event_sta_connected_t* event = (wifi_event_sta_connected_t*)event_data;
                strncpy(s_connected_ssid, (char*)event->ssid, sizeof(s_connected_ssid) - 1);
                s_connected_ssid[sizeof(s_connected_ssid) - 1] = '\0';
                ESP_LOGI(TAG, "WiFi 已连接: %s", s_connected_ssid);
                // 延迟到主线程更新 UI
                if (s_wifi_ui) {
                    s_pending_wifi_step = 1;
                    s_pending_wifi_ui = true;
                }
                break;
            }

            case WIFI_EVENT_STA_DISCONNECTED: {
                s_wifi_state = WIFI_DISCONNECTED;
                screenshot_sta_stop();
                ESP_LOGI(TAG, "WiFi 断开连接");
                if (s_wifi_ui && s_ui_step == WIFI_UI_STEP_CONNECTING) {
                    s_pending_qr_close = true;
                    s_pending_wifi_step = 3;  // error
                    s_pending_wifi_ui = true;
                } else if (s_ap_task_handle == NULL) {
                    s_connected_ssid[0] = '\0';
                    esp_wifi_connect();
                }
                break;
            }

            case WIFI_EVENT_AP_STACONNECTED: {
                wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*)event_data;
                ESP_LOGI(TAG, "设备接入热点: %02x:%02x:%02x:%02x:%02x:%02x",
                         event->mac[0], event->mac[1], event->mac[2],
                         event->mac[3], event->mac[4], event->mac[5]);
                break;
            }
        }
    }
    else if (event_base == IP_EVENT) {
        if (event_id == IP_EVENT_STA_GOT_IP) {
            ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
            ESP_LOGI(TAG, "获得 IP 地址: " IPSTR, IP2STR(&event->ip_info.ip));

            s_wifi_state = WIFI_CONNECTED;
            xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);

            // 延迟到主线程启动截图服务 (httpd必须在主任务启动)
            s_pending_scr_start = true;

            // 延迟到主线程: 关闭 QR, 显示成功
            s_pending_qr_close = true;
            if (s_wifi_ui && s_ui_step != WIFI_UI_STEP_SUCCESS) {
                s_pending_wifi_step = 2;  // success
                s_pending_wifi_ui = true;
            }
        }
    }
    // SmartConfig 功能 (需要安装组件后启用)
    // else if (event_base == SC_EVENT) {
    //     switch (event_id) {
    //         case SC_EVENT_SCAN_DONE:
    //             ESP_LOGI(TAG, "SmartConfig 扫描完成");
    //             break;
    //         case SC_EVENT_FOUND_CHANNEL:
    //             ESP_LOGI(TAG, "SmartConfig 找到信道");
    //             break;
    //         case SC_EVENT_GOT_SSID_PSWD:
    //             ESP_LOGI(TAG, "SmartConfig 获得 WiFi 账号密码");
    //             break;
    //         case SC_EVENT_SEND_ACK_DONE:
    //             ESP_LOGI(TAG, "SmartConfig 配网完成！");
    //             break;
    //     }
    // }
}

// ========== RSSI 更新任务 ==========
static void _rssi_update_task(void* pvParameters)
{
    while (1) {
        if (s_wifi_state == WIFI_CONNECTED) {
            wifi_ap_record_t ap_info;
            if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
                s_rssi = ap_info.rssi;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

// ========== 公共 API ==========

void wifi_init(void)
{
    s_wifi_event_group = xEventGroupCreate();

    // 初始化 NVS (用于WiFi存储)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 初始化 TCP/IP 和 WiFi
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // 注册事件处理器
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &_wifi_event_handler, NULL, NULL);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &_wifi_event_handler, NULL, NULL);
    // SmartConfig 事件注册 (需要安装组件后启用)
    // esp_event_handler_instance_register(SC_EVENT, ESP_EVENT_ANY_ID, &_wifi_event_handler, NULL, NULL);

    // 设置 WiFi 模式
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    // 启动 RSSI 更新任务
    xTaskCreate(_rssi_update_task, "rssi_update", 2048, NULL, 5, NULL);

    ESP_LOGI(TAG, "✓ WiFi 初始化完成");
}

// SmartConfig 功能 (需要安装组件后启用)
// void wifi_start_smartconfig(void)
// {
//     if (s_wifi_state == WIFI_CONFIG_MODE) return;
//     wifi_stop_ap_config();
//     s_wifi_state = WIFI_CONFIG_MODE;
//     ESP_ERROR_CHECK(esp_smartconfig_set_type(SC_TYPE_ESPTOUCH));
//     smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT();
//     ESP_ERROR_CHECK(esp_smartconfig_start(&cfg));
//     ESP_LOGI(TAG, "🚀 SmartConfig 配网模式已启动");
//     wifi_show_config_ui();
//     expression_set(EXPR_DIZZY, true);
// }
// 
// void wifi_stop_smartconfig(void)
// {
//     if (s_wifi_state != WIFI_CONFIG_MODE) return;
//     esp_smartconfig_stop();
//     s_wifi_state = WIFI_DISCONNECTED;
//     ESP_LOGI(TAG, "🛑 SmartConfig 配网模式已停止");
// }

void wifi_start_ap_config(void)
{
    if (s_wifi_state == WIFI_CONFIG_AP_MODE || s_ap_task_handle != NULL) return;



    s_wifi_state = WIFI_CONFIG_AP_MODE;
    xEventGroupClearBits(s_wifi_event_group, WIFI_AP_PROVISIONING_DONE_BIT | WIFI_CONNECTED_BIT);

    // 启动AP配网任务
    xTaskCreate(_ap_config_task, "ap_config", 8192, NULL, 5, &s_ap_task_handle);

    ESP_LOGI(TAG, "🚀 AP 配网模式已启动");

    // 如果 QR 码在显示, 跳过 text UI (避免双重覆盖+内存问题)
    if (!qrcode_is_shown()) {
        wifi_show_config_ui();
    }
    expression_set_drawing_enabled(false);
}

void wifi_stop_ap_config(void)
{
    if (s_ap_task_handle == NULL) return;

    xEventGroupSetBits(s_wifi_event_group, WIFI_AP_PROVISIONING_DONE_BIT);
    s_wifi_state = WIFI_DISCONNECTED;

    ESP_LOGI(TAG, "🛑 AP 配网模式已停止");
}

WifiState wifi_get_state(void)
{
    return s_wifi_state;
}

const char* wifi_get_ssid(void)
{
    return s_connected_ssid;
}

int wifi_get_rssi_percent(void)
{
    if (s_rssi >= -50) return 100;
    if (s_rssi <= -100) return 0;
    return (100 - ((-s_rssi - 50) * 2));
}

const char* wifi_get_status_icon(void)
{
    switch (s_wifi_state) {
        case WIFI_CONNECTED:
            return "WiFi";
        case WIFI_CONNECTING:
        case WIFI_CONFIG_AP_CONNECTING:
            return "WiFi..";
        case WIFI_CONFIG_MODE:
        case WIFI_CONFIG_AP_MODE:
            return "WiFi AP";
        default:
            return "No WiFi";
    }
}

// ========== WiFi 配置 UI V6.2 - 对齐 full_replica page_wifi_ap.c ==========
#include "xb_widgets.h"

typedef enum { AP_IDLE, AP_CONNECTING, AP_SUCCESS, AP_ERROR } ap_state_t;

typedef struct {
    lv_obj_t* body;
    lv_obj_t* page;
    ap_state_t state;
} ap_ctx_t;

static ap_ctx_t* s_ap_ctx = NULL;

// 主线程调用: 处理延迟的 UI 操作
void wifi_process_pending_ui(void)
{
    if (s_pending_scr_start) {
        s_pending_scr_start = false;
        ESP_LOGI(TAG, "Starting screenshot server from main task...");
        screenshot_sta_start();
    }
    if (s_pending_qr_close) {
        s_pending_qr_close = false;
        qrcode_close();
    }
    if (s_pending_wifi_ui) {
        s_pending_wifi_ui = false;
        s_ui_step = s_pending_wifi_step == 1 ? WIFI_UI_STEP_CONNECTING
                  : s_pending_wifi_step == 2 ? WIFI_UI_STEP_SUCCESS
                  : s_pending_wifi_step == 3 ? WIFI_UI_STEP_ERROR
                  : WIFI_UI_STEP_AP;
        wifi_show_config_ui();
    }
}

static void _qr_fullscreen_cb(lv_event_t* e) {
    qrcode_create(lv_screen_active());
}

static void _wifi_render(ap_ctx_t* ctx);
static void _btn_retry_cb(lv_event_t* e);
static void _btn_close_cb(lv_event_t* e);

// 关闭 WiFi 页面: 停止 AP, 回到表情界面, 不忘记已有网络
static void _wifi_stop_ap_and_close(void)
{
    if (s_ap_task_handle) {
        xEventGroupSetBits(s_wifi_event_group, WIFI_AP_PROVISIONING_DONE_BIT);
        s_ap_task_handle = NULL;
    }
    _dns_server_stop();
    if (s_http_server) { httpd_stop(s_http_server); s_http_server = NULL; }
    if (s_ap_netif) { esp_netif_destroy(s_ap_netif); s_ap_netif = NULL; }
    esp_wifi_set_mode(WIFI_MODE_STA);
    s_wifi_state = WIFI_DISCONNECTED;
    s_ui_step = WIFI_UI_STEP_AP;
    wifi_hide_config_ui();
}

static void _btn_back_cb(lv_event_t* e) { (void)e; _wifi_stop_ap_and_close(); }
static void _btn_close_cb(lv_event_t* e) { _wifi_stop_ap_and_close(); }
static void _btn_done_cb(lv_event_t* e) { _wifi_stop_ap_and_close(); }

static void _btn_retry_cb(lv_event_t* e)
{
    ap_ctx_t* ctx = s_ap_ctx;  // 用全局指针, 不用 e->user_data (会被 delete 释放)
    // 停止旧 AP 基础设施 (不删 UI)
    if (s_ap_task_handle) {
        xEventGroupSetBits(s_wifi_event_group, WIFI_AP_PROVISIONING_DONE_BIT);
        s_ap_task_handle = NULL;
    }
    _dns_server_stop();
    if (s_http_server) { httpd_stop(s_http_server); s_http_server = NULL; }
    if (s_ap_netif) { esp_netif_destroy(s_ap_netif); s_ap_netif = NULL; }
    s_wifi_state = WIFI_CONFIG_AP_MODE;
    s_ui_step = WIFI_UI_STEP_AP;
    // 渲染 IDLE 状态
    if (ctx) { ctx->state = AP_IDLE; _wifi_render(ctx); }
    // 重启 AP
    xEventGroupClearBits(s_wifi_event_group, WIFI_AP_PROVISIONING_DONE_BIT | WIFI_CONNECTED_BIT);
    xTaskCreate(_ap_config_task, "ap_config", 8192, NULL, 5, &s_ap_task_handle);
}

// ========== 4-state 渲染 (对齐 page_wifi_ap.c render) ==========
static void _wifi_render(ap_ctx_t* ctx)
{
    lv_color_t fg = theme_fg();
    lv_obj_clean(ctx->body);

    switch (ctx->state) {
    case AP_IDLE: {
        // Card with SSID + QR
        lv_obj_t* card = xb_card(ctx->body);
        lv_obj_set_size(card, 280, 130);
        lv_obj_center(card);

        lv_obj_t* t1 = lv_label_create(card);
        lv_label_set_text(t1, "Connect to AP:");
        lv_obj_set_style_text_color(t1, fg, 0);
        lv_obj_set_style_text_opa(t1, LV_OPA_60, 0);
        lv_obj_align(t1, LV_ALIGN_TOP_LEFT, 6, 4);

        lv_obj_t* ssid = lv_label_create(card);
        lv_label_set_text(ssid, AP_SSID);
        lv_obj_set_style_text_color(ssid, fg, 0);
        lv_obj_align(ssid, LV_ALIGN_TOP_LEFT, 6, 26);

        lv_obj_t* url = lv_label_create(card);
        lv_label_set_text(url, "Open  192.168.4.1");
        lv_obj_set_style_text_color(url, fg, 0);
        lv_obj_set_style_text_opa(url, LV_OPA_80, 0);
        lv_obj_align(url, LV_ALIGN_TOP_LEFT, 6, 56);

        // QR 码按钮 — 点击打开全屏 QR
        lv_obj_t* qr_btn = lv_btn_create(card);
        lv_obj_set_size(qr_btn, 64, 64);
        lv_obj_set_style_radius(qr_btn, 4, 0);
        lv_obj_set_style_bg_color(qr_btn, fg, 0);
        lv_obj_set_style_shadow_width(qr_btn, 0, 0);
        lv_obj_align(qr_btn, LV_ALIGN_RIGHT_MID, -8, 0);
        lv_obj_add_event_cb(qr_btn, _qr_fullscreen_cb, LV_EVENT_CLICKED, NULL);
        lv_obj_t* qr_lb = lv_label_create(qr_btn);
        lv_label_set_text(qr_lb, "QR");
        lv_obj_set_style_text_color(qr_lb, theme_bg(), 0);
        lv_obj_center(qr_lb);

        // Close 按钮
        lv_obj_t* btn_c = xb_button(ctx->body, "Close", _btn_close_cb);
        lv_obj_align(btn_c, LV_ALIGN_BOTTOM_MID, 0, -10);
        break;
    }
    case AP_CONNECTING: {
        lv_obj_t* dots = xb_dot_loading_create(ctx->body);
        lv_obj_center(dots);
        lv_obj_t* l = lv_label_create(ctx->body);
        lv_label_set_text(l, "Joining your network...");
        lv_obj_set_style_text_color(l, fg, 0);
        lv_obj_align(l, LV_ALIGN_CENTER, 0, 28);
        break;
    }
    case AP_SUCCESS: {
        lv_obj_t* check = lv_label_create(ctx->body);
        lv_label_set_text(check, "OK");
        lv_obj_set_style_text_color(check, lv_color_hex(0x22C55E), 0);
        lv_obj_align(check, LV_ALIGN_CENTER, 0, -16);
        lv_obj_t* l = lv_label_create(ctx->body);
        lv_label_set_text_fmt(l, "Connected\n%s", s_connected_ssid);
        lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_color(l, lv_color_hex(0x22C55E), 0);
        lv_obj_align(l, LV_ALIGN_CENTER, 0, 22);
        lv_obj_t* btn_d = xb_button(ctx->body, "Done", _btn_done_cb);
        lv_obj_align(btn_d, LV_ALIGN_BOTTOM_MID, 0, -10);
        break;
    }
    case AP_ERROR: {
        xb_error_inline(ctx->body, "Auth failed");
        lv_obj_t* btn_r = xb_button(ctx->body, "Retry", _btn_retry_cb);
        lv_obj_align(btn_r, LV_ALIGN_BOTTOM_MID, 0, -10);
        break;
    }
    }
}

static void _on_ap_del(lv_event_t* e)
{
    ap_ctx_t* ctx = (ap_ctx_t*)lv_event_get_user_data(e);
    if (ctx) { lv_free(ctx); s_ap_ctx = NULL; }
}

void wifi_show_config_ui(void)
{
    if (s_wifi_ui) {
        // 更新状态
        if (s_ap_ctx) {
            s_ap_ctx->state = (s_ui_step == WIFI_UI_STEP_SUCCESS) ? AP_SUCCESS
                           : (s_ui_step == WIFI_UI_STEP_CONNECTING) ? AP_CONNECTING
                           : (s_ui_step == WIFI_UI_STEP_ERROR) ? AP_ERROR
                           : AP_IDLE;
            _wifi_render(s_ap_ctx);
        }
        return;
    }

    lv_color_t bg = theme_bg();

    // 全屏页面容器
    s_wifi_ui = lv_obj_create(lv_screen_active());
    lv_obj_set_size(s_wifi_ui, 320, 240);
    lv_obj_set_pos(s_wifi_ui, 0, 0);
    lv_obj_set_style_bg_color(s_wifi_ui, bg, 0);
    lv_obj_set_style_border_width(s_wifi_ui, 0, 0);
    lv_obj_set_style_pad_all(s_wifi_ui, 0, 0);
    lv_obj_remove_flag(s_wifi_ui, LV_OBJ_FLAG_SCROLLABLE);

    // 状态栏 + 顶栏
    xb_statusbar_create(s_wifi_ui);
    lv_obj_t* tb = xb_topbar_create(s_wifi_ui, "Wi-Fi", true);
    lv_obj_add_flag(tb, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(tb, _btn_back_cb, LV_EVENT_CLICKED, NULL);

    // Context
    ap_ctx_t* ctx = (ap_ctx_t*)lv_malloc(sizeof(ap_ctx_t));
    ctx->state = AP_IDLE;
    s_ap_ctx = ctx;

    lv_obj_add_event_cb(s_wifi_ui, _on_ap_del, LV_EVENT_DELETE, ctx);

    // Body (内容区, topbar 下方 190px)
    ctx->body = lv_obj_create(s_wifi_ui);
    lv_obj_set_size(ctx->body, 320, 190);
    lv_obj_align(ctx->body, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_set_style_bg_opa(ctx->body, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ctx->body, 0, 0);
    lv_obj_remove_flag(ctx->body, LV_OBJ_FLAG_SCROLLABLE);

    _wifi_render(ctx);
    ESP_LOGI(TAG, "WiFi UI v6.2 已显示 (state=%d)", ctx->state);
}

void wifi_hide_config_ui(void)
{
    if (!s_wifi_ui) return;

    lv_obj_delete(s_wifi_ui);
    s_wifi_ui = NULL;
    s_status_label = NULL;
    s_ui_step = WIFI_UI_STEP_AP;  // V3.9: 重置步骤

    // 重新启用面部绘制 + 关闭 QR 码
    expression_set_drawing_enabled(true);
    qrcode_close();

    ESP_LOGI(TAG, "WiFi 配置界面已关闭");
}
