#ifndef WORLD_H
#define WORLD_H

#include <stdbool.h>
#include "raylib.h"
#include "game.h"

#define TILE_SIZE        40       /* pixels per tile */
#define ZONE_STRIDE_COLS 30       /* 29 open tiles + 1 seal column = 1200 px per zone */
#define WORLD_ROWS       18       /* 720 px tall */
#define WORLD_COLS       121      /* 4840 px wide (4 zones + 3 seals + outer borders) */
#define WORLD_GATE_COUNT 3

void     world_reset(void);                                 /* call when a new game starts */
void     world_update(GameState *g);                        /* seal unlocks, one-way lock & camera */
void     world_draw_play_bg(const GameState *g);            /* parallax sky & skyline */
void     world_draw(void);                                  /* ground, cover & seals */
void     world_draw_over(const GameState *g);               /* tree canopies & ground mist */
void     world_draw_menu_bg(bool dim);                      /* moon-only backdrop for start/menu */
bool     world_is_wall(float x, float y);                   /* boundary, cover, or locked seal? */
bool     world_circle_blocked(float x, float y, float r);   /* circle touches cover, edge, or seal? */

Camera2D world_camera(const GameState *g);                  /* camera following the player */
Vector2  world_mouse_pos(const GameState *g);               /* mouse position in world space */

float    world_width(void);
float    world_height(void);
float    world_ground_top(void);                            /* top y of playable ground */
float    world_ground_bottom(void);                         /* bottom y of playable ground */
float    world_unlocked_min_x(void);                        /* left playable edge of active zone */
float    world_unlocked_max_x(void);                        /* right edge of furthest unlocked zone */
int      world_active_zone(void);                           /* locked current zone 1..4 (one-way) */
int      world_zone_at(float x);                            /* returns zone number 1..4 for any x */

void     hud_draw(const GameState *g);                      /* screen-space HUD and banners */

#endif