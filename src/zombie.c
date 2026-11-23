#include <math.h>
#include <stdlib.h>
#include "raylib.h"
#include "zombie.h"
#include "assets.h"
#include "pickup.h"
#include "world.h"

#define PLAYER_RADIUS       20.0f
#define TOUCH_COOLDOWN      1.0f      /* seconds between hits on the player */
#define FIRST_BREAK         3.0f      /* quiet time before wave 1 */
#define BREAK_TIME          4.0f      /* quiet time between waves */
#define BOSS_TRIGGER_KILLS  65        /* total kills required in Zone 4 to summon the boss */
#define BOSS_MAX_MINIONS    6         /* cap on live minions during the boss fight */

typedef struct {
    int   hp;
    float speed;
    float radius;
    int   damage;
    float draw_height;
    float anim_fps;
    Color fallback_color;
    Color cue_color;
} ZombieStats;

/* one row per ZombieType, in the same order as the enum */
static const ZombieStats STATS[4] = {
    /* WALKER */ {   50,  70.0f, 18.0f, 10,  64.0f,  6.5f, {  30, 110,  40, 255 }, { 195, 225, 130, 220 } },
    /* RUNNER */ {   25, 150.0f, 14.0f,  8,  56.0f, 13.0f, { 230, 140,  30, 255 }, { 255, 155,  45, 235 } },
    /* TANK   */ {  160,  45.0f, 26.0f, 20,  86.0f,  5.5f, { 120,  40,  40, 255 }, { 255,  70,  80, 235 } },
    /* BOSS   */ { 1200,  68.0f, 38.0f, 25, 118.0f,  7.5f, { 145,  20,  75, 255 }, { 205,  70, 245, 240 } },
};

static int   wave        = 0;
static float break_timer = FIRST_BREAK;
static float spawn_timer = 0.0f;
static float hurt_timer  = 0.0f;
static float anim_clock  = 0.0f;

/* Final Boss state */
static bool  boss_spawned     = false;
static bool  boss_active      = false;
static bool  boss_defeated    = false;
static int   boss_cur_hp      = 0;
static int   boss_cur_max_hp  = 0;
static float boss_intro_t     = 0.0f;
static float boss_dash_cd     = 0.0f;
static float boss_telegraph_t = 0.0f;
static float boss_dash_t      = 0.0f;
static float boss_dash_vx     = 0.0f;
static float boss_dash_vy     = 0.0f;
static float boss_summon_cd   = 0.0f;
static float boss_summon_fx_t = 0.0f;

void zombies_reset(void) {
    wave        = 0;
    break_timer = FIRST_BREAK;
    spawn_timer = 0.0f;
    hurt_timer  = 0.0f;
    anim_clock  = 0.0f;

    boss_spawned     = false;
    boss_active      = false;
    boss_defeated    = false;
    boss_cur_hp      = 0;
    boss_cur_max_hp  = STATS[ZOMBIE_BOSS].hp;
    boss_intro_t     = 0.0f;
    boss_dash_cd     = 0.0f;
    boss_telegraph_t = 0.0f;
    boss_dash_t      = 0.0f;
    boss_dash_vx     = 0.0f;
    boss_dash_vy     = 0.0f;
    boss_summon_cd   = 0.0f;
    boss_summon_fx_t = 0.0f;
}

int   waves_number(void)      { return wave; }
bool  boss_is_active(void)    { return boss_active; }
bool  boss_is_defeated(void)  { return boss_defeated; }
int   boss_hp(void)           { return boss_cur_hp; }
int   boss_max_hp(void)       { return boss_cur_max_hp; }
int   boss_kills_needed(void) { return BOSS_TRIGGER_KILLS; }
float boss_intro_timer(void)  { return boss_intro_t; }

int waves_countdown(void) {
    if (boss_spawned) return 0;
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
        free(z);
    }
}

