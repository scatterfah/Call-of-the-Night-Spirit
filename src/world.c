#include <math.h>
#include "raylib.h"
#include "world.h"
#include "zombie.h"
#include "weapon.h"

#define MAX_HP            100
#define CAMERA_SMOOTH     8.0f
#define GATE_BANNER_TIME  3.5f
#define ZONE_STRIDE_COLS  21      /* 20 floor cols + 1 divider/gate col per zone */

static const int GATE_QUOTAS[WORLD_GATE_COUNT] = { 10, 25, 45 };

static int   open_gates        = 0;
static int   last_opened_gate  = 0;
static float gate_banner_timer = 0.0f;

/* 24 rows x 85 cols (4 zones of 20 tiles separated by gates '1', '2', '3').
   Each row is split into zone slices so every column stays aligned. */
static const char *MAP[WORLD_ROWS] = {
    "#" "####################" "#" "####################" "#" "####################" "#" "####################" "#",
    "#" "...................." "#" "...................." "#" "...................." "#" "...................." "#",
    "#" "...................." "#" "...................." "#" "...................." "#" "...................." "#",
    "#" "..###..........###.." "#" "...................." "#" "....######.........." "#" "...................." "#",
    "#" "..#..............#.." "#" "....##...##...##...." "#" "...................." "#" ".....###....###....." "#",
    "#" "..#..............#.." "#" "....##...##...##...." "#" "..........######...." "#" ".....#........#....." "#",
    "#" "...................." "#" "...................." "#" "...................." "#" ".....#........#....." "#",
    "#" "......##....##......" "#" "...##..........##..." "#" "....##........##...." "#" "...................." "#",
    "#" "......##....##......" "#" "...##..........##..." "#" "....##........##...." "#" "...................." "#",
    "#" "...................." "#" "...................." "#" "...................." "#" "...................." "#",
    "#" "...................." "1" "...................." "2" "...................." "3" "...................." "#",
    "#" "...................." "1" "...................." "2" "...................." "3" "...................." "#",
    "#" "...................." "1" "...................." "2" "...................." "3" "...................." "#",
    "#" "...................." "1" "...................." "2" "...................." "3" "...................." "#",
    "#" "...................." "#" "...................." "#" "...................." "#" "...................." "#",
    "#" "......##....##......" "#" "...##..........##..." "#" "....##........##...." "#" "...................." "#",
    "#" "......##....##......" "#" "...##..........##..." "#" "....##........##...." "#" "...................." "#",
    "#" "...................." "#" "...................." "#" "...................." "#" ".....#........#....." "#",
    "#" "..#..............#.." "#" "....##...##...##...." "#" "..........######...." "#" ".....#........#....." "#",
    "#" "..#..............#.." "#" "....##...##...##...." "#" "...................." "#" ".....###....###....." "#",
    "#" "..###..........###.." "#" "...................." "#" "....######.........." "#" "...................." "#",
    "#" "...................." "#" "...................." "#" "...................." "#" "...................." "#",
    "#" "...................." "#" "...................." "#" "...................." "#" "...................." "#",
    "#" "####################" "#" "####################" "#" "####################" "#" "####################" "#",
};

void world_reset(void) {
    open_gates        = 0;
    last_opened_gate  = 0;
    gate_banner_timer = 0.0f;
}

float world_width(void)  { return (float)(WORLD_COLS * TILE_SIZE); }
float world_height(void) { return (float)(WORLD_ROWS * TILE_SIZE); }

float world_unlocked_max_x(void) {
    return (float)((open_gates + 1) * ZONE_STRIDE_COLS * TILE_SIZE);
}

int world_zone_at(float x) {
    int col = (int)(x / (float)TILE_SIZE);
    if (col < ZONE_STRIDE_COLS)     return 1;
    if (col < ZONE_STRIDE_COLS * 2) return 2;
    if (col < ZONE_STRIDE_COLS * 3) return 3;
    return 4;
}

