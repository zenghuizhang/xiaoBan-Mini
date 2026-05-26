// xb_face.c — 2-eye + mouth geometric face. v7.6 color = accent_hi.
#include "xb_face.h"
#include "xb_theme.h"
#include "xb_event.h"

typedef struct {
    int eye_w, eye_h;     // each eye dim (px)
    int eye_gap;          // gap between eyes
    int eye_dy;           // vertical offset from center
    int mouth_w, mouth_h; // mouth dim
    int mouth_dy;         // vertical offset
    int mouth_r;          // mouth corner radius (rect) or 0 -> arc
    bool blink_eligible;  // whether random 150ms blink is allowed
} face_geom_t;

#define EYE(w,h,gap,dy)  .eye_w=(w),  .eye_h=(h),  .eye_gap=(gap), .eye_dy=(dy)
#define MOUTH(w,h,dy,r)  .mouth_w=(w),.mouth_h=(h),.mouth_dy=(dy), .mouth_r=(r)

// Geometry table for all 24 variants (scale 1.6 already applied).
// Source: React prototype src/components/Face.tsx VARIANTS map.
static const face_geom_t G[FACE_VARIANT_MAX] = {
    [FACE_IDLE]       = { EYE(32,32,48,0),  MOUTH(24, 4,28,2), .blink_eligible=true  },
    [FACE_BREATH]     = { EYE(32,32,48,0),  MOUTH(24, 4,28,2), .blink_eligible=true  },
    [FACE_HAPPY]      = { EYE(32,20,48,-4), MOUTH(40,12,28,6), .blink_eligible=false },
    [FACE_TALKING]    = { EYE(32,32,48,0),  MOUTH(28,16,28,8), .blink_eligible=true  },
    [FACE_LOOK_LEFT]  = { EYE(32,32,48,0),  MOUTH(24, 4,28,2), .blink_eligible=true  },
    [FACE_LOOK_RIGHT] = { EYE(32,32,48,0),  MOUTH(24, 4,28,2), .blink_eligible=true  },
    [FACE_CURIOUS]    = { EYE(28,36,48,-2), MOUTH(20, 4,32,2), .blink_eligible=true  },
    [FACE_THINKING]   = { EYE(28,32,48,-2), MOUTH(20, 4,32,2), .blink_eligible=true  },
    [FACE_ALERT]      = { EYE(36,36,52,0),  MOUTH(28, 4,28,2), .blink_eligible=true  },
    [FACE_LOST]       = { EYE(28,16,48,8),  MOUTH(28, 8,32,6), .blink_eligible=true  },
    [FACE_ANGRY]      = { EYE(32,16,48,-4), MOUTH(32, 6,32,4), .blink_eligible=true  },
    [FACE_DIZZY]      = { EYE(28,28,48,0),  MOUTH(24, 8,32,6), .blink_eligible=false },
    [FACE_WINK]       = { EYE(32,32,48,0),  MOUTH(28, 8,28,4), .blink_eligible=false },
    [FACE_CELEBRATE]  = { EYE(28,28,48,-4), MOUTH(40,16,28,8), .blink_eligible=false },
    [FACE_EXCITED]    = { EYE(36,36,52,-4), MOUTH(36,16,28,8), .blink_eligible=false },
    [FACE_CRYING]     = { EYE(28,20,48,8),  MOUTH(20, 8,32,6), .blink_eligible=false },
    [FACE_NAUGHTY]    = { EYE(32,28,48,-2), MOUTH(28,12,28,6), .blink_eligible=false },
    [FACE_SLEEP_WAKE] = { EYE(32,20,48,4),  MOUTH(24, 8,28,4), .blink_eligible=false },
    [FACE_YAWN]       = { EYE(32,16,48,0),  MOUTH(28,36,28,8), .blink_eligible=false },  // big yawn
    [FACE_SURPRISED]  = { EYE(36,36,52,0),  MOUTH(20,20,28,10),.blink_eligible=false },
    [FACE_DEEP_SLEEP] = { EYE(32, 2,48,0),  MOUTH(24, 2,28,1), .blink_eligible=false },
    [FACE_MENU]       = { EYE(28,28,52,0),  MOUTH(20, 4,28,2), .blink_eligible=true  },
    [FACE_BOOT_MID]   = { EYE(32,16,48,0),  MOUTH(28,36,28,8), .blink_eligible=false },  // half open + yawning
    [FACE_BOOT_OPEN]  = { EYE(32,64,48,0),  MOUTH(24, 4,28,2), .blink_eligible=false },  // fully open
};

static lv_obj_t* g_root  = NULL;
static lv_obj_t* g_eyeL  = NULL;
static lv_obj_t* g_eyeR  = NULL;
static lv_obj_t* g_mouth = NULL;
static face_variant_t g_current = FACE_IDLE;
static lv_timer_t* g_blink_timer = NULL;

