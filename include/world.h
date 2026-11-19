#ifndef WORLD_H
#define WORLD_H

#include <stdbool.h>
#include "game.h"

#define TILE_SIZE 40          /* 27 x 18 tiles fill a 1080 x 720 window */

void world_draw(void);                                      /* draw the tile map */
bool world_is_wall(float x, float y);                       /* is this point inside a wall? */
bool world_circle_blocked(float x, float y, float r);       /* would a circle here touch a wall? */
void hud_draw(const GameState *g);                          /* health bar, kills, zombie count */

#endif