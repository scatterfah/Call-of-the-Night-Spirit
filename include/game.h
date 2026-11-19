#ifndef GAME_H
#define GAME_H

#include "ds_list.h"
#include "ds_queue.h"
#include "ds_stack.h"
#include "ds_pool.h"

typedef struct {
    List  *zombies;        /* zombies alive now */
    Queue *wave_queue;     /* zombies waiting to spawn */
    Pool  *bullets;        /* bullet slots */
    Stack *weapons;        /* weapon and bomb inventory */

    int   kills;           /* zombies killed so far */
    int   kill_quota;      /* kills needed to open the next gate */

    float player_x, player_y;
    int   player_hp;
} GameState;

void game_init(GameState *g);    /* create the structures, set starting values */
void game_free(GameState *g);    /* free what game_init created */

#endif