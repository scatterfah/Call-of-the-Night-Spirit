#include <stdio.h>
#include "score.h"

static ScoreEntry entries[SCORE_MAX_ENTRIES];
static int entry_count = 0;

static void sort_entries(void) {
    for (int i = 0; i < entry_count - 1; i++) {
        for (int j = i + 1; j < entry_count; j++) {
            bool better = (entries[j].kills > entries[i].kills) ||
                          (entries[j].kills == entries[i].kills &&
                           entries[j].wave > entries[i].wave);
            if (better) {
                ScoreEntry tmp = entries[i];
                entries[i] = entries[j];
                entries[j] = tmp;
            }
        }
    }
}

static void score_save(void) {
    FILE *f = fopen(SCORE_FILE, "w");
    if (f == NULL) return;

    for (int i = 0; i < entry_count; i++) {
        fprintf(f, "%d %d\n", entries[i].kills, entries[i].wave);
    }

    fclose(f);
}

void score_load(void) {
    entry_count = 0;

    FILE *f = fopen(SCORE_FILE, "r");
    if (f == NULL) return;                       /* first run: file does not exist yet */

    int k = 0, w = 0;
    while (entry_count < SCORE_MAX_ENTRIES && fscanf(f, "%d %d", &k, &w) == 2) {
        if (k < 0) k = 0;
        if (w < 0) w = 0;
        entries[entry_count].kills = k;
        entries[entry_count].wave  = w;
        entry_count++;
    }

    fclose(f);
    sort_entries();
}

bool score_record(int kills, int wave) {
    if (kills < 0) kills = 0;
    if (wave < 0)  wave = 0;

    int prev_best = score_best();
    bool is_new_best = (kills > prev_best);

    /* find where this run ranks in the top list */
    int pos = entry_count;
    for (int i = 0; i < entry_count; i++) {
        if (kills > entries[i].kills ||
            (kills == entries[i].kills && wave > entries[i].wave)) {
            pos = i;
            break;
        }
    }

    if (pos < SCORE_MAX_ENTRIES) {
        if (entry_count < SCORE_MAX_ENTRIES) {
            entry_count++;
        }
        for (int i = entry_count - 1; i > pos; i--) {
            entries[i] = entries[i - 1];
        }
        entries[pos].kills = kills;
        entries[pos].wave  = wave;
        score_save();
    }

    return is_new_best;
}

int score_best(void) {
    if (entry_count <= 0) return 0;
    return entries[0].kills;
}

int score_count(void) {
    return entry_count;
}

ScoreEntry score_get(int index) {
    if (index < 0 || index >= entry_count) {
        return (ScoreEntry){ 0, 0 };
    }
    return entries[index];
}