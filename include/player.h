#ifndef PLAYER_H
#define PLAYER_H

#include "game.h"

void player_update(GameState *g);        /* move the player */
void player_draw(const GameState *g);    /* draw the player */

#endif