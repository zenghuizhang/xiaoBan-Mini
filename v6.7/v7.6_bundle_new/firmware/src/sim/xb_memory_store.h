// xb_memory_store.h — sqlite-backed memory entry store.
// On device: opens /littlefs/memory.db.  Off device: in-RAM fallback.
#pragma once
#include <stdint.h>
#include <stddef.h>

typedef struct {
    int64_t id;             // sqlite rowid
    char    title[64];
    char    snippet[160];
    int64_t created_ts;     // unix seconds
    char    persona[16];    // persona id that owned the entry
} mem_entry_t;

void   xb_memory_init(void);                       // open DB, ensure schema
size_t xb_memory_count(void);
int    xb_memory_list(mem_entry_t* out, size_t cap, size_t offset);  // returns N filled
int    xb_memory_add(const char* title, const char* snippet, const char* persona);
int    xb_memory_purge(void);                      // delete-all; posts XB_EVT_MEMORY_CHANGED
