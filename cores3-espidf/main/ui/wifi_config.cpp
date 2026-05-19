#include "wifi_config.h"
#include "theme_v3.h"
#include "expressions.h"
#include "qrcode.h"
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

// 事件组位定义
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_SMARTCONFIG_DONE_BIT BIT1
#define WIFI_AP_PROVISIONING_DONE_BIT BIT2

// AP配网配置
#define AP_SSID "ESP32-S3-Box-Config"
#define AP_PASSWORD ""  // 无密码
#define AP_TIMEOUT_MS (5 * 60 * 1000)  // 5分钟超时

// ========== LVGL 样式辅助 ==========
static lv_color_t _bg_color(void) {
    return lv_color_hex(theme_v3_get_current() == THEME_TECH ? 0x000000 : 0xFFFBEB);
}

static lv_color_t _text_color(void) {
    return lv_color_hex(theme_v3_get_current() == THEME_TECH ? 0xFFFFFF : 0x000000);
}

static lv_color_t _accent_color(void) {
    return lv_color_hex(theme_v3_get_current() == THEME_TECH ? 0x22D3EE : 0xF97316);
}

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

    // V3.9: 更新UI状态到"连接中"步骤
    s_wifi_state = WIFI_CONFIG_AP_CONNECTING;
    s_ui_step = WIFI_UI_STEP_CONNECTING;
    wifi_show_config_ui();
    expression_set(EXPR_DIZZY, true);

    // 停止AP，切换到STA模式
    wifi_config_t wifi_config = {0};
    memcpy(wifi_config.sta.ssid, ssid, strlen(ssid));
    memcpy(wifi_config.sta.password, password, strlen(password));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_connect());

    const char *resp = "{\"success\":true,\"message\":\"正在连接WiFi\"}";
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp, strlen(resp));

    // 通知任务配网完成
    xEventGroupSetBits(s_wifi_event_group, WIFI_AP_PROVISIONING_DONE_BIT);

    return ESP_OK;
}

// Captive Portal 重定向处理
static esp_err_t _http_captive_handler(httpd_req_t *req)
{
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "http://192.168.4.1/");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

// 404 处理器 → 所有未匹配的 URI 也重定向到配网主页
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

        // Captive Portal 常见检测地址
        httpd_uri_t captive_uris[] = {
            {.uri = "/generate_204", .method = HTTP_GET, .handler = _http_captive_handler, .user_ctx = NULL},
            {.uri = "/gen_204", .method = HTTP_GET, .handler = _http_captive_handler, .user_ctx = NULL},
            {.uri = "/hotspot-detect.html", .method = HTTP_GET, .handler = _http_captive_handler, .user_ctx = NULL},
            {.uri = "/ncsi.txt", .method = HTTP_GET, .handler = _http_captive_handler, .user_ctx = NULL},
            {.uri = "/redirect", .method = HTTP_GET, .handler = _http_captive_handler, .user_ctx = NULL},
        };

        for (size_t i = 0; i < sizeof(captive_uris)/sizeof(captive_uris[0]); i++) {
            httpd_register_uri_handler(server, &captive_uris[i]);
        }

        // 404 → 302 redirect (catch-all for captive portal)
        httpd_register_err_handler(server, HTTPD_404_NOT_FOUND, _http_404_handler);

        ESP_LOGI(TAG, "✓ Web服务器 + DNS劫持 + CaptivePortal 已就绪");
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
                break;
            }

            case WIFI_EVENT_STA_DISCONNECTED: {
                s_wifi_state = WIFI_DISCONNECTED;
                ESP_LOGI(TAG, "WiFi 断开连接");
                // V3.9: 配网流程 - 连接失败步骤
                if (s_wifi_ui && s_ui_step == WIFI_UI_STEP_CONNECTING) {
                    s_ui_step = WIFI_UI_STEP_ERROR;
                    wifi_show_config_ui();
                } else if (s_ap_task_handle == NULL) {
                    s_connected_ssid[0] = '\0';
                    esp_wifi_connect();  // 非配网模式下自动重连
                }
                break;
            }

            case WIFI_EVENT_AP_STACONNECTED: {
                wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*)event_data;
                ESP_LOGI(TAG, "设备接入热点: %02x:%02x:%02x:%02x:%02x:%02x",
                         event->mac[0], event->mac[1], event->mac[2],
                         event->mac[3], event->mac[4], event->mac[5]);
                if (s_status_label) {
                    lv_label_set_text(s_status_label, "📱 设备已连接\n正在打开配网页面...");
                }
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

            // V3.9: 配网流程 - 连接成功步骤
            if (s_wifi_ui && s_ui_step != WIFI_UI_STEP_SUCCESS) {
                s_ui_step = WIFI_UI_STEP_SUCCESS;
                wifi_show_config_ui();
            } else if (s_status_label) {
                char buf[128];
                snprintf(buf, sizeof(buf), "Connected: %s", s_connected_ssid);
                lv_label_set_text(s_status_label, buf);
            }

            // 成功反馈表情
            expression_set(EXPR_HAPPY, true);
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

