#include <math.h>
#include "raylib.h"
#include "player.h"
#include "assets.h"
#include "world.h"
#include "combat.h"
#include "weapon.h"
#include "zombie.h"

#define PLAYER_RADIUS        20.0f
#define PLAYER_SPEED         250.0f     /* pixels per second */
#define HERO_BASE_HEIGHT     84.0f      /* full-body scale matching Zone 1-4 props */
#define HERO_FOOT_OFFSET_Y   42.0f      /* centers hitbox on torso; keeps boots grounded */
#define THROW_ANIM_DURATION  0.30f      /* 0.11s wind-up + 0.19s release follow-through */
#define MUZZLE_FLASH_TIME    0.065f
#define RECOIL_TIME          0.095f

/* Per-frame anchors calibrated to hero.png (0..3 run, 4..5 armless idle, 6..7 throw) */
static const float FRAME_TORSO_X[8]    = { 0.56f, 0.62f, 0.56f, 0.62f, 0.54f, 0.54f, 0.58f, 0.48f };
static const float FRAME_SHOULDER_X[8] = { 0.44f, 0.48f, 0.44f, 0.48f, 0.16f, 0.16f, 0.48f, 0.48f };
static const float FRAME_SHOULDER_Y[8] = { 0.24f, 0.24f, 0.24f, 0.24f, 0.27f, 0.27f, 0.28f, 0.28f };

typedef struct {
    float target_w;
    float pivot_x_frac;
    float pivot_y_frac;
    float barrel_y_frac;
} WeaponRig;

/* Calibrated to the 2-handed modern tactical weapons sheet (0 pistol, 1 shotgun, 2 SMG) */
static const WeaponRig WEAPON_RIGS[3] = {
    /* WEAPON_PISTOL  */ { 48.0f, 0.10f, 0.37f, 0.25f },
    /* WEAPON_SHOTGUN */ { 60.0f, 0.15f, 0.27f, 0.33f },
    /* WEAPON_SMG     */ { 52.0f, 0.16f, 0.28f, 0.32f }
};

static bool  player_moving      = false;
static float run_anim_time      = 0.0f;
static float idle_anim_time     = 0.0f;
static float throw_anim_timer   = 0.0f;
static float muzzle_flash_timer = 0.0f;
static float recoil_timer       = 0.0f;
static float last_shot_angle    = 0.0f;

void player_reset(void) {
    player_moving      = false;
    run_anim_time      = 0.0f;
    idle_anim_time     = 0.0f;
    throw_anim_timer   = 0.0f;
    muzzle_flash_timer = 0.0f;
    recoil_timer       = 0.0f;
    last_shot_angle    = 0.0f;
}

void player_on_shoot(float aim_angle) {
    muzzle_flash_timer = MUZZLE_FLASH_TIME;
    recoil_timer       = RECOIL_TIME;
    last_shot_angle    = aim_angle;
}

void player_on_throw_bomb(void) {
    throw_anim_timer = THROW_ANIM_DURATION;
}

static int current_hero_frame(float *out_bob) {
    if (throw_anim_timer > 0.0f) {
        if (out_bob != NULL) *out_bob = 0.0f;
        return (throw_anim_timer > (THROW_ANIM_DURATION - 0.11f)) ? 6 : 7;
    }
    if (player_moving) {
        if (out_bob != NULL) *out_bob = -fabsf(sinf(run_anim_time * 1.5708f)) * 1.4f;
        return ((int)run_anim_time) % 4;
    }
    if (out_bob != NULL) *out_bob = sinf(idle_anim_time * 3.14159f) * 0.5f;
    return 4 + (((int)idle_anim_time) % 2);
}

static void compute_hero_pose(const GameState *g, const HeroAssets *h, float facing,
                              int frame, float stride_bob,
                              Rectangle *out_dst, Vector2 *out_shoulder) {
    float hero_scale = HERO_BASE_HEIGHT / h->hero_rects[4].height;
    Rectangle src    = h->hero_rects[frame];
    float draw_w     = src.width * hero_scale;
    float draw_h     = src.height * hero_scale;

    float torso_x = FRAME_TORSO_X[frame];
    float foot_y  = g->player_y + HERO_FOOT_OFFSET_Y;
    float draw_x  = (facing > 0.0f)
                  ? (g->player_x - draw_w * torso_x)
                  : (g->player_x - draw_w * (1.0f - torso_x));
    float draw_y  = foot_y - draw_h;

    if (out_dst != NULL) {
        *out_dst = (Rectangle){ draw_x, draw_y, draw_w, draw_h };
    }
    if (out_shoulder != NULL) {
        float sh_x_frac = (facing > 0.0f) ? FRAME_SHOULDER_X[frame] : (1.0f - FRAME_SHOULDER_X[frame]);
        out_shoulder->x = draw_x + draw_w * sh_x_frac;
        out_shoulder->y = draw_y + draw_h * FRAME_SHOULDER_Y[frame] + stride_bob;
    }
}

