// content_safety.h — Child-safety content filter for the parenting persona.
//
// Phase 1: keyword blocklist (violence / adult / sensitive / religion / politics).
// Phase 2+: add LLM-based secondary review. The filter is intentionally strict
// for child mode; parent mode uses a lighter list.
#pragma once
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SAFETY_MODE_CHILD = 0,   // strict: only game-appropriate content
    SAFETY_MODE_PARENT,      // lighter: block adult/violence/sensitive only
} safety_mode_t;

/* Returns true if `text` passes the filter for the given mode. */
bool content_safety_check(const char *text, safety_mode_t mode);

/* Convenience: child-mode check. */
bool content_safety_child_ok(const char *text);

#ifdef __cplusplus
}
#endif
