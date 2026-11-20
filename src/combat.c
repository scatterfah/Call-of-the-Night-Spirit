#include <math.h>
#include "raylib.h"
#include "combat.h"
#include "zombie.h"
#include "world.h"
#include "weapon.h"

#define BULLET_RADIUS       4.0f
#define GUN_DISTANCE        25.0f    /* bullets start this far from the player's centre */
#define OFFSCREEN_MARGIN    50.0f
#define DEG_TO_RAD          0.0174532925f

#define PLAYER_RADIUS       20.0f
#define BOMB_RADIUS         10.0f
#define BOMB_BLAST_RADIUS   130.0f
#define BOMB_MAX_DAMAGE     120
#define BOMB_SELF_DAMAGE    25       /* set to 0 if you want no self-damage */
#define BOMB_FUSE_TIME      1.25f
#define BOMB_EXPLOSION_TIME 0.30f
#define BOMB_THROW_COOLDOWN 0.60f
#define BOMB_MAX_SPEED      520.0f
#define BOMB_FRICTION       3.2f
#define BOMB_KNOCKBACK      45.0f

static float fire_cooldown = 0.0f;
static float bomb_cooldown = 0.0f;
static float ref_player_x  = 0.0f;
static float ref_player_y  = 0.0f;

void combat_reset(void) {
    fire_cooldown = 0.3f;    /* so the click that pressed START does not fire a shot */
    bomb_cooldown = 0.2f;
}

/* Min-heap comparator: zombies closer to the player bubble to the top */
int combat_compare_target(const void *a, const void *b) {
    const Zombie *za = a;
    const Zombie *zb = b;

    float dxa = za->x - ref_player_x;
    float dya = za->y - ref_player_y;
    float dxb = zb->x - ref_player_x;
    float dyb = zb->y - ref_player_y;

    float dist_a = dxa * dxa + dya * dya;
    float dist_b = dxb * dxb + dyb * dyb;

    if (dist_a < dist_b) return -1;
    if (dist_a > dist_b) return 1;
    return 0;
}

void combat_update_targets(GameState *g) {
    if (g->target_heap == NULL) return;

    heap_clear(g->target_heap);
    ref_player_x = g->player_x;
    ref_player_y = g->player_y;

    for (ListNode *n = g->zombies->head; n != NULL; n = n->next) {
        Zombie *z = n->data;
        if (z->hp > 0) {
            heap_push(g->target_heap, z);
        }
    }
}

const Zombie *combat_closest_target(const GameState *g) {
    if (g->target_heap == NULL) return NULL;
    return heap_peek(g->target_heap);
}

