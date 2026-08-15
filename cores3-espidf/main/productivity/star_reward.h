// star_reward.h — Star reward system for thinking games.
//
// Persists total stars to NVS (key "stars"). +1 per correct answer.
// Phase 1: simple counter. Phase 2: achievements / levels.
#pragma once
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void star_reward_init(void);
int  star_reward_get(void);
void star_reward_add(int n);     // add n stars (usually 1)
void star_reward_reset(void);

#ifdef __cplusplus
}
#endif
