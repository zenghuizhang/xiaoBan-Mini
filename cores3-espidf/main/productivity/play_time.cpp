// play_time.cpp — Child play-time limits with NVS persistence.
#include "play_time.h"
#include "settings_store.h"

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <esp_log.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

static const char *TAG = "PLAY_TIME";

#define PT_KEY_SESSION  "pt_session"   // current session elapsed seconds
#define PT_KEY_DAILY    "pt_daily"     // today's total elapsed seconds
#define PT_KEY_DATE     "pt_date"      // YYYYMMDD of last daily reset
#define PT_KEY_LIM_S    "pt_lim_s"     // session limit seconds
#define PT_KEY_LIM_D    "pt_lim_d"     // daily limit seconds

static SemaphoreHandle_t s_mutex = NULL;
static bool     s_active = false;
static int      s_session_elapsed = 0;   // seconds in current session
static int      s_daily_elapsed   = 0;   // seconds today
static int      s_session_limit   = PLAY_SESSION_DEFAULT_SEC;
static int      s_daily_limit     = PLAY_DAILY_DEFAULT_SEC;
static int      s_last_date       = 0;   // YYYYMMDD

static int _today_yyyymmdd(void)
{
    time_t now = time(NULL);
    if (now <= 0) return 0;  // time not set (no WiFi/SNTP yet)
    struct tm tm;
    localtime_r(&now, &tm);
    return (tm.tm_year + 1900) * 10000 + (tm.tm_mon + 1) * 100 + tm.tm_mday;
}

static void _persist(void)
{
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", s_session_elapsed);
    settings_store_set_string(PT_KEY_SESSION, buf);
    snprintf(buf, sizeof(buf), "%d", s_daily_elapsed);
    settings_store_set_string(PT_KEY_DAILY, buf);
    snprintf(buf, sizeof(buf), "%d", s_last_date);
    settings_store_set_string(PT_KEY_DATE, buf);
}

static int _load_int(const char *key, int def)
{
    char buf[16];
    settings_store_get_string(key, buf, sizeof(buf), "");
    if (!buf[0]) return def;
    return atoi(buf);
}

void play_time_init(void)
{
    if (!s_mutex) s_mutex = xSemaphoreCreateMutex();
    xSemaphoreTake(s_mutex, portMAX_DELAY);

    s_session_limit = _load_int(PT_KEY_LIM_S, PLAY_SESSION_DEFAULT_SEC);
    s_daily_limit   = _load_int(PT_KEY_LIM_D, PLAY_DAILY_DEFAULT_SEC);
    s_session_elapsed = _load_int(PT_KEY_SESSION, 0);
    s_daily_elapsed   = _load_int(PT_KEY_DAILY, 0);
    s_last_date       = _load_int(PT_KEY_DATE, 0);

    /* Daily rollover: if the calendar day changed, reset the daily budget. */
    int today = _today_yyyymmdd();
    if (today != 0 && today != s_last_date) {
        ESP_LOGI(TAG, "daily rollover: %d -> %d, reset daily budget", s_last_date, today);
        s_daily_elapsed = 0;
        s_session_elapsed = 0;
        s_last_date = today;
        _persist();
    } else if (today == 0) {
        /* Time not set yet — keep persisted values; rollover will apply once
         * WiFi/SNTP syncs the clock. */
        s_last_date = 0;
    }

    s_active = false;
    xSemaphoreGive(s_mutex);
    ESP_LOGI(TAG, "init: session=%d/%d daily=%d/%d date=%d",
             s_session_elapsed, s_session_limit,
             s_daily_elapsed, s_daily_limit, s_last_date);
}

void play_time_start(void)
{
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    s_active = true;
    xSemaphoreGive(s_mutex);
}

void play_time_stop(void)
{
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    s_active = false;
    xSemaphoreGive(s_mutex);
}

bool play_time_tick(void)
{
    bool hit = false;
    xSemaphoreTake(s_mutex, portMAX_DELAY);

    if (!s_active) { xSemaphoreGive(s_mutex); return false; }

    /* Daily rollover check (in case midnight passed while active). */
    int today = _today_yyyymmdd();
    if (today != 0 && today != s_last_date) {
        s_daily_elapsed = 0;
        s_session_elapsed = 0;
        s_last_date = today;
    }

    s_session_elapsed++;
    s_daily_elapsed++;

    if (s_session_elapsed >= s_session_limit || s_daily_elapsed >= s_daily_limit) {
        hit = true;
    }

    /* Persist every ~10s to limit flash wear. */
    if ((s_session_elapsed % 10) == 0) _persist();

    xSemaphoreGive(s_mutex);
    return hit;
}

int play_time_remaining_session(void)
{
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    int r = s_session_limit - s_session_elapsed;
    xSemaphoreGive(s_mutex);
    return r > 0 ? r : 0;
}

int play_time_remaining_daily(void)
{
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    int r = s_daily_limit - s_daily_elapsed;
    xSemaphoreGive(s_mutex);
    return r > 0 ? r : 0;
}

bool play_time_is_locked(void)
{
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    bool locked = s_daily_elapsed >= s_daily_limit;
    xSemaphoreGive(s_mutex);
    return locked;
}

void play_time_set_limits(int session_sec, int daily_sec)
{
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    if (session_sec > 0) s_session_limit = session_sec;
    if (daily_sec > 0)   s_daily_limit   = daily_sec;
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", s_session_limit);
    settings_store_set_string(PT_KEY_LIM_S, buf);
    snprintf(buf, sizeof(buf), "%d", s_daily_limit);
    settings_store_set_string(PT_KEY_LIM_D, buf);
    xSemaphoreGive(s_mutex);
    ESP_LOGI(TAG, "limits set: session=%d daily=%d", s_session_limit, s_daily_limit);
}
