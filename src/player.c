#include <math.h>
#include "raylib.h"
#include "player.h"
#include "world.h"
#include "combat.h"

#define PLAYER_RADIUS 20.0f
#define PLAYER_SPEED  250.0f      /* pixels per second */

void player_update(GameState *g) {
    /* 1. Read the keys: dx and dy are -1, 0 or +1 */
    float dx = 0.0f, dy = 0.0f;
    if (IsKeyDown(KEY_W)) dy -= 1.0f;
    if (IsKeyDown(KEY_S)) dy += 1.0f;
    if (IsKeyDown(KEY_A)) dx -= 1.0f;
    if (IsKeyDown(KEY_D)) dx += 1.0f;

    /* 2. Make diagonal movement the same speed as straight movement */
    float len = sqrtf(dx * dx + dy * dy);
    if (len > 0.0f) {
        dx /= len;
        dy /= len;

        /* 3. Move by speed x frame time, one axis at a time, so the player
              slides along a wall instead of sticking to it */
        float step = PLAYER_SPEED * GetFrameTime();

        float new_x = g->player_x + dx * step;
        if (!world_circle_blocked(new_x, g->player_y, PLAYER_RADIUS)) {
            g->player_x = new_x;
        }

        float new_y = g->player_y + dy * step;
        if (!world_circle_blocked(g->player_x, new_y, PLAYER_RADIUS)) {
            g->player_y = new_y;
        }
    }
}

void player_draw(const GameState *g) {
    /* Aim toward the locked zombie when auto-aim is on, otherwise toward the mouse */
    const Zombie *target = g->auto_aim ? combat_closest_target(g) : NULL;
    float angle;
    Color aim_color = WHITE;

    if (target != NULL && target->hp > 0) {
        angle = atan2f(target->y - g->player_y, target->x - g->player_x);
        aim_color = RED;
    } else {
        Vector2 mouse = GetMousePosition();
        angle = atan2f(mouse.y - g->player_y, mouse.x - g->player_x);
    }

    /* Body */
    DrawCircle((int)g->player_x, (int)g->player_y, PLAYER_RADIUS, SKYBLUE);

    /* Aim line: starts at the player, points toward the current target or mouse */
    float tipx = g->player_x + cosf(angle) * (PLAYER_RADIUS + 15.0f);
    float tipy = g->player_y + sinf(angle) * (PLAYER_RADIUS + 15.0f);
    DrawLine((int)g->player_x, (int)g->player_y, (int)tipx, (int)tipy, aim_color);
}