/* Spawn strictly inside the current active zone, safely away from the player */
static void spawn_near_player(GameState *g, ZombieType type) {
    float r    = STATS[type].radius;
    int   zone = world_active_zone();

    float min_x = world_unlocked_min_x() + r + 8.0f;
    float max_x = (float)(zone * ZONE_STRIDE_COLS * TILE_SIZE) - r - 8.0f;
    float min_y = world_ground_top() + r + 14.0f;
    float max_y = world_ground_bottom() - r - 14.0f;

    if (zone == 3) {
        min_y = world_ground_top() + 56.0f + r;
        max_y = world_ground_bottom() - 52.0f - r;
    }

    for (int attempt = 0; attempt < 40; attempt++) {
        float x = (float)GetRandomValue((int)min_x, (int)max_x);
        float y = (float)GetRandomValue((int)min_y, (int)max_y);

        float dx = x - g->player_x;
        float dy = y - g->player_y;
        if (dx * dx + dy * dy < 280.0f * 280.0f) continue;

        if (!world_circle_blocked(x, y, r)) {
            zombie_spawn(g, type, x, y);
            return;
        }
    }

    for (int attempt = 0; attempt < 40; attempt++) {
        float x = (float)GetRandomValue((int)min_x, (int)max_x);
        float y = (float)GetRandomValue((int)min_y, (int)max_y);

        float dx = x - g->player_x;
        float dy = y - g->player_y;
        if (dx * dx + dy * dy < 150.0f * 150.0f) continue;

        if (!world_circle_blocked(x, y, r)) {
            zombie_spawn(g, type, x, y);
            return;
        }
    }

    zombie_spawn(g, type, (min_x + max_x) * 0.5f, (min_y + max_y) * 0.5f);
}

/* Spawn The Night Spirit on the far open side of Zone 4 */
static void spawn_final_boss(GameState *g) {
    float r = STATS[ZOMBIE_BOSS].radius;
    float min_x = world_unlocked_min_x() + r + 30.0f;
    float max_x = (float)(4 * ZONE_STRIDE_COLS * TILE_SIZE) - r - 30.0f;
    float mid_x = (min_x + max_x) * 0.5f;
    float min_y = world_ground_top() + r + 24.0f;
    float max_y = world_ground_bottom() - r - 24.0f;

    float pref_min_x = (g->player_x < mid_x) ? (mid_x + 120.0f) : min_x;
    float pref_max_x = (g->player_x < mid_x) ? max_x : (mid_x - 120.0f);

    float bx = (pref_min_x + pref_max_x) * 0.5f;
    float by = (min_y + max_y) * 0.5f;

    for (int attempt = 0; attempt < 50; attempt++) {
        float tx = (float)GetRandomValue((int)pref_min_x, (int)pref_max_x);
        float ty = (float)GetRandomValue((int)min_y, (int)max_y);
        float dx = tx - g->player_x;
        float dy = ty - g->player_y;
        if (dx * dx + dy * dy < 320.0f * 320.0f) continue;

        if (!world_circle_blocked(tx, ty, r)) {
            bx = tx;
            by = ty;
            break;
        }
    }

    waves_clear(g);
    zombie_spawn(g, ZOMBIE_BOSS, bx, by);

    boss_spawned     = true;
    boss_active      = true;
    boss_cur_hp      = STATS[ZOMBIE_BOSS].hp;
    boss_cur_max_hp  = STATS[ZOMBIE_BOSS].hp;
    boss_intro_t     = 3.5f;
    boss_dash_cd     = 3.8f;
    boss_telegraph_t = 0.0f;
    boss_dash_t      = 0.0f;
    boss_summon_cd   = 6.5f;
    boss_summon_fx_t = 0.0f;
}

static void pull_into_active_zone(const GameState *g, Zombie *z) {
    int   zone  = world_active_zone();
    float r     = z->radius;
    float min_x = world_unlocked_min_x() + 420.0f;
    float max_x = (float)(zone * ZONE_STRIDE_COLS * TILE_SIZE) - r - 16.0f;
    if (min_x > max_x) min_x = world_unlocked_min_x() + r + 16.0f;

    float min_y = world_ground_top() + (zone == 3 ? 56.0f : 16.0f) + r;
    float max_y = world_ground_bottom() - (zone == 3 ? 52.0f : 16.0f) - r;

    for (int attempt = 0; attempt < 30; attempt++) {
        float tx = (float)GetRandomValue((int)min_x, (int)max_x);
        float ty = (float)GetRandomValue((int)min_y, (int)max_y);
        float dx = tx - g->player_x;
        float dy = ty - g->player_y;
        if (dx * dx + dy * dy < 240.0f * 240.0f) continue;

        if (!world_circle_blocked(tx, ty, r)) {
            z->x = tx;
            z->y = ty;
            return;
        }
    }

    z->x = max_x - 40.0f;
    z->y = (min_y + max_y) * 0.5f;
}

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

void waves_clear(GameState *g) {
    while (!queue_empty(g->wave_queue)) {
        free(queue_dequeue(g->wave_queue));
    }
}

