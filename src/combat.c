#include <math.h>
#include "raylib.h"
#include "combat.h"
#include "assets.h"
#include "player.h"
#include "zombie.h"
#include "world.h"
#include "weapon.h"

#define BULLET_RADIUS       4.0f
#define OFFSCREEN_MARGIN    50.0f
#define DEG_TO_RAD          0.0174532925f

#define PLAYER_RADIUS       20.0f
#define BOMB_RADIUS         10.0f
#define BOMB_BLAST_RADIUS   130.0f
#define BOMB_MAX_DAMAGE     120
#define BOMB_SELF_DAMAGE    25       /* set to 0 if you want no self-damage */
#define BOMB_FUSE_TIME      1.25f
#define BOMB_EXPLOSION_TIME 0.34f
#define BOMB_THROW_COOLDOWN 0.60f
#define BOMB_MAX_SPEED      520.0f
#define BOMB_FRICTION       3.2f
#define BOMB_KNOCKBACK      45.0f

static float fire_cooldown = 0.0f;
static float bomb_cooldown = 0.0f;
static float ref_player_x  = 0.0f;
static float ref_player_y  = 0.0f;

void combat_reset(void) {
    fire_cooldown = 0.3f;
    bomb_cooldown = 0.2f;
}

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

        int dmg = (int)((float)bomb->damage * (0.35f + 0.65f * falloff));
        if (dmg < 1) dmg = 1;
        z->hp -= dmg;

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

    const Weapon *weapon = weapon_current(g);
    if (weapon == NULL) return;
    const WeaponStats *stats = weapon_stats(weapon->type);

    float aim_origin_y = g->player_y - 19.0f;
    float aim = 0.0f;
    const Zombie *target = g->auto_aim ? combat_closest_target(g) : NULL;
    if (target != NULL && target->hp > 0) {
        float dx = target->x - g->player_x;
        float dy = target->y - aim_origin_y;
        if (dx * dx + dy * dy >= 1.0f) {
            aim = atan2f(dy, dx);
        }
    } else {
        Vector2 mouse = world_mouse_pos(g);
        float dx = mouse.x - g->player_x;
        float dy = mouse.y - aim_origin_y;
        if (sqrtf(dx * dx + dy * dy) < 1.0f) return;
        aim = atan2f(dy, dx);
    }

    /* Spawn bullets from the exact rotated barrel tip (with wall-safe fallback if pressed against cover) */
    Vector2 spawn = player_muzzle_pos(g, aim);
    if (world_is_wall(spawn.x, spawn.y)) {
        spawn.x = g->player_x + cosf(aim) * 14.0f;
        spawn.y = aim_origin_y + sinf(aim) * 14.0f;
    }

    int fired = 0;
    for (int i = 0; i < stats->pellets; i++) {
        float offset = 0.0f;
        if (stats->pellets > 1) {
            offset = ((float)i / (float)(stats->pellets - 1) - 0.5f) * stats->spread;
        } else if (stats->spread > 0.0f) {
            offset = ((float)GetRandomValue(-100, 100) / 100.0f) * stats->spread * 0.5f;
        }
        float angle = aim + offset * DEG_TO_RAD;

        Bullet *b = pool_acquire(g->bullets);
        if (b == NULL) break;

        b->x = spawn.x;
        b->y = spawn.y;
        b->vx = cosf(angle) * stats->speed;
        b->vy = sinf(angle) * stats->speed;
        b->damage = stats->damage;
        fired++;
    }

    if (fired > 0) {
        fire_cooldown = stats->interval;
        player_on_shoot(aim);
        weapon_use_ammo(g);
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
    b->y = g->player_y - 10.0f;

    if (dist > 1.0f) {
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
    player_on_throw_bomb();
}

void combat_update(GameState *g) {
    float dt = GetFrameTime();
    float w = world_width();
    float h = world_height();

    if (fire_cooldown > 0.0f) fire_cooldown -= dt;
    if (bomb_cooldown > 0.0f) bomb_cooldown -= dt;

    for (size_t i = 0; i < pool_capacity(g->bullets); i++) {
        Bullet *b = pool_get(g->bullets, i);
        if (b == NULL) continue;

        b->x += b->vx * dt;
        b->y += b->vy * dt;

        if (b->x < -OFFSCREEN_MARGIN || b->x > w + OFFSCREEN_MARGIN ||
            b->y < -OFFSCREEN_MARGIN || b->y > h + OFFSCREEN_MARGIN) {
            pool_release(g->bullets, b);
            continue;
        }

        if (world_is_wall(b->x, b->y)) {
            pool_release(g->bullets, b);
            continue;
        }

        for (ListNode *n = g->zombies->head; n != NULL; n = n->next) {
            Zombie *z = n->data;
            if (z->hp <= 0) continue;

            if (CheckCollisionCircles((Vector2){b->x, b->y}, BULLET_RADIUS,
                                      (Vector2){z->x, z->y}, z->radius)) {
                z->hp -= b->damage;
                pool_release(g->bullets, b);
                break;
            }
        }
    }

    float damp = expf(-BOMB_FRICTION * dt);
    for (size_t i = 0; i < pool_capacity(g->bombs); i++) {
        Bomb *b = pool_get(g->bombs, i);
        if (b == NULL) continue;

        if (b->state == BOMB_ARMED) {
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
                b->state = BOMB_EXPLODING;
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
    const HeroAssets *h = &assets_get()->hero;

    /* 1. Closest-target marker (and lock-on guide line when auto-aim is ON) */
    const Zombie *target = combat_closest_target(g);
    if (target != NULL && target->hp > 0) {
        float pulse = 2.0f * sinf((float)GetTime() * 8.0f);
        float r = target->radius + 7.0f + pulse;
        int tx = (int)target->x;
        int ty = (int)target->y;
        int tick = 7;

        Color marker = g->auto_aim ? RED : GOLD;

        if (g->auto_aim) {
            DrawLine((int)g->player_x, (int)(g->player_y - 19.0f), tx, ty, (Color){255, 60, 60, 85});
        }

        DrawCircleLines(tx, ty, r, marker);
        DrawLine(tx - (int)r - tick, ty, tx - (int)r + 2, ty, marker);
        DrawLine(tx + (int)r - 2, ty, tx + (int)r + tick, ty, marker);
        DrawLine(tx, ty - (int)r - tick, tx, ty - (int)r + 2, marker);
        DrawLine(tx, ty + (int)r - 2, tx, ty + (int)r + tick, marker);
    }

    /* 2. Directional tracer bullets */
    for (size_t i = 0; i < pool_capacity(g->bullets); i++) {
        const Bullet *b = pool_get(g->bullets, i);
        if (b == NULL) continue;

        Vector2 head = { b->x, b->y };
        Vector2 tail = { b->x - b->vx * 0.022f, b->y - b->vy * 0.022f };
        DrawLineEx(tail, head, 4.5f, (Color){ 255, 165, 45, 150 });
        DrawLineEx(tail, head, 2.2f, (Color){ 255, 245, 185, 235 });
        DrawCircle((int)b->x, (int)b->y, 3.0f, WHITE);
    }

    /* 3. Spinning bomb canisters and HD explosion clouds */
    for (size_t i = 0; i < pool_capacity(g->bombs); i++) {
        const Bomb *b = pool_get(g->bombs, i);
        if (b == NULL) continue;

        if (b->state == BOMB_ARMED) {
            float blink_speed = (b->timer < 0.45f) ? 18.0f : 7.0f;
            bool lit = ((int)(b->timer * blink_speed)) % 2 == 0;

            unsigned char ring_a = (b->timer < 0.45f && lit) ? 135 : 65;
            DrawCircleLines((int)b->x, (int)b->y, b->blast_radius, (Color){ 255, 95, 45, ring_a });

            if (h->weapons.id != 0 && h->weapon_rects[3].height > 0.0f) {
                Rectangle src = h->weapon_rects[3];
                float can_h   = 30.0f;
                float can_w   = src.width * (can_h / src.height);
                float speed   = sqrtf(b->vx * b->vx + b->vy * b->vy);
                float spin    = (speed > 20.0f)
                              ? (b->timer * -520.0f)
                              : (sinf((float)GetTime() * 14.0f) * 8.0f);

                DrawEllipse((int)b->x, (int)(b->y + 11.0f), 12.0f, 5.0f, (Color){ 0, 0, 0, 140 });
                DrawCircle((int)b->x, (int)b->y, lit ? 16.0f : 11.0f,
                           lit ? (Color){ 255, 145, 45, 75 } : (Color){ 45, 225, 255, 55 });

                Rectangle dst = { b->x, b->y, can_w, can_h };
                Vector2 orig  = { can_w * 0.5f, can_h * 0.5f };
                Color tint    = (b->timer < 0.40f && lit) ? (Color){ 255, 195, 165, 255 } : WHITE;
                DrawTexturePro(h->weapons, src, dst, orig, spin, tint);
            } else {
                DrawCircle((int)b->x, (int)b->y, BOMB_RADIUS, (Color){ 35, 35, 42, 255 });
                DrawCircleLines((int)b->x, (int)b->y, BOMB_RADIUS, LIGHTGRAY);
                DrawCircle((int)b->x, (int)b->y, 4.0f, lit ? ORANGE : RED);
            }
        } else {
            float progress = 1.0f - (b->timer / BOMB_EXPLOSION_TIME);
            if (progress < 0.0f) progress = 0.0f;
            if (progress > 1.0f) progress = 1.0f;

            float radius = b->blast_radius * (0.28f + 0.72f * progress);
            unsigned char ring_alpha = (unsigned char)((1.0f - progress) * 230.0f);
            DrawCircleLines((int)b->x, (int)b->y, radius, (Color){ 255, 215, 130, ring_alpha });

            if (h->weapons.id != 0 && h->weapon_rects[5].height > 0.0f) {
                Rectangle src = h->weapon_rects[5];
                float scale   = (b->blast_radius * 2.15f * (0.45f + 0.65f * progress)) / src.width;
                float draw_w  = src.width * scale;
                float draw_h  = src.height * scale;
                float fade    = (progress < 0.65f) ? 1.0f : ((1.0f - progress) / 0.35f);

                DrawCircle((int)b->x, (int)b->y, radius * 0.85f,
                           (Color){ 255, 120, 35, (unsigned char)(fade * 95.0f) });

                Rectangle dst = { b->x, b->y, draw_w, draw_h };
                Vector2 orig  = { draw_w * 0.5f, draw_h * 0.5f };
                DrawTexturePro(h->weapons, src, dst, orig, progress * 18.0f,
                               (Color){ 255, 255, 255, (unsigned char)(fade * 255.0f) });
            } else {
                unsigned char fill_alpha = (unsigned char)((1.0f - progress) * 165.0f);
                unsigned char core_alpha = (unsigned char)((1.0f - progress) * 220.0f);
                DrawCircle((int)b->x, (int)b->y, radius, (Color){ 255, 110, 30, fill_alpha });
                DrawCircle((int)b->x, (int)b->y, radius * 0.55f, (Color){ 255, 225, 100, core_alpha });
            }
        }
    }
}