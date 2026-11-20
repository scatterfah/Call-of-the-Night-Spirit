#ifndef ZOMBIE_H
#define ZOMBIE_H

#include "game.h"

typedef enum {
    ZOMBIE_WALKER,     /* normal: balanced */
    ZOMBIE_RUNNER,     /* fast but weak */
    ZOMBIE_TANK        /* slow but very tough and hits hard */
} ZombieType;

typedef struct {
    float x, y;
    int   hp, max_hp;
    float speed;       /* pixels per second */
    float radius;      /* used for drawing and for bullet hits */
    int   damage;      /* taken from the player on touch */
    ZombieType type;
} Zombie;

void zombie_spawn(GameState *g, ZombieType type, float x, float y);
void zombies_update(GameState *g);     /* waves, chase, damage, remove the dead */
void zombies_draw(const GameState *g);
void zombies_reset(void);              /* call when a new game starts */

void waves_clear(GameState *g);        /* free what is still waiting in the wave queue */
int  waves_number(void);               /* current wave, 0 before the first one */
int  waves_countdown(void);            /* seconds until the next wave, 0 while a wave runs */

#endif