static void zombie_move_toward(Zombie *z, float target_x, float target_y, float step) {
    float dx = target_x - z->x;
    float dy = target_y - z->y;
    float dist = sqrtf(dx * dx + dy * dy);
    if (dist < 0.001f) return;

    float nx = dx / dist;
    float ny = dy / dist;

    float direct_x = z->x + nx * step;
    float direct_y = z->y + ny * step;
    if (!world_circle_blocked(direct_x, direct_y, z->radius)) {
        z->x = direct_x;
        z->y = direct_y;
        return;
    }

    float slide_x = z->x;
    float slide_y = z->y;
    if (!world_circle_blocked(direct_x, slide_y, z->radius)) {
        slide_x = direct_x;
    }
    if (!world_circle_blocked(slide_x, direct_y, z->radius)) {
        slide_y = direct_y;
    }

    float mdx = slide_x - z->x;
    float mdy = slide_y - z->y;
    if (mdx * mdx + mdy * mdy >= (step * 0.45f) * (step * 0.45f)) {
        z->x = slide_x;
        z->y = slide_y;
        return;
    }

    static const float ANGLES[5] = { 0.55f, 1.05f, 1.55f, 2.05f, 2.50f };
    float mid_y = (world_ground_top() + world_ground_bottom()) * 0.5f;
    bool want_down = (z->y < mid_y);
    float pref = (nx >= 0.0f) ? (want_down ? 1.0f : -1.0f)
                              : (want_down ? -1.0f : 1.0f);

    for (int i = 0; i < 5; i++) {
        for (int s = 0; s < 2; s++) {
            float a  = ANGLES[i] * ((s == 0) ? pref : -pref);
            float ca = cosf(a);
            float sa = sinf(a);
            float sx = nx * ca - ny * sa;
            float sy = nx * sa + ny * ca;

            float cx = z->x + sx * step;
            float cy = z->y + sy * step;
            if (!world_circle_blocked(cx, cy, z->radius)) {
                z->x = cx;
                z->y = cy;
                return;
            }
        }
    }
}

/* Two-phase boss AI: steady pursuit, telegraphed shadow dashes, and minion summons */
static void boss_update_ai(GameState *g, Zombie *z, float dt) {
    bool enraged = (z->hp * 2 <= z->max_hp);
    boss_cur_hp  = z->hp;

    if (boss_summon_fx_t > 0.0f) {
        boss_summon_fx_t -= dt;
        if (boss_summon_fx_t < 0.0f) boss_summon_fx_t = 0.0f;
    }

    /* 1. Shadow Dash execution */
    if (boss_dash_t > 0.0f) {
        boss_dash_t -= dt;
        float dash_speed = enraged ? 360.0f : 300.0f;
        float sub_dt = dt * 0.5f;

        for (int step = 0; step < 2; step++) {
            float nx = z->x + boss_dash_vx * dash_speed * sub_dt;
            float ny = z->y + boss_dash_vy * dash_speed * sub_dt;
            if (!world_circle_blocked(nx, ny, z->radius)) {
                z->x = nx;
                z->y = ny;
            } else {
                boss_dash_t = 0.0f;
                break;
            }
        }
        return;
    }

    /* 2. Dash wind-up telegraph (aim locks in the second half so player can sidestep) */
    if (boss_telegraph_t > 0.0f) {
        boss_telegraph_t -= dt;
        if (boss_telegraph_t > 0.28f) {
            float dx = g->player_x - z->x;
            float dy = g->player_y - z->y;
            float d  = sqrtf(dx * dx + dy * dy);
            if (d > 0.001f) {
                boss_dash_vx = dx / d;
                boss_dash_vy = dy / d;
            }
        }
        if (boss_telegraph_t <= 0.0f) {
            boss_telegraph_t = 0.0f;
            boss_dash_t      = enraged ? 0.50f : 0.42f;
        }
        return;
    }

    /* 3. Standard pursuit */
    float move_speed = enraged ? 92.0f : z->speed;
    zombie_move_toward(z, g->player_x, g->player_y, move_speed * dt);

    /* 4. Start next shadow dash telegraph */
    boss_dash_cd -= dt;
    if (boss_dash_cd <= 0.0f) {
        float dx = g->player_x - z->x;
        float dy = g->player_y - z->y;
        float d  = sqrtf(dx * dx + dy * dy);
        if (d > 0.001f) {
            boss_dash_vx = dx / d;
            boss_dash_vy = dy / d;
        }
        boss_telegraph_t = enraged ? 0.55f : 0.75f;
        boss_dash_cd     = enraged ? 3.2f  : 5.0f;
    }

    /* 5. Summon minions to keep pressure up */
    boss_summon_cd -= dt;
    if (boss_summon_cd <= 0.0f) {
        boss_summon_cd = enraged ? 5.5f : 7.5f;
        if ((int)list_size(g->zombies) - 1 < BOSS_MAX_MINIONS) {
            boss_summon_fx_t = 0.65f;
            spawn_near_player(g, ZOMBIE_RUNNER);
            if ((int)list_size(g->zombies) - 1 < BOSS_MAX_MINIONS) {
                spawn_near_player(g, enraged ? ZOMBIE_RUNNER : ZOMBIE_WALKER);
            }
        }
    }
}

