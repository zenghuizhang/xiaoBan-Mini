// page_console.cpp — v7.6 §11: 4-tab developer console
#include "page_console.h"
#include "xb_widgets.h"
#include "theme_v3.h"
#include "expressions.h"
#include "font_zh_14.h"
#include "robot_memory.h"
#include <stdio.h>
#include <esp_log.h>

static const char* TAG = "CONSOLE";
static lv_obj_t* s_page = NULL;

static const char* _T(const char* cn, const char* en) {
    extern bool s_lang_cn; return s_lang_cn ? cn : en;
}
static const lv_font_t* _F(void) {
    extern bool s_lang_cn;
    return s_lang_cn ? (const lv_font_t*)&font_zh_14 : &lv_font_montserrat_14;
}

static void on_back(lv_event_t* e) {
    (void)e; if(s_page){lv_obj_delete(s_page);s_page=NULL;}
    expression_set_drawing_enabled(true);
}

static void _btn_cb_face(lv_event_t* e) {
    const char* tag = (const char*)lv_event_get_user_data(e);
    ESP_LOGI(TAG, "face: %s", tag);
    if(strstr(tag,"idle")||strstr(tag,"空")) expression_set(EXPR_IDLE,true);
    else if(strstr(tag,"think")||strstr(tag,"思")) expression_set(EXPR_THINKING,true);
    else if(strstr(tag,"talk")||strstr(tag,"说")) expression_set(EXPR_TALKING,true);
}
static void _btn_cb_expr(lv_event_t* e) {
    static int cycle=0;
    Expression ex[]={EXPR_HAPPY,EXPR_WINK,EXPR_DIZZY,EXPR_CRYING,EXPR_NAUGHTY,EXPR_SURPRISED};
    expression_set(ex[(cycle++)%6],true);
}
static void _btn_cb_theme(lv_event_t* e) {
    ThemeV3 c=theme_v3_get_current();
    ThemeV3 n=(c==THEME_TECH)?THEME_LAVENDER:(c==THEME_LAVENDER)?THEME_CHILD:(c==THEME_CHILD)?THEME_COCOA:THEME_TECH;
    theme_v3_switch(n);
    memory_save_theme((int)n);
    expression_refresh_theme();
}
static void _btn_cb_imu(lv_event_t* e) {
    const char* tag = (const char*)lv_event_get_user_data(e);
    ESP_LOGI(TAG,"IMU: %s",tag);
    if(strstr(tag,"前")) expression_set(EXPR_CURIOUS,true);
    else if(strstr(tag,"后")) expression_set(EXPR_YAWN,true);
    else if(strstr(tag,"左")) expression_set(EXPR_LOOK_LEFT,true);
    else if(strstr(tag,"右")) expression_set(EXPR_LOOK_RIGHT,true);
    else if(strstr(tag,"摇")) expression_set(EXPR_DIZZY,true);
    else expression_set(EXPR_ALERT,true);
}
static void _btn_cb_scene(lv_event_t* e) {
    const char* tag = (const char*)lv_event_get_user_data(e);
    ESP_LOGI(TAG,"Scene: %s",tag);
    if(strstr(tag,"OTA")) expression_set(EXPR_THINKING,true);
    else if(strstr(tag,"报")) expression_set(EXPR_SAD,true);
    else if(strstr(tag,"电")) expression_set(EXPR_CRYING,true);
    else if(strstr(tag,"高")) expression_set(EXPR_DIZZY,true);
    else if(strstr(tag,"唤")) expression_set(EXPR_ALERT,true);
    else expression_set(EXPR_HAPPY,true);
}

// ── 按钮网格 ──────────────────────────────────────────────────────
static lv_obj_t* _make_btn(lv_obj_t* p, const char* cn, const char* en, lv_event_cb_t cb) {
    lv_obj_t* b=xb_button(p,_T(cn,en),cb);
    lv_obj_set_width(b,140);
    lv_obj_set_user_data(b,(void*)(_T(cn,en)?cn:en));  // store tag for callback
    // Apply font to child label
    if(lv_obj_get_child_cnt(b)>0) lv_obj_set_style_text_font(lv_obj_get_child(b,0),_F(),0);
    return b;
}

