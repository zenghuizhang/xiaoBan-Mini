// productivity_ctx.h — claw_core context provider bridging productivity state
// (todos + pomodoro) into the LLM system prompt.
#pragma once
#include "claw_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Returns a pointer to the static "productivity" context provider.
 * Register via claw_core_add_context_provider() between claw_core_init and
 * claw_core_start. The collect callback injects a CN snapshot of today's
 * todos + running pomodoro into the system prompt; returns ESP_ERR_NOT_FOUND
 * when there is nothing to report (no todos, pomodoro idle) so claw_core
 * skips it. */
const claw_core_context_provider_t* productivity_context_provider(void);

#ifdef __cplusplus
}
#endif
