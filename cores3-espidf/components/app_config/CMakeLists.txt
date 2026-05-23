idf_component_register(
    SRCS "app_config.c"
    INCLUDE_DIRS "include" "../../components/memory_store/include"
    REQUIRES "nvs_flash" "memory_store"
)