void world_update(GameState *g) {
    float dt = GetFrameTime();

    /* 1. Check kill quota and unlock any qualifying gates */
    while (g->gates_open < WORLD_GATE_COUNT && g->kills >= GATE_QUOTAS[g->gates_open]) {
        g->gates_open++;
        open_gates        = g->gates_open;
        last_opened_gate  = g->gates_open;
        gate_banner_timer = GATE_BANNER_TIME;
    }

    if (g->gates_open < WORLD_GATE_COUNT) {
        g->kill_quota = GATE_QUOTAS[g->gates_open];
    } else {
        g->kill_quota = GATE_QUOTAS[WORLD_GATE_COUNT - 1];
    }

    if (gate_banner_timer > 0.0f) {
        gate_banner_timer -= dt;
        if (gate_banner_timer < 0.0f) gate_banner_timer = 0.0f;
    }

    /* 2. Smoothly move the camera toward the player and clamp to the map bounds */
    float follow = 1.0f - expf(-CAMERA_SMOOTH * dt);
    g->camera_x += (g->player_x - g->camera_x) * follow;
    g->camera_y += (g->player_y - g->camera_y) * follow;

    float half_w = (float)GetScreenWidth() * 0.5f;
    float half_h = (float)GetScreenHeight() * 0.5f;
    float max_x  = world_width() - half_w;
    float max_y  = world_height() - half_h;

    if (half_w >= max_x) {
        g->camera_x = world_width() * 0.5f;
    } else {
        if (g->camera_x < half_w) g->camera_x = half_w;
        if (g->camera_x > max_x)  g->camera_x = max_x;
    }

    if (half_h >= max_y) {
        g->camera_y = world_height() * 0.5f;
    } else {
        if (g->camera_y < half_h) g->camera_y = half_h;
        if (g->camera_y > max_y)  g->camera_y = max_y;
    }
}

Camera2D world_camera(const GameState *g) {
    Camera2D cam = { 0 };
    cam.offset   = (Vector2){ (float)GetScreenWidth() * 0.5f, (float)GetScreenHeight() * 0.5f };
    cam.target   = (Vector2){ g->camera_x, g->camera_y };
    cam.rotation = 0.0f;
    cam.zoom     = 1.0f;
    return cam;
}

Vector2 world_mouse_pos(const GameState *g) {
    return GetScreenToWorld2D(GetMousePosition(), world_camera(g));
}

bool world_is_wall(float x, float y) {
    if (x < 0.0f || y < 0.0f) return true;            /* outside the map counts as wall */

    int col = (int)(x / (float)TILE_SIZE);
    int row = (int)(y / (float)TILE_SIZE);
    if (col >= WORLD_COLS || row >= WORLD_ROWS) return true;

    char tile = MAP[row][col];
    if (tile == '#') return true;
    if (tile >= '1' && tile <= '3') {
        int gate_idx = tile - '1';
        return gate_idx >= open_gates;                /* solid while locked, open once unlocked */
    }
    return false;
}

/* A circle touches a wall if any corner of its bounding square is inside one. */
bool world_circle_blocked(float x, float y, float r) {
    return world_is_wall(x - r, y - r) || world_is_wall(x + r, y - r) ||
           world_is_wall(x - r, y + r) || world_is_wall(x + r, y + r);
}

void world_draw(void) {
    static const Color ZONE_FLOORS[4] = {
        {24, 24, 34, 255},    /* Zone 1: Courtyard */
        {22, 30, 26, 255},    /* Zone 2: Graveyard */
        {30, 22, 34, 255},    /* Zone 3: Catacombs */
        {36, 20, 24, 255},    /* Zone 4: Sanctum   */
    };

    Color wall_color = {78, 66, 104, 255};
    Color wall_edge  = {120, 104, 160, 255};

    for (int row = 0; row < WORLD_ROWS; row++) {
        for (int col = 0; col < WORLD_COLS; col++) {
            int x = col * TILE_SIZE;
            int y = row * TILE_SIZE;
            char tile = MAP[row][col];

            int zone_idx = col / ZONE_STRIDE_COLS;
            if (zone_idx > 3) zone_idx = 3;
            Color floor_color = ZONE_FLOORS[zone_idx];

            if (tile == '#') {
                DrawRectangle(x, y, TILE_SIZE, TILE_SIZE, wall_color);
                DrawRectangleLines(x, y, TILE_SIZE, TILE_SIZE, wall_edge);
            } else if (tile >= '1' && tile <= '3') {
                int gate_idx = tile - '1';
                DrawRectangle(x, y, TILE_SIZE, TILE_SIZE, floor_color);

                if (gate_idx >= open_gates) {
                    /* closed gate: dark iron slab with warning bars */
                    DrawRectangle(x + 4, y, TILE_SIZE - 8, TILE_SIZE, (Color){125, 42, 32, 255});
                    DrawRectangle(x + 12, y, 4, TILE_SIZE, (Color){220, 150, 50, 255});
                    DrawRectangle(x + 24, y, 4, TILE_SIZE, (Color){220, 150, 50, 255});
                    DrawRectangleLines(x + 4, y, TILE_SIZE - 8, TILE_SIZE, ORANGE);
                } else {
                    /* open gate threshold */
                    DrawRectangle(x + 2, y, 4, TILE_SIZE, (Color){70, 180, 110, 180});
                    DrawRectangle(x + TILE_SIZE - 6, y, 4, TILE_SIZE, (Color){70, 180, 110, 180});
                }
            } else {
                DrawRectangle(x, y, TILE_SIZE, TILE_SIZE, floor_color);
            }
        }
    }

    /* Subtle floor labels in each zone */
    static const char *ZONE_NAMES[4] = {
        "ZONE 1: COURTYARD",
        "ZONE 2: GRAVEYARD",
        "ZONE 3: CATACOMBS",
        "ZONE 4: SANCTUM"
    };
    for (int z = 0; z < 4; z++) {
        int cx = (z * ZONE_STRIDE_COLS + 11) * TILE_SIZE;
        int cy = 12 * TILE_SIZE;
        int tw = MeasureText(ZONE_NAMES[z], 24);
        DrawText(ZONE_NAMES[z], cx - tw / 2, cy - 12, 24, (Color){255, 255, 255, 28});
    }

    /* Label closed gates with their unlock quota */
    for (int g = open_gates; g < WORLD_GATE_COUNT; g++) {
        int gx = (g + 1) * ZONE_STRIDE_COLS * TILE_SIZE + TILE_SIZE / 2;
        int gy = 12 * TILE_SIZE;
        const char *g_title = TextFormat("GATE %d", g + 1);
        const char *g_req   = TextFormat("%d KILLS", GATE_QUOTAS[g]);

        int tw1 = MeasureText(g_title, 18);
        int tw2 = MeasureText(g_req, 16);
        int box_w = (tw1 > tw2 ? tw1 : tw2) + 18;

        DrawRectangle(gx - box_w / 2, gy - 24, box_w, 48, (Color){0, 0, 0, 200});
        DrawRectangleLines(gx - box_w / 2, gy - 24, box_w, 48, ORANGE);
        DrawText(g_title, gx - tw1 / 2, gy - 18, 18, ORANGE);
        DrawText(g_req,   gx - tw2 / 2, gy + 3,  16, WHITE);
    }
}

