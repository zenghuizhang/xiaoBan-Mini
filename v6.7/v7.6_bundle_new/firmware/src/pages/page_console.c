// page_console.c — developer console with 4 Chinese tabs.
//   系统 · AI · 显示 · 服务
// 系统 tab: 6 somatosensory + 6 scenario buttons
// AI    tab: state toggles (idle/think/talking), bubble inject
// 显示 tab: theme cycle, statusbar mode cycle, face variant cycle
// 服务 tab: WiFi state inject, battery slider, MCP toggle, OTA % poke
#include "xb_pages.h"
#include "../core/xb_theme.h"
#include "../core/xb_face.h"
#include "../core/xb_event.h"
#include "../sim/xb_imu_sim.h"
#include "../sim/xb_scenario_sim.h"
#include "../sim/xb_persona.h"
#include "../widgets/xb_widgets.h"
#include <string.h>

static lv_obj_t* g_tabview = NULL;

// ---------------- tab: 系统 ---------------- //
static void imu_cb(lv_event_t* e) {
    xb_imu_sim_fire((imu_event_t)(intptr_t)lv_event_get_user_data(e));
}
static void scn_cb(lv_event_t* e) {
    xb_scenario_fire((scene_t)(intptr_t)lv_event_get_user_data(e));
}
static void build_system_tab(lv_obj_t* tab) {
    const theme_t* th = xb_theme_get();

    lv_obj_t* l1 = lv_label_create(tab);
    lv_label_set_text(l1, "体感测试");
    lv_obj_set_style_text_color(l1, th->accent_hi, 0);
    lv_obj_set_pos(l1, 4, 0);

    static const char* IMU_LBL[6] = {"前倾","后仰","左倾","右倾","摇晃","旋转"};
    for (int i = 0; i < 6; ++i) {
        lv_obj_t* b = xb_button(tab, IMU_LBL[i], imu_cb);
        lv_obj_set_size(b, 64, 26);
        lv_obj_set_pos(b, 4 + (i % 3) * 96, 18 + (i / 3) * 30);
        lv_obj_set_user_data(b, (void*)(intptr_t)i);
        lv_obj_add_event_cb(b, imu_cb, LV_EVENT_CLICKED, (void*)(intptr_t)i);
    }

    lv_obj_t* l2 = lv_label_create(tab);
    lv_label_set_text(l2, "情景测试");
    lv_obj_set_style_text_color(l2, th->accent_hi, 0);
    lv_obj_set_pos(l2, 4, 84);

    for (int i = 0; i < 6; ++i) {
        lv_obj_t* b = xb_button(tab, XB_SCENES[i].name_zh, scn_cb);
        lv_obj_set_size(b, 64, 26);
        lv_obj_set_pos(b, 4 + (i % 3) * 96, 102 + (i / 3) * 30);
        lv_obj_add_event_cb(b, scn_cb, LV_EVENT_CLICKED, (void*)(intptr_t)i);
    }
}

// ---------------- tab: AI ---------------- //
static void ai_state_cb(lv_event_t* e) {
    int s = (int)(intptr_t)lv_event_get_user_data(e);
    xb_event_post(XB_EVT_AI_STATE, (void*)(intptr_t)s);
    xb_face_set(s == 1 ? FACE_THINKING : s == 2 ? FACE_TALKING : FACE_IDLE);
}
static void bubble_cb(lv_event_t* e) {
    (void)e;
    static bubble_evt_t ev = { XB_BUBBLE_TEXT, "示例气泡：今天感觉怎么样？", NULL };
    xb_event_post(XB_EVT_BUBBLE, &ev);
}
static void build_ai_tab(lv_obj_t* tab) {
    const theme_t* th = xb_theme_get();
    lv_obj_t* l = lv_label_create(tab);
    lv_label_set_text(l, "AI 状态");
    lv_obj_set_style_text_color(l, th->accent_hi, 0);
    lv_obj_set_pos(l, 4, 0);

    static const char* AI[3] = {"空闲","思考","说话"};
    for (int i = 0; i < 3; ++i) {
        lv_obj_t* b = xb_button(tab, AI[i], ai_state_cb);
        lv_obj_set_size(b, 80, 28);
        lv_obj_set_pos(b, 4 + i * 90, 20);
        lv_obj_add_event_cb(b, ai_state_cb, LV_EVENT_CLICKED, (void*)(intptr_t)i);
    }
    lv_obj_t* bb = xb_button(tab, "弹示例气泡", bubble_cb);
    lv_obj_set_size(bb, 260, 30);
    lv_obj_set_pos(bb, 4, 60);
}

