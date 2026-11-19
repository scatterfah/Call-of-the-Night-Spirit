#include <math.h>
#include <stdlib.h>
#include "raylib.h"
#include "zombie.h"

#define ZOMBIE_RADIUS     18.0f
#define ZOMBIE_SPEED      70.0f     /* pixels per second */
#define ZOMBIE_START_HP   50
#define SPAWN_INTERVAL    2.0f      /* seconds between spawns */
#define MAX_ZOMBIES       40
#define PLAYER_RADIUS     20.0f
#define TOUCH_DISTANCE    (ZOMBIE_RADIUS + PLAYER_RADIUS)
#define TOUCH_DAMAGE      10
#define TOUCH_COOLDOWN    1.0f      /* seconds between hits on the player */

static float spawn_timer = 0.0f;
static float hurt_timer  = 0.0f;

void zombies_reset(void) {
    spawn_timer = 0.0f;
    hurt_timer  = 0.0f;
}

void zombie_spawn(GameState *g, float x, float y) {
    Zombie *z = malloc(sizeof(Zombie));
    if (z == NULL) return;

    z->x = x;
    z->y = y;
    z->hp = ZOMBIE_START_HP;
    z->speed = ZOMBIE_SPEED;

    if (list_push_back(g->zombies, z) == NULL) {
        free(z);                    /* the list could not store it: do not leak */
    }
}

/* Spawn just outside a random edge of the window */
static void spawn_at_edge(GameState *g) {
    int w = GetScreenWidth();
    int h = GetScreenHeight();
    float x, y;

    switch (GetRandomValue(0, 3)) {
        case 0:  x = (float)GetRandomValue(0, w); y = -ZOMBIE_RADIUS;    break;  /* top */
        case 1:  x = (float)GetRandomValue(0, w); y = h + ZOMBIE_RADIUS; break;  /* bottom */
        case 2:  x = -ZOMBIE_RADIUS;    y = (float)GetRandomValue(0, h); break;  /* left */
        default: x = w + ZOMBIE_RADIUS; y = (float)GetRandomValue(0, h); break;  /* right */
    }
    zombie_spawn(g, x, y);
}

void zombies_update(GameState *g) {
    float dt = GetFrameTime();

    /* 1. Spawn a new zombie on a timer */
    spawn_timer += dt;
    if (spawn_timer >= SPAWN_INTERVAL && list_size(g->zombies) < MAX_ZOMBIES) {
        spawn_timer = 0.0f;
        spawn_at_edge(g);
    }

    if (hurt_timer > 0.0f) hurt_timer -= dt;

    /* 2. Walk the list: remove the dead, move and fight with the living */
    ListNode *node = g->zombies->head;
    while (node != NULL) {
        ListNode *next = node->next;        /* save next BEFORE any removal */
        Zombie *z = node->data;

        if (z->hp <= 0) {
            g->kills++;
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

            if (dist < TOUCH_DISTANCE && hurt_timer <= 0.0f) {
                g->player_hp -= TOUCH_DAMAGE;
                if (g->player_hp < 0) g->player_hp = 0;
                hurt_timer = TOUCH_COOLDOWN;
            }
        }

        node = next;
    }
}

void zombies_draw(const GameState *g) {
    for (ListNode *node = g->zombies->head; node != NULL; node = node->next) {
        const Zombie *z = node->data;

        DrawCircle((int)z->x, (int)z->y, ZOMBIE_RADIUS, DARKGREEN);
        DrawCircleLines((int)z->x, (int)z->y, ZOMBIE_RADIUS, LIME);

        /* small health bar above the zombie */
        float ratio = (float)z->hp / (float)ZOMBIE_START_HP;
        if (ratio < 0.0f) ratio = 0.0f;
        DrawRectangle((int)(z->x - ZOMBIE_RADIUS), (int)(z->y - ZOMBIE_RADIUS - 8),
                      (int)(ZOMBIE_RADIUS * 2.0f), 4, DARKGRAY);
        DrawRectangle((int)(z->x - ZOMBIE_RADIUS), (int)(z->y - ZOMBIE_RADIUS - 8),
                      (int)(ZOMBIE_RADIUS * 2.0f * ratio), 4, RED);
    }
}