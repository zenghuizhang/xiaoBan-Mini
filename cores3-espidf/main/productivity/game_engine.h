// game_engine.h — Thinking-game engine for the parenting persona.
//
// State machine: IDLE → (select game) → ASKING → (answer) → FEEDBACK+NEXT.
// Driven by voice: game_engine_handle_input(user_text, resp, resp_size) returns
// the text to speak (question / feedback + next question).
//
// Phase 2 ships the classification game; sort and pattern games follow.
#pragma once
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    GAME_NONE = 0,
    GAME_CLASSIFY,   // 分类
    GAME_SORT,       // 排序
    GAME_PATTERN,    // 找规律
} game_type_t;

/* Init the engine + star reward. Call once at boot. */
void game_engine_init(void);

/* True when a game session is in progress. */
bool game_engine_is_active(void);

/* End the current game (back to idle). */
void game_engine_stop(void);

/* Parent mode: when true, voice input is routed to the LLM for parenting Q&A
 * instead of the game engine. Toggled by the user saying "家长模式" / "儿童模式". */
bool game_engine_is_parent_mode(void);
void game_engine_set_parent_mode(bool on);

/* Handle one user utterance. Writes the response (question / feedback) into
 * `resp` and returns ESP_OK. When idle, interprets the input as a game
 * selection; when active, checks the answer and produces the next question.
 * Returns 1 if the caller should route the input to the LLM (parent mode
 * hand-off), 0 if handled. */
int game_engine_handle_input(const char *user_text, char *resp, size_t resp_size);

/* Write a CN progress report (games played, accuracy per type, stars) into buf.
 * Used by parent mode ("孩子学得怎么样"). */
void game_engine_progress_report(char *buf, size_t buf_size);

#ifdef __cplusplus
}
#endif
