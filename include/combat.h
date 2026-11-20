#ifndef COMBAT_H
#define COMBAT_H

#include "game.h"
#include "zombie.h"

#define BOMB_START_AMMO 3
#define BOMB_MAX_AMMO   5

typedef struct {
    float x, y;        /* position */
    float vx, vy;      /* velocity in pixels per second */
    int   damage;
} Bullet;

typedef enum {
    BOMB_ARMED,        /* flying or ticking on the ground */
    BOMB_EXPLODING     /* blast animation playing after damage is dealt */
} BombState;

typedef struct {
    float x, y;
    float vx, vy;
    float timer;       /* fuse seconds left (armed) or effect seconds left (exploding) */
    float blast_radius;
    int   damage;
    BombState state;
} Bomb;

int           combat_compare_target(const void *a, const void *b);
void          combat_update_targets(GameState *g);        /* rebuild min-heap after zombies update */
const Zombie *combat_closest_target(const GameState *g);  /* nearest living zombie, or NULL */

void combat_fire(GameState *g);       /* fire toward the closest target (auto-aim) or mouse */
void combat_throw_bomb(GameState *g); /* throw a bomb toward the mouse on right-click or G */
void combat_update(GameState *g);     /* move bullets & bombs, check hits, release used slots */
void combat_draw(const GameState *g);
void combat_reset(void);              /* call when a new game starts */

#endif