// ---------------- tab: 显示 ---------------- //
static int g_face_idx = 0;
static void cycle_face_cb(lv_event_t* e) {
    (void)e;
    g_face_idx = (g_face_idx + 1) % FACE_VARIANT_MAX;
    xb_face_set((face_variant_t)g_face_idx);
}
static const char* THEME_IDS[4] = {"tech","lavender","child","cocoa"};
static int g_theme_idx = 0;
static void cycle_theme_cb(lv_event_t* e) {
    (void)e;
    g_theme_idx = (g_theme_idx + 1) % 4;
    xb_theme_set(THEME_IDS[g_theme_idx]);
}
static int g_sb_mode = 0;
static void cycle_sb_cb(lv_event_t* e) {
    (void)e;
    g_sb_mode = (g_sb_mode + 1) % 4;
    xb_statusbar_set_mode((statusbar_mode_t)g_sb_mode);
}
static void build_display_tab(lv_obj_t* tab) {
    lv_obj_t* a = xb_button(tab, "下一表情", cycle_face_cb);
    lv_obj_set_size(a, 260, 30); lv_obj_set_pos(a, 4, 4);
    lv_obj_t* b = xb_button(tab, "下一主题", cycle_theme_cb);
    lv_obj_set_size(b, 260, 30); lv_obj_set_pos(b, 4, 44);
    lv_obj_t* c = xb_button(tab, "状态栏模式", cycle_sb_cb);
    lv_obj_set_size(c, 260, 30); lv_obj_set_pos(c, 4, 84);
}

// ---------------- tab: 服务 ---------------- //
static int g_wifi_state = 2;
static void cycle_wifi_cb(lv_event_t* e) {
    (void)e;
    g_wifi_state = (g_wifi_state + 1) % 3;
    xb_event_post(XB_EVT_WIFI_STATE, (void*)(intptr_t)g_wifi_state);
}
static int g_battery = 100;
static void cycle_bat_cb(lv_event_t* e) {
    (void)e;
    g_battery -= 20; if (g_battery < 0) g_battery = 100;
    xb_event_post(XB_EVT_BATTERY, (void*)(intptr_t)g_battery);
}
static bool g_mcp = false;
static void cycle_mcp_cb(lv_event_t* e) {
    (void)e;
    g_mcp = !g_mcp;
    xb_event_post(XB_EVT_MCP_ACTIVE, (void*)(intptr_t)g_mcp);
}
static int g_ota = -1;
static void poke_ota_cb(lv_event_t* e) {
    (void)e;
    g_ota = (g_ota < 0 || g_ota >= 100) ? 0 : g_ota + 20;
    xb_event_post(XB_EVT_OTA_PROGRESS, (void*)(intptr_t)g_ota);
}
static void build_service_tab(lv_obj_t* tab) {
    lv_obj_t* w = xb_button(tab, "WiFi 状态", cycle_wifi_cb);
    lv_obj_set_size(w, 260, 28); lv_obj_set_pos(w, 4, 4);
    lv_obj_t* b = xb_button(tab, "电量 -20%", cycle_bat_cb);
    lv_obj_set_size(b, 260, 28); lv_obj_set_pos(b, 4, 38);
    lv_obj_t* m = xb_button(tab, "MCP 切换", cycle_mcp_cb);
    lv_obj_set_size(m, 260, 28); lv_obj_set_pos(m, 4, 72);
    lv_obj_t* o = xb_button(tab, "OTA +20%", poke_ota_cb);
    lv_obj_set_size(o, 260, 28); lv_obj_set_pos(o, 4, 106);
}

static void back_cb(lv_event_t* e) { (void)e; xb_router_back(); }

lv_obj_t* page_console_create(lv_obj_t* parent) {
    lv_obj_t* root = lv_obj_create(parent);
    lv_obj_set_size(root, 320, 240);
    lv_obj_center(root);
    lv_obj_set_style_bg_opa(root, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_set_style_pad_all(root, 0, 0);

    xb_topbar(root, "控制台 [测试]", back_cb);

    g_tabview = lv_tabview_create(root);
    lv_obj_set_size(g_tabview, 320, 210);
    lv_obj_set_pos(g_tabview, 0, 28);
    lv_tabview_set_tab_bar_size(g_tabview, 26);

    build_system_tab (lv_tabview_add_tab(g_tabview, "系统"));
    build_ai_tab     (lv_tabview_add_tab(g_tabview, "AI"));
    build_display_tab(lv_tabview_add_tab(g_tabview, "显示"));
    build_service_tab(lv_tabview_add_tab(g_tabview, "服务"));
    return root;
}