void zombies_update(GameState *g) {
    float dt = GetFrameTime();
    anim_clock += dt;

    if (boss_intro_t > 0.0f) {
        boss_intro_t -= dt;
        if (boss_intro_t < 0.0f) boss_intro_t = 0.0f;
    }

    /* Awaken the Final Boss once the player reaches Zone 4 and hits the kill target */
    if (!boss_spawned && world_active_zone() == 4 && g->kills >= BOSS_TRIGGER_KILLS) {
        spawn_final_boss(g);
    }

    bool waiting_for_next_zone = (world_active_zone() <= g->gates_open && world_active_zone() < 4);

    /* 1. Wave control (stops spawning normal waves once the boss awakens) */
    if (!waiting_for_next_zone && !boss_spawned) {
        if (break_timer > 0.0f) {
            break_timer -= dt;
            if (break_timer <= 0.0f) {
                break_timer = 0.0f;
                wave++;
                plan_wave(g);
                spawn_timer = 0.0f;
            }
        } else {
            float interval = 1.2f - 0.1f * (float)wave;
            if (interval < 0.4f) interval = 0.4f;

            spawn_timer += dt;
            if (!queue_empty(g->wave_queue) && spawn_timer >= interval) {
                spawn_timer = 0.0f;
                ZombieType *next_type = queue_dequeue(g->wave_queue);
                spawn_near_player(g, *next_type);
                free(next_type);
            }
        }
    }

    if (hurt_timer > 0.0f) hurt_timer -= dt;

    /* 2. Walk the list: remove the dead, update boss & minions */
    float min_valid_x = world_unlocked_min_x();
    bool boss_just_died = false;
    ListNode *node = g->zombies->head;
    while (node != NULL) {
        ListNode *next = node->next;
        Zombie *z = node->data;

        if (z->hp <= 0) {
            g->kills++;
            if (z->type == ZOMBIE_BOSS) {
                boss_active    = false;
                boss_cur_hp    = 0;
                boss_defeated  = true;
                boss_just_died = true;
            } else {
                pickup_drop(g, z->x, z->y);
            }
            list_remove(g->zombies, node);
            free(z);
        } else {
            if (z->x < min_valid_x + z->radius) {
                pull_into_active_zone(g, z);
            }

            if (z->type == ZOMBIE_BOSS) {
                boss_update_ai(g, z, dt);
            } else {
                zombie_move_toward(z, g->player_x, g->player_y, z->speed * dt);
            }

            float dx = g->player_x - z->x;
            float dy = g->player_y - z->y;
            float dist = sqrtf(dx * dx + dy * dy);
            if (dist < z->radius + PLAYER_RADIUS && hurt_timer <= 0.0f) {
                g->player_hp -= z->damage;
                if (g->player_hp < 0) g->player_hp = 0;
                hurt_timer = TOUCH_COOLDOWN;
            }
        }

        node = next;
    }

    /* Shatter the curse: wipe all remaining minions when the Night Spirit falls */
    if (boss_just_died) {
        waves_clear(g);
        ListNode *rem = g->zombies->head;
        while (rem != NULL) {
            ListNode *next = rem->next;
            free(rem->data);
            list_remove(g->zombies, rem);
            rem = next;
        }
        return;
    }

    /* 3. Wave finished: nothing waiting and nothing alive */
    if (!waiting_for_next_zone && !boss_spawned && break_timer <= 0.0f &&
        queue_empty(g->wave_queue) && list_size(g->zombies) == 0) {
        break_timer = BREAK_TIME;
    }
}

