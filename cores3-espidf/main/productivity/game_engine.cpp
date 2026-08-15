// game_engine.cpp — Thinking-game state machine + classification game.
//
// Phase 2: implements the classification game (按颜色/类别分类). Sort and
// pattern games are stubs that redirect to classify for now.
#include "game_engine.h"
#include "star_reward.h"
#include "content_safety.h"
#include "settings_store.h"
#include "parenting_knowledge.h"

#include <esp_log.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static const char *TAG = "GAME";

/* ---- classification knowledge base (Phase 2 seed) ---- */

typedef struct { const char *item; const char *color; } color_item_t;

static const color_item_t s_color_items[] = {
    { "苹果", "红色" }, { "太阳", "红色" }, { "草莓", "红色" }, { "红旗", "红色" },
    { "香蕉", "黄色" }, { "柠檬", "黄色" }, { "太阳", "黄色" }, { "月亮", "黄色" },
    { "天空", "蓝色" }, { "大海", "蓝色" }, { "蓝莓", "蓝色" },
    { "草",   "绿色" }, { "树叶", "绿色" }, { "西瓜", "绿色" },
};
#define COLOR_ITEM_CNT  (sizeof(s_color_items) / sizeof(s_color_items[0]))

static const char *s_colors[] = { "红色", "黄色", "蓝色", "绿色" };
#define COLOR_CNT  (sizeof(s_colors) / sizeof(s_colors[0]))

/* ---- engine state ---- */

typedef enum {
    ST_IDLE = 0,
    ST_ASKING,   // asked a question, waiting for answer
} game_state_t;

static game_state_t s_state = ST_IDLE;
static game_type_t  s_game  = GAME_NONE;
static bool s_parent_mode = false;
static char s_target_color[16] = {0};   // for classify: the color to find

/* sort game state */
static int  s_sort_nums[6];
static int  s_sort_cnt;
static int  s_sort_sorted[6];

/* pattern game state */
static const char *s_pattern_seq[8];
static int  s_pattern_len;
static const char *s_pattern_next;

/* ---- progress stats (persisted) ---- */

typedef struct {
    int classify_total, classify_correct;
    int sort_total,     sort_correct;
    int pattern_total,  pattern_correct;
} game_stats_t;

static game_stats_t s_stats = {0};

#define STATS_KEY "game_stats"

static void _stats_load(void)
{
    char buf[128];
    settings_store_get_string(STATS_KEY, buf, sizeof(buf), "");
    /* format: ct,cc,st,sc,pt,pc */
    int ct=0,cc=0,st=0,sc=0,pt=0,pc=0;
    if (sscanf(buf, "%d,%d,%d,%d,%d,%d", &ct,&cc,&st,&sc,&pt,&pc) == 6) {
        s_stats.classify_total = ct; s_stats.classify_correct = cc;
        s_stats.sort_total = st;     s_stats.sort_correct = sc;
        s_stats.pattern_total = pt;  s_stats.pattern_correct = pc;
    }
}

static void _stats_save(void)
{
    char buf[128];
    snprintf(buf, sizeof(buf), "%d,%d,%d,%d,%d,%d",
        s_stats.classify_total, s_stats.classify_correct,
        s_stats.sort_total,     s_stats.sort_correct,
        s_stats.pattern_total,  s_stats.pattern_correct);
    settings_store_set_string(STATS_KEY, buf);
}

static void _stats_record(game_type_t g, bool correct)
{
    if (g == GAME_CLASSIFY) { s_stats.classify_total++; if (correct) s_stats.classify_correct++; }
    else if (g == GAME_SORT) { s_stats.sort_total++; if (correct) s_stats.sort_correct++; }
    else if (g == GAME_PATTERN) { s_stats.pattern_total++; if (correct) s_stats.pattern_correct++; }
    _stats_save();
}

/* ---- helpers ---- */

static bool _str_contains(const char *hay, const char *needle)
{
    return hay && needle && *needle && strstr(hay, needle);
}

static int _child_age_group(void)
{
    char buf[16];
    settings_store_get_string("child_age", buf, sizeof(buf), "3-4");
    return (strcmp(buf, "5-6") == 0) ? 2 : 1;  // 1 = 3-4, 2 = 5-6
}

/* ---- classification game ---- */

static void _classify_gen_question(char *q, size_t qs)
{
    int ci = rand() % COLOR_CNT;
    strlcpy(s_target_color, s_colors[ci], sizeof(s_target_color));
    snprintf(q, qs, "请说一个%s的东西。", s_target_color);
}