static void detonate_bomb(GameState *g, Bomb *bomb) {
    /* 1. Damage and push back every living zombie inside the blast radius */
    for (ListNode *n = g->zombies->head; n != NULL; n = n->next) {
        Zombie *z = n->data;
        if (z->hp <= 0) continue;

        float dx = z->x - bomb->x;
        float dy = z->y - bomb->y;
        float reach = bomb->blast_radius + z->radius;
        float dist_sq = dx * dx + dy * dy;
        if (dist_sq > reach * reach) continue;

        float dist = sqrtf(dist_sq);
        float falloff = 1.0f - (dist / reach);
        if (falloff < 0.0f) falloff = 0.0f;

        /* 100% damage at the centre, tapering down to 35% at the outer edge */
        int dmg = (int)((float)bomb->damage * (0.35f + 0.65f * falloff));
        if (dmg < 1) dmg = 1;
        z->hp -= dmg;

        /* push surviving zombies away without shoving them into walls or closed gates */
        if (z->hp > 0 && dist > 0.001f) {
            float push = BOMB_KNOCKBACK * (0.3f + 0.7f * falloff);
            float new_x = z->x + (dx / dist) * push;
            if (!world_circle_blocked(new_x, z->y, z->radius)) {
                z->x = new_x;
            }
            float new_y = z->y + (dy / dist) * push;
            if (!world_circle_blocked(z->x, new_y, z->radius)) {
                z->y = new_y;
            }
        }
    }

    /* 2. Reduced self-damage if the player stands inside the blast */
    if (BOMB_SELF_DAMAGE > 0) {
        float pdx = g->player_x - bomb->x;
        float pdy = g->player_y - bomb->y;
        float preach = bomb->blast_radius + PLAYER_RADIUS;
        float pdist_sq = pdx * pdx + pdy * pdy;

        if (pdist_sq <= preach * preach) {
            float pdist = sqrtf(pdist_sq);
            float pfalloff = 1.0f - (pdist / preach);
            if (pfalloff < 0.0f) pfalloff = 0.0f;

            int pdmg = (int)((float)BOMB_SELF_DAMAGE * (0.35f + 0.65f * pfalloff));
            g->player_hp -= pdmg;
            if (g->player_hp < 0) g->player_hp = 0;
        }
    }

    /* 3. Chain reaction: shorten the fuse of other armed bombs caught in the blast */
    for (size_t i = 0; i < pool_capacity(g->bombs); i++) {
        Bomb *other = pool_get(g->bombs, i);
        if (other == NULL || other == bomb || other->state != BOMB_ARMED) continue;

        float dx = other->x - bomb->x;
        float dy = other->y - bomb->y;
        if (dx * dx + dy * dy <= bomb->blast_radius * bomb->blast_radius) {
            if (other->timer > 0.12f) {
                other->timer = 0.12f;
            }
        }
    }
}

void combat_fire(GameState *g) {
    if (IsKeyPressed(KEY_Q)) {
        g->auto_aim = !g->auto_aim;
    }

    if (fire_cooldown > 0.0f) return;
    if (!IsMouseButtonDown(MOUSE_LEFT_BUTTON)) return;

    /* the weapon in hand is the top of the stack */
    const Weapon *weapon = weapon_current(g);
    if (weapon == NULL) return;
    const WeaponStats *stats = weapon_stats(weapon->type);

    /* aim at the closest zombie when auto-aim is on, otherwise toward the mouse */
    float aim = 0.0f;
    const Zombie *target = g->auto_aim ? combat_closest_target(g) : NULL;
    if (target != NULL && target->hp > 0) {
        float dx = target->x - g->player_x;
        float dy = target->y - g->player_y;
        if (dx * dx + dy * dy >= 1.0f) {
            aim = atan2f(dy, dx);
        }
    } else {
        Vector2 mouse = world_mouse_pos(g);
        float dx = mouse.x - g->player_x;
        float dy = mouse.y - g->player_y;
        if (sqrtf(dx * dx + dy * dy) < 1.0f) return; /* mouse is on the player: no direction */
        aim = atan2f(dy, dx);
    }

    int fired = 0;
    for (int i = 0; i < stats->pellets; i++) {
        float offset = 0.0f;                         /* angle change in degrees */
        if (stats->pellets > 1) {
            /* several pellets: spread them evenly across the fan */
            offset = ((float)i / (float)(stats->pellets - 1) - 0.5f) * stats->spread;
        } else if (stats->spread > 0.0f) {
            /* one bullet: a small random wobble */
            offset = ((float)GetRandomValue(-100, 100) / 100.0f) * stats->spread * 0.5f;
        }
        float angle = aim + offset * DEG_TO_RAD;

        Bullet *b = pool_acquire(g->bullets);
        if (b == NULL) break;                        /* the pool is full: no more bullets */

        b->x = g->player_x + cosf(angle) * GUN_DISTANCE;
        b->y = g->player_y + sinf(angle) * GUN_DISTANCE;
        b->vx = cosf(angle) * stats->speed;
        b->vy = sinf(angle) * stats->speed;
        b->damage = stats->damage;
        fired++;
    }

    if (fired > 0) {
        fire_cooldown = stats->interval;
        weapon_use_ammo(g);        /* may pop the weapon, so it must be the LAST use of it */
    }
}

