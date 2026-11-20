#ifndef ZOMBIE_H
#define ZOMBIE_H

#include <stdbool.h>
#include "game.h"

typedef enum {
    ZOMBIE_WALKER,
    ZOMBIE_RUNNER,
    ZOMBIE_TANK,
    ZOMBIE_BOSS
} ZombieType;

typedef struct {
    float      x;
    float      y;
    ZombieType type;
    int        hp;
    int        max_hp;
    float      speed;
    float      radius;
    int        damage;
} Zombie;

void zombies_reset(void);
void zombie_spawn(GameState *g, ZombieType type, float x, float y);
void zombies_update(GameState *g);
void zombies_draw(const GameState *g);

int  waves_number(void);
int  waves_countdown(void);
void waves_clear(GameState *g);

/* Final Boss (The Night Spirit) status for HUD and victory trigger */
bool  boss_is_active(void);
bool  boss_is_defeated(void);
int   boss_hp(void);
int   boss_max_hp(void);
int   boss_kills_needed(void);
float boss_intro_timer(void);

#endif