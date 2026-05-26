// xb_widgets.c — v7.6 widgets, theme_colors_t + is_light
#include "xb_widgets.h"
#include "theme_v3.h"
#define C theme_get_colors()

lv_obj_t* xb_card(lv_obj_t* p){
    lv_obj_t* o=lv_obj_create(p);
    lv_obj_set_style_radius(o,4,0);lv_obj_set_style_bg_color(o,C->panel,0);
    lv_obj_set_style_bg_opa(o,C->is_light?LV_OPA_COVER:0x4D,0);
    lv_obj_set_style_border_color(o,C->border,0);lv_obj_set_style_border_width(o,1,0);
    lv_obj_set_style_pad_all(o,6,0);lv_obj_remove_flag(o,LV_OBJ_FLAG_SCROLLABLE);
    return o;
}
lv_obj_t* xb_button(lv_obj_t* p,const char* t,lv_event_cb_t cb){
    lv_obj_t* b=lv_button_create(p);lv_obj_set_style_bg_color(b,C->accent,0);
    lv_obj_set_style_radius(b,4,0);lv_obj_set_style_shadow_width(b,0,0);lv_obj_set_style_pad_hor(b,12,0);
    lv_obj_t* l=lv_label_create(b);lv_label_set_text(l,t);lv_obj_set_style_text_color(l,C->bg,0);
    lv_obj_center(l);if(cb)lv_obj_add_event_cb(b,cb,LV_EVENT_CLICKED,NULL);
    return b;
}
lv_obj_t* xb_topbar_create(lv_obj_t* p,const char* t,bool back){
    lv_obj_t* tb=lv_obj_create(p);lv_obj_set_size(tb,320,28);
    lv_obj_align(tb,LV_ALIGN_TOP_MID,0,0);
    lv_obj_set_style_bg_color(tb,C->panel,0);lv_obj_set_style_bg_opa(tb,C->is_light?LV_OPA_COVER:0x4D,0);
    lv_obj_set_style_border_width(tb,0,0);lv_obj_remove_flag(tb,LV_OBJ_FLAG_SCROLLABLE);
    if(back){lv_obj_t* b2=lv_label_create(tb);lv_label_set_text(b2,"<");lv_obj_set_style_text_color(b2,C->accent,0);lv_obj_align(b2,LV_ALIGN_LEFT_MID,6,0);}
    lv_obj_t* tl=lv_label_create(tb);lv_label_set_text(tl,t);lv_obj_set_style_text_color(tl,C->accent,0);lv_obj_align(tl,LV_ALIGN_CENTER,0,0);
    return tb;
}
static void _dot_cb(void* v, int32_t val) { lv_obj_set_style_opa((lv_obj_t*)v, val, 0); }
lv_obj_t* xb_dot_loading_create(lv_obj_t* p){
    lv_obj_t* row=xb_card(p);lv_obj_set_size(row,42,20);lv_obj_set_flex_flow(row,LV_FLEX_FLOW_ROW);lv_obj_set_style_pad_gap(row,4,0);
    for(int i=0;i<3;i++){lv_obj_t* d=lv_obj_create(row);lv_obj_set_size(d,6,6);lv_obj_set_style_radius(d,3,0);
        lv_obj_set_style_bg_color(d,C->text,0);lv_obj_set_style_border_width(d,0,0);
        lv_anim_t a;lv_anim_init(&a);lv_anim_set_var(&a,d);lv_anim_set_values(&a,LV_OPA_30,LV_OPA_COVER);
        lv_anim_set_duration(&a,400);lv_anim_set_playback_duration(&a,400);
        lv_anim_set_repeat_count(&a,LV_ANIM_REPEAT_INFINITE);lv_anim_set_delay(&a,i*200);
        lv_anim_set_exec_cb(&a,_dot_cb);lv_anim_start(&a);
    }
    return row;
}
lv_obj_t* xb_statusbar_create(lv_obj_t* p){lv_obj_t* o=lv_obj_create(p);lv_obj_set_size(o,320,16);lv_obj_set_style_bg_opa(o,LV_OPA_TRANSP,0);lv_obj_set_style_border_width(o,0,0);return o;}
lv_obj_t* xb_error_inline(lv_obj_t* p,const char* t){
    lv_obj_t* o=xb_card(p);lv_obj_set_style_bg_color(o,C->danger,0);lv_obj_set_style_bg_opa(o,LV_OPA_20,0);
    lv_obj_set_style_border_color(o,C->danger,0);lv_obj_t* l=lv_label_create(o);lv_label_set_text(l,t);
    lv_obj_set_style_text_color(l,C->danger,0);lv_obj_align(l,LV_ALIGN_CENTER,0,0);
    return o;
}