static Vector2 weapon_tip_from_shoulder(const HeroAssets *h, WeaponType type,
                                        Vector2 pivot, float aim_angle, bool flip_y) {
    int idx = (int)type;
    if (idx < 0 || idx > 2 || h->weapons.id == 0 || h->weapon_rects[idx].height <= 0.0f) {
        return (Vector2){ pivot.x + cosf(aim_angle) * 26.0f, pivot.y + sinf(aim_angle) * 26.0f };
    }

    const WeaponRig *rig = &WEAPON_RIGS[idx];
    Rectangle src = h->weapon_rects[idx];
    float draw_w  = rig->target_w;
    float draw_h  = src.height * (draw_w / src.width);

    float origin_x = draw_w * rig->pivot_x_frac;
    float dx_local = (draw_w - origin_x) - 1.0f;
    float dy_local = flip_y ? ((rig->pivot_y_frac - rig->barrel_y_frac) * draw_h)
                            : ((rig->barrel_y_frac - rig->pivot_y_frac) * draw_h);

    float ca = cosf(aim_angle);
    float sa = sinf(aim_angle);
    return (Vector2){
        pivot.x + dx_local * ca - dy_local * sa,
        pivot.y + dx_local * sa + dy_local * ca
    };
}

Vector2 player_muzzle_pos(const GameState *g, float aim_angle) {
    const HeroAssets *h = &assets_get()->hero;
    const Weapon *w = weapon_current(g);
    float facing = (cosf(aim_angle) < 0.0f) ? -1.0f : 1.0f;

    if (w == NULL || h->hero.id == 0 || h->hero_rects[4].height <= 0.0f) {
        return (Vector2){
            g->player_x + cosf(aim_angle) * 24.0f,
            g->player_y - 18.0f + sinf(aim_angle) * 24.0f
        };
    }

    float bob = 0.0f;
    int frame = current_hero_frame(&bob);
    Vector2 shoulder;
    compute_hero_pose(g, h, facing, frame, bob, NULL, &shoulder);
    return weapon_tip_from_shoulder(h, w->type, shoulder, aim_angle, facing < 0.0f);
}

void player_update(GameState *g) {
    float dt = GetFrameTime();

    float dx = 0.0f, dy = 0.0f;
    if (IsKeyDown(KEY_W)) dy -= 1.0f;
    if (IsKeyDown(KEY_S)) dy += 1.0f;
    if (IsKeyDown(KEY_A)) dx -= 1.0f;
    if (IsKeyDown(KEY_D)) dx += 1.0f;

    float len = sqrtf(dx * dx + dy * dy);
    player_moving = (len > 0.0f);

    if (player_moving) {
        dx /= len;
        dy /= len;

        float step = PLAYER_SPEED * dt;
        float new_x = g->player_x + dx * step;
        if (!world_circle_blocked(new_x, g->player_y, PLAYER_RADIUS)) {
            g->player_x = new_x;
        }

        float new_y = g->player_y + dy * step;
        if (!world_circle_blocked(g->player_x, new_y, PLAYER_RADIUS)) {
            g->player_y = new_y;
        }

        run_anim_time += dt * 9.5f;
    } else {
        idle_anim_time += dt * 2.6f;
    }

    if (throw_anim_timer > 0.0f) {
        throw_anim_timer -= dt;
        if (throw_anim_timer < 0.0f) throw_anim_timer = 0.0f;
    }
    if (muzzle_flash_timer > 0.0f) {
        muzzle_flash_timer -= dt;
        if (muzzle_flash_timer < 0.0f) muzzle_flash_timer = 0.0f;
    }
    if (recoil_timer > 0.0f) {
        recoil_timer -= dt;
        if (recoil_timer < 0.0f) recoil_timer = 0.0f;
    }
}

