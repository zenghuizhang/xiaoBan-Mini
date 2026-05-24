// page_wifi_pair.cpp — 6-digit cloud pairing code page (v6.2.1)
// User reads code from screen and types into companion app.
// Alternative to AP+Captive portal flow.
#include "page_wifi_pair.h"
#include "xb_widgets.h"
#include "theme_v3.h"

// ── defaults (override on device) ────────────────────────────────────────────
static const char* s_pair_code = "73K9X2";   // 6-char alphanumeric code
static int         s_pair_ttl   = 300;       // seconds remaining

void page_wifi_pair_set_code(const char* code) { s_pair_code = code; }
void page_wifi_pair_set_ttl(int secs)        { s_pair_ttl = secs;   }

// ── callbacks ─────────────────────────────────────────────────────────────────
static void _on_back(lv_event_t* e)
{
    (void)e;
    // Walk up to find page root and delete it
    lv_obj_t* t = (lv_obj_t*)lv_event_get_target(e);
    while (t) {
        lv_obj_t* p = lv_obj_get_parent(t);
        if (!p || p == lv_screen_active()) { lv_obj_delete(t); return; }
        t = p;
    }
}

// ── page builder ─────────────────────────────────────────────────────────────
lv_obj_t* page_wifi_pair_create(lv_obj_t* parent)
{
    lv_color_t fg = theme_fg();
    lv_color_t bg = theme_bg();

    // Page root
    lv_obj_t* p = lv_obj_create(parent);
    lv_obj_set_size(p, 320, 240);
    lv_obj_center(p);
    lv_obj_set_style_bg_color(p, bg, 0);
    lv_obj_set_style_border_width(p, 0, 0);
    lv_obj_set_style_pad_all(p, 0, 0);
    lv_obj_remove_flag(p, LV_OBJ_FLAG_SCROLLABLE);

    // Statusbar + Topbar (back arrow triggers page delete)
    xb_statusbar_create(p);
    lv_obj_t* tb = xb_topbar_create(p, "Pair", true);
    lv_obj_add_flag(tb, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(tb, _on_back, LV_EVENT_CLICKED, NULL);

    // Hint line
    lv_obj_t* hint = lv_label_create(p);
    lv_label_set_text(hint, "Enter this code in the app:");
    lv_obj_set_style_text_color(hint, fg, 0);
    lv_obj_set_style_text_opa(hint, LV_OPA_60, 0);
    lv_obj_align(hint, LV_ALIGN_TOP_MID, 0, 78);

    // 6 digit boxes in a row
    lv_obj_t* row = lv_obj_create(p);
    lv_obj_set_size(row, 300, 56);
    lv_obj_align(row, LV_ALIGN_CENTER, 0, 10);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_EVENLY,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    for (int i = 0; i < 6; i++) {
        lv_obj_t* box = xb_card(row);
        lv_obj_set_size(box, 38, 50);
        lv_obj_set_style_pad_all(box, 0, 0);

        lv_obj_t* d = lv_label_create(box);
        char s[2] = { s_pair_code[i], 0 };
        lv_label_set_text(d, s);
        lv_obj_set_style_text_color(d, fg, 0);
        lv_obj_set_style_text_font(d, &lv_font_montserrat_14, 0);
        lv_obj_center(d);
    }

    // Countdown footer (static for now; real device would update via timer)
    lv_obj_t* tip = lv_label_create(p);
    lv_label_set_text_fmt(tip, "Code refreshes in %d:%02d",
                          s_pair_ttl / 60, s_pair_ttl % 60);
    lv_obj_set_style_text_color(tip, fg, 0);
    lv_obj_set_style_text_opa(tip, LV_OPA_50, 0);
    lv_obj_align(tip, LV_ALIGN_BOTTOM_MID, 0, -20);

    return p;
}
