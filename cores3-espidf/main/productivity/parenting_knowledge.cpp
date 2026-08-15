// parenting_knowledge.cpp — Curated parenting tips (3-6 years).
//
// Phase 3 seed: a small set of evidence-based tips. The LLM handles open-ended
// parenting Q&A; this module provides the daily rotating tip.
#include "parenting_knowledge.h"
#include "settings_store.h"

#include <time.h>
#include <string.h>

/* Tips for 3-4 year olds */
static const char *s_tips_34[] = {
    "3-4岁孩子正处于自我意识发展期，多说「你来选」比「你不要」更有效。",
    "每天陪孩子读15分钟绘本，比任何早教APP都更能培养语言能力。",
    "孩子发脾气时，先共情「你很生气对吗」，再讲道理。",
    "3岁孩子的注意力只有5-8分钟，一次只教一个新本领。",
    "鼓励孩子自己穿衣服、吃饭，独立感比速度更重要。",
    "用「先…再…」代替「不要…」，孩子更容易配合。",
};

/* Tips for 5-6 year olds */
static const char *s_tips_56[] = {
    "5-6岁是逻辑思维发展关键期，多玩分类、排序、找规律游戏。",
    "每天问孩子「今天有什么开心的事」，培养积极情绪记忆。",
    "让孩子参与简单家务，责任感和自信心会同步增长。",
    "孩子犯错时，问「你觉得下次怎么做更好」，培养反思能力。",
    "睡前10分钟聊天，比任何玩具都更能建立安全感。",
    "鼓励孩子用完整句子表达，为入学后的语言表达打基础。",
};

#define TIPS_34_CNT  (sizeof(s_tips_34) / sizeof(s_tips_34[0]))
#define TIPS_56_CNT  (sizeof(s_tips_56) / sizeof(s_tips_56[0]))

static int _age_group(void)
{
    char buf[16];
    settings_store_get_string("child_age", buf, sizeof(buf), "3-4");
    return (strcmp(buf, "5-6") == 0) ? 2 : 1;
}

const char *parenting_daily_tip(void)
{
    int age = _age_group();
    const char **tips = (age == 2) ? s_tips_56 : s_tips_34;
    int cnt = (age == 2) ? TIPS_56_CNT : TIPS_34_CNT;

    /* Rotate by day-of-year so the tip changes daily. */
    time_t now = time(NULL);
    int day = 0;
    if (now > 0) {
        struct tm tm;
        localtime_r(&now, &tm);
        day = tm.tm_yday;
    }
    return tips[day % cnt];
}

int parenting_tip_count(void)
{
    return TIPS_34_CNT + TIPS_56_CNT;
}
