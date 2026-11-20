#ifndef GAME_H
#define GAME_H

#include <stdbool.h>
#include "ds_list.h"
#include "ds_queue.h"
#include "ds_stack.h"
#include "ds_pool.h"
#include "ds_heap.h"

typedef struct {
    List  *zombies;        /* zombies alive now */
    Queue *wave_queue;     /* zombies waiting to spawn */
    Pool  *bullets;        /* bullet slots */
    Pool  *bombs;          /* active bomb slots */
    Stack *weapons;        /* weapon inventory: the top is the weapon in hand */
    List  *pickups;        /* items lying on the ground */
    Heap  *target_heap;    /* min-heap of living zombies ordered by distance to player */

    int   kills;           /* zombies killed so far */
    int   kill_quota;      /* kills needed to open the next gate */

    float player_x, player_y;
    int   player_hp;
    int   bomb_count;      /* bombs the player is carrying */
    bool  auto_aim;        /* true: bullets lock onto the closest zombie */
} GameState;

void game_init(GameState *g);    /* create the structures, set starting values */
void game_free(GameState *g);    /* free what game_init created */

#endif