static bool _classify_check(const char *ans)
{
    for (int i = 0; i < (int)COLOR_ITEM_CNT; i++) {
        if (strcmp(s_color_items[i].color, s_target_color) == 0 &&
            _str_contains(ans, s_color_items[i].item)) {
            return true;
        }
    }
    return false;
}

/* ---- sort game ---- */

static void _sort_gen_question(char *q, size_t qs)
{
    int age = _child_age_group();
    s_sort_cnt = (age == 2) ? 4 : 3;   // 5-6: 4 numbers, 3-4: 3 numbers

    /* generate distinct random numbers 1-9 */
    int used[10] = {0};
    for (int i = 0; i < s_sort_cnt; i++) {
        int n;
        do { n = 1 + rand() % 9; } while (used[n]);
        used[n] = 1;
        s_sort_nums[i] = n;
    }
    /* sorted target */
    for (int i = 0; i < s_sort_cnt; i++) s_sort_sorted[i] = s_sort_nums[i];
    for (int i = 0; i < s_sort_cnt - 1; i++)
        for (int j = i + 1; j < s_sort_cnt; j++)
            if (s_sort_sorted[i] > s_sort_sorted[j]) {
                int t = s_sort_sorted[i]; s_sort_sorted[i] = s_sort_sorted[j]; s_sort_sorted[j] = t;
            }

    char nums[64];
    int off = 0;
    for (int i = 0; i < s_sort_cnt; i++) {
        off += snprintf(nums + off, sizeof(nums) - off, "%d", s_sort_nums[i]);
        if (i < s_sort_cnt - 1) off += snprintf(nums + off, sizeof(nums) - off, "、");
    }
    snprintf(q, qs, "把 %s 从小到大排，说出来。", nums);
}

static bool _sort_check(const char *ans)
{
    /* Check that the answer contains the sorted numbers in order. */
    const char *p = ans;
    for (int i = 0; i < s_sort_cnt; i++) {
        char num[4];
        snprintf(num, sizeof(num), "%d", s_sort_sorted[i]);
        const char *f = strstr(p, num);
        if (!f) return false;
        p = f + strlen(num);
    }
    return true;
}

/* ---- pattern game ---- */

static const char *s_pattern_items[] = { "苹果", "香蕉", "橘子" };

static void _pattern_gen_question(char *q, size_t qs)
{
    int age = _child_age_group();
    /* 3-4: ABAB (2 items). 5-6: ABCABC (3 items). */
    int item_cnt = (age == 2) ? 3 : 2;
    s_pattern_len = item_cnt * 2;
    for (int i = 0; i < s_pattern_len; i++) {
        s_pattern_seq[i] = s_pattern_items[i % item_cnt];
    }
    s_pattern_next = s_pattern_items[s_pattern_len % item_cnt];

    char seq[128];
    int off = 0;
    for (int i = 0; i < s_pattern_len; i++) {
        off += snprintf(seq + off, sizeof(seq) - off, "%s", s_pattern_seq[i]);
        if (i < s_pattern_len - 1) off += snprintf(seq + off, sizeof(seq) - off, "、");
    }
    snprintf(q, qs, "%s，接下来是什么？", seq);
}

static bool _pattern_check(const char *ans)
{
    return _str_contains(ans, s_pattern_next);
}

/* ---- public API ---- */

void game_engine_init(void)
{
    star_reward_init();
    _stats_load();
    s_state = ST_IDLE;
    s_game = GAME_NONE;
    s_parent_mode = false;
    ESP_LOGI(TAG, "game engine init");
}

bool game_engine_is_active(void)
{
    return s_state != ST_IDLE;
}

void game_engine_stop(void)
{
    s_state = ST_IDLE;
    s_game = GAME_NONE;
}

bool game_engine_is_parent_mode(void)
{
    return s_parent_mode;
}

void game_engine_set_parent_mode(bool on)
{
    s_parent_mode = on;
    if (on) {
        /* entering parent mode: pause any active game */
        s_state = ST_IDLE;
        s_game = GAME_NONE;
    }
    ESP_LOGI(TAG, "parent mode: %d", on);
}

