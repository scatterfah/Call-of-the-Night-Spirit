#include "game.h"

void game_init(GameState *g) {
    g->zombies    = list_create();
    g->wave_queue = queue_create();
    g->weapons    = stack_create();
    g->bullets    = pool_create(sizeof(float) * 4, 100);   /* placeholder size, C changes it later */

    g->kills      = 0;
    g->kill_quota = 10;

    g->player_x   = 540.0f;
    g->player_y   = 360.0f;
    g->player_hp  = 100;
}

void game_free(GameState *g) {
    list_destroy(g->zombies);
    queue_destroy(g->wave_queue);
    stack_destroy(g->weapons);
    pool_destroy(g->bullets);
}