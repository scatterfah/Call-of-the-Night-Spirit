#include <stdlib.h>
#include "game.h"
#include "combat.h"
#include "zombie.h"
#include "weapon.h"
#include "pickup.h"

void game_init(GameState *g) {
    g->zombies    = list_create();
    g->wave_queue = queue_create();
    g->weapons    = stack_create();
    g->bullets    = pool_create(sizeof(Bullet), 100);   /* up to 100 bullets at once */
    g->bombs      = pool_create(sizeof(Bomb), 16);      /* up to 16 active bombs at once */
    g->pickups    = list_create();

    weapons_init(g);                /* the pistol starts at the bottom of the stack */

    g->kills      = 0;
    g->kill_quota = 10;

    g->player_x   = 540.0f;
    g->player_y   = 360.0f;
    g->player_hp  = 100;
    g->bomb_count = BOMB_START_AMMO;
}

void game_free(GameState *g) {
    /* the list only frees its nodes, so free every zombie first */
    for (ListNode *n = g->zombies->head; n != NULL; n = n->next) {
        free(n->data);
    }

    waves_clear(g);                 /* free the zombies still waiting in the queue */
    weapons_clear(g);               /* free every weapon on the stack */
    pickups_clear(g);               /* free every pickup on the ground */

    list_destroy(g->zombies);
    queue_destroy(g->wave_queue);
    stack_destroy(g->weapons);
    pool_destroy(g->bullets);
    pool_destroy(g->bombs);
    list_destroy(g->pickups);
}