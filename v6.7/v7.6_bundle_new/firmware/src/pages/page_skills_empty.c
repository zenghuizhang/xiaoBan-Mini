// page_skills_empty.c — placeholder for v7.7 Skills marketplace.
// Skills was demoted from a menu sector in v7.6; we keep this page reachable
// from Settings > 关于 to preview the upcoming surface.
#include "xb_pages.h"
#include "../core/xb_theme.h"
#include "../widgets/xb_widgets.h"

static void back_cb(lv_event_t* e) { (void)e; xb_router_back(); }

lv_obj_t* page_skills_empty_create(lv_obj_t* parent) {
    const theme_t* th = xb_theme_get();
    lv_obj_t* root = lv_obj_create(parent);
    lv_obj_set_size(root, 320, 240);
    lv_obj_center(root);
    lv_obj_set_style_bg_opa(root, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(root, 0, 0);

    xb_topbar(root, "技能", back_cb);

    lv_obj_t* l = lv_label_create(root);
    lv_label_set_text(l,
        "技能商店\n\n即将在 v7.7 上线\n敬请期待。");
    lv_obj_set_style_text_color(l, th->text_dim, 0);
    lv_obj_center(l);
    return root;
}
