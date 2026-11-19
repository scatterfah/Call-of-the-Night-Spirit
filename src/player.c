#include <math.h>
#include "raylib.h"
#include "player.h"

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

        /* 3. Move by speed x time of the frame, so every computer is equal */
        float step = PLAYER_SPEED * GetFrameTime();
        g->player_x += dx * step;
        g->player_y += dy * step;
    }

    /* 4. Keep the player inside the window */
    float w = (float)GetScreenWidth();
    float h = (float)GetScreenHeight();
    if (g->player_x < PLAYER_RADIUS)     g->player_x = PLAYER_RADIUS;
    if (g->player_x > w - PLAYER_RADIUS) g->player_x = w - PLAYER_RADIUS;
    if (g->player_y < PLAYER_RADIUS)     g->player_y = PLAYER_RADIUS;
    if (g->player_y > h - PLAYER_RADIUS) g->player_y = h - PLAYER_RADIUS;
}

void player_draw(const GameState *g) {
    /* Direction from the player to the mouse */
    Vector2 mouse = GetMousePosition();
    float angle = atan2f(mouse.y - g->player_y, mouse.x - g->player_x);

    /* Body */
    DrawCircle((int)g->player_x, (int)g->player_y, PLAYER_RADIUS, SKYBLUE);

    /* Aim line: starts at the player, points toward the mouse */
    float tipx = g->player_x + cosf(angle) * (PLAYER_RADIUS + 15.0f);
    float tipy = g->player_y + sinf(angle) * (PLAYER_RADIUS + 15.0f);
    DrawLine((int)g->player_x, (int)g->player_y, (int)tipx, (int)tipy, WHITE);
}