static void draw_boss(const GameState *g, const Zombie *z) {
    const EnemyAssets *ea = &assets_get()->enemies;
    float time    = (float)GetTime();
    bool  enraged = (z->hp * 2 <= z->max_hp);
    bool  dashing = (boss_dash_t > 0.0f);
    bool  windup  = (boss_telegraph_t > 0.0f);
    float bob     = sinf(time * 4.2f) * 5.0f;
    float cx      = z->x;
    float cy      = z->y + bob;

    Color aura_col = enraged ? (Color){ 255,  45,  75, 255 } : (Color){ 175,  55, 235, 255 };
    Color eye_col  = enraged ? (Color){ 255, 235,  90, 255 } : (Color){ 255,  70, 115, 255 };

    /* 1. Telegraph laser beam right before a Shadow Dash */
    if (windup) {
        float pulse = 0.5f + 0.5f * sinf(time * 28.0f);
        Color beam = { 255, 50, 85, (unsigned char)(110.0f + 120.0f * pulse) };
        Vector2 start = { cx, cy };
        Vector2 end   = { cx + boss_dash_vx * 290.0f, cy + boss_dash_vy * 290.0f };
        DrawLineEx(start, end, 4.0f + 3.0f * pulse, beam);
        DrawCircleLines((int)cx, (int)cy, z->radius + 14.0f + 8.0f * pulse, beam);
    }

    /* 2. Ground shadow & pulsing cursed aura (keeps cloak silhouette readable over dark rock) */
    DrawEllipse((int)z->x, (int)(z->y + z->radius * 0.78f), 44.0f, 14.0f, (Color){ 0, 0, 0, 175 });
    float aura_r = z->radius + 12.0f + 4.0f * sinf(time * 6.0f);
    DrawCircle((int)cx, (int)cy, aura_r + 10.0f, (Color){ aura_col.r, aura_col.g, aura_col.b, 42 });
    DrawCircle((int)cx, (int)cy, aura_r,        (Color){ aura_col.r, aura_col.g, aura_col.b, 78 });

    /* 3. Orbiting spirit wisps */
    float orbit_speed = enraged ? 4.4f : 2.6f;
    for (int i = 0; i < 3; i++) {
        float a  = time * orbit_speed + (float)i * 2.094f;
        float ox = cx + cosf(a) * (z->radius + 14.0f);
        float oy = cy + sinf(a) * (z->radius * 0.75f);
        DrawCircle((int)ox, (int)oy, 5.5f, aura_col);
        DrawCircle((int)ox, (int)oy, 2.5f, WHITE);
    }

    /* 4. HD Night Spirit sprite sheet (4x2: 0..3 hover, 4 telegraph, 5 dash, 6 summon, 7 enraged) */
    if (ea->boss.id != 0 && ea->boss_rects[0].height > 0.0f) {
        int frame;
        if (dashing)                frame = 5;
        else if (windup)            frame = 4;
        else if (boss_summon_fx_t > 0.0f) frame = 6;
        else if (enraged)           frame = 7;
        else                        frame = ((int)(anim_clock * STATS[ZOMBIE_BOSS].anim_fps)) % 4;

        Rectangle src = ea->boss_rects[frame];
        float target_h = STATS[ZOMBIE_BOSS].draw_height;
        float scale    = target_h / src.height;
        float draw_w   = src.width * scale;
        float draw_h   = target_h;

        float dir_x = dashing ? boss_dash_vx : (g->player_x - cx);
        if (dir_x < -0.01f) {
            src.width = -src.width;
        }

        float foot_y = z->y + z->radius * 0.85f + bob * 0.5f;
        Rectangle dst = { cx - draw_w * 0.5f, foot_y - draw_h, draw_w, draw_h };
        Color tint = enraged ? (Color){ 255, 225, 235, 255 } : WHITE;
        DrawTexturePro(ea->boss, src, dst, (Vector2){ 0.0f, 0.0f }, 0.0f, tint);
        return;
    }

    /* Fallback vector boss if assets/enemies/boss.png is absent */
    DrawCircle((int)cx, (int)cy, z->radius, (Color){ 22, 8, 34, 245 });
    DrawCircleLines((int)cx, (int)cy, z->radius, aura_col);
    DrawCircle((int)cx, (int)(cy - 6.0f), z->radius * 0.68f, (Color){ 42, 12, 56, 255 });

    DrawTriangle((Vector2){ cx - 22.0f, cy - 18.0f },
                 (Vector2){ cx - 10.0f, cy - 28.0f },
                 (Vector2){ cx - 26.0f, cy - 42.0f }, aura_col);
    DrawTriangle((Vector2){ cx + 10.0f, cy - 28.0f },
                 (Vector2){ cx + 22.0f, cy - 18.0f },
                 (Vector2){ cx + 26.0f, cy - 42.0f }, aura_col);

    float dx = g->player_x - cx;
    float dy = g->player_y - cy;
    float d  = sqrtf(dx * dx + dy * dy);
    float look_x = (d > 0.001f) ? (dx / d) * 6.0f : 0.0f;
    float look_y = (d > 0.001f) ? (dy / d) * 4.0f : 0.0f;

    DrawCircle((int)(cx - 10.0f + look_x), (int)(cy - 6.0f + look_y), 5.0f, eye_col);
    DrawCircle((int)(cx + 10.0f + look_x), (int)(cy - 6.0f + look_y), 5.0f, eye_col);
    DrawCircle((int)(cx - 10.0f + look_x), (int)(cy - 6.0f + look_y), 2.2f, WHITE);
    DrawCircle((int)(cx + 10.0f + look_x), (int)(cy - 6.0f + look_y), 2.2f, WHITE);
}

