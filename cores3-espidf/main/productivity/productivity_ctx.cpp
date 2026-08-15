// productivity_ctx.cpp — claw_core context provider for productivity state.
//
// collect() merges todo_store_snapshot_cn() (pending todos) and
// pomodoro_snapshot_cn() (running timer) into one heap CN string for claw_core
// to append as "\n\n## productivity\n<content>". Returns ESP_ERR_NOT_FOUND when
// there is nothing to report (no todos, pomodoro idle) so claw_core skips it.
#include "productivity_ctx.h"
#include "todo_store.h"
#include "pomodoro.h"
#include <esp_log.h>
#include <string.h>
#include <stdlib.h>

static const char* TAG = "PROD_CTX";

static esp_err_t _collect(const claw_core_request_t* request,
                          claw_core_context_t* out_context,
                          void* user_ctx)
{
    (void)request; (void)user_ctx;
    if (!out_context) return ESP_ERR_INVALID_ARG;

    // Inject pending todos + a running pomodoro snapshot. Either may be NULL
    // (nothing to report); claw_core skips this provider only when both are.
    char *todo = todo_store_snapshot_cn();  // NULL when nothing pending
    char *pomo = pomodoro_snapshot_cn();    // NULL when IDLE

    if (!todo && !pomo) {
        return ESP_ERR_NOT_FOUND;  // claw_core skips this provider
    }

    out_context->kind = CLAW_CORE_CONTEXT_KIND_SYSTEM_PROMPT;

    if (todo && pomo) {
        // Both present: merge into one heap string (todo + newline + pomo).
        // On malloc failure, hand over just the todo snapshot and drop pomo.
        size_t n = strlen(todo) + 1 + strlen(pomo) + 1;
        char *merged = (char *)malloc(n);
        if (merged) {
            snprintf(merged, n, "%s\n%s", todo, pomo);
            out_context->content = merged;  // claw_core frees this
            free(todo);
            free(pomo);
        } else {
            out_context->content = todo;    // hand over; do NOT free
            free(pomo);
        }
    } else {
        // Exactly one present: hand it over (claw_core frees it).
        out_context->content = todo ? todo : pomo;
    }
    return ESP_OK;
}

static const claw_core_context_provider_t s_provider = {
    .name      = "productivity",
    .collect   = _collect,
    .user_ctx  = NULL,
    .flags     = CLAW_CORE_CONTEXT_PROVIDER_FLAG_REQUEST_START_ONLY,
};

const claw_core_context_provider_t* productivity_context_provider(void)
{
    return &s_provider;
}