// ========== WiFi 配置 UI V3.9 - 多步骤流程对齐原型 ==========

static void _spinner_rotate_cb(void *obj, int32_t val)
{
    lv_obj_set_style_transform_rotation((lv_obj_t *)obj, val * 10, 0);
}

static void _btn_close_cb(lv_event_t *e)
{
    wifi_stop_ap_config();
    wifi_hide_config_ui();
}

static void _btn_retry_cb(lv_event_t *e)
{
    // 重试：回到 AP 步骤，但保持当前的 AP 模式
    s_ui_step = WIFI_UI_STEP_AP;
    lv_obj_delete(s_wifi_ui);
    s_wifi_ui = NULL;
    s_status_label = NULL;
    wifi_show_config_ui();
}

static void _btn_done_cb(lv_event_t *e)
{
    wifi_hide_config_ui();
}

// 重建 UI 帮助函数
static void _wifi_ui_rebuild(void)
{
    if (!s_wifi_ui) return;

    lv_obj_t *parent = s_wifi_ui;
    // 清空所有子对象
    while (lv_obj_get_child_cnt(parent) > 0) {
        lv_obj_t *child = lv_obj_get_child(parent, 0);
        if (child) lv_obj_delete(child);
    }
    s_status_label = NULL;
}

void wifi_show_config_ui(void)
{
    if (s_wifi_ui) {
        // UI 已存在，重建内容以反映当前步骤
        _wifi_ui_rebuild();
    } else {
        // 创建全屏容器
        s_wifi_ui = lv_obj_create(lv_screen_active());
        lv_obj_set_size(s_wifi_ui, 320, 240);
        lv_obj_set_pos(s_wifi_ui, 0, 0);
        lv_obj_set_style_radius(s_wifi_ui, 0, 0);
        lv_obj_set_style_bg_color(s_wifi_ui, _bg_color(), 0);
        lv_obj_set_style_bg_opa(s_wifi_ui, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(s_wifi_ui, 0, 0);
        lv_obj_set_style_pad_all(s_wifi_ui, 0, 0);
        lv_obj_set_style_layout(s_wifi_ui, LV_LAYOUT_FLEX, 0);
        lv_obj_set_flex_flow(s_wifi_ui, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(s_wifi_ui, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_all(s_wifi_ui, 24, 0);
    }

    switch (s_ui_step) {
        case WIFI_UI_STEP_AP: {
            // === AP 模式: 热点信息 + 配网说明 ===
            lv_obj_t *icon = lv_label_create(s_wifi_ui);
            lv_label_set_text(icon, "WiFi");
            lv_obj_set_style_text_font(icon, &lv_font_montserrat_14, 0);
            lv_obj_set_style_text_color(icon, _accent_color(), 0);
            lv_obj_set_style_pad_bottom(icon, 16, 0);

            lv_obj_t *title = lv_label_create(s_wifi_ui);
            lv_label_set_text(title, "WiFi Setup");
            lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
            lv_obj_set_style_text_color(title, _text_color(), 0);
            lv_obj_set_style_pad_bottom(title, 4, 0);

            s_status_label = lv_label_create(s_wifi_ui);
            lv_label_set_text_fmt(s_status_label,
                "Hotspot: %s\n"
                "Open http://192.168.4.1\n"
                "to configure WiFi",
                AP_SSID);
            lv_obj_set_style_text_align(s_status_label, LV_TEXT_ALIGN_CENTER, 0);
            lv_obj_set_style_text_color(s_status_label, _text_color(), 0);
            lv_obj_set_style_pad_top(s_status_label, 12, 0);
            lv_obj_set_style_pad_bottom(s_status_label, 24, 0);

            lv_obj_t *btn_cancel = lv_btn_create(s_wifi_ui);
            lv_obj_set_size(btn_cancel, 140, 40);
            lv_obj_set_style_radius(btn_cancel, 8, 0);
            lv_obj_set_style_bg_color(btn_cancel, _accent_color(), 0);
            lv_obj_add_event_cb(btn_cancel, _btn_close_cb, LV_EVENT_CLICKED, NULL);

            lv_obj_t *btn_label = lv_label_create(btn_cancel);
            lv_label_set_text(btn_label, "Cancel");
            lv_obj_set_style_text_color(btn_label, lv_color_hex(0xFFFFFF), 0);
            lv_obj_center(btn_label);

            ESP_LOGI(TAG, "WiFi UI: AP 模式步骤");
            break;
        }

        case WIFI_UI_STEP_CONNECTING: {
            // === 连接中: 简易旋转指示 + 文字 ===
            lv_obj_t *spinner = lv_label_create(s_wifi_ui);
            lv_label_set_text(spinner, ">");
            lv_obj_set_style_text_font(spinner, &lv_font_montserrat_14, 0);
            lv_obj_set_style_text_color(spinner, _accent_color(), 0);
            lv_obj_set_style_pad_bottom(spinner, 12, 0);

            // 简易旋转动画
            lv_anim_t spin_anim;
            lv_anim_init(&spin_anim);
            lv_anim_set_var(&spin_anim, spinner);
            lv_anim_set_exec_cb(&spin_anim, _spinner_rotate_cb);
            lv_anim_set_values(&spin_anim, 0, 3600);
            lv_anim_set_time(&spin_anim, 1000);
            lv_anim_set_repeat_count(&spin_anim, LV_ANIM_REPEAT_INFINITE);
            lv_anim_set_path_cb(&spin_anim, lv_anim_path_linear);
            lv_anim_start(&spin_anim);

            s_status_label = lv_label_create(s_wifi_ui);
            lv_label_set_text(s_status_label, "Connecting...");
            lv_obj_set_style_text_color(s_status_label, _accent_color(), 0);
            lv_obj_set_style_pad_top(s_status_label, 12, 0);
            lv_obj_set_style_pad_bottom(s_status_label, 24, 0);

            lv_obj_t *btn_cancel2 = lv_btn_create(s_wifi_ui);
            lv_obj_set_size(btn_cancel2, 140, 40);
            lv_obj_set_style_radius(btn_cancel2, 8, 0);
            lv_obj_set_style_bg_color(btn_cancel2, lv_color_hex(0x3F3F46), 0);
            lv_obj_add_event_cb(btn_cancel2, _btn_close_cb, LV_EVENT_CLICKED, NULL);

            lv_obj_t *btn_label2 = lv_label_create(btn_cancel2);
            lv_label_set_text(btn_label2, "Cancel");
            lv_obj_set_style_text_color(btn_label2, _text_color(), 0);
            lv_obj_center(btn_label2);

            ESP_LOGI(TAG, "WiFi UI: 连接中步骤");
            break;
        }

        case WIFI_UI_STEP_SUCCESS: {
            // === 连接成功 ===
            lv_obj_t *check = lv_label_create(s_wifi_ui);
            lv_label_set_text(check, "OK");
            lv_obj_set_style_text_font(check, &lv_font_montserrat_14, 0);
            lv_obj_set_style_text_color(check, lv_color_hex(0x22C55E), 0);
            lv_obj_set_style_pad_bottom(check, 12, 0);

            s_status_label = lv_label_create(s_wifi_ui);
            lv_label_set_text_fmt(s_status_label, "Connected!\nSSID: %s", s_connected_ssid);
            lv_obj_set_style_text_align(s_status_label, LV_TEXT_ALIGN_CENTER, 0);
            lv_obj_set_style_text_color(s_status_label, _accent_color(), 0);
            lv_obj_set_style_pad_bottom(s_status_label, 24, 0);

            lv_obj_t *btn_done = lv_btn_create(s_wifi_ui);
            lv_obj_set_size(btn_done, 140, 40);
            lv_obj_set_style_radius(btn_done, 8, 0);
            lv_obj_set_style_bg_color(btn_done, _accent_color(), 0);
            lv_obj_add_event_cb(btn_done, _btn_done_cb, LV_EVENT_CLICKED, NULL);

            lv_obj_t *btn_label3 = lv_label_create(btn_done);
            lv_label_set_text(btn_label3, "Done");
            lv_obj_set_style_text_color(btn_label3, lv_color_hex(0xFFFFFF), 0);
            lv_obj_center(btn_label3);

            ESP_LOGI(TAG, "WiFi UI: 成功步骤");
            break;
        }

        case WIFI_UI_STEP_ERROR: {
            // === 连接失败 ===
            lv_obj_t *x_icon = lv_label_create(s_wifi_ui);
            lv_label_set_text(x_icon, "X");
            lv_obj_set_style_text_font(x_icon, &lv_font_montserrat_14, 0);
            lv_obj_set_style_text_color(x_icon, lv_color_hex(0xEF4444), 0);
            lv_obj_set_style_pad_bottom(x_icon, 12, 0);

            s_status_label = lv_label_create(s_wifi_ui);
            lv_label_set_text(s_status_label, "Connection failed\nPlease try again");
            lv_obj_set_style_text_align(s_status_label, LV_TEXT_ALIGN_CENTER, 0);
            lv_obj_set_style_text_color(s_status_label, lv_color_hex(0xF87171), 0);
            lv_obj_set_style_pad_bottom(s_status_label, 20, 0);

            // 按钮行
            lv_obj_t *btn_row = lv_obj_create(s_wifi_ui);
            lv_obj_set_size(btn_row, 280, 44);
            lv_obj_set_style_bg_opa(btn_row, LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_width(btn_row, 0, 0);
            lv_obj_set_style_pad_all(btn_row, 0, 0);
            lv_obj_set_style_layout(btn_row, LV_LAYOUT_FLEX, 0);
            lv_obj_set_flex_flow(btn_row, LV_FLEX_FLOW_ROW);
            lv_obj_set_flex_align(btn_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

            lv_obj_t *btn_back = lv_btn_create(btn_row);
            lv_obj_set_size(btn_back, 110, 40);
            lv_obj_set_style_radius(btn_back, 8, 0);
            lv_obj_set_style_bg_color(btn_back, lv_color_hex(0x3F3F46), 0);
            lv_obj_add_event_cb(btn_back, _btn_retry_cb, LV_EVENT_CLICKED, NULL);

            lv_obj_t *back_label = lv_label_create(btn_back);
            lv_label_set_text(back_label, "Back");
            lv_obj_set_style_text_color(back_label, _text_color(), 0);
            lv_obj_center(back_label);

            lv_obj_t *btn_retry = lv_btn_create(btn_row);
            lv_obj_set_size(btn_retry, 110, 40);
            lv_obj_set_style_radius(btn_retry, 8, 0);
            lv_obj_set_style_bg_color(btn_retry, lv_color_hex(0x7F1D1D), 0);
            lv_obj_add_event_cb(btn_retry, _btn_retry_cb, LV_EVENT_CLICKED, NULL);

            lv_obj_t *retry_label = lv_label_create(btn_retry);
            lv_label_set_text(retry_label, "Retry");
            lv_obj_set_style_text_color(retry_label, lv_color_hex(0xFCA5A5), 0);
            lv_obj_center(retry_label);

            ESP_LOGI(TAG, "WiFi UI: 错误步骤");
            break;
        }
    }

    ESP_LOGI(TAG, "WiFi 配置界面 V3.9 已显示 (step=%d)", s_ui_step);
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
