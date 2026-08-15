// play_time.h — Child play-time limits (compliance: Article 18 of the
// 拟人化互动办法: 2-hour reminder + anti-addiction).
//
// Two limits:
//   - session: continuous play before a break reminder (default 15 min)
//   - daily:   total play per calendar day before lockout (default 60 min)
// Persisted to NVS (namespace "xiaoban", keys "pt_session", "pt_daily",
// "pt_date", "pt_lim_s", "pt_lim_d"). Call play_time_tick() every second from
// the main loop while a child game is active.
#pragma once
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PLAY_SESSION_DEFAULT_SEC  (15 * 60)
#define PLAY_DAILY_DEFAULT_SEC    (60 * 60)

/* Load persisted state + limits. Call once at boot after app_config_init(). */
void play_time_init(void);

/* Start/stop the current play session (e.g. when a thinking game begins/ends). */
void play_time_start(void);
void play_time_stop(void);

/* Advance the clock by 1 second. Call every second while a child session is
 * active. Returns true if a limit was hit this tick (session-break or daily
 * lockout) so the caller can show the reminder UI. */
bool play_time_tick(void);

/* Remaining seconds in the current session / today's budget. */
int play_time_remaining_session(void);
int play_time_remaining_daily(void);

/* True when the daily budget is exhausted (games locked until tomorrow). */
bool play_time_is_locked(void);

/* Configure limits (seconds). 0 = use default. Persisted. */
void play_time_set_limits(int session_sec, int daily_sec);

#ifdef __cplusplus
}
#endif
