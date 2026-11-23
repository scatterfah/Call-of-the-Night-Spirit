#include <stdlib.h>
#include "raylib.h"
#include "weapon.h"
#include "combat.h"
#include "assets.h"

static const WeaponStats STATS[3] = {
    /* PISTOL  */ { "Pistol",  10, 0.20f, 1,  0.0f, 700.0f,  0 },
    /* SHOTGUN */ { "Shotgun", 12, 0.70f, 5, 30.0f, 650.0f, 10 },
    /* SMG     */ { "SMG",      6, 0.07f, 1,  6.0f, 800.0f, 60 },
};

const WeaponStats *weapon_stats(WeaponType type) {
    return &STATS[type];
}

void weapons_init(GameState *g) {
    Weapon *pistol = malloc(sizeof(Weapon));
    if (pistol == NULL) return;

    pistol->type = WEAPON_PISTOL;
    pistol->ammo = -1;

    if (!stack_push(g->weapons, pistol)) {
        free(pistol);
    }
}

void weapons_clear(GameState *g) {
    while (stack_size(g->weapons) > 0) {
        free(stack_pop(g->weapons));
    }
}

Weapon *weapon_current(const GameState *g) {
    return stack_peek(g->weapons);
}

void weapon_give(GameState *g, WeaponType type) {
    if (type == WEAPON_PISTOL) return;

    /* Search using pure stack push/pop so duplicate pickups refill ammo and move to top */
    Stack *tmp = stack_create();
    Weapon *found = NULL;

    if (tmp != NULL) {
        while (stack_size(g->weapons) > 0) {
            Weapon *w = stack_pop(g->weapons);
            if (found == NULL && w->type == type) {
                found = w;
                break;
            }
            stack_push(tmp, w);
        }
        while (stack_size(tmp) > 0) {
            stack_push(g->weapons, stack_pop(tmp));
        }
        stack_destroy(tmp);
    }

    if (found != NULL) {
        found->ammo += STATS[type].pickup_ammo;
        stack_push(g->weapons, found);
        return;
    }

    Weapon *w = malloc(sizeof(Weapon));
    if (w == NULL) return;
    w->type = type;
    w->ammo = STATS[type].pickup_ammo;

    if (!stack_push(g->weapons, w)) {
        free(w);
    }
}

void weapon_use_ammo(GameState *g) {
    Weapon *w = weapon_current(g);
    if (w == NULL || w->ammo < 0) return;

    w->ammo--;
    if (w->ammo <= 0 && stack_size(g->weapons) > 1) {
        free(stack_pop(g->weapons));
    }
}

/* Rotate weapon stack via strictly LIFO stack operations */
void weapon_cycle(GameState *g, int direction) {
    if (g->weapons == NULL || stack_size(g->weapons) <= 1 || direction == 0) return;

    Stack *tmp = stack_create();
    if (tmp == NULL) return;

    if (direction > 0) {
        Weapon *top = stack_pop(g->weapons);
        while (stack_size(g->weapons) > 0) {
            stack_push(tmp, stack_pop(g->weapons));
        }
        stack_push(g->weapons, top);
        while (stack_size(tmp) > 0) {
            stack_push(g->weapons, stack_pop(tmp));
        }
    } else {
        while (stack_size(g->weapons) > 1) {
            stack_push(tmp, stack_pop(g->weapons));
        }
        Weapon *bottom = stack_pop(g->weapons);
        while (stack_size(tmp) > 0) {
            stack_push(g->weapons, stack_pop(tmp));
        }
        stack_push(g->weapons, bottom);
    }

    stack_destroy(tmp);
}

void weapon_update(GameState *g) {
    float wheel = GetMouseWheelMove();
    if (wheel > 0.0f) {
        weapon_cycle(g, 1);
    } else if (wheel < 0.0f) {
        weapon_cycle(g, -1);
    } else if (IsKeyPressed(KEY_TAB)) {
        weapon_cycle(g, 1);
    }
}

void weapon_hud_draw(const GameState *g) {
    int sw = GetScreenWidth();

    const char *aim = g->auto_aim ? "Auto-Aim [Q]: ON" : "Auto-Aim [Q]: OFF";
    int ax = sw - MeasureText(aim, 20) - 20;
    DrawText(aim, ax, 638, 20, g->auto_aim ? GREEN : GRAY);

    const char *bombs = TextFormat("Bombs [RMB / G]: %d / %d", g->bomb_count, BOMB_MAX_AMMO);
    int bx = sw - MeasureText(bombs, 20) - 20;
    DrawText(bombs, bx, 664, 20, g->bomb_count > 0 ? ORANGE : GRAY);

    const Weapon *w = weapon_current(g);
    if (w == NULL) return;

    const WeaponStats *s = weapon_stats(w->type);
    int backups = (int)stack_size(g->weapons) - 1;
    const char *line;
    if (w->ammo < 0) {
        line = (backups > 0)
             ? TextFormat("%s  [Inf]  (Wheel/Tab: +%d)", s->name, backups)
             : TextFormat("%s   Ammo: unlimited", s->name);
    } else {
        line = TextFormat("%s   Ammo: %d  (Wheel/Tab: +%d)", s->name, w->ammo, backups);
    }

    int text_w = MeasureText(line, 22);
    int x = sw - text_w - 20;
    DrawText(line, x, 690, 22, YELLOW);

    const HeroAssets *h = &assets_get()->hero;
    int idx = (int)w->type;
    if (idx >= 0 && idx < 3 && h->weapons.id != 0 && h->weapon_rects[idx].height > 0.0f) {
        Rectangle src = h->weapon_rects[idx];
        float target_h = 26.0f;
        float draw_w = src.width * (target_h / src.height);
        if (draw_w > 64.0f) {
            float clamp_s = 64.0f / draw_w;
            draw_w   *= clamp_s;
            target_h *= clamp_s;
        }
        Rectangle dst = { (float)x - draw_w - 12.0f, 701.0f - target_h * 0.5f, draw_w, target_h };
        DrawTexturePro(h->weapons, src, dst, (Vector2){ 0.0f, 0.0f }, 0.0f, WHITE);
    }
}