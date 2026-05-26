// xb_memory_store.c — minimal sqlite wrapper (with RAM fallback when no FS).
// Schema:
//   CREATE TABLE memory (id INTEGER PRIMARY KEY, title TEXT, snippet TEXT,
//                        ts INTEGER, persona TEXT)
//
// On real hardware: link against esp_sqlite + LittleFS mount at /littlefs.
// In simulator / unit test:  RAM-only mode kicks in automatically.
#include "xb_memory_store.h"
#include "../core/xb_event.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define RAM_CAP 64
static mem_entry_t g_ram[RAM_CAP];
static size_t      g_n = 0;
static int64_t     g_next_id = 1;

#ifdef XB_HAS_SQLITE
#include "sqlite3.h"
static sqlite3* g_db = NULL;
#endif

void xb_memory_init(void) {
#ifdef XB_HAS_SQLITE
    if (sqlite3_open("/littlefs/memory.db", &g_db) == SQLITE_OK) {
        const char* ddl =
          "CREATE TABLE IF NOT EXISTS memory ("
          "id INTEGER PRIMARY KEY AUTOINCREMENT,"
          "title TEXT, snippet TEXT, ts INTEGER, persona TEXT);";
        sqlite3_exec(g_db, ddl, NULL, NULL, NULL);
        return;
    }
#endif
    g_n = 0;
}

size_t xb_memory_count(void) {
#ifdef XB_HAS_SQLITE
    if (g_db) {
        sqlite3_stmt* st;
        size_t n = 0;
        if (sqlite3_prepare_v2(g_db, "SELECT COUNT(*) FROM memory", -1, &st, NULL) == SQLITE_OK) {
            if (sqlite3_step(st) == SQLITE_ROW) n = sqlite3_column_int(st, 0);
            sqlite3_finalize(st);
        }
        return n;
    }
#endif
    return g_n;
}

int xb_memory_list(mem_entry_t* out, size_t cap, size_t offset) {
    if (!out || cap == 0) return 0;
#ifdef XB_HAS_SQLITE
    if (g_db) {
        sqlite3_stmt* st;
        const char* q = "SELECT id,title,snippet,ts,persona FROM memory "
                        "ORDER BY ts DESC LIMIT ? OFFSET ?";
        if (sqlite3_prepare_v2(g_db, q, -1, &st, NULL) != SQLITE_OK) return 0;
        sqlite3_bind_int(st, 1, (int)cap);
        sqlite3_bind_int(st, 2, (int)offset);
        int n = 0;
        while (sqlite3_step(st) == SQLITE_ROW && (size_t)n < cap) {
            out[n].id = sqlite3_column_int64(st, 0);
            strncpy(out[n].title,   (const char*)sqlite3_column_text(st, 1), sizeof(out[n].title)-1);
            strncpy(out[n].snippet, (const char*)sqlite3_column_text(st, 2), sizeof(out[n].snippet)-1);
            out[n].created_ts = sqlite3_column_int64(st, 3);
            strncpy(out[n].persona, (const char*)sqlite3_column_text(st, 4), sizeof(out[n].persona)-1);
            n++;
        }
        sqlite3_finalize(st);
        return n;
    }
#endif
    size_t i = 0, k = 0;
    while (i < g_n && k < cap) {
        if (i >= offset) out[k++] = g_ram[g_n - 1 - i];   // newest first
        ++i;
    }
    return (int)k;
}

int xb_memory_add(const char* title, const char* snippet, const char* persona) {
    if (!title || !snippet) return -1;
#ifdef XB_HAS_SQLITE
    if (g_db) {
        sqlite3_stmt* st;
        const char* q = "INSERT INTO memory(title,snippet,ts,persona) VALUES(?,?,?,?)";
        if (sqlite3_prepare_v2(g_db, q, -1, &st, NULL) != SQLITE_OK) return -1;
        sqlite3_bind_text(st, 1, title,   -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(st, 2, snippet, -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(st, 3, (int64_t)time(NULL));
        sqlite3_bind_text(st, 4, persona ? persona : "", -1, SQLITE_TRANSIENT);
        int rc = sqlite3_step(st);
        sqlite3_finalize(st);
        if (rc == SQLITE_DONE) {
            xb_event_post(XB_EVT_MEMORY_CHANGED, NULL);
            return 0;
        }
        return -1;
    }
#endif
    if (g_n >= RAM_CAP) {
        // drop oldest
        memmove(&g_ram[0], &g_ram[1], (RAM_CAP - 1) * sizeof(mem_entry_t));
        g_n--;
    }
    mem_entry_t* e = &g_ram[g_n++];
    e->id = g_next_id++;
    strncpy(e->title,   title,   sizeof(e->title)-1);   e->title[sizeof(e->title)-1] = 0;
    strncpy(e->snippet, snippet, sizeof(e->snippet)-1); e->snippet[sizeof(e->snippet)-1] = 0;
    e->created_ts = (int64_t)time(NULL);
    strncpy(e->persona, persona ? persona : "", sizeof(e->persona)-1);
    xb_event_post(XB_EVT_MEMORY_CHANGED, NULL);
    return 0;
}

int xb_memory_purge(void) {
#ifdef XB_HAS_SQLITE
    if (g_db) sqlite3_exec(g_db, "DELETE FROM memory", NULL, NULL, NULL);
#endif
    g_n = 0;
    xb_event_post(XB_EVT_MEMORY_CHANGED, NULL);
    return 0;
}