int game_engine_handle_input(const char *user_text, char *resp, size_t resp_size)
{
    if (!user_text || !resp || resp_size == 0) return -1;

    /* Mode-switch commands work in either mode. */
    if (_str_contains(user_text, "家长模式")) {
        s_parent_mode = true;
        s_state = ST_IDLE;
        s_game = GAME_NONE;
        const char *tip = parenting_daily_tip();
        snprintf(resp, resp_size,
            "好的，现在是家长模式。今日育儿贴士：%s 你可以问我任何育儿问题。", tip);
        return 0;
    }
    if (_str_contains(user_text, "儿童模式") || _str_contains(user_text, "玩游戏")) {
        s_parent_mode = false;
        snprintf(resp, resp_size, "好，我们继续玩思维游戏吧。想玩分类、排序还是找规律？");
        return 0;
    }

    /* Parent-mode commands handled locally. */
    if (s_parent_mode) {
        if (_str_contains(user_text, "进度") || _str_contains(user_text, "学得怎么样")) {
            game_engine_progress_report(resp, resp_size);
            return 0;
        }
        if (_str_contains(user_text, "贴士") || _str_contains(user_text, "建议")) {
            snprintf(resp, resp_size, "今日育儿贴士：%s", parenting_daily_tip());
            return 0;
        }
        return 1;  // otherwise route to LLM for parenting Q&A
    }

    /* Safety: never pass unsafe content through. */
    if (!content_safety_child_ok(user_text)) {
        snprintf(resp, resp_size, "这个我们不聊哦，我们来玩游戏吧。");
        return 0;
    }

    if (s_state == ST_IDLE) {
        /* Parse game selection. */
        game_type_t pick = GAME_NONE;
        if (_str_contains(user_text, "分类")) pick = GAME_CLASSIFY;
        else if (_str_contains(user_text, "排序")) pick = GAME_SORT;
        else if (_str_contains(user_text, "规律")) pick = GAME_PATTERN;

        if (pick == GAME_NONE) {
            snprintf(resp, resp_size,
                "我是 AI 小伴，陪你玩思维游戏。想玩分类、排序还是找规律？");
            return 0;
        }

        s_game = pick;
        s_state = ST_ASKING;

        char q[128];
        if (pick == GAME_CLASSIFY) {
            _classify_gen_question(q, sizeof(q));
            snprintf(resp, resp_size, "好，我们玩分类游戏。%s", q);
        } else if (pick == GAME_SORT) {
            _sort_gen_question(q, sizeof(q));
            snprintf(resp, resp_size, "好，我们玩排序游戏。%s", q);
        } else {
            _pattern_gen_question(q, sizeof(q));
            snprintf(resp, resp_size, "好，我们玩找规律。%s", q);
        }
        return 0;
    }

    /* ST_ASKING: check the answer. */
    bool correct = false;
    char feedback[160];

    if (s_game == GAME_CLASSIFY) {
        correct = _classify_check(user_text);
        if (correct)
            snprintf(feedback, sizeof(feedback),
                "答对啦！真棒，加一颗星。现在有 %d 颗星。", star_reward_get());
        else
            snprintf(feedback, sizeof(feedback),
                "再想想？%s的东西还有什么呢？", s_target_color);
    } else if (s_game == GAME_SORT) {
        correct = _sort_check(user_text);
        if (correct)
            snprintf(feedback, sizeof(feedback),
                "排对啦！加一颗星。现在有 %d 颗星。", star_reward_get());
        else
            snprintf(feedback, sizeof(feedback), "不对哦，从小到大再想想？");
    } else {
        correct = _pattern_check(user_text);
        if (correct)
            snprintf(feedback, sizeof(feedback),
                "找到规律啦！加一颗星。现在有 %d 颗星。", star_reward_get());
        else
            snprintf(feedback, sizeof(feedback), "再看看前面的顺序，接下来是什么？");
    }

    if (correct) star_reward_add(1);
    _stats_record(s_game, correct);

    /* Next question. */
    char q[128];
    if (s_game == GAME_CLASSIFY) _classify_gen_question(q, sizeof(q));
    else if (s_game == GAME_SORT) _sort_gen_question(q, sizeof(q));
    else _pattern_gen_question(q, sizeof(q));

    snprintf(resp, resp_size, "%s %s", feedback, q);
    return 0;
}

void game_engine_progress_report(char *buf, size_t buf_size)
{
    int total = s_stats.classify_total + s_stats.sort_total + s_stats.pattern_total;
    int correct = s_stats.classify_correct + s_stats.sort_correct + s_stats.pattern_correct;
    int acc = (total > 0) ? (correct * 100 / total) : 0;

    int c_acc = (s_stats.classify_total > 0) ? (s_stats.classify_correct * 100 / s_stats.classify_total) : 0;
    int s_acc = (s_stats.sort_total > 0) ? (s_stats.sort_correct * 100 / s_stats.sort_total) : 0;
    int p_acc = (s_stats.pattern_total > 0) ? (s_stats.pattern_correct * 100 / s_stats.pattern_total) : 0;

    snprintf(buf, buf_size,
        "孩子共玩了 %d 道题，正确率 %d%%。分类 %d%%，排序 %d%%，找规律 %d%%。共获得 %d 颗星。",
        total, acc, c_acc, s_acc, p_acc, star_reward_get());
}
