// page_boot.cpp — Boot animation page (v6.2.1)
// Wraps/improves existing boot_anim.c with StatusBar+TopBar layout matching
// the full_replica_v6.2 style.
//
// Timeline:  0..600ms    fade-in brand mark
//           600..1800ms  ring sweep (arc 0→360°)
//          1800..3000ms  hold + bottom tag visible
//          3000..3500ms  fade-out → callback
//
// Theme-aware: Tech uses cyan arc, Child uses coral arc, Dev uses green arc.
#include "page_boot.h"
#include "xb_widgets.h"
#include "theme_v3.h"

// ── animation data ───────────────────────────────────────────────────────────
typedef struct {
    lv_obj_t* page;
    lv_obj_t* mark;     // brand label
    lv_obj_t* arc;      // loading arc
    lv_obj_t* tag;      // version tag
    void (*on_done)(void);
} boot_ctx_t;

// ── anim execution callbacks (LVGL 8.x compat) ───────────────────────────────
static void _anim_opa(void* var, int32_t v)
{
    lv_obj_set_style_opa((lv_obj_t*)var, (lv_opa_t)v, 0);
}

static void _anim_arc_end(void* var, int32_t v)
{
    lv_arc_set_end_angle((lv_obj_t*)var, (uint16_t)v);
}

static void _boot_done_cb(lv_anim_t* a)
{
    boot_ctx_t* ctx = (boot_ctx_t*)lv_anim_get_user_data(a);
    if (!ctx) return;
    if (ctx->on_done) ctx->on_done();
}

// ── page builder ─────────────────────────────────────────────────────────────
lv_obj_t* page_boot_create(lv_obj_t* parent, void (*on_done)(void))
{
    lv_color_t fg = theme_fg();
    lv_color_t bg = theme_bg();

    // Allocate context
    boot_ctx_t* ctx = (boot_ctx_t*)lv_malloc(sizeof(boot_ctx_t));
    if (!ctx) return NULL;

    // Page root — fullscreen, no scrolling
    lv_obj_t* p = lv_obj_create(parent);
    lv_obj_set_size(p, 320, 240);
    lv_obj_center(p);
    lv_obj_set_style_bg_color(p, bg, 0);
    lv_obj_set_style_border_width(p, 0, 0);
    lv_obj_set_style_pad_all(p, 0, 0);
    lv_obj_remove_flag(p, LV_OBJ_FLAG_SCROLLABLE);

    ctx->page    = p;
    ctx->on_done = on_done;

    // ── StatusBar + TopBar ──
    xb_statusbar_create(p);
    lv_obj_t* tb = xb_topbar_create(p, "xiaobao", false);
    lv_obj_set_style_text_opa(tb, LV_OPA_40, 0);

    // ── Content area (below topbar: y=50..240) ──
    lv_obj_t* content = lv_obj_create(p);
    lv_obj_set_size(content, 320, 190);
    lv_obj_align(content, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_pad_all(content, 0, 0);
    lv_obj_remove_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    // ── Brand mark label (centered, initially transparent) ──
    ctx->mark = lv_label_create(content);
    lv_label_set_text(ctx->mark, "XB");
    lv_obj_set_style_text_color(ctx->mark, fg, 0);
    lv_obj_set_style_text_font(ctx->mark, &lv_font_montserrat_14, 0);
    lv_obj_align(ctx->mark, LV_ALIGN_CENTER, 0, -20);
    lv_obj_set_style_opa(ctx->mark, LV_OPA_TRANSP, 0);

    // Fade-in: 0→600ms
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, ctx->mark);
    lv_anim_set_values(&a, LV_OPA_TRANSP, LV_OPA_COVER);
    lv_anim_set_time(&a, 600);
    lv_anim_set_exec_cb(&a, _anim_opa);
    lv_anim_start(&a);

    // ── Loading arc (ring sweep, initially empty) ──
    ctx->arc = lv_arc_create(content);
    lv_obj_set_size(ctx->arc, 80, 80);
    lv_obj_align(ctx->arc, LV_ALIGN_CENTER, 0, -20);
    lv_obj_remove_style_all(ctx->arc);
    lv_obj_set_style_arc_color(ctx->arc, fg, 0);
    lv_obj_set_style_arc_width(ctx->arc, 4, 0);
    lv_obj_set_style_arc_opa(ctx->arc, LV_OPA_30, 0);
    lv_obj_set_style_bg_opa(ctx->arc, LV_OPA_TRANSP, 0);
    lv_arc_set_range(ctx->arc, 0, 3600);  // tenths of degree
    lv_arc_set_bg_angles(ctx->arc, 0, 3600);
    lv_arc_set_rotation(ctx->arc, 270);    // start from top
    lv_arc_set_end_angle(ctx->arc, 0);

    // Ring sweep: 600→1800ms (0→360°)
    lv_anim_init(&a);
    lv_anim_set_var(&a, ctx->arc);
    lv_anim_set_values(&a, 0, 3600);
    lv_anim_set_time(&a, 1200);
    lv_anim_set_delay(&a, 600);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
    lv_anim_set_exec_cb(&a, _anim_arc_end);
    lv_anim_start(&a);

    // ── Version tag (bottom, fades in at 1800ms) ──
    ctx->tag = lv_label_create(content);
    lv_label_set_text(ctx->tag, "xiaobao v6.2");
    lv_obj_set_style_text_color(ctx->tag, fg, 0);
    lv_obj_set_style_text_opa(ctx->tag, LV_OPA_60, 0);
    lv_obj_set_style_text_font(ctx->tag, &lv_font_montserrat_14, 0);
    lv_obj_align(ctx->tag, LV_ALIGN_BOTTOM_MID, 0, -8);

    // ── Page fade-out: 3000→3500ms (LVGL 8.x: only opa, no time field) ──
    // We use a delayed animation on the page root
    lv_anim_init(&a);
    lv_anim_set_var(&a, p);
    lv_anim_set_values(&a, LV_OPA_COVER, LV_OPA_TRANSP);
    lv_anim_set_time(&a, 500);
    lv_anim_set_delay(&a, 3000);
    lv_anim_set_exec_cb(&a, _anim_opa);
    lv_anim_set_user_data(&a, ctx);
    lv_anim_set_ready_cb(&a, _boot_done_cb);
    lv_anim_start(&a);

    return p;
}
