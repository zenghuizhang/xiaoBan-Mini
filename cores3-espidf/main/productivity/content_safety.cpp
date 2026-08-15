// content_safety.cpp — Keyword-based child-safety filter.
//
// Phase 1 implementation: a blocklist of sensitive terms. The child mode uses
// the full list; parent mode drops the "child-inappropriate but parent-OK"
// subset. This is a first line of defence; LLM review can be layered on top.
#include "content_safety.h"
#include <string.h>
#include <ctype.h>

/* Full blocklist — applies to child mode. */
static const char *s_blocklist[] = {
    /* violence */
    "杀", "死", "血", "刀", "枪", "打", "揍", "杀", "尸体", "自杀", "打架",
    /* adult / sexual */
    "性", "做爱", "色情", "裸体", "床戏", "脱衣",
    /* sensitive / politics / religion */
    "政治", "政府", "共产党", "国民党", "台独", "港独", "藏独",
    "宗教", "耶稣", "真主", "佛", "上帝",
    /* dependency-inducing (regulation Article 8/10) */
    "我爱你", "你是我最好的朋友", "离不开你",
    /* other harmful */
    "毒品", "赌博", "抽烟", "喝酒", "自杀方法",
};
#define BLOCKLIST_CNT  (sizeof(s_blocklist) / sizeof(s_blocklist[0]))

/* Terms allowed in parent mode but blocked in child mode (subset of above).
 * For now parent mode uses the same list minus a few; kept simple. */
static bool _contains(const char *haystack, const char *needle)
{
    if (!haystack || !needle || !*needle) return false;
    return strstr(haystack, needle) != NULL;
}

bool content_safety_check(const char *text, safety_mode_t mode)
{
    if (!text || !*text) return true;  // empty is safe

    for (int i = 0; i < (int)BLOCKLIST_CNT; i++) {
        if (_contains(text, s_blocklist[i])) {
            return false;
        }
    }
    (void)mode;  // parent mode currently uses the same list (Phase 1)
    return true;
}

bool content_safety_child_ok(const char *text)
{
    return content_safety_check(text, SAFETY_MODE_CHILD);
}
