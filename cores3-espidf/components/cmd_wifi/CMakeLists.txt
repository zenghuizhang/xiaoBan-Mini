idf_component_register(
    SRCS "cmd_wifi.c"
    INCLUDE_DIRS "include"
    REQUIRES "esp_wifi" "app_config" "wifi_manager"
)