/* Draw 0 to 5 live bomb canisters clipped along his rear hip belt */
static void draw_belt_canisters(const HeroAssets *h, float px, Rectangle hero_dst,
                                float facing, int bomb_count) {
    if (bomb_count <= 0 || h->weapons.id == 0 || h->weapon_rects[3].height <= 0.0f) return;
    if (bomb_count > BOMB_MAX_AMMO) bomb_count = BOMB_MAX_AMMO;

    static const float SLOT_X[5] = { -11.0f, -15.0f, -19.0f, -13.0f, -17.0f };
    static const float SLOT_Y[5] = {  -2.0f,  -1.0f,   0.0f,   5.0f,   6.0f };

    Rectangle src = h->weapon_rects[3];
    float can_h   = 11.5f;
    float can_w   = src.width * (can_h / src.height);
    float tilt    = (facing > 0.0f) ? -14.0f : 14.0f;
    float hip_y   = hero_dst.y + hero_dst.height * 0.50f;

    if (facing < 0.0f) {
        src.width = -src.width;
    }

    for (int i = 0; i < bomb_count; i++) {
        float cx = px + SLOT_X[i] * facing;
        float cy = hip_y + SLOT_Y[i];

        DrawCircle((int)cx, (int)cy, 4.0f, (Color){ 45, 225, 255, 32 });

        Rectangle dst  = { cx, cy, can_w, can_h };
        Vector2 origin = { can_w * 0.5f, can_h * 0.5f };
        DrawTexturePro(h->weapons, src, dst, origin, tilt, WHITE);
    }
}

/* Draw the reactive glowing cyan charm aura clipped at his waist belt */
static void draw_talisman_glow(const GameState *g, Rectangle hero_dst, float facing) {
    float t = (float)GetTime();
    bool boss_on = boss_is_active();
    bool low_hp  = (g->player_hp <= 30);

    float pulse;
    if (boss_on) {
        pulse = 0.72f + 0.28f * sinf(t * 9.5f);
    } else if (low_hp) {
        pulse = 0.35f + 0.25f * sinf(t * 16.0f);
    } else {
        pulse = 0.78f + 0.22f * sinf(t * 3.8f);
    }

    float tx = g->player_x - 3.5f * facing;
    float ty = hero_dst.y + hero_dst.height * 0.53f;
    float outer_r = (boss_on ? 20.0f : 14.0f) * pulse;
    float inner_r = (boss_on ?  8.5f :  6.0f) * pulse;

    unsigned char outer_a = (unsigned char)((boss_on ? 64.0f : 38.0f) * pulse);
    unsigned char inner_a = (unsigned char)((boss_on ? 120.0f : 76.0f) * pulse);

    DrawCircle((int)tx, (int)ty, outer_r, (Color){ 35, 215, 255, outer_a });
    DrawCircle((int)tx, (int)ty, inner_r, (Color){ 155, 245, 255, inner_a });
}

/* Draw the grey-sleeved modern weapon locked directly onto his rounded shoulder cap */
static void draw_equipped_weapon(const GameState *g, const HeroAssets *h,
                                 Vector2 shoulder, float aim_angle, float facing) {
    const Weapon *w = weapon_current(g);
    if (w == NULL || h->weapons.id == 0) return;

    int idx = (int)w->type;
    if (idx < 0 || idx > 2 || h->weapon_rects[idx].height <= 0.0f) return;

    const WeaponRig *rig = &WEAPON_RIGS[idx];
    Rectangle src = h->weapon_rects[idx];
    float draw_w  = rig->target_w;
    float draw_h  = src.height * (draw_w / src.width);

    float kick = (recoil_timer > 0.0f) ? (4.5f * (recoil_timer / RECOIL_TIME)) : 0.0f;
    Vector2 pivot = {
        shoulder.x - cosf(aim_angle) * kick,
        shoulder.y - sinf(aim_angle) * kick
    };

    bool flip_y = (facing < 0.0f);
    Rectangle draw_src = src;
    if (flip_y) {
        draw_src.height = -draw_src.height;
    }

    float origin_x = draw_w * rig->pivot_x_frac;
    float origin_y = flip_y ? (draw_h * (1.0f - rig->pivot_y_frac)) : (draw_h * rig->pivot_y_frac);
    float rot_deg  = aim_angle * RAD2DEG;

    Rectangle dst = { pivot.x, pivot.y, draw_w, draw_h };
    DrawTexturePro(h->weapons, draw_src, dst, (Vector2){ origin_x, origin_y }, rot_deg, WHITE);

    /* Standalone muzzle flash snapped onto the exact rotated barrel tip */
    if (muzzle_flash_timer > 0.0f && h->weapon_rects[4].height > 0.0f) {
        Vector2 tip = weapon_tip_from_shoulder(h, w->type, pivot, last_shot_angle, flip_y);

        float frac = muzzle_flash_timer / MUZZLE_FLASH_TIME;
        float fl_s = (w->type == WEAPON_SHOTGUN) ? 1.35f : (w->type == WEAPON_SMG ? 0.95f : 1.0f);
        Rectangle fl_src = h->weapon_rects[4];
        float fl_w = 26.0f * fl_s * (0.75f + 0.35f * frac);
        float fl_h = fl_src.height * (fl_w / fl_src.width);

        DrawCircle((int)tip.x, (int)tip.y, 15.0f * fl_s * frac,
                   (Color){ 255, 190, 75, (unsigned char)(115.0f * frac) });

        Rectangle fl_dst = { tip.x, tip.y, fl_w, fl_h };
        Vector2 fl_orig  = { fl_w * 0.06f, fl_h * 0.50f };
        DrawTexturePro(h->weapons, fl_src, fl_dst, fl_orig, last_shot_angle * RAD2DEG,
                       (Color){ 255, 255, 255, (unsigned char)(255.0f * frac) });
    }
}