void hud_draw(const GameState *g) {
    int screen_w = GetScreenWidth();
    int zone = world_zone_at(g->player_x);

    /* dark strip behind the HUD */
    DrawRectangle(0, 0, screen_w, 38, (Color){0, 0, 0, 190});

    /* health bar */
    float ratio = (float)g->player_hp / (float)MAX_HP;
    if (ratio < 0.0f) ratio = 0.0f;
    if (ratio > 1.0f) ratio = 1.0f;

    DrawText("HP", 15, 9, 20, WHITE);
    DrawRectangle(50, 10, 180, 18, DARKGRAY);
    DrawRectangle(50, 10, (int)(180.0f * ratio), 18, ratio > 0.3f ? GREEN : RED);
    DrawRectangleLines(50, 10, 180, 18, WHITE);
    DrawText(TextFormat("%d", g->player_hp), 238, 9, 20, WHITE);

    /* zone, wave, zombies alive and incoming, in the middle */
    const char *info = TextFormat("Zone %d/4   Wave %d   Alive: %d   Incoming: %d",
                                  zone, waves_number(), (int)list_size(g->zombies),
                                  (int)queue_size(g->wave_queue));
    DrawText(info, (screen_w - MeasureText(info, 20)) / 2, 9, 20, WHITE);

    /* kills out of the next gate quota, on the right */
    const char *kills;
    Color kill_color = WHITE;
    if (g->gates_open < WORLD_GATE_COUNT) {
        kills = TextFormat("Gate %d: %d / %d", g->gates_open + 1, g->kills, g->kill_quota);
    } else {
        kills = TextFormat("Kills: %d (ALL OPEN)", g->kills);
        kill_color = GREEN;
    }
    DrawText(kills, screen_w - MeasureText(kills, 20) - 15, 9, 20, kill_color);

    /* gate-open celebration banner */
    if (gate_banner_timer > 0.0f) {
        const char *title = TextFormat("GATE %d OPEN!", last_opened_gate);
        const char *sub   = TextFormat("Head East to Zone %d ->", last_opened_gate + 1);
        DrawText(title, (screen_w - MeasureText(title, 54)) / 2, 95, 54, GREEN);
        DrawText(sub,   (screen_w - MeasureText(sub, 24)) / 2,   155, 24, WHITE);
    }

    /* directional prompt when a new zone is open to the east */
    if (zone <= g->gates_open) {
        int bob = (int)(5.0f * sinf((float)GetTime() * 6.0f));
        const char *prompt = "NEXT ZONE ->";
        int pw = MeasureText(prompt, 22);
        DrawText(prompt, screen_w - pw - 24 + bob, 348, 22, GREEN);
    }

    /* between waves: a big banner with the countdown */
    int wait = waves_countdown();
    if (wait > 0) {
        const char *title = TextFormat("WAVE %d", waves_number() + 1);
        const char *sub   = TextFormat("starts in %d", wait);
        DrawText(title, (screen_w - MeasureText(title, 70)) / 2, 230, 70, YELLOW);
        DrawText(sub,   (screen_w - MeasureText(sub, 30)) / 2,   310, 30, LIGHTGRAY);
    }

    weapon_hud_draw(g);                 /* current weapon, bombs, and auto-aim, bottom right */
}