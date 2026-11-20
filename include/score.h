#ifndef SCORE_H
#define SCORE_H

#include <stdbool.h>

#define SCORE_MAX_ENTRIES 5
#define SCORE_FILE        "scores.txt"

typedef struct {
    int kills;
    int wave;
} ScoreEntry;

void       score_load(void);                     /* read saved scores from SCORE_FILE */
bool       score_record(int kills, int wave);    /* save run if in top 5; returns true if new best */
int        score_best(void);                     /* highest kill count saved so far (0 if none) */
int        score_count(void);                    /* how many entries are currently saved */
ScoreEntry score_get(int index);                 /* entry at 0..score_count()-1 */

#endif