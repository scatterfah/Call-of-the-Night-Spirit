#ifndef PICKUP_H
#define PICKUP_H

#include "game.h"

typedef enum {
    PICKUP_HEALTH,
    PICKUP_SHOTGUN,
    PICKUP_SMG
} PickupType;

typedef struct {
    float x, y;
    PickupType type;
    float life;        /* seconds left before it disappears */
} Pickup;

void pickup_drop(GameState *g, float x, float y);   /* maybe drop an item where a zombie died */
void pickups_update(GameState *g);                  /* timers, and the player picking items up */
void pickups_draw(const GameState *g);
void pickups_clear(GameState *g);                   /* free every pickup still on the ground */

#endif