void combat_throw_bomb(GameState *g) {
    if (bomb_cooldown > 0.0f) return;
    if (g->bomb_count <= 0) return;
    if (!IsMouseButtonPressed(MOUSE_RIGHT_BUTTON) && !IsKeyPressed(KEY_G)) return;

    Bomb *b = pool_acquire(g->bombs);
    if (b == NULL) return;

    Vector2 mouse = world_mouse_pos(g);
    float dx = mouse.x - g->player_x;
    float dy = mouse.y - g->player_y;
    float dist = sqrtf(dx * dx + dy * dy);

    b->x = g->player_x;
    b->y = g->player_y;

    if (dist > 1.0f) {
        /* scale throw speed with cursor distance so the bomb settles near the click */
        float speed = dist * BOMB_FRICTION;
        if (speed > BOMB_MAX_SPEED) speed = BOMB_MAX_SPEED;
        b->vx = (dx / dist) * speed;
        b->vy = (dy / dist) * speed;
    } else {
        b->vx = 0.0f;
        b->vy = 0.0f;
    }

    b->timer        = BOMB_FUSE_TIME;
    b->blast_radius = BOMB_BLAST_RADIUS;
    b->damage       = BOMB_MAX_DAMAGE;
    b->state        = BOMB_ARMED;

    g->bomb_count--;
    bomb_cooldown = BOMB_THROW_COOLDOWN;
}

void combat_update(GameState *g) {
    float dt = GetFrameTime();
    float w = world_width();
    float h = world_height();

    if (fire_cooldown > 0.0f) fire_cooldown -= dt;
    if (bomb_cooldown > 0.0f) bomb_cooldown -= dt;

    /* 1. Update bullets */
    for (size_t i = 0; i < pool_capacity(g->bullets); i++) {
        Bullet *b = pool_get(g->bullets, i);
        if (b == NULL) continue;          /* slot i is free */

        b->x += b->vx * dt;
        b->y += b->vy * dt;

        /* left the world: give the slot back */
        if (b->x < -OFFSCREEN_MARGIN || b->x > w + OFFSCREEN_MARGIN ||
            b->y < -OFFSCREEN_MARGIN || b->y > h + OFFSCREEN_MARGIN) {
            pool_release(g->bullets, b);
            continue;
        }

        /* hit a wall or closed gate: the bullet is used up */
        if (world_is_wall(b->x, b->y)) {
            pool_release(g->bullets, b);
            continue;
        }

        /* did it hit a zombie? */
        for (ListNode *n = g->zombies->head; n != NULL; n = n->next) {
            Zombie *z = n->data;
            if (z->hp <= 0) continue;   /* already dead, waiting to be removed */

            if (CheckCollisionCircles((Vector2){b->x, b->y}, BULLET_RADIUS,
                                      (Vector2){z->x, z->y}, z->radius)) {
                z->hp -= b->damage;              /* only reduce hp: zombie.c removes the dead */
                pool_release(g->bullets, b);     /* the bullet is used up */
                break;
            }
        }
    }

    /* 2. Update bombs */
    float damp = expf(-BOMB_FRICTION * dt);
    for (size_t i = 0; i < pool_capacity(g->bombs); i++) {
        Bomb *b = pool_get(g->bombs, i);
        if (b == NULL) continue;

        if (b->state == BOMB_ARMED) {
            /* move one axis at a time and bounce softly off walls and closed gates */
            float next_x = b->x + b->vx * dt;
            if (world_circle_blocked(next_x, b->y, BOMB_RADIUS)) {
                b->vx = -b->vx * 0.5f;
            } else {
                b->x = next_x;
            }

            float next_y = b->y + b->vy * dt;
            if (world_circle_blocked(b->x, next_y, BOMB_RADIUS)) {
                b->vy = -b->vy * 0.5f;
            } else {
                b->y = next_y;
            }

            b->vx *= damp;
            b->vy *= damp;

            b->timer -= dt;
            if (b->timer <= 0.0f) {
                b->state = BOMB_EXPLODING;       /* lock state first so damage fires once */
                b->timer = BOMB_EXPLOSION_TIME;
                b->vx = 0.0f;
                b->vy = 0.0f;
                detonate_bomb(g, b);
            }
        } else {
            b->timer -= dt;
            if (b->timer <= 0.0f) {
                pool_release(g->bombs, b);
            }
        }
    }
}

