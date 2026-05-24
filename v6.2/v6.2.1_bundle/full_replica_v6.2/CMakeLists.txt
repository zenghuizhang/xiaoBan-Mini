# CMakeLists.txt for ESP-IDF component "xb_ui".
#
# Drop into your project as components/xb_ui/CMakeLists.txt and the entire
# UI tree (core / widgets / pages / asset stubs) will compile against
# lvgl + esp_lvgl_port.
idf_component_register(
    SRCS
        "src/main.c"
        "src/core/xb_event.c"
        "src/core/xb_theme.c"
        "src/core/xb_face.c"
        "src/widgets/xb_widgets.c"
        "src/pages/xb_router.c"
        "src/pages/page_boot.c"
        "src/pages/page_home.c"
        "src/pages/page_menu.c"
        "src/pages/page_chat.c"
        "src/pages/page_wifi_ap.c"
        "src/pages/page_wifi_pair.c"
        "src/pages/page_ota.c"
        "src/pages/page_skills.c"
        "src/pages/page_skill_detail.c"
        "src/pages/page_settings.c"
        "src/pages/page_console.c"
        "src/assets/xb_icons_stub.c"
    INCLUDE_DIRS
        "src"
        "src/core"
        "src/widgets"
        "src/pages"
        "assets/themes"
    REQUIRES
        lvgl
        esp_lvgl_port
)
