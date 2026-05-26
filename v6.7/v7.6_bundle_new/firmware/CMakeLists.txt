# CMakeLists.txt — xb_ui ESP-IDF component for claw_xb v7.6.
#
# Place this entire folder at:   <esp-idf-project>/components/xb_ui/
# Then in your top-level project:
#
#   set(COMPONENTS xb_ui main lvgl esp_lvgl_port nvs_flash ...)
#
# The component is self-contained — it has NO HW dependency other than NVS.

idf_component_register(
    SRCS
        # core
        "src/core/xb_event.c"
        "src/core/xb_theme.c"
        "src/core/xb_face.c"
        # widgets
        "src/widgets/xb_widgets.c"
        # sim
        "src/sim/xb_persona.c"
        "src/sim/xb_memory_store.c"
        "src/sim/xb_imu_sim.c"
        "src/sim/xb_scenario_sim.c"
        # pages
        "src/pages/xb_router.c"
        "src/pages/page_boot.c"
        "src/pages/page_home.c"
        "src/pages/page_menu.c"
        "src/pages/page_chat.c"
        "src/pages/page_model_picker.c"
        "src/pages/page_theme_picker.c"
        "src/pages/page_persona_grid.c"
        "src/pages/page_memory_browser.c"
        "src/pages/page_settings.c"
        "src/pages/page_wifi_ap.c"
        "src/pages/page_skills_empty.c"
        "src/pages/page_console.c"
        # assets
        "src/assets/xb_icons_stub.c"
    INCLUDE_DIRS
        "src"
        "src/core"
        "src/widgets"
        "src/sim"
        "src/pages"
        "assets/themes"
    REQUIRES
        lvgl
        esp_lvgl_port
        nvs_flash
)

# Optional: enable sqlite-backed memory store if esp_sqlite is available.
# target_compile_definitions(${COMPONENT_LIB} PRIVATE XB_HAS_SQLITE=1)
