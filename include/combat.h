#ifndef COMBAT_H
#define COMBAT_H

#include "game.h"

typedef struct {
    float x, y;        /* position */
    float vx, vy;      /* velocity in pixels per second */
    int   damage;
} Bullet;

void combat_fire(GameState *g);      /* fire toward the mouse while the button is held */
void combat_update(GameState *g);    /* move bullets, check hits, release used slots */
void combat_draw(const GameState *g);
void combat_reset(void);             /* call when a new game starts */

#endif