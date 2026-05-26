// xb_persona.h — 6 persona registry, NVS-backed.
#pragma once
#include <stdbool.h>

typedef struct {
    const char* id;          // "lyra" | "echo" | "nova" | "sage" | "pico" | "doc"
    const char* name_zh;
    const char* tagline_zh;
    const char* face_color;  // CSS hex, only used by the React preview
} persona_t;

#define PERSONA_COUNT 6
extern const persona_t XB_PERSONAS[PERSONA_COUNT];

void               xb_persona_init(void);              // load NVS "xb_persona/active"
const persona_t*   xb_persona_active(void);
void               xb_persona_set(const char* id);     // persists + posts XB_EVT_PERSONA_CHANGED
const persona_t*   xb_persona_lookup(const char* id);  // NULL on unknown