static void draw_minion_zombie(const GameState *g, const Zombie *z) {
    const EnemyAssets *ea = &assets_get()->enemies;
    const ZombieStats *st = &STATS[z->type];

    int row = 0;
    if (z->type == ZOMBIE_RUNNER)    row = 1;
    else if (z->type == ZOMBIE_TANK) row = 2;

    int base_idx = row * 4;
    float foot_y = z->y + z->radius * 0.85f;

    /* Ground shadow + subtle contrast cue so dark clothing/husk stands out over dark floors */
    DrawEllipse((int)z->x, (int)(foot_y - 2.0f), z->radius * 0.95f, 6.5f, (Color){ 0, 0, 0, 175 });
    DrawEllipseLines((int)z->x, (int)(foot_y - 2.0f), z->radius * 0.95f, 6.5f,
                     (Color){ st->cue_color.r, st->cue_color.g, st->cue_color.b, 85 });

    if (ea->zombies.id != 0 && ea->zombie_rects[base_idx].height > 0.0f) {
        int frame = ((int)(anim_clock * st->anim_fps)) % 4;
        Rectangle src = ea->zombie_rects[base_idx + frame];

        /* Scale relative to the Walker's baseline height so Tank is visibly massive and Runner nimble */
        float base_h   = ea->zombie_rects[0].height;
        float target_h = st->draw_height * (src.height / base_h);
        float draw_w   = src.width * (target_h / src.height);
        float draw_h   = target_h;

        float dx = g->player_x - z->x;
        if (dx < -0.01f) {
            src.width = -src.width;
        }

        /* Slight brightness lift + crisp outline separation against dark zones */
        Color tint = (z->type == ZOMBIE_TANK)
                   ? (Color){ 255, 232, 232, 255 }
                   : (Color){ 248, 244, 252, 255 };

        Rectangle dst = { z->x - draw_w * 0.5f, foot_y - draw_h, draw_w, draw_h };
        DrawTexturePro(ea->zombies, src, dst, (Vector2){ 0.0f, 0.0f }, 0.0f, tint);
    } else {
        DrawCircle((int)z->x, (int)z->y, z->radius, st->fallback_color);
        DrawCircleLines((int)z->x, (int)z->y, z->radius, WHITE);
    }

    /* Crisp floating health bar above damaged zombies */
    if (z->hp < z->max_hp) {
        float ratio = (float)z->hp / (float)z->max_hp;
        if (ratio < 0.0f) ratio = 0.0f;
        int bar_w = (int)(z->radius * 2.0f);
        int bar_x = (int)(z->x - z->radius);
        int bar_y = (int)(foot_y - st->draw_height - 10.0f);
        DrawRectangle(bar_x - 1, bar_y - 1, bar_w + 2, 6, (Color){ 10, 6, 16, 220 });
        DrawRectangle(bar_x, bar_y, bar_w, 4, DARKGRAY);
        DrawRectangle(bar_x, bar_y, (int)((float)bar_w * ratio), 4, RED);
    }
}

void zombies_draw(const GameState *g) {
    for (ListNode *node = g->zombies->head; node != NULL; node = node->next) {
        const Zombie *z = node->data;

        if (z->type == ZOMBIE_BOSS) {
            draw_boss(g, z);
        } else {
            draw_minion_zombie(g, z);
        }
    }
}