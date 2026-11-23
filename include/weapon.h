#ifndef WEAPON_H
#define WEAPON_H

#include "game.h"

typedef enum {
    WEAPON_PISTOL,     /* unlimited ammo, always kept in the stack */
    WEAPON_SHOTGUN,    /* fires a fan of pellets, slow */
    WEAPON_SMG         /* very fast, a little inaccurate */
} WeaponType;

typedef struct {
    WeaponType type;
    int ammo;          /* -1 means unlimited */
} Weapon;

typedef struct {
    const char *name;
    int   damage;      /* per bullet */
    float interval;    /* seconds between shots */
    int   pellets;     /* bullets per shot */
    float spread;      /* degrees: the fan width, or the random wobble for one bullet */
    float speed;       /* bullet speed in pixels per second */
    int   pickup_ammo; /* shots a pickup gives */
} WeaponStats;

const WeaponStats *weapon_stats(WeaponType type);

void    weapons_init(GameState *g);                   /* push the pistol */
void    weapons_clear(GameState *g);                  /* pop and free every weapon */
Weapon *weapon_current(const GameState *g);           /* the top of the stack, NULL if empty */
void    weapon_give(GameState *g, WeaponType type);   /* pickup: add ammo or push a new weapon */
void    weapon_use_ammo(GameState *g);                /* one shot used; pops the weapon at 0 */
void    weapon_cycle(GameState *g, int direction);    /* rotate stack: >0 next, <0 previous */
void    weapon_update(GameState *g);                  /* mouse wheel / Tab weapon switching */
void    weapon_hud_draw(const GameState *g);          /* icon, name and ammo, bottom right */

#endif