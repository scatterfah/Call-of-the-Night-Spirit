#ifndef ZOMBIE_H
#define ZOMBIE_H

#include "game.h"

typedef struct {
    float x, y;
    int   hp;
    float speed;
} Zombie;

void zombie_spawn(GameState *g, float x, float y);  /* make one, add it to g->zombies */
void zombies_update(GameState *g);                  /* spawn, chase, hurt player, remove dead */
void zombies_draw(const GameState *g);              /* draw every zombie */
void zombies_reset(void);                           /* reset timers when a new game starts */

#endif