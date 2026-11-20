#include <stdlib.h>
#include "raylib.h"
#include "weapon.h"

/* one row per WeaponType, in the same order as the enum */
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
    pistol->ammo = -1;                         /* unlimited */

    if (!stack_push(g->weapons, pistol)) {
        free(pistol);
    }
}

/* The stack only stores pointers, so every weapon must be freed by us. */
void weapons_clear(GameState *g) {
    while (stack_size(g->weapons) > 0) {
        free(stack_pop(g->weapons));
    }
}

Weapon *weapon_current(const GameState *g) {
    return stack_peek(g->weapons);             /* look at the top without removing it */
}

void weapon_give(GameState *g, WeaponType type) {
    if (type == WEAPON_PISTOL) return;         /* the pistol is always there */

    Weapon *top = weapon_current(g);
    if (top != NULL && top->type == type) {    /* already holding it: just add ammo */
        top->ammo += STATS[type].pickup_ammo;
        return;
    }

    Weapon *w = malloc(sizeof(Weapon));        /* a different weapon goes on TOP */
    if (w == NULL) return;
    w->type = type;
    w->ammo = STATS[type].pickup_ammo;

    if (!stack_push(g->weapons, w)) {
        free(w);
    }
}

void weapon_use_ammo(GameState *g) {
    Weapon *w = weapon_current(g);
    if (w == NULL || w->ammo < 0) return;      /* unlimited: nothing to use */

    w->ammo--;
    if (w->ammo <= 0 && stack_size(g->weapons) > 1) {
        free(stack_pop(g->weapons));           /* empty: pop it, the weapon below comes back */
    }
}

void weapon_hud_draw(const GameState *g) {
    const Weapon *w = weapon_current(g);
    if (w == NULL) return;

    const WeaponStats *s = weapon_stats(w->type);
    const char *line;
    if (w->ammo < 0) {
        line = TextFormat("%s   Ammo: unlimited", s->name);
    } else {
        line = TextFormat("%s   Ammo: %d   (backup weapons: %d)",
                          s->name, w->ammo, (int)stack_size(g->weapons) - 1);
    }

    int x = GetScreenWidth() - MeasureText(line, 22) - 20;
    DrawText(line, x, 692, 22, YELLOW);
}