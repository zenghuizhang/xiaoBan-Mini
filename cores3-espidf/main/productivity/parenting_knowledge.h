// parenting_knowledge.h — Built-in parenting knowledge base for the mentor persona.
//
// Phase 3: a small curated set of age-appropriate parenting tips (3-6 years)
// plus a daily-tip picker. Parent Q&A is handled by the LLM with the mentor
// system prompt; this module provides the curated fallback + daily tip.
#pragma once
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Returns a daily parenting tip for the child's age group (3-4 or 5-6).
 * The tip rotates by day-of-year so it changes daily. */
const char *parenting_daily_tip(void);

/* Returns the total number of curated tips (for progress/UI). */
int parenting_tip_count(void);

#ifdef __cplusplus
}
#endif
