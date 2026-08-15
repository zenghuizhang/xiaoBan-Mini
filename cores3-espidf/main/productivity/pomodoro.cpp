// pomodoro.cpp — Pomodoro state machine + 1s tick + NVS persistence.
//
// Pure state engine (no LVGL / no audio deps), mirroring todo_store.c's layering:
// pomodoro_tick() advances the clock and returns true on a phase transition
// (focus→break, break→idle); the caller (app_main main loop) renders the
// completion beep/bubble/expression so all LVGL + hardware access stays on the
// main task. Thread-safe via mutex: tick (main loop, core0) vs snapshot collect
// (claw_core worker, core1).
//
// NVS key "pomo" = "<state>|<remaining_sec>|<task>". On reboot any active phase
// (RUNNING/BREAK) is restored as PAUSED — no RTC, can't trust elapsed time.
// Persistence is throttled to ~every 5s + on every state transition.
#include "pomodoro.h"
#include "settings_store.h"

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <esp_log.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static const char *TAG = "POMODORO";
#define POMO_NVS_KEY   "pomo"
#define POMO_SAVE_MS   5000   // throttle: persist at most every 5s while counting
/* "<state>|<remaining>|<task>" — task up to 63 chars + overhead. */
#define POMO_SER_BUF   (POMO_TASK_MAX + 24)

static SemaphoreHandle_t s_mutex = NULL;
static pomo_state_t s_state = POMO_IDLE;
static int          s_remaining = 0;
static char         s_task[POMO_TASK_MAX] = {0};
static uint32_t     s_last_save_ms = 0;

/* ---- serialization helpers (caller holds mutex) ---- */

static void _save(void)
{
    char buf[POMO_SER_BUF];
    snprintf(buf, sizeof(buf), "%d|%d|%s", (int)s_state, s_remaining, s_task);
    settings_store_set_string(POMO_NVS_KEY, buf);
}

static void _load(void)
{
    s_state = POMO_IDLE;
    s_remaining = 0;
    s_task[0] = '\0';

    char buf[POMO_SER_BUF];
    buf[0] = '\0';
    settings_store_get_string(POMO_NVS_KEY, buf, sizeof(buf), "");

    int st = POMO_IDLE, rem = 0;
    char task[POMO_TASK_MAX] = {0};
    /* "state|remaining|task" — task may be empty; %[] needs ≥1 char so an empty
     * task just leaves task="" (sscanf returns 2, still valid). */
    if (sscanf(buf, "%d|%d|%63[^\n]", &st, &rem, task) >= 2) {
        if (st >= POMO_IDLE && st <= POMO_BREAK) {
            s_state = (pomo_state_t)st;
            s_remaining = (rem > 0 && rem <= POMO_FOCUS_SEC) ? rem : 0;
            strlcpy(s_task, task, POMO_TASK_MAX);
        }
    }
    /* No RTC: any actively-counting phase is frozen as PAUSED on reboot. */
    if (s_state == POMO_RUNNING || s_state == POMO_BREAK) {
        ESP_LOGI(TAG, "restore %d→PAUSED (rem %d, task '%s')",
                 (int)s_state, s_remaining, s_task);
        s_state = POMO_PAUSED;
    }
    s_last_save_ms = 0;
}

static void _persist(void)
{
    _save();
    s_last_save_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
}

/* ---- public API ---- */

void pomodoro_init(void)
{
    if (!s_mutex) s_mutex = xSemaphoreCreateMutex();
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    _load();
    xSemaphoreGive(s_mutex);
    ESP_LOGI(TAG, "init: state=%d remaining=%d task='%s'",
             (int)s_state, s_remaining, s_task);
}

void pomodoro_start(const char *task)
{
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    strlcpy(s_task, task ? task : "", POMO_TASK_MAX);
    s_state = POMO_RUNNING;
    s_remaining = POMO_FOCUS_SEC;
    _persist();
    xSemaphoreGive(s_mutex);
}

void pomodoro_pause(void)
{
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    if (s_state == POMO_RUNNING || s_state == POMO_BREAK) {
        /* Freeze the current phase; resume re-enters RUNNING. (A break paused
         * then resumed focuses — acceptable MVP, avoids a separate phase field.) */
        s_state = POMO_PAUSED;
        _persist();
    }
    xSemaphoreGive(s_mutex);
}

void pomodoro_resume(void)
{
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    if (s_state == POMO_PAUSED) {
        s_state = POMO_RUNNING;
        _persist();
    }
    xSemaphoreGive(s_mutex);
}

void pomodoro_reset(void)
{
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    s_state = POMO_IDLE;
    s_remaining = 0;
    s_task[0] = '\0';
    _persist();
    xSemaphoreGive(s_mutex);
}

pomo_state_t pomodoro_state(void)
{
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    pomo_state_t s = s_state;
    xSemaphoreGive(s_mutex);
    return s;
}

int pomodoro_remaining_seconds(void)
{
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    int r = s_remaining;
    xSemaphoreGive(s_mutex);
    return r;
}

const char *pomodoro_task(void)
{
    /* Task buffer is stable between mutations (like todo_store_get); return the
     * pointer under a brief lock. Caller should copy if it must outlive a call. */
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    const char *r = s_task;
    xSemaphoreGive(s_mutex);
    return r;
}

bool pomodoro_tick(void)
{
    bool transitioned = false;
    xSemaphoreTake(s_mutex, portMAX_DELAY);

    if (s_state == POMO_RUNNING || s_state == POMO_BREAK) {
        if (s_remaining > 0) s_remaining--;
        if (s_remaining <= 0) {
            if (s_state == POMO_RUNNING) {
                s_state = POMO_BREAK;          /* focus done → break */
                s_remaining = POMO_BREAK_SEC;
            } else {
                s_state = POMO_IDLE;           /* break done → idle */
                s_remaining = 0;
                s_task[0] = '\0';
            }
            transitioned = true;
        }
        /* Throttle: persist on every transition + ~every 5s while counting. */
        uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
        if (transitioned || (now - s_last_save_ms) >= POMO_SAVE_MS) {
            _save();
            s_last_save_ms = now;
        }
    }

    xSemaphoreGive(s_mutex);
    return transitioned;
}

char *pomodoro_snapshot_cn(void)
{
    /* Copy everything needed under the lock, then format outside. */
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    pomo_state_t st = s_state;
    int rem = s_remaining;
    char task[POMO_TASK_MAX];
    strlcpy(task, s_task, sizeof(task));
    xSemaphoreGive(s_mutex);

    if (st == POMO_IDLE) return NULL;

    char buf[128];
    int off = 0, n;
    int mm = rem / 60, ss = rem % 60;
    const char *phase = (st == POMO_RUNNING) ? "专注中"
                      : (st == POMO_BREAK)   ? "休息中"
                      :                         "已暂停";

    if (st == POMO_PAUSED)
        n = snprintf(buf, sizeof(buf), "当前番茄钟：已暂停（剩余 %02d:%02d）", mm, ss);
    else
        n = snprintf(buf, sizeof(buf), "当前番茄钟：%s，剩余 %02d:%02d", phase, mm, ss);
    if (n < 0) return NULL;
    off = n;

    if (task[0]) {
        n = snprintf(buf + off, sizeof(buf) - off, "，任务：%s", task);
        if (n > 0 && off + n < (int)sizeof(buf)) off += n;
    }

    char *out = (char *)malloc(off + 1);
    if (out) {
        memcpy(out, buf, off);
        out[off] = '\0';
    }
    return out;
}
