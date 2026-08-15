// pomodoro.h — Pomodoro state machine + 1s tick + NVS persistence.
//
// States: IDLE → RUNNING ⇄ PAUSED → BREAK → IDLE. Focus 25:00, break 05:00.
// On completion the caller (app main loop) plays a beep + dialog_bubble +
// EXPR_HAPPY — the engine itself stays LVGL/hardware-free.
// Persisted to NVS key "pomo" = "<state>|<remaining_sec>|<task>"; on reboot a
// RUNNING/BREAK state is restored as PAUSED (no RTC, can't trust elapsed time).
// Persistence is throttled to ~every 5s + on every state transition.
// Thread-safe (mutex): tick (main loop, core0) vs collect (claw_core, core1).
#pragma once
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    POMO_IDLE = 0,
    POMO_RUNNING,
    POMO_PAUSED,
    POMO_BREAK,
} pomo_state_t;

#define POMO_TASK_MAX    64
#define POMO_FOCUS_SEC   (25 * 60)
#define POMO_BREAK_SEC   (5 * 60)

/* Call once at boot after app_config_init(). Restores persisted state
 * (RUNNING/BREAK→PAUSED). Does NOT auto-start a timer; pomodoro_tick() must be
 * polled every second (the app main loop does this). */
void pomodoro_init(void);

/* State control (thread-safe, persist on transition). */
void pomodoro_start(const char *task);   // begin focus with optional task name
void pomodoro_pause(void);
void pomodoro_resume(void);
void pomodoro_reset(void);               // stop, clear task, back to IDLE

/* Accessors (thread-safe). */
pomo_state_t pomodoro_state(void);
int pomodoro_remaining_seconds(void);
const char *pomodoro_task(void);

/* Advance the clock by 1 second. Poll from a 1s timer (the app main loop)
 * while RUNNING/BREAK. Returns true if a transition fired this tick (focus→break
 * or break→idle); the CALLER renders the completion beep/bubble/expression so
 * all LVGL + hardware access stays on the main task. Safe to call every second
 * even when idle (no-op). */
bool pomodoro_tick(void);

/* Heap-allocated CN snapshot for the LLM system prompt, e.g.
 *   "当前番茄钟：专注中，剩余 14:32，任务：写周报"
 * Returns NULL when IDLE. Caller MUST free() the result. */
char *pomodoro_snapshot_cn(void);

#ifdef __cplusplus
}
#endif
