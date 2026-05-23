# icons_pack 作为 ESP-IDF 组件
# 用法：把整个 icons_pack/ 目录放到工程的 components/ 下即可被自动发现。

idf_component_register(
    SRCS
        "c/icon_smile.c"
        "c/icon_message.c"
        "c/icon_settings.c"
        "c/icon_palette.c"
        "c/icon_puzzle.c"
        "c/icon_shuffle.c"
        "c/menu_icons.c"
    INCLUDE_DIRS
        "include"
    REQUIRES
        lvgl                # LVGL 9.x（managed component 名一般为 lvgl__lvgl，按工程实际改）
)
