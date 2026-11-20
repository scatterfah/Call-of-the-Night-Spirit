#include <math.h>
#include <stdlib.h>
#include "raylib.h"
#include "zombie.h"
#include "pickup.h"

#define PLAYER_RADIUS  20.0f
#define TOUCH_COOLDOWN 1.0f      /* seconds between hits on the player */
#define FIRST_BREAK    3.0f      /* quiet time before wave 1 */
#define BREAK_TIME     4.0f      /* quiet time between waves */

typedef struct {
    int   hp;
    float speed;
    float radius;
    int   damage;
    Color color;
} ZombieStats;

/* one row per ZombieType, in the same order as the enum */
static const ZombieStats STATS[3] = {
    /* WALKER */ {  50,  70.0f, 18.0f, 10, { 30, 110,  40, 255} },
    /* RUNNER */ {  25, 150.0f, 14.0f,  8, {230, 140,  30, 255} },
    /* TANK   */ { 160,  45.0f, 26.0f, 20, {120,  40,  40, 255} },
};

static int   wave        = 0;
static float break_timer = FIRST_BREAK;   /* above 0: waiting for the next wave */
static float spawn_timer = 0.0f;
static float hurt_timer  = 0.0f;

void zombies_reset(void) {
    wave        = 0;
    break_timer = FIRST_BREAK;
    spawn_timer = 0.0f;
    hurt_timer  = 0.0f;
}

int waves_number(void) { return wave; }

int waves_countdown(void) {
    return break_timer > 0.0f ? (int)ceilf(break_timer) : 0;
}

void zombie_spawn(GameState *g, ZombieType type, float x, float y) {
    Zombie *z = malloc(sizeof(Zombie));
    if (z == NULL) return;

    z->x = x;
    z->y = y;
    z->type   = type;
    z->hp     = STATS[type].hp;
    z->max_hp = STATS[type].hp;
    z->speed  = STATS[type].speed;
    z->radius = STATS[type].radius;
    z->damage = STATS[type].damage;

    if (list_push_back(g->zombies, z) == NULL) {
        free(z);                    /* the list could not store it: do not leak */
    }
}

/* Spawn just outside a random edge of the window */
static void spawn_at_edge(GameState *g, ZombieType type) {
    int w = GetScreenWidth();
    int h = GetScreenHeight();
    float r = STATS[type].radius;
    float x, y;

    switch (GetRandomValue(0, 3)) {
        case 0:  x = (float)GetRandomValue(0, w); y = -r;     break;   /* top */
        case 1:  x = (float)GetRandomValue(0, w); y = h + r;  break;   /* bottom */
        case 2:  x = -r;    y = (float)GetRandomValue(0, h);  break;   /* left */
        default: x = w + r; y = (float)GetRandomValue(0, h);  break;   /* right */
    }
    zombie_spawn(g, type, x, y);
}

/* Fill the wave queue. The queue keeps the ORDER the zombies will appear in. */
static void plan_wave(GameState *g) {
    int count = 4 + 3 * wave;

    for (int i = 0; i < count; i++) {
        ZombieType type = ZOMBIE_WALKER;
        if (wave >= 3 && i % 5 == 4)      type = ZOMBIE_TANK;
        else if (wave >= 2 && i % 3 == 2) type = ZOMBIE_RUNNER;

        ZombieType *entry = malloc(sizeof(ZombieType));
        if (entry == NULL) return;
        *entry = type;

        if (!queue_enqueue(g->wave_queue, entry)) {
            free(entry);
            return;
        }
    }
}

/* The queue only frees its nodes, so free every entry that is still waiting. */
void waves_clear(GameState *g) {
    while (!queue_empty(g->wave_queue)) {
        free(queue_dequeue(g->wave_queue));
    }
}

void zombies_update(GameState *g) {
    float dt = GetFrameTime();

    /* 1. Wave control */
    if (break_timer > 0.0f) {
        break_timer -= dt;
        if (break_timer <= 0.0f) {          /* the break is over: start the next wave */
            break_timer = 0.0f;
            wave++;
            plan_wave(g);
            spawn_timer = 0.0f;
        }
    } else {
        float interval = 1.2f - 0.1f * (float)wave;   /* later waves spawn faster */
        if (interval < 0.4f) interval = 0.4f;

        spawn_timer += dt;
        if (!queue_empty(g->wave_queue) && spawn_timer >= interval) {
            spawn_timer = 0.0f;
            ZombieType *next_type = queue_dequeue(g->wave_queue);   /* first in line */
            spawn_at_edge(g, *next_type);
            free(next_type);
        }
    }

    if (hurt_timer > 0.0f) hurt_timer -= dt;

    /* 2. Walk the list: remove the dead, move and fight with the living */
    ListNode *node = g->zombies->head;
    while (node != NULL) {
        ListNode *next = node->next;        /* save next BEFORE any removal */
        Zombie *z = node->data;

        if (z->hp <= 0) {
            g->kills++;
            pickup_drop(g, z->x, z->y);     /* maybe leave an item behind */
            list_remove(g->zombies, node);
            free(z);                        /* the list never frees the zombie itself */
        } else {
            float dx = g->player_x - z->x;
            float dy = g->player_y - z->y;
            float dist = sqrtf(dx * dx + dy * dy);

            if (dist > 0.0f) {              /* chase the player */
                z->x += dx / dist * z->speed * dt;
                z->y += dy / dist * z->speed * dt;
            }

            if (dist < z->radius + PLAYER_RADIUS && hurt_timer <= 0.0f) {
                g->player_hp -= z->damage;
                if (g->player_hp < 0) g->player_hp = 0;
                hurt_timer = TOUCH_COOLDOWN;
            }
        }

        node = next;
    }

    /* 3. Wave finished: nothing waiting and nothing alive */
    if (break_timer <= 0.0f && queue_empty(g->wave_queue) && list_size(g->zombies) == 0) {
        break_timer = BREAK_TIME;
    }
}

void zombies_draw(const GameState *g) {
    for (ListNode *node = g->zombies->head; node != NULL; node = node->next) {
        const Zombie *z = node->data;

        DrawCircle((int)z->x, (int)z->y, z->radius, STATS[z->type].color);
        DrawCircleLines((int)z->x, (int)z->y, z->radius, WHITE);

        /* small health bar above the zombie */
        float ratio = (float)z->hp / (float)z->max_hp;
        if (ratio < 0.0f) ratio = 0.0f;
        int bar_w = (int)(z->radius * 2.0f);
        int bar_x = (int)(z->x - z->radius);
        int bar_y = (int)(z->y - z->radius - 8.0f);
        DrawRectangle(bar_x, bar_y, bar_w, 4, DARKGRAY);
        DrawRectangle(bar_x, bar_y, (int)((float)bar_w * ratio), 4, RED);
    }
}