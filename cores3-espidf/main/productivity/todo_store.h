// todo_store.h — In-memory todo list + mutex + NVS persistence.
//
// Backed by settings_store (namespace "xiaoban", key "todos"). Persisted as
// one string of lines "<0|1>\t<text>\n", rewritten atomically on every change.
// Thread-safe: collect (claw_core worker, core1) and UI mutations (core0) both
// go through the mutex. Call todo_store_init() once after app_config_init().
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TODO_MAX_ITEMS  24
#define TODO_TEXT_MAX   64

/* Load todos from NVS. Call once at boot after app_config_init(). */
void todo_store_init(void);

/* Item counts (thread-safe). */
int todo_store_count(void);          /* total */
int todo_store_pending_count(void);  /* incomplete only */

/* Get item at idx. *done is set to the done flag. Returns a pointer to the
 * text (valid until the next mutation) or NULL if idx is out of range. */
const char *todo_store_get(int idx, bool *done);

/* Mutations (thread-safe, persist to NVS). */
esp_err_t todo_store_add(const char *text);
esp_err_t todo_store_toggle(int idx);
esp_err_t todo_store_delete(int idx);
esp_err_t todo_store_clear_done(void);

/* Heap-allocated CN snapshot of PENDING todos for the LLM system prompt, e.g.
 *   "【今日待办】\n1. 写周报\n2. 15:00 开会"
 * Returns NULL when there are no pending todos. Caller MUST free() the result. */
char *todo_store_snapshot_cn(void);

#ifdef __cplusplus
}
#endif