void player_draw(const GameState *g) {
    const HeroAssets *h = &assets_get()->hero;

    float aim_origin_y = g->player_y - 19.0f;

    /* Aim toward the locked zombie when auto-aim is ON, otherwise toward the cursor */
    const Zombie *target = g->auto_aim ? combat_closest_target(g) : NULL;
    float aim_angle;
    if (target != NULL && target->hp > 0) {
        aim_angle = atan2f(target->y - aim_origin_y, target->x - g->player_x);
    } else {
        Vector2 mouse = world_mouse_pos(g);
        aim_angle = atan2f(mouse.y - aim_origin_y, mouse.x - g->player_x);
    }

    float facing = (cosf(aim_angle) < 0.0f) ? -1.0f : 1.0f;

    /* Fallback circle if assets/hero/hero.png has not been saved yet */
    if (h->hero.id == 0 || h->hero_rects[4].height <= 0.0f) {
        DrawCircle((int)g->player_x, (int)g->player_y, PLAYER_RADIUS, SKYBLUE);
        float tipx = g->player_x + cosf(aim_angle) * (PLAYER_RADIUS + 15.0f);
        float tipy = g->player_y + sinf(aim_angle) * (PLAYER_RADIUS + 15.0f);
        DrawLine((int)g->player_x, (int)g->player_y, (int)tipx, (int)tipy,
                 (target != NULL && target->hp > 0) ? RED : WHITE);
        return;
    }

    float stride_bob = 0.0f;
    int frame        = current_hero_frame(&stride_bob);
    bool throwing    = (throw_anim_timer > 0.0f);

    Rectangle hero_dst;
    Vector2 shoulder;
    compute_hero_pose(g, h, facing, frame, stride_bob, &hero_dst, &shoulder);

    /* 1. Ground contact shadow */
    float foot_y = g->player_y + HERO_FOOT_OFFSET_Y;
    DrawEllipse((int)g->player_x, (int)(foot_y - 2.0f), 22.0f, 8.0f, (Color){ 6, 4, 14, 165 });

    /* 2. Live 0-5 bomb canisters clipped behind his hip belt */
    draw_belt_canisters(h, g->player_x, hero_dst, facing, g->bomb_count);

    /* 3. Animated hero body */
    Rectangle draw_src = h->hero_rects[frame];
    if (facing < 0.0f) {
        draw_src.width = -draw_src.width;
    }
    DrawTexturePro(h->hero, draw_src, hero_dst, (Vector2){ 0.0f, 0.0f }, 0.0f, WHITE);

    /* 4. Reactive glowing cyan belt talisman aura */
    draw_talisman_glow(g, hero_dst, facing);

    /* 5. Shoulder-mounted 360-degree rotating modern weapon + muzzle flash */
    if (!throwing) {
        draw_equipped_weapon(g, h, shoulder, aim_angle, facing);
    }

    /* 6. World-space cursor crosshair when aiming manually */
    if (!g->auto_aim) {
        Vector2 m = world_mouse_pos(g);
        int mx = (int)m.x;
        int my = (int)m.y;
        Color ret = { 120, 235, 255, 195 };
        DrawCircleLines(mx, my, 8.0f, ret);
        DrawLine(mx - 12, my, mx - 5, my, ret);
        DrawLine(mx + 5,  my, mx + 12, my, ret);
        DrawLine(mx, my - 12, mx, my - 5, ret);
        DrawLine(mx, my + 5,  mx, my + 12, ret);
    }
}