static void apply_geom(face_variant_t v) {
    if (!g_eyeL || !g_eyeR || !g_mouth) return;
    const face_geom_t* g = &G[v];
    const theme_t* th = xb_theme_get();
    lv_color_t c = th->accent_hi;     // v7.6: use accent_hi, not accent

    int cx = 160, cy = 130;
    lv_obj_set_size(g_eyeL, g->eye_w, g->eye_h);
    lv_obj_set_size(g_eyeR, g->eye_w, g->eye_h);
    lv_obj_set_pos(g_eyeL, cx - g->eye_gap/2 - g->eye_w, cy + g->eye_dy);
    lv_obj_set_pos(g_eyeR, cx + g->eye_gap/2,           cy + g->eye_dy);
    lv_obj_set_style_radius(g_eyeL, g->eye_h/2, 0);
    lv_obj_set_style_radius(g_eyeR, g->eye_h/2, 0);
    lv_obj_set_style_bg_color(g_eyeL, c, 0);
    lv_obj_set_style_bg_color(g_eyeR, c, 0);
    lv_obj_set_style_shadow_color(g_eyeL, c, 0);
    lv_obj_set_style_shadow_color(g_eyeR, c, 0);
    lv_obj_set_style_shadow_width(g_eyeL, 20, 0);
    lv_obj_set_style_shadow_width(g_eyeR, 20, 0);
    lv_obj_set_style_shadow_opa(g_eyeL, 0xB3, 0);
    lv_obj_set_style_shadow_opa(g_eyeR, 0xB3, 0);

    lv_obj_set_size(g_mouth, g->mouth_w, g->mouth_h);
    lv_obj_set_pos(g_mouth, cx - g->mouth_w/2, cy + g->mouth_dy);
    lv_obj_set_style_radius(g_mouth, g->mouth_r, 0);
    lv_obj_set_style_bg_color(g_mouth, c, 0);
    lv_obj_set_style_shadow_color(g_mouth, c, 0);
    lv_obj_set_style_shadow_width(g_mouth, 18, 0);
    lv_obj_set_style_shadow_opa(g_mouth, 0xB3, 0);
}

// 150ms blink reopen — plain-C callback (was a C++ lambda; ESP-IDF compiles C99)
static void reopen_cb(lv_timer_t* tt) {
    apply_geom(g_current);
    lv_timer_del(tt);
}

static void blink_tick(lv_timer_t* t) {
    (void)t;
    if (!G[g_current].blink_eligible) return;
    if (!g_eyeL || !g_eyeR) return;
    lv_obj_set_height(g_eyeL, 2);
    lv_obj_set_height(g_eyeR, 2);
    lv_timer_create(reopen_cb, 150, NULL);
}

static void on_theme(xb_event_id_t id, void* p, void* u) {
    (void)id; (void)p; (void)u;
    apply_geom(g_current);
}

static void style_child(lv_obj_t* o) {
    lv_obj_set_style_border_width(o, 0, 0);
    lv_obj_set_style_pad_all(o, 0, 0);
    lv_obj_remove_flag(o, LV_OBJ_FLAG_SCROLLABLE);
}

lv_obj_t* xb_face_create(lv_obj_t* parent) {
    g_root = lv_obj_create(parent);
    lv_obj_set_size(g_root, 320, 240);
    lv_obj_center(g_root);
    lv_obj_set_style_bg_opa(g_root, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(g_root, 0, 0);
    lv_obj_set_style_pad_all(g_root, 0, 0);
    lv_obj_remove_flag(g_root, LV_OBJ_FLAG_SCROLLABLE);

    g_eyeL  = lv_obj_create(g_root);
    g_eyeR  = lv_obj_create(g_root);
    g_mouth = lv_obj_create(g_root);
    style_child(g_eyeL);
    style_child(g_eyeR);
    style_child(g_mouth);
    apply_geom(FACE_IDLE);

    // random blink ~4s
    g_blink_timer = lv_timer_create(blink_tick, 4000, NULL);
    xb_event_subscribe(XB_EVT_THEME_CHANGED, on_theme, NULL);
    return g_root;
}

void xb_face_destroy(void) {
    if (g_blink_timer) { lv_timer_del(g_blink_timer); g_blink_timer = NULL; }
    xb_event_unsubscribe(on_theme, NULL);
    if (g_root) lv_obj_delete(g_root);
    g_root = g_eyeL = g_eyeR = g_mouth = NULL;
}

void xb_face_set(face_variant_t v) {
    if (v >= FACE_VARIANT_MAX) return;
    g_current = v;
    apply_geom(v);
    xb_event_post(XB_EVT_FACE_REQUEST, &v);
}

face_variant_t xb_face_get(void) { return g_current; }
void xb_face_redraw(void)        { apply_geom(g_current); }
