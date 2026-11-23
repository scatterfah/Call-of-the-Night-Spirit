#ifndef PLAYER_H
#define PLAYER_H

#include "raylib.h"
#include "game.h"

void    player_reset(void);                                    /* reset animation & FX on new run */
void    player_on_shoot(float aim_angle);                      /* trigger weapon recoil & muzzle flash */
void    player_on_throw_bomb(void);                            /* trigger 2-frame bomb throw animation */
Vector2 player_muzzle_pos(const GameState *g, float aim_angle);/* exact rotated gun barrel tip */
void    player_update(GameState *g);                           /* move player & tick timers */
void    player_draw(const GameState *g);                       /* draw hero, canisters, talisman & gun */

#endif