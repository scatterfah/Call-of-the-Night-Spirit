#ifndef WORLD_H
#define WORLD_H

#include <stdbool.h>
#include "raylib.h"
#include "game.h"

#define TILE_SIZE        40       /* pixels per tile */
#define WORLD_ROWS       24       /* 960 px tall */
#define WORLD_COLS       85       /* 3400 px wide (4 zones + 3 gates) */
#define WORLD_GATE_COUNT 3

void     world_reset(void);                                 /* call when a new game starts */
void     world_update(GameState *g);                        /* gate unlocks and smooth camera */
void     world_draw(void);                                  /* draw map and gates (in camera space) */
bool     world_is_wall(float x, float y);                   /* wall or closed gate? */
bool     world_circle_blocked(float x, float y, float r);   /* circle touches wall or closed gate? */

Camera2D world_camera(const GameState *g);                  /* camera following the player */
Vector2  world_mouse_pos(const GameState *g);               /* mouse position in world space */

float    world_width(void);
float    world_height(void);
float    world_unlocked_max_x(void);                        /* right edge of furthest unlocked zone */
int      world_zone_at(float x);                            /* returns zone number 1..4 */

void     hud_draw(const GameState *g);                      /* screen-space HUD and banners */

#endif