void combat_draw(const GameState *g) {
    /* 1. Draw closest-target marker (and lock-on guide line if auto-aim is ON) */
    const Zombie *target = combat_closest_target(g);
    if (target != NULL && target->hp > 0) {
        float pulse = 2.0f * sinf((float)GetTime() * 8.0f);
        float r = target->radius + 7.0f + pulse;
        int tx = (int)target->x;
        int ty = (int)target->y;
        int tick = 7;

        Color marker = g->auto_aim ? RED : GOLD;

        if (g->auto_aim) {
            DrawLine((int)g->player_x, (int)g->player_y, tx, ty, (Color){255, 60, 60, 90});
        }

        DrawCircleLines(tx, ty, r, marker);
        DrawLine(tx - (int)r - tick, ty, tx - (int)r + 2, ty, marker);
        DrawLine(tx + (int)r - 2, ty, tx + (int)r + tick, ty, marker);
        DrawLine(tx, ty - (int)r - tick, tx, ty - (int)r + 2, marker);
        DrawLine(tx, ty + (int)r - 2, tx, ty + (int)r + tick, marker);
    }

    /* 2. Draw bullets */
    for (size_t i = 0; i < pool_capacity(g->bullets); i++) {
        const Bullet *b = pool_get(g->bullets, i);
        if (b == NULL) continue;          /* slot i is free */

        DrawCircle((int)b->x, (int)b->y, BULLET_RADIUS, YELLOW);
    }

    /* 3. Draw bombs and blast waves */
    for (size_t i = 0; i < pool_capacity(g->bombs); i++) {
        const Bomb *b = pool_get(g->bombs, i);
        if (b == NULL) continue;

        if (b->state == BOMB_ARMED) {
            /* faint preview ring showing the blast radius */
            DrawCircleLines((int)b->x, (int)b->y, b->blast_radius, (Color){255, 90, 40, 65});

            /* bomb body */
            DrawCircle((int)b->x, (int)b->y, BOMB_RADIUS, (Color){35, 35, 42, 255});
            DrawCircleLines((int)b->x, (int)b->y, BOMB_RADIUS, LIGHTGRAY);

            /* fuse indicator blinks faster as detonation approaches */
            float blink_speed = (b->timer < 0.45f) ? 18.0f : 7.0f;
            bool lit = ((int)(b->timer * blink_speed)) % 2 == 0;
            DrawCircle((int)b->x, (int)b->y, 4.0f, lit ? ORANGE : RED);
        } else {
            float progress = 1.0f - (b->timer / BOMB_EXPLOSION_TIME);
            if (progress < 0.0f) progress = 0.0f;
            if (progress > 1.0f) progress = 1.0f;

            float radius = b->blast_radius * (0.25f + 0.75f * progress);
            unsigned char fill_alpha = (unsigned char)((1.0f - progress) * 165.0f);
            unsigned char core_alpha = (unsigned char)((1.0f - progress) * 220.0f);
            unsigned char ring_alpha = (unsigned char)((1.0f - progress) * 255.0f);

            DrawCircle((int)b->x, (int)b->y, radius, (Color){255, 110, 30, fill_alpha});
            DrawCircle((int)b->x, (int)b->y, radius * 0.55f, (Color){255, 225, 100, core_alpha});
            DrawCircleLines((int)b->x, (int)b->y, radius, (Color){255, 240, 180, ring_alpha});
        }
    }
}