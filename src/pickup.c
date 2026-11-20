#include <stdlib.h>
#include "raylib.h"
#include "pickup.h"
#include "weapon.h"

#define PICKUP_RADIUS  12.0f
#define PLAYER_RADIUS  20.0f
#define PLAYER_MAX_HP  100
#define PICKUP_LIFE    12.0f     /* seconds on the ground */
#define DROP_CHANCE    25       /* percent chance when a zombie dies */
#define HEAL_AMOUNT    25

void pickup_drop(GameState *g, float x, float y) {
    if (GetRandomValue(0, 99) >= DROP_CHANCE) return;      /* most zombies drop nothing */

    Pickup *p = malloc(sizeof(Pickup));
    if (p == NULL) return;

    int roll = GetRandomValue(0, 99);
    if (roll < 45)      p->type = PICKUP_HEALTH;           /* 45% health */
    else if (roll < 75) p->type = PICKUP_SHOTGUN;          /* 30% shotgun */
    else                p->type = PICKUP_SMG;              /* 25% SMG */

    p->x = x;
    p->y = y;
    p->life = PICKUP_LIFE;

    if (list_push_back(g->pickups, p) == NULL) {
        free(p);
    }
}

void pickups_update(GameState *g) {
    float dt = GetFrameTime();

    ListNode *node = g->pickups->head;
    while (node != NULL) {
        ListNode *next = node->next;           /* save next BEFORE any removal */
        Pickup *p = node->data;

        p->life -= dt;

        float dx = g->player_x - p->x;
        float dy = g->player_y - p->y;
        bool touching = (dx * dx + dy * dy) < (PICKUP_RADIUS + PLAYER_RADIUS) *
                                              (PICKUP_RADIUS + PLAYER_RADIUS);
        bool collected = false;

        if (touching) {
            if (p->type == PICKUP_HEALTH) {
                if (g->player_hp < PLAYER_MAX_HP) {        /* do not waste it at full health */
                    g->player_hp += HEAL_AMOUNT;
                    if (g->player_hp > PLAYER_MAX_HP) g->player_hp = PLAYER_MAX_HP;
                    collected = true;
                }
            } else if (p->type == PICKUP_SHOTGUN) {
                weapon_give(g, WEAPON_SHOTGUN);
                collected = true;
            } else {
                weapon_give(g, WEAPON_SMG);
                collected = true;
            }
        }

        if (collected || p->life <= 0.0f) {
            list_remove(g->pickups, node);
            free(p);
        }

        node = next;
    }
}

void pickups_draw(const GameState *g) {
    for (ListNode *node = g->pickups->head; node != NULL; node = node->next) {
        const Pickup *p = node->data;

        /* blink during the last 3 seconds so the player sees it is about to vanish */
        if (p->life < 3.0f && ((int)(p->life * 5.0f)) % 2 == 0) continue;

        Color color;
        const char *label;
        if (p->type == PICKUP_HEALTH)       { color = (Color){ 40, 170,  70, 255}; label = "+"; }
        else if (p->type == PICKUP_SHOTGUN) { color = (Color){210, 120,  30, 255}; label = "S"; }
        else                                { color = (Color){ 60, 130, 220, 255}; label = "M"; }

        int x = (int)p->x;
        int y = (int)p->y;
        DrawRectangle(x - 12, y - 12, 24, 24, color);
        DrawRectangleLines(x - 12, y - 12, 24, 24, WHITE);
        DrawText(label, x - MeasureText(label, 20) / 2, y - 10, 20, WHITE);
    }
}

void pickups_clear(GameState *g) {
    while (g->pickups->head != NULL) {
        free(list_remove(g->pickups, g->pickups->head));   /* remove the node, free the pickup */
    }
}