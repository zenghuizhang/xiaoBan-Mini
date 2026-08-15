// todo_store.c — In-memory todo list + mutex + NVS persistence.
#include "todo_store.h"
#include "settings_store.h"

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <esp_log.h>
#include <string.h>
#include <stdlib.h>

static const char *TAG = "TODO_STORE";

#define TODO_NVS_KEY   "todos"
/* "<0|1>\t<text>\n" per item; 64 + 3 overhead, 24 items => ~1608 bytes. */
#define TODO_SER_BUF   2048

typedef struct {
    char  text[TODO_TEXT_MAX];
    bool  done;
} todo_item_t;

static SemaphoreHandle_t s_mutex = NULL;
static todo_item_t s_items[TODO_MAX_ITEMS];
static int s_count = 0;

/* ---- serialization helpers (caller holds mutex) ---- */

static void _load(void)
{
    s_count = 0;
    char buf[TODO_SER_BUF];
    buf[0] = '\0';
    settings_store_get_string(TODO_NVS_KEY, buf, sizeof(buf), "");

    char *line = buf;
    while (line && *line && s_count < TODO_MAX_ITEMS) {
        char *nl = strchr(line, '\n');
        if (nl) *nl = '\0';
        /* line = "<0|1>\t<text>" */
        if (line[0] == '0' || line[0] == '1') {
            char *tab = strchr(line, '\t');
            if (tab && tab[1]) {
                s_items[s_count].done = (line[0] == '1');
                strlcpy(s_items[s_count].text, tab + 1, TODO_TEXT_MAX);
                s_count++;
            }
        }
        line = nl ? nl + 1 : NULL;
    }
    ESP_LOGI(TAG, "loaded %d todos", s_count);
}

static void _save(void)
{
    char buf[TODO_SER_BUF];
    int off = 0;
    for (int i = 0; i < s_count && off < (int)sizeof(buf) - 4; i++) {
        int n = snprintf(buf + off, sizeof(buf) - off, "%d\t%s\n",
                         s_items[i].done ? 1 : 0, s_items[i].text);
        if (n < 0) break;
        off += n;
    }
    buf[off < (int)sizeof(buf) ? off : (int)sizeof(buf) - 1] = '\0';
    settings_store_set_string(TODO_NVS_KEY, buf);
}

/* ---- public API ---- */

void todo_store_init(void)
{
    s_mutex = xSemaphoreCreateMutex();
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    _load();
    xSemaphoreGive(s_mutex);
}

int todo_store_count(void)
{
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    int n = s_count;
    xSemaphoreGive(s_mutex);
    return n;
}

int todo_store_pending_count(void)
{
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    int n = 0;
    for (int i = 0; i < s_count; i++) if (!s_items[i].done) n++;
    xSemaphoreGive(s_mutex);
    return n;
}

const char *todo_store_get(int idx, bool *done)
{
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    const char *r = NULL;
    if (idx >= 0 && idx < s_count) {
        if (done) *done = s_items[idx].done;
        r = s_items[idx].text;
    }
    xSemaphoreGive(s_mutex);
    return r;
}

esp_err_t todo_store_add(const char *text)
{
    if (!text || !text[0]) return ESP_ERR_INVALID_ARG;
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    esp_err_t err = ESP_ERR_NO_MEM;
    if (s_count < TODO_MAX_ITEMS) {
        strlcpy(s_items[s_count].text, text, TODO_TEXT_MAX);
        s_items[s_count].done = false;
        s_count++;
        _save();
        err = ESP_OK;
    }
    xSemaphoreGive(s_mutex);
    return err;
}

esp_err_t todo_store_toggle(int idx)
{
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    esp_err_t err = ESP_ERR_INVALID_ARG;
    if (idx >= 0 && idx < s_count) {
        s_items[idx].done = !s_items[idx].done;
        _save();
        err = ESP_OK;
    }
    xSemaphoreGive(s_mutex);
    return err;
}

esp_err_t todo_store_delete(int idx)
{
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    esp_err_t err = ESP_ERR_INVALID_ARG;
    if (idx >= 0 && idx < s_count) {
        for (int i = idx; i < s_count - 1; i++)
            s_items[i] = s_items[i + 1];
        s_count--;
        _save();
        err = ESP_OK;
    }
    xSemaphoreGive(s_mutex);
    return err;
}

esp_err_t todo_store_clear_done(void)
{
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    int w = 0;
    for (int r = 0; r < s_count; r++) {
        if (!s_items[r].done) {
            if (w != r) s_items[w] = s_items[r];
            w++;
        }
    }
    int removed = s_count - w;
    s_count = w;
    if (removed) _save();
    xSemaphoreGive(s_mutex);
    return ESP_OK;
}

char *todo_store_snapshot_cn(void)
{
    /* Build under mutex into a stack buffer, then heap-copy. */
    char buf[TODO_SER_BUF];
    int off = 0;
    int n;

    xSemaphoreTake(s_mutex, portMAX_DELAY);

    int pending = 0;
    for (int i = 0; i < s_count; i++) {
        if (s_items[i].done) continue;
        if (pending == 0) {
            n = snprintf(buf + off, sizeof(buf) - off, "【今日待办】");
            if (n < 0) goto done;
            off += n;
        }
        n = snprintf(buf + off, sizeof(buf) - off, "\n%d. %s",
                     pending + 1, s_items[i].text);
        if (n < 0 || off + n >= (int)sizeof(buf)) break;
        off += n;
        pending++;
    }

done:
    xSemaphoreGive(s_mutex);

    if (pending == 0) return NULL;
    char *out = (char *)malloc(off + 1);
    if (out) {
        memcpy(out, buf, off);
        out[off] = '\0';
    }
    return out;
}
