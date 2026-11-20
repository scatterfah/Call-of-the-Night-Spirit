#include "raylib.h"
#include "world.h"
#include "zombie.h"
#include "weapon.h"

#define ROWS   18
#define COLS   27
#define MAX_HP 100

/* The map. '#' is a wall and '.' is floor. Every row must have exactly 27
   characters, and there must be 18 rows. Edit it freely to design a level. */
static const char *MAP[ROWS] = {
    "###########################",
    "#.........................#",
    "#.........................#",
    "#..###...............###..#",
    "#..#...................#..#",
    "#..#...................#..#",
    "#.........................#",
    "#.........#.....#.........#",
    "#.........................#",
    "#.........................#",
    "#.........#.....#.........#",
    "#.........................#",
    "#..#...................#..#",
    "#..#...................#..#",
    "#..###...............###..#",
    "#.........................#",
    "#.........................#",
    "###########################",
};

bool world_is_wall(float x, float y) {
    if (x < 0.0f || y < 0.0f) return true;            /* outside the map counts as wall */

    int col = (int)(x / TILE_SIZE);
    int row = (int)(y / TILE_SIZE);
    if (col >= COLS || row >= ROWS) return true;

    return MAP[row][col] == '#';
}

/* A circle touches a wall if any corner of its bounding square is inside one. */
bool world_circle_blocked(float x, float y, float r) {
    return world_is_wall(x - r, y - r) || world_is_wall(x + r, y - r) ||
           world_is_wall(x - r, y + r) || world_is_wall(x + r, y + r);
}

void world_draw(void) {
    Color floor_color = {24, 24, 34, 255};
    Color wall_color  = {78, 66, 104, 255};
    Color wall_edge   = {120, 104, 160, 255};

    for (int row = 0; row < ROWS; row++) {
        for (int col = 0; col < COLS; col++) {
            int x = col * TILE_SIZE;
            int y = row * TILE_SIZE;

            if (MAP[row][col] == '#') {
                DrawRectangle(x, y, TILE_SIZE, TILE_SIZE, wall_color);
                DrawRectangleLines(x, y, TILE_SIZE, TILE_SIZE, wall_edge);
            } else {
                DrawRectangle(x, y, TILE_SIZE, TILE_SIZE, floor_color);
            }
        }
    }
}

void hud_draw(const GameState *g) {
    int screen_w = GetScreenWidth();

    /* dark strip behind the HUD */
    DrawRectangle(0, 0, screen_w, 38, (Color){0, 0, 0, 190});

    /* health bar */
    float ratio = (float)g->player_hp / (float)MAX_HP;
    if (ratio < 0.0f) ratio = 0.0f;
    if (ratio > 1.0f) ratio = 1.0f;

    DrawText("HP", 15, 9, 20, WHITE);
    DrawRectangle(50, 10, 200, 18, DARKGRAY);
    DrawRectangle(50, 10, (int)(200.0f * ratio), 18, ratio > 0.3f ? GREEN : RED);
    DrawRectangleLines(50, 10, 200, 18, WHITE);
    DrawText(TextFormat("%d", g->player_hp), 260, 9, 20, WHITE);

    /* wave, zombies alive and zombies still to come, in the middle */
    const char *info = TextFormat("Wave %d    Alive: %d    Incoming: %d",
                                  waves_number(), (int)list_size(g->zombies),
                                  (int)queue_size(g->wave_queue));
    DrawText(info, (screen_w - MeasureText(info, 20)) / 2, 9, 20, WHITE);

    /* kills out of the quota, on the right */
    const char *kills = TextFormat("Kills: %d / %d", g->kills, g->kill_quota);
    DrawText(kills, screen_w - MeasureText(kills, 20) - 15, 9, 20, WHITE);

    /* between waves: a big banner with the countdown */
    int wait = waves_countdown();
    if (wait > 0) {
        const char *title = TextFormat("WAVE %d", waves_number() + 1);
        const char *sub   = TextFormat("starts in %d", wait);
        DrawText(title, (screen_w - MeasureText(title, 70)) / 2, 230, 70, YELLOW);
        DrawText(sub,   (screen_w - MeasureText(sub, 30)) / 2,   310, 30, LIGHTGRAY);
    }
    weapon_hud_draw(g);                 /* current weapon and ammo, bottom right */
}