// ══════════════════════════════════════════════════════════════════════
lv_obj_t* page_console_create(lv_obj_t* parent) {
    if(s_page) return s_page;
    lv_color_t fg=theme_fg(), bg=theme_bg();
    extern bool s_lang_cn;

    s_page=lv_obj_create(parent);
    lv_obj_set_size(s_page,320,240);lv_obj_center(s_page);
    lv_obj_set_style_bg_color(s_page,bg,0);lv_obj_set_style_border_width(s_page,0,0);
    lv_obj_set_style_pad_all(s_page,0,0);lv_obj_remove_flag(s_page,LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* tb=xb_topbar_create(s_page,_T("控制台","Console"),true);
    lv_obj_add_flag(tb,LV_OBJ_FLAG_CLICKABLE);lv_obj_add_event_cb(tb,on_back,LV_EVENT_CLICKED,NULL);
    if(lv_obj_get_child_cnt(tb)>=2)lv_obj_set_style_text_font(lv_obj_get_child(tb,1),_F(),0);

    // Tabview
    lv_obj_t* tv=lv_tabview_create(s_page);
    lv_tabview_set_tab_bar_position(tv,LV_DIR_TOP);lv_tabview_set_tab_bar_size(tv,24);
    lv_obj_set_size(tv,320,212);lv_obj_align(tv,LV_ALIGN_TOP_MID,0,28);
    lv_obj_set_style_bg_color(tv,bg,0);

    lv_obj_t* t1=lv_tabview_add_tab(tv,_T("系统","System"));
    lv_obj_t* t2=lv_tabview_add_tab(tv,_T("AI","AI"));
    lv_obj_t* t3=lv_tabview_add_tab(tv,_T("显示","Display"));
    lv_obj_t* t4=lv_tabview_add_tab(tv,_T("服务","Service"));
    // Font for tab bar
    lv_obj_t* tbar=lv_tabview_get_tab_bar(tv);
    if(tbar)lv_obj_set_style_text_font(tbar,_F(),0);

    // ── Tab 1: 系统 ──
    lv_obj_t* c1=lv_obj_create(t1);lv_obj_set_size(c1,320,180);
    lv_obj_set_style_bg_opa(c1,LV_OPA_TRANSP,0);lv_obj_set_style_border_width(c1,0,0);
    lv_obj_set_style_pad_all(c1,4,0);lv_obj_set_flex_flow(c1,LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_gap(c1,6,0);

    // Section: IMU
    lv_obj_t* imu_hdr=lv_label_create(c1);lv_obj_set_width(imu_hdr,304);
    lv_label_set_text(imu_hdr,_T("体感模拟","IMU Sim"));
    lv_obj_set_style_text_color(imu_hdr,fg,0);lv_obj_set_style_text_font(imu_hdr,_F(),0);
    _make_btn(c1,"前倾","Tilt Fwd",_btn_cb_imu);
    _make_btn(c1,"后仰","Tilt Back",_btn_cb_imu);
    _make_btn(c1,"左倾","Tilt Left",_btn_cb_imu);
    _make_btn(c1,"右倾","Tilt Right",_btn_cb_imu);
    _make_btn(c1,"摇晃","Shake",_btn_cb_imu);
    _make_btn(c1,"旋转","Rotate",_btn_cb_imu);

    // Section: Scenario
    lv_obj_t* sc_hdr=lv_label_create(c1);lv_obj_set_width(sc_hdr,304);
    lv_label_set_text(sc_hdr,_T("情景模拟","Scenario"));
    lv_obj_set_style_text_color(sc_hdr,fg,0);lv_obj_set_style_text_font(sc_hdr,_F(),0);
    _make_btn(c1,"语音唤醒","Wake",_btn_cb_scene);
    _make_btn(c1,"OTA升级","OTA",_btn_cb_scene);
    _make_btn(c1,"系统报错","Error",_btn_cb_scene);
    _make_btn(c1,"来电提醒","Call",_btn_cb_scene);
    _make_btn(c1,"低电警告","Low Bat",_btn_cb_scene);
    _make_btn(c1,"高温警报","Hot",_btn_cb_scene);

    // ── Tab 2: AI ──
    lv_obj_t* c2=lv_obj_create(t2);lv_obj_set_size(c2,320,180);
    lv_obj_set_style_bg_opa(c2,LV_OPA_TRANSP,0);lv_obj_set_style_border_width(c2,0,0);
    lv_obj_set_style_pad_all(c2,8,0);lv_obj_set_flex_flow(c2,LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(c2,8,0);

    lv_obj_t* ai_hdr=lv_label_create(c2);
    lv_label_set_text(ai_hdr,_T("AI 状态切换","AI State"));
    lv_obj_set_style_text_color(ai_hdr,fg,0);lv_obj_set_style_text_font(ai_hdr,_F(),0);
    _make_btn(c2,"空闲态","Idle",_btn_cb_face);
    _make_btn(c2,"思考态","Thinking",_btn_cb_face);
    _make_btn(c2,"说话态","Talking",_btn_cb_face);

    lv_obj_t* demo_l=lv_label_create(c2);
    lv_label_set_text(demo_l,_T("弹示例气泡:","Demo bubble:"));
    lv_obj_set_style_text_color(demo_l,fg,0);lv_obj_set_style_text_font(demo_l,_F(),0);
    lv_obj_t* bubble=xb_card(c2);lv_obj_set_width(bubble,280);
    lv_obj_t* bl=lv_label_create(bubble);
    lv_label_set_text(bl,_T("你好！我是小伴，有什么可以帮你的吗？","Hello! I'm xiaoBan, how can I help?"));
    lv_obj_set_style_text_color(bl,fg,0);lv_obj_set_style_text_font(bl,_F(),0);

    // ── Tab 3: 显示 ──
    lv_obj_t* c3=lv_obj_create(t3);lv_obj_set_size(c3,320,180);
    lv_obj_set_style_bg_opa(c3,LV_OPA_TRANSP,0);lv_obj_set_style_border_width(c3,0,0);
    lv_obj_set_style_pad_all(c3,8,0);lv_obj_set_flex_flow(c3,LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(c3,8,0);

    _make_btn(c3,"下一表情","Next Face",_btn_cb_expr);
    _make_btn(c3,"下一主题","Next Theme",_btn_cb_theme);
    lv_obj_t* sb_l=lv_label_create(c3);
    lv_label_set_text(sb_l,_T("状态栏模式:","StatusBar mode:"));
    lv_obj_set_style_text_color(sb_l,fg,0);lv_obj_set_style_text_font(sb_l,_F(),0);
    _make_btn(c3,"IDLE","IDLE",NULL);
    _make_btn(c3,"ALWAYS","ALWAYS",NULL);
    _make_btn(c3,"TRANSIENT","TRANSIENT",NULL);

    // ── Tab 4: 服务 ──
    lv_obj_t* c4=lv_obj_create(t4);lv_obj_set_size(c4,320,180);
    lv_obj_set_style_bg_opa(c4,LV_OPA_TRANSP,0);lv_obj_set_style_border_width(c4,0,0);
    lv_obj_set_style_pad_all(c4,8,0);lv_obj_set_flex_flow(c4,LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(c4,8,0);

    _make_btn(c4,"WiFi状态循环","WiFi Cycle",[](lv_event_t*){
        ESP_LOGI(TAG,"WiFi state cycle");
        expression_set(EXPR_HAPPY,true);
    });
    _make_btn(c4,"电量-20%","Battery-20%",[](lv_event_t*){
        ESP_LOGI(TAG,"Battery -20%%");
        expression_set(EXPR_SAD,true);
    });
    _make_btn(c4,"MCP切换","MCP Toggle",[](lv_event_t*){
        ESP_LOGI(TAG,"MCP toggle");
        expression_set(EXPR_THINKING,true);
    });
    _make_btn(c4,"OTA+20%%","OTA +20%%",[](lv_event_t*){
        ESP_LOGI(TAG,"OTA progress +20%%");
        expression_set(EXPR_EXCITED,true);
    });

    expression_set_drawing_enabled(false);
    ESP_LOGI(TAG,"Console v7.6 4-tab created");
    return s_page;
}
