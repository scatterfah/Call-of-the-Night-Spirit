#include <math.h>
#include <stdlib.h>
#include "raylib.h"
#include "pickup.h"
#include "assets.h"
#include "weapon.h"
#include "combat.h"

#define PICKUP_RADIUS  16.0f
#define PLAYER_RADIUS  20.0f
#define PLAYER_MAX_HP  100
#define PICKUP_LIFE    12.0f
#define DROP_CHANCE    25
#define HEAL_AMOUNT    25

void pickup_drop(GameState *g, float x, float y) {
    if (GetRandomValue(0, 99) >= DROP_CHANCE) return;

    Pickup *p = malloc(sizeof(Pickup));
    if (p == NULL) return;

    int roll = GetRandomValue(0, 99);
    if (roll < 35)      p->type = PICKUP_HEALTH;
    else if (roll < 60) p->type = PICKUP_SHOTGUN;
    else if (roll < 80) p->type = PICKUP_SMG;
    else                p->type = PICKUP_BOMB;

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
        ListNode *next = node->next;
        Pickup *p = node->data;

        p->life -= dt;

        float dx = g->player_x - p->x;
        float dy = g->player_y - p->y;
        bool touching = (dx * dx + dy * dy) < (PICKUP_RADIUS + PLAYER_RADIUS) *
                                              (PICKUP_RADIUS + PLAYER_RADIUS);
        bool collected = false;

        if (touching) {
            if (p->type == PICKUP_HEALTH) {
                if (g->player_hp < PLAYER_MAX_HP) {
                    g->player_hp += HEAL_AMOUNT;
                    if (g->player_hp > PLAYER_MAX_HP) g->player_hp = PLAYER_MAX_HP;
                    collected = true;
                }
            } else if (p->type == PICKUP_SHOTGUN) {
                weapon_give(g, WEAPON_SHOTGUN);
                collected = true;
            } else if (p->type == PICKUP_SMG) {
                weapon_give(g, WEAPON_SMG);
                collected = true;
            } else if (p->type == PICKUP_BOMB) {
                if (g->bomb_count < BOMB_MAX_AMMO) {
                    g->bomb_count++;
                    collected = true;
                }
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
    const HeroAssets *h = &assets_get()->hero;
    float time = (float)GetTime();

    for (ListNode *node = g->pickups->head; node != NULL; node = node->next) {
        const Pickup *p = node->data;

        if (p->life < 3.0f && ((int)(p->life * 5.0f)) % 2 == 0) continue;

        int cell;
        Color glow;
        const char *tag;
        float size_h = 34.0f;

        if (p->type == PICKUP_HEALTH) {
            cell = 0;  /* medical pouch */
            glow = (Color){ 65, 225, 110, 62 };
            tag  = "HP";
        } else if (p->type == PICKUP_SMG) {
            cell = 1;  /* ammo tin */
            glow = (Color){ 75, 185, 255, 62 };
            tag  = "SMG";
        } else if (p->type == PICKUP_BOMB) {
            cell = 2;  /* tied bomb canister bundle */
            glow = (Color){ 255, 130, 55, 65 };
            tag  = "BOMB";
        } else {
            cell = 3;  /* Call of the Night Spirit weapon crate */
            glow = (Color){ 55, 230, 255, 72 };
            tag  = "SHOTGUN";
            size_h = 38.0f;
        }

        float bob = sinf(time * 4.2f + p->x * 0.04f) * 3.0f;

        if (h->pickups.id != 0 && h->pickup_rects[cell].height > 0.0f) {
            Rectangle src = h->pickup_rects[cell];
            float draw_h  = size_h;
            float draw_w  = src.width * (draw_h / src.height);

            DrawEllipse((int)p->x, (int)(p->y + 13.0f), 16.0f, 6.0f, (Color){ 0, 0, 0, 150 });
            DrawCircle((int)p->x, (int)(p->y + bob), 21.0f, glow);

            Rectangle dst = { p->x - draw_w * 0.5f, p->y - draw_h * 0.5f + bob, draw_w, draw_h };
            DrawTexturePro(h->pickups, src, dst, (Vector2){ 0.0f, 0.0f }, 0.0f, WHITE);

            int tw = MeasureText(tag, 12);
            DrawText(tag, (int)p->x - tw / 2 + 1, (int)(p->y + 17.0f) + 1, 12, BLACK);
            DrawText(tag, (int)p->x - tw / 2,     (int)(p->y + 17.0f),     12,
                     (Color){ glow.r, glow.g, glow.b, 240 });
        } else {
            int x = (int)p->x;
            int y = (int)p->y;
            Color solid = { glow.r, glow.g, glow.b, 255 };
            DrawRectangle(x - 12, y - 12, 24, 24, solid);
            DrawRectangleLines(x - 12, y - 12, 24, 24, WHITE);
            DrawText(tag, x - MeasureText(tag, 12) / 2, y - 6, 12, WHITE);
        }
    }
}

void pickups_clear(GameState *g) {
    while (g->pickups->head != NULL) {
        free(list_remove(g->pickups, g->pickups->head));
    }
}