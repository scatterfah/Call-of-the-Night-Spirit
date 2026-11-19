#include <math.h>
#include "raylib.h"
#include "combat.h"
#include "zombie.h"

#define BULLET_SPEED     700.0f   /* pixels per second */
#define BULLET_RADIUS    4.0f
#define BULLET_DAMAGE    10       /* a zombie has 50 hp, so 5 hits */
#define FIRE_INTERVAL    0.20f    /* seconds between shots */
#define GUN_DISTANCE     25.0f    /* bullet starts this far from the player's centre */
#define OFFSCREEN_MARGIN 50.0f

/* must match ZOMBIE_RADIUS in zombie.c */
#define ZOMBIE_HIT_RADIUS 18.0f

static float fire_cooldown = 0.0f;

void combat_reset(void) {
    fire_cooldown = 0.3f;    /* so the click that pressed START does not fire a shot */
}

void combat_fire(GameState *g) {
    if (fire_cooldown > 0.0f) return;
    if (!IsMouseButtonDown(MOUSE_LEFT_BUTTON)) return;

    /* direction from the player to the mouse */
    Vector2 mouse = GetMousePosition();
    float dx = mouse.x - g->player_x;
    float dy = mouse.y - g->player_y;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 1.0f) return;          /* mouse is on the player: no direction */
    dx /= len;
    dy /= len;

    Bullet *b = pool_acquire(g->bullets);
    if (b == NULL) return;           /* the pool is full: no shot */

    b->x = g->player_x + dx * GUN_DISTANCE;
    b->y = g->player_y + dy * GUN_DISTANCE;
    b->vx = dx * BULLET_SPEED;
    b->vy = dy * BULLET_SPEED;
    b->damage = BULLET_DAMAGE;

    fire_cooldown = FIRE_INTERVAL;
}

void combat_update(GameState *g) {
    float dt = GetFrameTime();
    float w = (float)GetScreenWidth();
    float h = (float)GetScreenHeight();

    if (fire_cooldown > 0.0f) fire_cooldown -= dt;

    /* visit every slot of the pool, and work only on the active ones */
    for(size_t i = 0; i < pool_capacity(g->bullets); i++){
        Bullet *b = pool_get(g->bullets, i);
        if (b == NULL) continue;          /* slot i is free */

        b->x += b->vx * dt;
        b->y += b->vy * dt;

        /* left the screen: give the slot back */
        if (b->x < -OFFSCREEN_MARGIN || b->x > w + OFFSCREEN_MARGIN ||
            b->y < -OFFSCREEN_MARGIN || b->y > h + OFFSCREEN_MARGIN) {
            pool_release(g->bullets, b);
            continue;
        }

        /* did it hit a zombie? */
        for (ListNode *n = g->zombies->head; n != NULL; n = n->next) {
            Zombie *z = n->data;
            if (z->hp <= 0) continue;   /* already dead, waiting to be removed */

            if (CheckCollisionCircles((Vector2){b->x, b->y}, BULLET_RADIUS,
                                      (Vector2){z->x, z->y}, ZOMBIE_HIT_RADIUS)) {
                z->hp -= b->damage;              /* only reduce hp: zombie.c removes the dead */
                pool_release(g->bullets, b);     /* the bullet is used up */
                break;
            }
        }
    }
}

void combat_draw(const GameState *g) {
    for (size_t i = 0; i < g->bullets->capacity; i++) {
        const Bullet *b = pool_get(g->bullets, i);
        if (b == NULL) continue;          /* slot i is free */

        DrawCircle((int)b->x, (int)b->y, BULLET_RADIUS, YELLOW);
    }
}