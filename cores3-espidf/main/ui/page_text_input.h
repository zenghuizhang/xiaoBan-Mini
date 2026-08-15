// page_text_input.h — Reusable full-screen keyboard input page (ASCII only)
#pragma once
#include "lvgl.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Callback fired on Save with the entered text (NUL-terminated, UTF-8/ASCII).
 * Called after the input page is already closed and freed; user_ctx is passed
 * through verbatim. The text buffer is only valid for the duration of the call. */
typedef void (*page_text_input_done_cb)(const char *text, void *user_ctx);

/* Create a full-screen text input page on `parent` (usually lv_screen_active()).
 *  title         : topbar title (may be CN/EN string)
 *  initial_value : pre-filled text (NULL → empty)
 *  max_len       : max chars (<=0 → 127)
 *  password_mode : mask display (for API keys)
 *  on_done_cb    : called on Save (NULL → just close)
 *  user_ctx      : opaque pointer passed to on_done_cb
 * Back ("<") / cancel = close without callback. Save = callback then close. */
lv_obj_t* page_text_input_create(lv_obj_t* parent,
                                 const char* title,
                                 const char* initial_value,
                                 int max_len,
                                 bool password_mode,
                                 page_text_input_done_cb on_done_cb,
                                 void* user_ctx);

#ifdef __cplusplus
}
#endif
