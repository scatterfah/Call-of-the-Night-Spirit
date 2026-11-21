#include <math.h>
#include <stdlib.h>
#include "raylib.h"
#include "world.h"
#include "assets.h"
#include "zombie.h"
#include "weapon.h"

#define MAX_HP            100
#define CAMERA_SMOOTH     8.0f
#define GATE_BANNER_TIME  3.5f
#define ZONE_BANNER_TIME  3.0f
#define GROUND_TOP_ROW    7       /* rows 0..6 are the sky & parallax skyline */
#define GROUND_BOTTOM_ROW 17      /* row 17 is the bottom foreground edge */

static const int GATE_QUOTAS[WORLD_GATE_COUNT] = { 10, 25, 45 };

static const char *ZONE_NAMES[4] = {
    "ZONE 1: BLOOD-MOON COURTYARD",
    "ZONE 2: CURSED CATACOMBS",
    "ZONE 3: DEAD STREETS",
    "ZONE 4: CRIMSON CANYON"
};

static int   active_zone       = 1;     /* advances 1 -> 2 -> 3 -> 4 and never goes back */
static int   open_gates        = 0;
static int   last_opened_gate  = 0;
static float gate_banner_timer = 0.0f;
static float zone_banner_timer = 0.0f;

/* Zone 1 Blood-Moon Courtyard drifting autumn leaves */
#define MAX_COURTYARD_LEAVES 24

typedef struct {
    float x;
    float y;
    float vx;
    float vy;
    float phase;
    float size;
    bool  crimson;
} CourtyardLeaf;

static CourtyardLeaf courtyard_leaves[MAX_COURTYARD_LEAVES];

/* Zone 2 Cursed Catacombs ambient smoke & ceiling drips */
#define MAX_CRYPT_SMOKE 18
#define MAX_CRYPT_DRIPS 12

typedef struct {
    float x;
    float y;
    float vx;
    float vy;
    float life;
    float max_life;
    float base_r;
} CryptSmoke;

typedef struct {
    float x;
    float y;
    float target_y;
    float vy;
    float splash_t;
} CryptDrip;

static CryptSmoke crypt_smoke[MAX_CRYPT_SMOKE];
static CryptDrip  crypt_drips[MAX_CRYPT_DRIPS];

/* Zone 3 rain & ambient effects */
#define MAX_RAIN_PARTICLES 85
#define MAX_SPLASH_EFFECTS 40

typedef struct {
    float x;
    float y;
    float speed_x;
    float speed_y;
    float len;
    float thickness;
} RainDrop;

typedef struct {
    float x;
    float y;
    float life;
    float max_life;
} Splash;

static RainDrop rain[MAX_RAIN_PARTICLES];
static Splash   splashes[MAX_SPLASH_EFFECTS];
static int      splash_count = 0;

/* Zone 4 Crimson Canyon volcanic embers & Night Spirit eclipse sparks */
#define MAX_CANYON_EMBERS 36

typedef struct {
    float x;
    float y;
    float vx;
    float vy;
    float life;
    float max_life;
    float size;
    bool  spirit;     /* violet-magenta spirit spark vs crimson-orange volcanic ember */
} CanyonEmber;

static CanyonEmber canyon_embers[MAX_CANYON_EMBERS];
static float       eclipse_flash_t  = 0.0f;
static float       eclipse_flash_cd = 2.6f;
static float       ambient_timer    = 0.0f;

typedef enum {
    PROP_GY_TREE,
    PROP_GY_STATUE,
    PROP_GY_GRAVES,
    PROP_GY_LAMP,
    PROP_CRYPT_COFFIN,
    PROP_CRYPT_BRAZIER,
    PROP_CRYPT_ALTAR,
    PROP_CRYPT_GATE,
    PROP_CITY_CAR_TAXI,
    PROP_CITY_CAR_POLICE,
    PROP_CITY_DUMPSTER,
    PROP_CITY_BARRICADE,
    PROP_CANYON_BOULDER,
    PROP_CANYON_OBELISK,
    PROP_CANYON_BONES,
    PROP_CANYON_TREE
} PropKind;

typedef struct {
    float    x;          /* world x centre */
    float    y;          /* world y ground contact (bottom of trunk/base) */
    float    radius;     /* solid collision radius (0 = walkable) */
    float    split_frac; /* top fraction drawn OVER player/zombies (0 = draw fully below) */
    PropKind kind;
    float    height;     /* target pixel height */
    Color    tint;
} ArenaProp;

/* Interactive cover inside the shooting ground across all 4 zones.
   Zone 1: x = 40..1200 | Zone 2: x = 1240..2400 | Zone 3: x = 2440..3600 | Zone 4: x = 3640..4800 */
static const ArenaProp ARENA_PROPS[] = {
    /* Zone 1: Blood-Moon Courtyard — gnarled oaks, angel statues, crooked graves & glowing lamps */
    {  175.0f, 455.0f, 13.0f, 0.62f, PROP_GY_TREE,         146.0f, {225, 212, 232, 255} },
    {  295.0f, 610.0f, 13.0f, 0.00f, PROP_GY_GRAVES,        74.0f, {218, 208, 228, 255} },
    {  345.0f, 348.0f, 10.0f, 0.52f, PROP_GY_LAMP,         116.0f, {235, 225, 235, 255} },
    {  615.0f, 610.0f, 14.0f, 0.56f, PROP_GY_STATUE,       112.0f, {225, 214, 235, 255} },
    {  665.0f, 445.0f, 13.0f, 0.62f, PROP_GY_TREE,         150.0f, {225, 212, 232, 255} },
    {  795.0f, 352.0f, 10.0f, 0.52f, PROP_GY_LAMP,         116.0f, {235, 225, 235, 255} },
    {  885.0f, 570.0f, 14.0f, 0.00f, PROP_GY_GRAVES,        76.0f, {218, 208, 228, 255} },
    { 1030.0f, 455.0f, 13.0f, 0.62f, PROP_GY_TREE,         144.0f, {225, 212, 232, 255} },
    { 1085.0f, 610.0f, 14.0f, 0.56f, PROP_GY_STATUE,       108.0f, {225, 214, 235, 255} },

    /* Zone 2: Cursed Catacombs — open coffins, blazing skull braziers, altars & spiked gates */
    { 1405.0f, 360.0f, 13.0f, 0.00f, PROP_CRYPT_BRAZIER,    68.0f, {235, 215, 215, 255} },
    { 1535.0f, 595.0f, 16.0f, 0.00f, PROP_CRYPT_COFFIN,     68.0f, {215, 198, 205, 255} },
    { 1690.0f, 445.0f, 15.0f, 0.00f, PROP_CRYPT_ALTAR,      64.0f, {220, 200, 205, 255} },
    { 1835.0f, 360.0f, 13.0f, 0.00f, PROP_CRYPT_BRAZIER,    68.0f, {235, 215, 215, 255} },
    { 1945.0f, 600.0f, 16.0f, 0.00f, PROP_CRYPT_GATE,       76.0f, {215, 198, 205, 255} },
    { 2110.0f, 450.0f, 16.0f, 0.00f, PROP_CRYPT_COFFIN,     68.0f, {215, 198, 205, 255} },
    { 2255.0f, 595.0f, 15.0f, 0.00f, PROP_CRYPT_ALTAR,      64.0f, {220, 200, 205, 255} },
    { 2305.0f, 360.0f, 13.0f, 0.00f, PROP_CRYPT_BRAZIER,    68.0f, {235, 215, 215, 255} },

    /* Zone 3: Dead Streets — illustrated cars, dumpsters & concrete barricades */
    { 2570.0f, 342.0f, 15.0f, 0.00f, PROP_CITY_DUMPSTER,    58.0f, {200, 200, 220, 255} },
    { 2725.0f, 445.0f, 17.0f, 0.00f, PROP_CITY_CAR_TAXI,    78.0f, {205, 205, 225, 255} },
    { 2890.0f, 595.0f, 15.0f, 0.00f, PROP_CITY_BARRICADE,   52.0f, {200, 200, 220, 255} },
    { 3010.0f, 342.0f, 15.0f, 0.00f, PROP_CITY_DUMPSTER,    58.0f, {200, 200, 220, 255} },
    { 3125.0f, 455.0f, 17.0f, 0.00f, PROP_CITY_CAR_POLICE,  80.0f, {210, 210, 230, 255} },
    { 3315.0f, 600.0f, 17.0f, 0.00f, PROP_CITY_CAR_TAXI,    78.0f, {205, 205, 225, 255} },
    { 3465.0f, 445.0f, 15.0f, 0.00f, PROP_CITY_BARRICADE,   52.0f, {200, 200, 220, 255} },
    { 3515.0f, 342.0f, 15.0f, 0.00f, PROP_CITY_DUMPSTER,    58.0f, {200, 200, 220, 255} },

    /* Zone 4: Crimson Canyon (Boss Arena) — open kiting lanes with dramatic perimeter cover */
    { 3785.0f, 355.0f, 13.0f, 0.58f, PROP_CANYON_OBELISK,  132.0f, {230, 212, 228, 255} },
    { 3875.0f, 610.0f, 16.0f, 0.00f, PROP_CANYON_BOULDER,   78.0f, {225, 205, 215, 255} },
    { 4045.0f, 415.0f, 13.0f, 0.60f, PROP_CANYON_TREE,     140.0f, {225, 208, 222, 255} },
    { 4215.0f, 615.0f, 15.0f, 0.00f, PROP_CANYON_BONES,     74.0f, {225, 210, 218, 255} },
    { 4315.0f, 355.0f, 13.0f, 0.58f, PROP_CANYON_OBELISK,  136.0f, {230, 212, 228, 255} },
    { 4480.0f, 595.0f, 16.0f, 0.00f, PROP_CANYON_BOULDER,   82.0f, {225, 205, 215, 255} },
    { 4615.0f, 435.0f, 13.0f, 0.60f, PROP_CANYON_TREE,     142.0f, {225, 208, 222, 255} },
    { 4685.0f, 610.0f, 13.0f, 0.58f, PROP_CANYON_OBELISK,  128.0f, {230, 212, 228, 255} },
};

static bool is_wide_prop(PropKind kind) {
    return kind == PROP_CITY_CAR_TAXI || kind == PROP_CITY_CAR_POLICE ||
           kind == PROP_CRYPT_COFFIN  || kind == PROP_CRYPT_GATE ||
           kind == PROP_GY_GRAVES     || kind == PROP_CANYON_BONES ||
           kind == PROP_CANYON_BOULDER;
}

static bool is_gy_sheet_prop(PropKind kind) {
    return kind == PROP_GY_TREE || kind == PROP_GY_STATUE ||
           kind == PROP_GY_GRAVES || kind == PROP_GY_LAMP;
}

static bool is_crypt_sheet_prop(PropKind kind) {
    return kind == PROP_CRYPT_COFFIN || kind == PROP_CRYPT_BRAZIER ||
           kind == PROP_CRYPT_ALTAR  || kind == PROP_CRYPT_GATE;
}

static bool is_city_sheet_prop(PropKind kind) {
    return kind == PROP_CITY_CAR_TAXI || kind == PROP_CITY_CAR_POLICE ||
           kind == PROP_CITY_DUMPSTER || kind == PROP_CITY_BARRICADE;
}

static bool is_canyon_sheet_prop(PropKind kind) {
    return kind == PROP_CANYON_BOULDER || kind == PROP_CANYON_OBELISK ||
           kind == PROP_CANYON_BONES   || kind == PROP_CANYON_TREE;
}

static int gy_prop_quadrant(PropKind kind) {
    if (kind == PROP_GY_TREE)   return 0;
    if (kind == PROP_GY_STATUE) return 1;
    if (kind == PROP_GY_GRAVES) return 2;
    return 3;
}

static int canyon_prop_quadrant(PropKind kind) {
    if (kind == PROP_CANYON_BOULDER) return 0;
    if (kind == PROP_CANYON_OBELISK) return 1;
    if (kind == PROP_CANYON_BONES)   return 2;
    return 3;
}

static Color fade_tint(Color c, float alpha) {
    if (alpha <= 0.0f) return (Color){ c.r, c.g, c.b, 0 };
    if (alpha >= 1.0f) return c;
    c.a = (unsigned char)((float)c.a * alpha);
    return c;
}

static Color blend4_color(Color a, float wa, Color b, float wb,
                          Color c, float wc, Color d, float wd) {
    float r  = (float)a.r * wa + (float)b.r * wb + (float)c.r * wc + (float)d.r * wd;
    float g  = (float)a.g * wa + (float)b.g * wb + (float)c.g * wc + (float)d.g * wd;
    float bl = (float)a.b * wa + (float)b.b * wb + (float)c.b * wc + (float)d.b * wd;
    float al = (float)a.a * wa + (float)b.a * wb + (float)c.a * wc + (float)d.a * wd;
    if (r  > 255.0f) r  = 255.0f;
    if (g  > 255.0f) g  = 255.0f;
    if (bl > 255.0f) bl = 255.0f;
    if (al > 255.0f) al = 255.0f;
    return (Color){ (unsigned char)r, (unsigned char)g, (unsigned char)bl, (unsigned char)al };
}

static float zone_blend(float camera_x, float start_x, float end_x) {
    float span = 180.0f;

    float in_t = (camera_x - (start_x - span)) / (2.0f * span);
    if (in_t < 0.0f) in_t = 0.0f;
    if (in_t > 1.0f) in_t = 1.0f;

    float out_t = ((end_x + span) - camera_x) / (2.0f * span);
    if (out_t < 0.0f) out_t = 0.0f;
    if (out_t > 1.0f) out_t = 1.0f;

    return (in_t < out_t) ? in_t : out_t;
}

static void draw_prop_anchored(Texture2D tex, float cx, float bottom_y,
                               float target_h, Color tint) {
    if (tex.id == 0 || tex.height <= 0) return;

    float scale = target_h / (float)tex.height;
    float draw_w = (float)tex.width * scale;
    Rectangle src = { 0.0f, 0.0f, (float)tex.width, (float)tex.height };
    Rectangle dst = { cx - draw_w * 0.5f, bottom_y - target_h, draw_w, target_h };
    DrawTexturePro(tex, src, dst, (Vector2){ 0.0f, 0.0f }, 0.0f, tint);
}

/* Draw either the bottom base or top canopy of a sprite-sheet prop for 2.5D depth */
static void draw_sheet_prop_split(Texture2D sheet, Rectangle rect, float cx, float bottom_y,
                                  float target_h, float split_frac, bool draw_top, Color tint) {
    if (sheet.id == 0 || rect.height <= 0.0f) return;

    float scale  = target_h / rect.height;
    float draw_w = rect.width * scale;

    if (split_frac <= 0.0f) {
        if (draw_top) return;
        Rectangle dst = { cx - draw_w * 0.5f, bottom_y - target_h, draw_w, target_h };
        DrawTexturePro(sheet, rect, dst, (Vector2){ 0.0f, 0.0f }, 0.0f, tint);
        return;
    }

    float split_h = rect.height * split_frac;
    if (draw_top) {
        float dst_h = split_h * scale;
        Rectangle src = { rect.x, rect.y, rect.width, split_h };
        Rectangle dst = { cx - draw_w * 0.5f, bottom_y - target_h, draw_w, dst_h };
        DrawTexturePro(sheet, src, dst, (Vector2){ 0.0f, 0.0f }, 0.0f, tint);
    } else {
        float bot_h = rect.height - split_h;
        float dst_h = bot_h * scale;
        Rectangle src = { rect.x, rect.y + split_h, rect.width, bot_h };
        Rectangle dst = { cx - draw_w * 0.5f, bottom_y - dst_h, draw_w, dst_h };
        DrawTexturePro(sheet, src, dst, (Vector2){ 0.0f, 0.0f }, 0.0f, tint);
    }
}

/* Subtle animated flapping bat silhouettes across the blood-moon sky */
static void draw_sky_bats(float sw, float max_y, float alpha) {
    if (alpha <= 0.05f) return;
    float t = (float)GetTime();
    Color bat_col = { 12, 4, 18, (unsigned char)(195.0f * alpha) };

    for (int i = 0; i < 6; i++) {
        float speed = 34.0f + (float)(i % 3) * 16.0f;
        float bx = fmodf(t * speed + (float)i * 215.0f, sw + 120.0f) - 60.0f;
        float by = 52.0f + (float)((i * 37) % (int)(max_y - 75.0f))
                 + sinf(t * (2.6f + (float)(i % 2)) + (float)i * 1.7f) * 12.0f;
        float wing = sinf(t * (13.0f + (float)(i % 3) * 2.5f) + (float)i) * 5.5f;
        float span = 7.0f + (float)(i % 3) * 2.2f;

        Vector2 c = { bx, by };
        Vector2 l = { bx - span, by - wing };
        Vector2 r = { bx + span, by - wing };
        DrawLineEx(l, c, 2.0f, bat_col);
        DrawLineEx(c, r, 2.0f, bat_col);
        DrawCircle((int)bx, (int)by, 1.8f, bat_col);
    }
}

/* Draw the HD weeping angel guardian statue at each frontier seal (with fallback) */
static void draw_guardian_statue(const WorldAssets *w, float gx, float bottom_y,
                                 float target_h, Color tint) {
    if (w->props.id != 0 && w->prop_rects[1].height > 0.0f) {
        draw_sheet_prop_split(w->props, w->prop_rects[1], gx, bottom_y, target_h, 0.0f, false, tint);
    } else if (w->fallback_statue.id != 0) {
        draw_prop_anchored(w->fallback_statue, gx, bottom_y, target_h, tint);
    }
}

/* Draw one of the 4 illustrated HD gothic courtyard props from assets/zone_1/props.png */
static void draw_gy_sheet_prop(const WorldAssets *w, const ArenaProp *p, bool draw_top, Color tint) {
    int q = gy_prop_quadrant(p->kind);
    if (w->props.id == 0 || w->prop_rects[q].height <= 0.0f) return;

    Rectangle rect = w->prop_rects[q];
    float scale  = p->height / rect.height;
    float draw_w = rect.width * scale;

    if (!draw_top) {
        DrawEllipse((int)p->x, (int)(p->y - 3.0f), draw_w * 0.42f, 11.0f, (Color){ 6, 4, 14, 165 });

        if (p->kind == PROP_GY_LAMP) {
            float fl = 0.82f + 0.18f * sinf(ambient_timer * 7.5f + p->x * 0.03f);
            DrawEllipse((int)p->x, (int)(p->y + 2.0f), 64.0f * fl, 24.0f * fl,
                        (Color){ 255, 175, 65, (unsigned char)(34.0f * fl) });
        } else if (p->kind == PROP_GY_TREE) {
            float fl = 0.84f + 0.16f * sinf(ambient_timer * 6.0f + p->x * 0.04f);
            DrawEllipse((int)(p->x + draw_w * 0.18f), (int)(p->y - 2.0f), 44.0f * fl, 16.0f * fl,
                        (Color){ 255, 165, 55, (unsigned char)(26.0f * fl) });
        }
    }

    draw_sheet_prop_split(w->props, rect, p->x, p->y, p->height, p->split_frac, draw_top, tint);

    if (draw_top) {
        if (p->kind == PROP_GY_LAMP) {
            float fl = 0.82f + 0.18f * sinf(ambient_timer * 8.5f + p->x);
            float ly = p->y - p->height * 0.84f;
            DrawCircle((int)p->x, (int)ly, 34.0f * fl, (Color){ 255, 175, 55, 42 });
            DrawCircle((int)p->x, (int)ly, 14.0f * fl, (Color){ 255, 225, 130, 52 });
        } else if (p->kind == PROP_GY_TREE) {
            float fl = 0.82f + 0.18f * sinf(ambient_timer * 7.0f + p->x);
            float lx = p->x + draw_w * 0.23f;
            float ly = p->y - p->height * 0.56f;
            DrawCircle((int)lx, (int)ly, 24.0f * fl, (Color){ 255, 165, 50, 38 });
            DrawCircle((int)lx, (int)ly, 10.0f * fl, (Color){ 255, 220, 125, 48 });
        }
    }
}

/* Draw one of the 4 illustrated HD gothic horror props from assets/zone_2/props.png */
static void draw_crypt_sheet_prop(const Zone2Assets *z2, PropKind kind,
                                  float cx, float bottom_y, float target_h, Color tint) {
    int q = 0;
    if (kind == PROP_CRYPT_COFFIN)       q = 0;
    else if (kind == PROP_CRYPT_BRAZIER) q = 1;
    else if (kind == PROP_CRYPT_ALTAR)   q = 2;
    else                                 q = 3;

    if (z2->props.id != 0 && z2->prop_rects[q].height > 0.0f) {
        Rectangle src = z2->prop_rects[q];
        float scale  = target_h / src.height;
        float draw_w = src.width * scale;

        DrawEllipse((int)cx, (int)(bottom_y - 3.0f), draw_w * 0.46f, 11.0f, (Color){ 6, 3, 6, 175 });

        if (kind == PROP_CRYPT_BRAZIER) {
            float flicker = 0.78f + 0.22f * sinf(ambient_timer * 11.0f + cx * 0.05f);
            DrawEllipse((int)cx, (int)(bottom_y - 4.0f), draw_w * 0.95f, 22.0f,
                        (Color){ 235, 80, 25, (unsigned char)(38.0f * flicker) });
        } else if (kind == PROP_CRYPT_COFFIN) {
            float pulse = 0.65f + 0.35f * sinf(ambient_timer * 2.8f + cx * 0.02f);
            DrawEllipse((int)cx, (int)(bottom_y - 4.0f), draw_w * 0.52f, 13.0f,
                        (Color){ 210, 28, 48, (unsigned char)(34.0f * pulse) });
        }

        Rectangle dst = { cx - draw_w * 0.5f, bottom_y - target_h, draw_w, target_h };
        DrawTexturePro(z2->props, src, dst, (Vector2){ 0.0f, 0.0f }, 0.0f, tint);

        if (kind == PROP_CRYPT_BRAZIER) {
            float flicker = 0.75f + 0.25f * sinf(ambient_timer * 13.0f + cx);
            float fy = bottom_y - target_h * 0.78f;
            DrawCircle((int)cx, (int)fy, 34.0f * flicker, (Color){ 255, 85, 25, 36 });
            DrawCircle((int)cx, (int)fy, 16.0f * flicker, (Color){ 255, 165, 65, 42 });
        } else if (kind == PROP_CRYPT_ALTAR) {
            float flicker = 0.80f + 0.20f * sinf(ambient_timer * 9.0f + cx);
            float fy = bottom_y - target_h * 0.80f;
            DrawCircle((int)cx, (int)fy, 24.0f * flicker, (Color){ 255, 115, 45, 28 });
        }
        return;
    }

    DrawEllipse((int)cx, (int)(bottom_y - 2.0f), 36.0f, 9.0f, (Color){ 10, 4, 8, 150 });
    DrawRectangle((int)cx - 28, (int)(bottom_y - target_h * 0.65f), 56, (int)(target_h * 0.65f),
                  (Color){ 46, 34, 42, 255 });
    DrawRectangleLines((int)cx - 28, (int)(bottom_y - target_h * 0.65f), 56, (int)(target_h * 0.65f),
                       (Color){ 16, 10, 14, 255 });
}

/* Draw one of the 4 illustrated HD street props from assets/zone_3/props.png */
static void draw_city_sheet_prop(const Zone3Assets *z3, PropKind kind,
                                 float cx, float bottom_y, float target_h, Color tint) {
    int q = 0;
    if (kind == PROP_CITY_CAR_TAXI)        q = 0;
    else if (kind == PROP_CITY_CAR_POLICE) q = 1;
    else if (kind == PROP_CITY_DUMPSTER)   q = 2;
    else                                   q = 3;

    if (z3->props.id != 0 && z3->prop_rects[q].height > 0.0f) {
        Rectangle src = z3->prop_rects[q];
        float scale  = target_h / src.height;
        float draw_w = src.width * scale;

        DrawEllipse((int)cx, (int)(bottom_y - 3.0f), draw_w * 0.44f, 12.0f, (Color){ 6, 6, 14, 155 });

        Rectangle dst = { cx - draw_w * 0.5f, bottom_y - target_h, draw_w, target_h };
        DrawTexturePro(z3->props, src, dst, (Vector2){ 0.0f, 0.0f }, 0.0f, tint);

        if (kind == PROP_CITY_CAR_POLICE) {
            bool blink  = ((int)(ambient_timer * 10.0f) % 3) != 0;
            bool is_red = ((int)(ambient_timer * 5.0f) % 2) == 0;
            if (blink) {
                float sx = cx + (is_red ? 2.0f : 12.0f);
                float sy = bottom_y - target_h * 0.90f;
                Color glow = is_red ? (Color){ 255, 45, 70, 68 } : (Color){ 55, 165, 255, 68 };
                Color core = is_red ? (Color){ 255, 130, 145, 175 } : (Color){ 155, 220, 255, 175 };
                DrawCircle((int)sx, (int)sy, 26.0f, glow);
                DrawCircle((int)sx, (int)sy, 7.0f,  core);
            }
        }
        return;
    }

    DrawEllipse((int)cx, (int)(bottom_y - 2.0f), 42.0f, 10.0f, (Color){ 8, 8, 16, 140 });
    DrawRectangle((int)cx - 34, (int)(bottom_y - target_h * 0.65f), 68, (int)(target_h * 0.65f),
                  (Color){ 48, 52, 66, 255 });
    DrawRectangleLines((int)cx - 34, (int)(bottom_y - target_h * 0.65f), 68, (int)(target_h * 0.65f),
                       (Color){ 16, 18, 26, 255 });
}

/* Draw one of the 4 illustrated HD Crimson Canyon boss arena props from assets/zone_4/props.png */
static void draw_canyon_sheet_prop(const Zone4Assets *z4, const ArenaProp *p, bool draw_top, Color tint) {
    int q = canyon_prop_quadrant(p->kind);
    if (z4->props.id == 0 || z4->prop_rects[q].height <= 0.0f) return;

    Rectangle rect = z4->prop_rects[q];
    float scale  = p->height / rect.height;
    float draw_w = rect.width * scale;
    bool boss_on = boss_is_active();
    bool boss_enraged = boss_on && (boss_hp() * 2 <= boss_max_hp());

    if (!draw_top) {
        DrawEllipse((int)p->x, (int)(p->y - 3.0f), draw_w * 0.44f, 11.0f, (Color){ 12, 4, 14, 170 });

        if (p->kind == PROP_CANYON_OBELISK) {
            float pulse = 0.68f + 0.32f * sinf(ambient_timer * (boss_enraged ? 7.5f : 3.8f) + p->x * 0.03f);
            Color rune_glow = boss_enraged ? (Color){ 255, 45, 85, (unsigned char)(54.0f * pulse) }
                                           : (Color){ 195, 55, 235, (unsigned char)((boss_on ? 48.0f : 32.0f) * pulse) };
            DrawEllipse((int)p->x, (int)(p->y - 2.0f), draw_w * 0.72f, 16.0f, rune_glow);
        } else if (p->kind == PROP_CANYON_BOULDER) {
            float pulse = 0.70f + 0.30f * sinf(ambient_timer * 3.2f + p->x * 0.02f);
            DrawEllipse((int)p->x, (int)(p->y - 2.0f), draw_w * 0.56f, 14.0f,
                        (Color){ 245, 85, 35, (unsigned char)(34.0f * pulse) });
        }
    }

    draw_sheet_prop_split(z4->props, rect, p->x, p->y, p->height, p->split_frac, draw_top, tint);

    /* Pulsing spirit rune aura on obelisks & ember glow in charred trees */
    if (draw_top) {
        if (p->kind == PROP_CANYON_OBELISK) {
            float pulse = 0.72f + 0.28f * sinf(ambient_timer * (boss_enraged ? 8.5f : 4.2f) + p->x);
            float oy = p->y - p->height * 0.62f;
            Color aura = boss_enraged ? (Color){ 255, 55, 95, 44 }
                                      : (Color){ 205, 65, 245, boss_on ? 42 : 28 };
            DrawCircle((int)p->x, (int)oy, 32.0f * pulse, aura);
        } else if (p->kind == PROP_CANYON_TREE) {
            float pulse = 0.76f + 0.24f * sinf(ambient_timer * 6.5f + p->x);
            float ty = p->y - p->height * 0.66f;
            DrawCircle((int)p->x, (int)ty, 28.0f * pulse, (Color){ 235, 70, 85, 24 });
        }
    }
}

static void draw_parallax_layer(Texture2D tex, float target_h, float bottom_y,
                                float scroll_x, Color tint) {
    if (tex.id == 0 || tex.height <= 0 || tint.a == 0) return;

    float sw = (float)GetScreenWidth();
    float scale = target_h / (float)tex.height;
    float tile_w = (float)tex.width * scale;
    if (tile_w < 1.0f) return;

    float offset = fmodf(scroll_x, tile_w);
    if (offset < 0.0f) offset += tile_w;

    Rectangle src = { 0.0f, 0.0f, (float)tex.width, (float)tex.height };
    for (float x = -offset; x < sw + tile_w; x += tile_w) {
        Rectangle dst = { x, bottom_y - target_h, tile_w + 1.0f, target_h };
        DrawTexturePro(tex, src, dst, (Vector2){ 0.0f, 0.0f }, 0.0f, tint);
    }
}

static char map_tile_at(int col, int row) {
    int left_wall_col = (active_zone - 1) * ZONE_STRIDE_COLS;
    if (col <= left_wall_col || col >= WORLD_COLS - 1) return '#';
    if (row < GROUND_TOP_ROW || row >= GROUND_BOTTOM_ROW) return '#';

    if (col == ZONE_STRIDE_COLS)     return '1';
    if (col == ZONE_STRIDE_COLS * 2) return '2';
    if (col == ZONE_STRIDE_COLS * 3) return '3';
    return '.';
}

static bool map_tile_solid(float x, float y) {
    if (x < 0.0f || y < 0.0f) return true;

    int col = (int)(x / (float)TILE_SIZE);
    int row = (int)(y / (float)TILE_SIZE);
    if (col >= WORLD_COLS || row >= WORLD_ROWS) return true;

    char tile = map_tile_at(col, row);
    if (tile == '#') return true;
    if (tile >= '1' && tile <= '3') {
        int gate_idx = tile - '1';
        return gate_idx >= open_gates;
    }
    return false;
}

/* Colossal Blood-Moon Cathedral backdrop for the Start Menu, High Scores & End Screens */
void world_draw_menu_bg(bool dim) {
    const WorldAssets *w = &assets_get()->world;
    float sw = (float)GetScreenWidth();
    float sh = (float)GetScreenHeight();
    Texture2D menu_tex = (w->bg.id != 0) ? w->bg : w->fallback_moon;

    if (menu_tex.id != 0) {
        float sx = sw / (float)menu_tex.width;
        float sy = sh / (float)menu_tex.height;
        float scale = (sx > sy) ? sx : sy;
        float draw_w = (float)menu_tex.width * scale;
        float draw_h = (float)menu_tex.height * scale;
        Rectangle src = { 0.0f, 0.0f, (float)menu_tex.width, (float)menu_tex.height };
        Rectangle dst = { (sw - draw_w) * 0.5f, (sh - draw_h) * 0.5f, draw_w, draw_h };
        DrawTexturePro(menu_tex, src, dst, (Vector2){ 0.0f, 0.0f }, 0.0f, WHITE);
    } else {
        ClearBackground((Color){ 16, 10, 24, 255 });
    }

    draw_sky_bats(sw, sh * 0.45f, dim ? 0.45f : 0.85f);

    if (dim) {
        DrawRectangle(0, 0, (int)sw, (int)sh, (Color){ 10, 4, 18, 145 });
    } else {
        DrawRectangleGradientV(0, (int)(sh * 0.55f), (int)sw, (int)(sh * 0.45f),
                               (Color){ 10, 4, 18, 0 }, (Color){ 10, 4, 18, 130 });
    }
}

void world_draw_play_bg(const GameState *g) {
    const WorldAssets *w  = &assets_get()->world;
    const Zone2Assets *z2 = &assets_get()->zone2;
    const Zone3Assets *z3 = &assets_get()->zone3;
    const Zone4Assets *z4 = &assets_get()->zone4;
    float sw = (float)GetScreenWidth();
    float sh = (float)GetScreenHeight();
    float ground_y = world_ground_top();

    float z2_start = (float)(ZONE_STRIDE_COLS * TILE_SIZE);
    float z3_start = (float)(ZONE_STRIDE_COLS * 2 * TILE_SIZE);
    float z4_start = (float)(ZONE_STRIDE_COLS * 3 * TILE_SIZE);

    bool has_z4 = (z4->bg.id != 0 || z4->buildings.id != 0);

    float z2_t = ((z2->bg.id != 0 || z2->buildings.id != 0) && active_zone <= 2)
               ? zone_blend(g->camera_x, z2_start, z3_start) : 0.0f;
    float z3_t = ((z3->bg.id != 0 || z3->buildings.id != 0) && active_zone <= 3)
               ? zone_blend(g->camera_x, z3_start, z4_start) : 0.0f;
    float z4_t = has_z4
               ? zone_blend(g->camera_x, z4_start, world_width() + 500.0f) : 0.0f;
    float z1_t = (active_zone == 1 || (!has_z4 && active_zone >= 3))
               ? (1.0f - z2_t - z3_t - z4_t) : 0.0f;
    if (z1_t < 0.0f) z1_t = 0.0f;
    if (z1_t + z2_t + z3_t + z4_t < 0.01f) z1_t = 1.0f;

    /* 1. Zone 1 Blood-Moon Courtyard: colossal blood-moon cathedral + iron gates & mausoleums */
    if (z1_t > 0.01f) {
        Texture2D sky_tex = (w->bg.id != 0) ? w->bg : w->fallback_moon;
        if (sky_tex.id != 0) {
            float extra_w = 180.0f;
            float draw_w  = sw + extra_w;
            float draw_h  = draw_w * ((float)sky_tex.height / (float)sky_tex.width);
            float min_h   = ground_y + 70.0f;
            if (draw_h < min_h) draw_h = min_h;

            float z1_span  = (float)(ZONE_STRIDE_COLS * TILE_SIZE) - sw;
            float progress = (z1_span > 1.0f) ? ((g->camera_x - sw * 0.5f) / z1_span) : 0.0f;
            if (progress < 0.0f) progress = 0.0f;
            if (progress > 1.0f) progress = 1.0f;

            Rectangle src = { 0.0f, 0.0f, (float)sky_tex.width, (float)sky_tex.height };
            Rectangle dst = { -extra_w * progress, -22.0f, draw_w, draw_h };
            DrawTexturePro(sky_tex, src, dst, (Vector2){ 0.0f, 0.0f }, 0.0f, fade_tint(WHITE, z1_t));
        } else {
            DrawRectangle(0, 0, (int)sw, (int)ground_y, fade_tint((Color){ 18, 8, 30, 255 }, z1_t));
        }

        draw_sky_bats(sw, ground_y - 30.0f, z1_t);

        DrawRectangleGradientV(0, (int)ground_y - 115, (int)sw, 125,
                               (Color){ 95, 28, 65, 0 },
                               fade_tint((Color){ 28, 12, 34, 118 }, z1_t));

        if (w->buildings.id != 0) {
            float lantern_pulse = 0.96f + 0.04f * sinf(ambient_timer * 6.5f);
            Color fence_tint = fade_tint((Color){
                (unsigned char)(215.0f * lantern_pulse),
                (unsigned char)(200.0f * lantern_pulse),
                (unsigned char)(220.0f * lantern_pulse),
                255
            }, z1_t);
            draw_parallax_layer(w->buildings, 188.0f, ground_y + 14.0f,
                                g->camera_x * 0.36f, fence_tint);
        }
    }

    /* 2. Zone 2 Cursed Catacombs: vaulted horror cavern + flickering torch-lit crypt walls */
    if (z2_t > 0.01f) {
        if (z2->bg.id != 0) {
            draw_parallax_layer(z2->bg, 290.0f, ground_y + 18.0f,
                                g->camera_x * 0.14f, fade_tint(WHITE, z2_t));
        } else {
            DrawRectangleGradientV(0, 0, (int)sw, (int)ground_y + 24,
                                   fade_tint((Color){ 14, 6, 12, 255 }, z2_t),
                                   fade_tint((Color){ 48, 14, 22, 255 }, z2_t));
        }

        DrawRectangleGradientV(0, (int)ground_y - 125, (int)sw, 135,
                               (Color){ 75, 16, 24, 0 },
                               fade_tint((Color){ 36, 10, 16, 135 }, z2_t));

        if (z2->buildings.id != 0) {
            float torch_flicker = 0.94f + 0.06f * sinf(ambient_timer * 9.5f)
                                        + 0.03f * sinf(ambient_timer * 23.0f);
            unsigned char r_c = (unsigned char)(215.0f * torch_flicker);
            unsigned char g_c = (unsigned char)(185.0f * torch_flicker);
            unsigned char b_c = (unsigned char)(182.0f * torch_flicker);
            draw_parallax_layer(z2->buildings, 205.0f, ground_y + 14.0f,
                                g->camera_x * 0.38f, fade_tint((Color){ r_c, g_c, b_c, 255 }, z2_t));
        }
    }

    /* 3. Zone 3 Dead Streets: moonlit ruined skyline + night-tinted storefronts flush with sidewalk */
    if (z3_t > 0.01f) {
        if (z3->bg.id != 0) {
            draw_parallax_layer(z3->bg, 290.0f, ground_y + 18.0f,
                                g->camera_x * 0.14f, fade_tint(WHITE, z3_t));
        } else {
            DrawRectangleGradientV(0, 0, (int)sw, (int)ground_y + 24,
                                   fade_tint((Color){ 16, 14, 34, 255 }, z3_t),
                                   fade_tint((Color){ 42, 34, 62, 255 }, z3_t));
        }

        DrawRectangleGradientV(0, (int)ground_y - 120, (int)sw, 130,
                               (Color){ 42, 36, 68, 0 },
                               fade_tint((Color){ 30, 26, 52, 115 }, z3_t));

        if (z3->buildings.id != 0) {
            Color night_brick = fade_tint((Color){ 175, 172, 205, 255 }, z3_t);
            draw_parallax_layer(z3->buildings, 205.0f, ground_y + 14.0f,
                                g->camera_x * 0.38f, night_brick);
        }
    }

    /* 4. Zone 4 Crimson Canyon (Final Boss Arena): eclipsed sky + obsidian ritual rim */
    if (z4_t > 0.01f) {
        bool boss_on      = boss_is_active();
        bool boss_enraged = boss_on && (boss_hp() * 2 <= boss_max_hp());

        if (z4->bg.id != 0) {
            /* Keep the colossal solar eclipse centered heroically over the final arena */
            float extra_w = 180.0f;
            float draw_w  = sw + extra_w;
            float draw_h  = draw_w * ((float)z4->bg.height / (float)z4->bg.width);
            float min_h   = ground_y + 70.0f;
            if (draw_h < min_h) draw_h = min_h;

            float z4_span  = (world_width() - z4_start) - sw;
            float progress = (z4_span > 1.0f) ? ((g->camera_x - (z4_start + sw * 0.5f)) / z4_span) : 0.5f;
            if (progress < 0.0f) progress = 0.0f;
            if (progress > 1.0f) progress = 1.0f;

            Color sky_tint = boss_enraged ? (Color){ 255, 210, 225, 255 } : WHITE;
            Rectangle src = { 0.0f, 0.0f, (float)z4->bg.width, (float)z4->bg.height };
            Rectangle dst = { -extra_w * progress, -24.0f, draw_w, draw_h };
            DrawTexturePro(z4->bg, src, dst, (Vector2){ 0.0f, 0.0f }, 0.0f, fade_tint(sky_tint, z4_t));
        } else {
            DrawRectangleGradientV(0, 0, (int)sw, (int)ground_y + 24,
                                   fade_tint((Color){ 48, 12, 34, 255 }, z4_t),
                                   fade_tint((Color){ 155, 42, 36, 255 }, z4_t));
        }

        /* Subtle crimson-violet sky pulse when the Night Spirit is enraged */
        if (eclipse_flash_t > 0.0f) {
            unsigned char fa = (unsigned char)(eclipse_flash_t * 110.0f * z4_t);
            Color flash_col = boss_enraged ? (Color){ 255, 55, 95, fa } : (Color){ 195, 75, 245, fa };
            DrawRectangle(0, 0, (int)sw, (int)ground_y + 24, flash_col);
        }

        /* Glowing crimson-violet chasm mist separating distant mesas from the ritual ridge */
        Color haze_top = boss_on ? (Color){ 145, 35, 165, 0 } : (Color){ 155, 42, 48, 0 };
        Color haze_bot = boss_enraged ? (Color){ 88, 14, 42, 138 }
                       : boss_on      ? (Color){ 58, 16, 68, 128 }
                                      : (Color){ 52, 16, 28, 118 };
        DrawRectangleGradientV(0, (int)ground_y - 120, (int)sw, 130,
                               haze_top, fade_tint(haze_bot, z4_t));

        if (z4->buildings.id != 0) {
            float spirit_pulse = 0.94f + 0.06f * sinf(ambient_timer * (boss_enraged ? 9.5f : 5.0f));
            Color ridge_tint = fade_tint((Color){
                (unsigned char)(228.0f * spirit_pulse),
                (unsigned char)(200.0f * spirit_pulse),
                (unsigned char)((boss_on ? 226.0f : 208.0f) * spirit_pulse),
                255
            }, z4_t);
            draw_parallax_layer(z4->buildings, 200.0f, ground_y + 14.0f,
                                g->camera_x * 0.36f, ridge_tint);
        }
    }

    /* 5. Base ground gradient (all 4 zones now draw illustrated HD floors on top) */
    Color z1_top_col = { 18, 16, 28, 255 };
    Color z1_bot_col = { 12, 10, 20, 255 };
    Color z2_top_col = { 26, 16, 18, 255 };
    Color z2_bot_col = { 16, 10, 12, 255 };
    Color z3_top_col = { 26, 29, 40, 255 };
    Color z3_bot_col = { 18, 20, 30, 255 };
    Color z4_top_col = { 34, 14, 24, 255 };
    Color z4_bot_col = { 20,  8, 16, 255 };

    Color ground_top_col = blend4_color(z1_top_col, z1_t, z2_top_col, z2_t,
                                        z3_top_col, z3_t, z4_top_col, z4_t);
    Color ground_bot_col = blend4_color(z1_bot_col, z1_t, z2_bot_col, z2_t,
                                        z3_bot_col, z3_t, z4_bot_col, z4_t);

    DrawRectangleGradientV(0, (int)ground_y, (int)sw, (int)(sh - ground_y),
                           ground_top_col, ground_bot_col);
}

void world_reset(void) {
    active_zone       = 1;
    open_gates        = 0;
    last_opened_gate  = 0;
    gate_banner_timer = 0.0f;
    zone_banner_timer = 0.0f;
    ambient_timer     = 0.0f;
    eclipse_flash_t   = 0.0f;
    eclipse_flash_cd  = 2.6f;

    /* Reset Zone 1 drifting autumn leaves */
    for (int i = 0; i < MAX_COURTYARD_LEAVES; i++) {
        courtyard_leaves[i].x       = (float)GetRandomValue(40, 1220);
        courtyard_leaves[i].y       = (float)GetRandomValue(180, 680);
        courtyard_leaves[i].vx      = -26.0f - (float)GetRandomValue(0, 28);
        courtyard_leaves[i].vy      = 14.0f + (float)GetRandomValue(0, 18);
        courtyard_leaves[i].phase   = (float)GetRandomValue(0, 628) * 0.01f;
        courtyard_leaves[i].size    = 3.0f + (float)GetRandomValue(0, 25) * 0.1f;
        courtyard_leaves[i].crimson = (i % 3 != 0);
    }

    /* Reset Zone 2 brazier smoke & vaulted ceiling drips */
    static const float BRAZIER_XS[3] = { 1405.0f, 1835.0f, 2305.0f };
    for (int i = 0; i < MAX_CRYPT_SMOKE; i++) {
        float bx = BRAZIER_XS[i % 3];
        crypt_smoke[i].x        = bx + (float)GetRandomValue(-10, 10);
        crypt_smoke[i].y        = 302.0f - (float)GetRandomValue(0, 48);
        crypt_smoke[i].vx       = (float)GetRandomValue(-10, 14);
        crypt_smoke[i].vy       = -18.0f - (float)GetRandomValue(0, 16);
        crypt_smoke[i].max_life = 1.8f + (float)GetRandomValue(0, 12) * 0.1f;
        crypt_smoke[i].life     = (float)GetRandomValue(1, 18) * 0.1f;
        crypt_smoke[i].base_r   = 9.0f + (float)GetRandomValue(0, 7);
    }
    for (int i = 0; i < MAX_CRYPT_DRIPS; i++) {
        crypt_drips[i].x        = (float)GetRandomValue(1260, 2360);
        crypt_drips[i].y        = (float)GetRandomValue(-260, 260);
        crypt_drips[i].target_y = (float)GetRandomValue(330, 620);
        crypt_drips[i].vy       = 340.0f + (float)GetRandomValue(0, 120);
        crypt_drips[i].splash_t = 0.0f;
    }

    /* Reset Zone 3 rain */
    for (int i = 0; i < MAX_RAIN_PARTICLES; i++) {
        rain[i].x         = (float)GetRandomValue(2400, 3680);
        rain[i].y         = (float)GetRandomValue(-200, 720);
        rain[i].speed_x   = -135.0f - (float)GetRandomValue(0, 70);
        rain[i].speed_y   = 520.0f + (float)GetRandomValue(0, 180);
        rain[i].len       = 0.015f + (float)GetRandomValue(0, 8) * 0.001f;
        rain[i].thickness = 0.85f + (float)GetRandomValue(0, 8) * 0.1f;
    }
    splash_count = 0;

    /* Reset Zone 4 rising embers & spirit sparks */
    for (int i = 0; i < MAX_CANYON_EMBERS; i++) {
        canyon_embers[i].x        = (float)GetRandomValue(3640, 4780);
        canyon_embers[i].y        = (float)GetRandomValue(260, 700);
        canyon_embers[i].vx       = (float)GetRandomValue(-24, 24);
        canyon_embers[i].vy       = -28.0f - (float)GetRandomValue(0, 36);
        canyon_embers[i].max_life = 2.0f + (float)GetRandomValue(0, 18) * 0.1f;
        canyon_embers[i].life     = (float)GetRandomValue(1, 20) * 0.1f;
        canyon_embers[i].size     = 2.0f + (float)GetRandomValue(0, 20) * 0.1f;
        canyon_embers[i].spirit   = (i % 3 == 0);
    }
}

float world_width(void)         { return (float)(WORLD_COLS * TILE_SIZE); }
float world_height(void)        { return (float)(WORLD_ROWS * TILE_SIZE); }
float world_ground_top(void)    { return (float)(GROUND_TOP_ROW * TILE_SIZE); }
float world_ground_bottom(void) { return (float)(GROUND_BOTTOM_ROW * TILE_SIZE); }
int   world_active_zone(void)   { return active_zone; }

float world_unlocked_min_x(void) {
    return (float)(((active_zone - 1) * ZONE_STRIDE_COLS + 1) * TILE_SIZE);
}

float world_unlocked_max_x(void) {
    return (float)((open_gates + 1) * ZONE_STRIDE_COLS * TILE_SIZE);
}

int world_zone_at(float x) {
    int col = (int)(x / (float)TILE_SIZE);
    if (col < ZONE_STRIDE_COLS)     return 1;
    if (col < ZONE_STRIDE_COLS * 2) return 2;
    if (col < ZONE_STRIDE_COLS * 3) return 3;
    return 4;
}

void world_update(GameState *g) {
    float dt = GetFrameTime();
    ambient_timer += dt;

    /* Update Zone 1 swaying autumn leaves */
    if (active_zone == 1) {
        for (int i = 0; i < MAX_COURTYARD_LEAVES; i++) {
            courtyard_leaves[i].phase += dt * 2.8f;
            courtyard_leaves[i].x     += courtyard_leaves[i].vx * dt;
            courtyard_leaves[i].y     += (courtyard_leaves[i].vy + sinf(courtyard_leaves[i].phase) * 18.0f) * dt;

            if (courtyard_leaves[i].x < 20.0f || courtyard_leaves[i].y > 700.0f) {
                courtyard_leaves[i].x = (float)GetRandomValue(180, 1260);
                courtyard_leaves[i].y = (float)GetRandomValue(160, 320);
            }
        }
    }

    /* Update Zone 2 brazier smoke curls & dark ceiling drips */
    if (active_zone == 2) {
        static const float BRAZIER_XS[3] = { 1405.0f, 1835.0f, 2305.0f };
        for (int i = 0; i < MAX_CRYPT_SMOKE; i++) {
            crypt_smoke[i].x    += crypt_smoke[i].vx * dt;
            crypt_smoke[i].y    += crypt_smoke[i].vy * dt;
            crypt_smoke[i].life -= dt;
            if (crypt_smoke[i].life <= 0.0f) {
                float bx = BRAZIER_XS[i % 3];
                crypt_smoke[i].x        = bx + (float)GetRandomValue(-9, 9);
                crypt_smoke[i].y        = 302.0f;
                crypt_smoke[i].vx       = (float)GetRandomValue(-8, 12);
                crypt_smoke[i].vy       = -18.0f - (float)GetRandomValue(0, 16);
                crypt_smoke[i].max_life = 1.8f + (float)GetRandomValue(0, 12) * 0.1f;
                crypt_smoke[i].life     = crypt_smoke[i].max_life;
                crypt_smoke[i].base_r   = 9.0f + (float)GetRandomValue(0, 7);
            }
        }

        for (int i = 0; i < MAX_CRYPT_DRIPS; i++) {
            if (crypt_drips[i].splash_t > 0.0f) {
                crypt_drips[i].splash_t -= dt;
                if (crypt_drips[i].splash_t <= 0.0f) {
                    crypt_drips[i].x        = (float)GetRandomValue(1260, 2360);
                    crypt_drips[i].y        = (float)GetRandomValue(-320, -20);
                    crypt_drips[i].target_y = (float)GetRandomValue(330, 620);
                    crypt_drips[i].vy       = 340.0f + (float)GetRandomValue(0, 120);
                }
            } else {
                crypt_drips[i].y += crypt_drips[i].vy * dt;
                if (crypt_drips[i].y >= crypt_drips[i].target_y) {
                    crypt_drips[i].y        = crypt_drips[i].target_y;
                    crypt_drips[i].splash_t = 0.28f;
                }
            }
        }
    }

    /* Update Zone 3 falling rain and pavement splashes */
    if (active_zone == 3) {
        float min_limit = world_unlocked_min_x();

        for (int i = 0; i < MAX_RAIN_PARTICLES; i++) {
            rain[i].x += rain[i].speed_x * dt;
            rain[i].y += rain[i].speed_y * dt;

            if (rain[i].y > 720.0f || rain[i].x < min_limit - 100.0f) {
                if (splash_count < MAX_SPLASH_EFFECTS && GetRandomValue(0, 9) < 4) {
                    Splash *sp = &splashes[splash_count++];
                    sp->x        = rain[i].x + (float)GetRandomValue(-12, 12);
                    sp->y        = world_ground_top() + (float)GetRandomValue(20, 360);
                    sp->life     = 0.22f;
                    sp->max_life = 0.22f;
                }

                rain[i].x = g->camera_x + (float)GetRandomValue(-540, 740);
                rain[i].y = (float)GetRandomValue(-150, -10);
            }
        }

        for (int i = 0; i < splash_count; i++) {
            splashes[i].life -= dt;
            if (splashes[i].life <= 0.0f) {
                splashes[i] = splashes[--splash_count];
                i--;
            }
        }
    }

    /* Update Zone 4 rising volcanic embers, spirit sparks & boss-reactive eclipse flashes */
    if (active_zone == 4) {
        bool boss_on      = boss_is_active();
        bool boss_enraged = boss_on && (boss_hp() * 2 <= boss_max_hp());
        float speed_mult  = boss_enraged ? 1.85f : (boss_on ? 1.35f : 1.0f);

        for (int i = 0; i < MAX_CANYON_EMBERS; i++) {
            canyon_embers[i].x    += (canyon_embers[i].vx + sinf(ambient_timer * 3.0f + (float)i) * 16.0f) * dt;
            canyon_embers[i].y    += canyon_embers[i].vy * speed_mult * dt;
            canyon_embers[i].life -= dt;

            if (canyon_embers[i].life <= 0.0f || canyon_embers[i].y < 140.0f) {
                canyon_embers[i].x        = (float)GetRandomValue(3640, 4780);
                canyon_embers[i].y        = (float)GetRandomValue(340, 710);
                canyon_embers[i].vx       = (float)GetRandomValue(-22, 22);
                canyon_embers[i].vy       = -28.0f - (float)GetRandomValue(0, 38);
                canyon_embers[i].max_life = 1.8f + (float)GetRandomValue(0, 16) * 0.1f;
                canyon_embers[i].life     = canyon_embers[i].max_life;
                canyon_embers[i].size     = 2.0f + (float)GetRandomValue(0, 22) * 0.1f;
                canyon_embers[i].spirit   = boss_on ? (GetRandomValue(0, 9) < 5) : (GetRandomValue(0, 9) < 2);
            }
        }

        if (eclipse_flash_t > 0.0f) {
            eclipse_flash_t -= dt * 3.2f;
            if (eclipse_flash_t < 0.0f) eclipse_flash_t = 0.0f;
        }
        if (boss_on) {
            eclipse_flash_cd -= dt;
            if (eclipse_flash_cd <= 0.0f) {
                eclipse_flash_t  = boss_enraged ? 0.55f : 0.32f;
                eclipse_flash_cd = boss_enraged ? (1.4f + (float)GetRandomValue(0, 16) * 0.1f)
                                                : (3.2f + (float)GetRandomValue(0, 24) * 0.1f);
            }
        }
    }

    /* Unlock only the seal belonging to the current active zone */
    if (g->gates_open < active_zone &&
        g->gates_open < WORLD_GATE_COUNT &&
        g->kills >= GATE_QUOTAS[g->gates_open]) {
        g->gates_open++;
        open_gates        = g->gates_open;
        last_opened_gate  = g->gates_open;
        gate_banner_timer = GATE_BANNER_TIME;
    }

    if (g->gates_open < WORLD_GATE_COUNT) {
        g->kill_quota = GATE_QUOTAS[g->gates_open];
    } else {
        g->kill_quota = GATE_QUOTAS[WORLD_GATE_COUNT - 1];
    }

    /* Once the player crosses into the next zone, lock the previous zone permanently */
    if (active_zone <= g->gates_open && active_zone < 4) {
        float next_zone_min_x = (float)(active_zone * ZONE_STRIDE_COLS + 1) * (float)TILE_SIZE;
        if (g->player_x >= next_zone_min_x + 30.0f) {
            active_zone++;
            zone_banner_timer = ZONE_BANNER_TIME;
            gate_banner_timer = 0.0f;

            float new_min_cam = world_unlocked_min_x() + (float)GetScreenWidth() * 0.5f;
            if (g->camera_x < new_min_cam) {
                g->camera_x = new_min_cam;
            }
        }
    }

    if (gate_banner_timer > 0.0f) {
        gate_banner_timer -= dt;
        if (gate_banner_timer < 0.0f) gate_banner_timer = 0.0f;
    }
    if (zone_banner_timer > 0.0f) {
        zone_banner_timer -= dt;
        if (zone_banner_timer < 0.0f) zone_banner_timer = 0.0f;
    }

    /* Smooth camera clamped strictly between active zone's left edge and frontier seal */
    float follow = 1.0f - expf(-CAMERA_SMOOTH * dt);
    g->camera_x += (g->player_x - g->camera_x) * follow;
    g->camera_y += (g->player_y - g->camera_y) * follow;

    float half_w = (float)GetScreenWidth() * 0.5f;
    float half_h = (float)GetScreenHeight() * 0.5f;

    float min_view_x = (active_zone > 1) ? world_unlocked_min_x() : 0.0f;
    float max_view_x = (g->gates_open < WORLD_GATE_COUNT)
                     ? (world_unlocked_max_x() + (float)TILE_SIZE)
                     : world_width();

    float min_x = min_view_x + half_w;
    float max_x = max_view_x - half_w;
    float max_y = world_height() - half_h;

    if (min_x >= max_x) {
        g->camera_x = (min_view_x + max_view_x) * 0.5f;
    } else {
        if (g->camera_x < min_x) g->camera_x = min_x;
        if (g->camera_x > max_x) g->camera_x = max_x;
    }

    if (half_h >= max_y) {
        g->camera_y = world_height() * 0.5f;
    } else {
        if (g->camera_y < half_h) g->camera_y = half_h;
        if (g->camera_y > max_y)  g->camera_y = max_y;
    }
}

Camera2D world_camera(const GameState *g) {
    Camera2D cam = { 0 };
    cam.offset   = (Vector2){ (float)GetScreenWidth() * 0.5f, (float)GetScreenHeight() * 0.5f };
    cam.target   = (Vector2){ g->camera_x, g->camera_y };
    cam.rotation = 0.0f;
    cam.zoom     = 1.0f;
    return cam;
}

Vector2 world_mouse_pos(const GameState *g) {
    return GetScreenToWorld2D(GetMousePosition(), world_camera(g));
}

static bool prop_hits_circle(const ArenaProp *p, float x, float y, float r) {
    if (p->radius <= 0.0f) return false;

    if (is_wide_prop(p->kind)) {
        float offset_x = (p->kind == PROP_CITY_CAR_TAXI || p->kind == PROP_CITY_CAR_POLICE) ? 30.0f : 22.0f;
        float cy = p->y - 16.0f;
        float min_d = r + p->radius;
        float min_d2 = min_d * min_d;

        float dx1 = x - (p->x - offset_x);
        float dy1 = y - cy;
        if (dx1 * dx1 + dy1 * dy1 < min_d2) return true;

        float dx2 = x - (p->x + offset_x);
        float dy2 = y - cy;
        if (dx2 * dx2 + dy2 * dy2 < min_d2) return true;

        return false;
    }

    float dx = x - p->x;
    float dy = y - (p->y - 12.0f);
    float min_d = r + p->radius;
    return (dx * dx + dy * dy) < (min_d * min_d);
}

bool world_is_wall(float x, float y) {
    if (map_tile_solid(x, y)) return true;

    int count = (int)(sizeof(ARENA_PROPS) / sizeof(ARENA_PROPS[0]));
    for (int i = 0; i < count; i++) {
        if (prop_hits_circle(&ARENA_PROPS[i], x, y, 0.0f)) {
            return true;
        }
    }
    return false;
}

bool world_circle_blocked(float x, float y, float r) {
    if (map_tile_solid(x - r, y - r) || map_tile_solid(x + r, y - r) ||
        map_tile_solid(x - r, y + r) || map_tile_solid(x + r, y + r)) {
        return true;
    }

    int count = (int)(sizeof(ARENA_PROPS) / sizeof(ARENA_PROPS[0]));
    for (int i = 0; i < count; i++) {
        if (prop_hits_circle(&ARENA_PROPS[i], x, y, r)) {
            return true;
        }
    }
    return false;
}

/* Helper to tile an illustrated HD battlefield floor across any single zone */
static void draw_zone_floor_tex(Texture2D tex, float zone_origin, float zone_right,
                                float top_y, float min_limit, float lock_limit,
                                Color tint, Color wall_shadow) {
    float left  = (zone_origin < min_limit) ? min_limit : zone_origin;
    float right = (zone_right > lock_limit) ? lock_limit : zone_right;
    if (left >= right || tex.id == 0 || tex.height <= 0) return;

    float draw_y = top_y;
    float draw_h = world_height() - top_y;
    float scale  = draw_h / (float)tex.height;
    float tile_w = (float)tex.width * scale;

    for (float tx = zone_origin; tx < right; tx += tile_w) {
        float seg_left  = (tx < left) ? left : tx;
        float seg_right = (tx + tile_w > right) ? right : (tx + tile_w);
        if (seg_right <= seg_left) continue;

        float src_x = (seg_left - tx) / scale;
        float src_w = (seg_right - seg_left) / scale;
        Rectangle src = { src_x, 0.0f, src_w, (float)tex.height };
        Rectangle dst = { seg_left, draw_y, (seg_right - seg_left) + 1.0f, draw_h };
        DrawTexturePro(tex, src, dst, (Vector2){ 0.0f, 0.0f }, 0.0f, tint);
    }

    DrawRectangleGradientV((int)left, (int)top_y, (int)(right - left), 24,
                           wall_shadow, (Color){ wall_shadow.r, wall_shadow.g, wall_shadow.b, 0 });
}

void world_draw(void) {
    const WorldAssets *w  = &assets_get()->world;
    const Zone2Assets *z2 = &assets_get()->zone2;
    const Zone3Assets *z3 = &assets_get()->zone3;
    const Zone4Assets *z4 = &assets_get()->zone4;
    float top_y      = world_ground_top();
    float bot_y      = world_ground_bottom();
    float time       = (float)GetTime();
    float min_limit  = (active_zone > 1) ? world_unlocked_min_x() : 0.0f;
    float lock_limit = (open_gates < WORLD_GATE_COUNT) ? world_unlocked_max_x() : world_width();

    /* 1. Illustrated HD floors across all 4 zones */
    float z1_end = (float)(ZONE_STRIDE_COLS * TILE_SIZE);
    float z2_end = (float)(ZONE_STRIDE_COLS * 2 * TILE_SIZE);
    float z3_end = (float)(ZONE_STRIDE_COLS * 3 * TILE_SIZE);

    draw_zone_floor_tex(w->floor,  0.0f,   z1_end,        top_y, min_limit, lock_limit,
                        (Color){ 222, 212, 232, 255 }, (Color){ 10, 6, 20, 175 });
    draw_zone_floor_tex(z2->floor, z1_end, z2_end,        top_y, min_limit, lock_limit,
                        (Color){ 215, 200, 205, 255 }, (Color){ 10, 4,  8, 185 });
    draw_zone_floor_tex(z3->road,  z2_end, z3_end,        top_y, min_limit, lock_limit,
                        (Color){ 215, 212, 232, 255 }, (Color){ 10, 8, 22, 165 });
    draw_zone_floor_tex(z4->floor, z3_end, world_width(), top_y, min_limit, lock_limit,
                        (Color){ 225, 208, 222, 255 }, (Color){ 14, 4, 16, 185 });

    /* 2. Arena props (only inside active/unlocked range) */
    int arena_count = (int)(sizeof(ARENA_PROPS) / sizeof(ARENA_PROPS[0]));
    for (int i = 0; i < arena_count; i++) {
        const ArenaProp *p = &ARENA_PROPS[i];
        if (p->x < min_limit || p->x > lock_limit) continue;

        if (is_gy_sheet_prop(p->kind)) {
            draw_gy_sheet_prop(w, p, false, p->tint);
        } else if (is_crypt_sheet_prop(p->kind)) {
            draw_crypt_sheet_prop(z2, p->kind, p->x, p->y, p->height, p->tint);
        } else if (is_city_sheet_prop(p->kind)) {
            draw_city_sheet_prop(z3, p->kind, p->x, p->y, p->height, p->tint);
        } else if (is_canyon_sheet_prop(p->kind)) {
            draw_canyon_sheet_prop(z4, p, false, p->tint);
        }
    }

    /* 3. Closed one-way barrier on the left edge when past Zone 1 */
    if (active_zone > 1) {
        int bx = (int)min_limit;
        DrawRectangle(bx - 12, (int)top_y, 16, (int)(bot_y - top_y), (Color){ 18, 10, 26, 235 });
        DrawLine(bx + 3, (int)top_y, bx + 3, (int)bot_y, (Color){ 165, 55, 85, 210 });
    }

    /* 4. Frontier seal separating the current zone from the next */
    for (int g = active_zone - 1; g <= open_gates && g < WORLD_GATE_COUNT; g++) {
        float gx = (float)((g + 1) * ZONE_STRIDE_COLS * TILE_SIZE + TILE_SIZE / 2);

        if (g >= open_gates) {
            float pulse = 0.5f + 0.5f * sinf(time * 5.0f);
            unsigned char glow_a = (unsigned char)(70.0f + 50.0f * pulse);
            unsigned char bar_a  = (unsigned char)(170.0f + 70.0f * pulse);

            DrawRectangle((int)gx - 22, (int)top_y, 44, (int)(bot_y - top_y),
                          (Color){ 190, 25, 65, glow_a });
            DrawRectangle((int)gx - 6,  (int)top_y, 12, (int)(bot_y - top_y),
                          (Color){ 255, 85, 115, bar_a });
            DrawLine((int)gx - 14, (int)top_y, (int)gx - 14, (int)bot_y, (Color){ 255, 150, 90, bar_a });
            DrawLine((int)gx + 14, (int)top_y, (int)gx + 14, (int)bot_y, (Color){ 255, 150, 90, bar_a });

            int gy = (int)((top_y + bot_y) * 0.5f);
            const char *g_title = TextFormat("SEAL %d", g + 1);
            const char *g_req   = TextFormat("%d KILLS", GATE_QUOTAS[g]);
            int tw1 = MeasureText(g_title, 18);
            int tw2 = MeasureText(g_req, 16);
            int box_w = (tw1 > tw2 ? tw1 : tw2) + 20;

            int bx = (int)gx - box_w - 18;
            DrawRectangle(bx, gy - 26, box_w, 52, (Color){ 16, 6, 24, 230 });
            DrawRectangleLines(bx, gy - 26, box_w, 52, (Color){ 255, 90, 110, 255 });
            DrawText(g_title, bx + (box_w - tw1) / 2, gy - 20, 18, (Color){ 255, 120, 110, 255 });
            DrawText(g_req,   bx + (box_w - tw2) / 2, gy + 3,  16, WHITE);
        } else {
            float pulse = 0.5f + 0.5f * sinf(time * 6.0f);
            unsigned char a = (unsigned char)(55.0f + 45.0f * pulse);
            DrawRectangle((int)gx - 10, (int)top_y, 20, (int)(bot_y - top_y),
                          (Color){ 80, 235, 150, a });
        }

        draw_guardian_statue(w, gx, top_y + 82.0f, 88.0f,
                             (g >= open_gates) ? (Color){ 255, 180, 190, 255 }
                                               : (Color){ 175, 245, 205, 240 });
    }
}

void world_draw_over(const GameState *g) {
    const WorldAssets *w  = &assets_get()->world;
    const Zone4Assets *z4 = &assets_get()->zone4;
    float time       = (float)GetTime();
    float bot_y      = world_ground_bottom();
    float min_limit  = (active_zone > 1) ? world_unlocked_min_x() : 0.0f;
    float lock_limit = (open_gates < WORLD_GATE_COUNT) ? world_unlocked_max_x() : world_width();

    /* 1. Upper canopies (Zone 1 oaks/statues/lamps & Zone 4 obelisks/charred trees) */
    int arena_count = (int)(sizeof(ARENA_PROPS) / sizeof(ARENA_PROPS[0]));
    for (int i = 0; i < arena_count; i++) {
        const ArenaProp *p = &ARENA_PROPS[i];
        if (p->split_frac <= 0.0f || p->x < min_limit || p->x > lock_limit) continue;

        Color tint = p->tint;
        float half_w = p->height * 0.42f;
        if (g->player_y < p->y - 16.0f && g->player_y > p->y - p->height - 16.0f &&
            fabsf(g->player_x - p->x) < half_w) {
            tint.a = 140;
        }

        if (is_gy_sheet_prop(p->kind)) {
            draw_gy_sheet_prop(w, p, true, tint);
        } else if (is_canyon_sheet_prop(p->kind)) {
            draw_canyon_sheet_prop(z4, p, true, tint);
        }
    }

    /* 2. Lower frontier seal guardian statues */
    for (int gate = active_zone - 1; gate <= open_gates && gate < WORLD_GATE_COUNT; gate++) {
        float gx = (float)((gate + 1) * ZONE_STRIDE_COLS * TILE_SIZE + TILE_SIZE / 2);
        draw_guardian_statue(w, gx, bot_y + 38.0f, 94.0f,
                             (gate >= open_gates) ? (Color){ 225, 160, 170, 230 }
                                                  : (Color){ 155, 220, 185, 210 });
    }

    /* 3. Zone 1 swaying autumn leaves & layered blood-moon courtyard mist */
    if (active_zone == 1) {
        for (int i = 0; i < MAX_COURTYARD_LEAVES; i++) {
            float lx = courtyard_leaves[i].x;
            float ly = courtyard_leaves[i].y;
            float sz = courtyard_leaves[i].size;
            float tilt = sinf(courtyard_leaves[i].phase) * 0.8f;
            Color col = courtyard_leaves[i].crimson
                      ? (Color){ 165, 42, 48, 210 }
                      : (Color){ 142, 92, 52, 195 };
            DrawEllipse((int)lx, (int)ly, sz * (1.2f + 0.3f * tilt), sz * 0.55f, col);
        }

        float span = lock_limit - min_limit;
        if (span < 100.0f) span = 100.0f;
        for (int i = 0; i < 9; i++) {
            float mx = min_limit + fmodf((float)i * 270.0f + time * (10.0f + (float)(i % 3) * 3.5f),
                                         span);
            float my = 325.0f + (float)((i * 83) % 325);
            float rx = 125.0f + (float)(i % 4) * 30.0f;
            float ry = 22.0f + (float)(i % 3) * 8.0f;
            Color mist = (i % 2 == 0) ? (Color){ 165, 65, 115, 18 }
                                      : (Color){ 135, 105, 175, 16 };
            DrawEllipse((int)mx, (int)my, rx, ry, mist);
        }
    }

    /* 4. Zone 2 curling dark brazier smoke & vaulted ceiling drips */
    if (active_zone == 2) {
        for (int i = 0; i < MAX_CRYPT_SMOKE; i++) {
            float t    = crypt_smoke[i].life / crypt_smoke[i].max_life;
            float fade = sinf(t * 3.14159f);
            float rad  = crypt_smoke[i].base_r * (1.0f + (1.0f - t) * 1.5f);
            unsigned char alpha = (unsigned char)(fade * 42.0f);
            DrawEllipse((int)crypt_smoke[i].x, (int)crypt_smoke[i].y, rad * 1.25f, rad * 0.85f,
                        (Color){ 48, 22, 28, alpha });
        }

        for (int i = 0; i < MAX_CRYPT_DRIPS; i++) {
            if (crypt_drips[i].splash_t > 0.0f) {
                float frac = crypt_drips[i].splash_t / 0.28f;
                float r_sz = 3.0f + (1.0f - frac) * 7.0f;
                DrawEllipseLines((int)crypt_drips[i].x, (int)crypt_drips[i].y, r_sz, r_sz * 0.42f,
                                 (Color){ 155, 32, 46, (unsigned char)(frac * 155.0f) });
            } else if (crypt_drips[i].y > 0.0f) {
                Vector2 top = { crypt_drips[i].x, crypt_drips[i].y - 10.0f };
                Vector2 bot = { crypt_drips[i].x, crypt_drips[i].y };
                DrawLineEx(top, bot, 1.8f, (Color){ 155, 30, 44, 150 });
            }
        }
    }

    /* 5. Zone 3 rain streaks & pavement splashes */
    if (active_zone == 3) {
        for (int i = 0; i < MAX_RAIN_PARTICLES; i++) {
            Vector2 start = { rain[i].x, rain[i].y };
            Vector2 end   = { rain[i].x + rain[i].speed_x * rain[i].len,
                              rain[i].y + rain[i].speed_y * rain[i].len };
            DrawLineEx(start, end, rain[i].thickness, (Color){ 145, 185, 230, 72 });
        }

        for (int i = 0; i < splash_count; i++) {
            float frac = splashes[i].life / splashes[i].max_life;
            float r_sz = 3.0f + (1.0f - frac) * 4.0f;
            DrawEllipse((int)splashes[i].x, (int)splashes[i].y, r_sz, r_sz * 0.4f,
                        (Color){ 175, 205, 240, (unsigned char)(frac * 95.0f) });
        }
    }

    /* 6. Zone 4 rising volcanic embers & violet Night Spirit sparks */
    if (active_zone == 4) {
        for (int i = 0; i < MAX_CANYON_EMBERS; i++) {
            float t    = canyon_embers[i].life / canyon_embers[i].max_life;
            float fade = sinf(t * 3.14159f);
            float ex   = canyon_embers[i].x;
            float ey   = canyon_embers[i].y;
            float sz   = canyon_embers[i].size;

            Color glow = canyon_embers[i].spirit
                       ? (Color){ 205, 65, 245, (unsigned char)(fade * 85.0f) }
                       : (Color){ 255, 90,  35, (unsigned char)(fade * 95.0f) };
            Color core = canyon_embers[i].spirit
                       ? (Color){ 245, 185, 255, (unsigned char)(fade * 225.0f) }
                       : (Color){ 255, 220, 120, (unsigned char)(fade * 235.0f) };

            DrawCircle((int)ex, (int)ey, sz * 2.2f, glow);
            DrawCircle((int)ex, (int)ey, sz * 0.75f, core);
        }
    }
}

void hud_draw(const GameState *g) {
    int screen_w = GetScreenWidth();
    int screen_h = GetScreenHeight();
    int zone = active_zone;
    bool zone_cleared = (active_zone <= g->gates_open && active_zone < 4);

    /* Subtle dark atmospheric border (pulses crimson during enraged boss phase) */
    bool boss_enraged = boss_is_active() && (boss_hp() * 2 <= boss_max_hp());
    Color edge_col = boss_enraged ? (Color){ 65, 6, 18, 125 } : (Color){ 6, 2, 12, 115 };
    DrawRectangleGradientH(0, 0, 55, screen_h, edge_col, (Color){ edge_col.r, edge_col.g, edge_col.b, 0 });
    DrawRectangleGradientH(screen_w - 55, 0, 55, screen_h, (Color){ edge_col.r, edge_col.g, edge_col.b, 0 }, edge_col);

    /* Dark strip behind the top HUD */
    DrawRectangle(0, 0, screen_w, 38, (Color){ 8, 4, 16, 200 });

    /* Subtle dark strip along the bottom edge so controls and weapon HUD stay readable */
    DrawRectangleGradientV(0, screen_h - 90, screen_w, 90,
                           (Color){ 8, 4, 16, 0 }, (Color){ 8, 4, 16, 215 });

    /* Health bar */
    float ratio = (float)g->player_hp / (float)MAX_HP;
    if (ratio < 0.0f) ratio = 0.0f;
    if (ratio > 1.0f) ratio = 1.0f;

    DrawText("HP", 15, 9, 20, WHITE);
    DrawRectangle(50, 10, 180, 18, DARKGRAY);
    DrawRectangle(50, 10, (int)(180.0f * ratio), 18, ratio > 0.3f ? GREEN : RED);
    DrawRectangleLines(50, 10, 180, 18, WHITE);
    DrawText(TextFormat("%d", g->player_hp), 238, 9, 20, WHITE);

    /* Zone, wave, zombies alive and incoming */
    const char *info = TextFormat("Zone %d/4   Wave %d   Alive: %d   Incoming: %d",
                                  zone, waves_number(), (int)list_size(g->zombies),
                                  (int)queue_size(g->wave_queue));
    DrawText(info, (screen_w - MeasureText(info, 20)) / 2, 9, 20, WHITE);

    /* Kills / boss objective on the right */
    const char *kills;
    Color kill_color = WHITE;
    if (zone_cleared) {
        kills = TextFormat("ENTER ZONE %d ->", active_zone + 1);
        kill_color = GREEN;
    } else if (g->gates_open < WORLD_GATE_COUNT) {
        kills = TextFormat("Seal %d: %d / %d", g->gates_open + 1, g->kills, g->kill_quota);
    } else if (boss_is_active()) {
        kills = "BOSS: NIGHT SPIRIT";
        kill_color = (Color){ 255, 75, 110, 255 };
    } else {
        kills = TextFormat("Boss in: %d / %d", g->kills, boss_kills_needed());
        kill_color = GOLD;
    }
    DrawText(kills, screen_w - MeasureText(kills, 20) - 15, 9, 20, kill_color);

    /* Wide Boss Health Bar across top-centre during the final fight */
    if (boss_is_active() && boss_max_hp() > 0) {
        int bar_w = 500;
        int bar_h = 20;
        int bar_x = (screen_w - bar_w) / 2;
        int bar_y = 70;
        float b_ratio = (float)boss_hp() / (float)boss_max_hp();
        if (b_ratio < 0.0f) b_ratio = 0.0f;
        if (b_ratio > 1.0f) b_ratio = 1.0f;

        const char *b_name = boss_enraged ? "THE NIGHT SPIRIT  [ENRAGED]" : "THE NIGHT SPIRIT";
        int nw = MeasureText(b_name, 20);
        DrawRectangle(bar_x - 12, bar_y - 26, bar_w + 24, 56, (Color){ 14, 4, 20, 210 });
        DrawText(b_name, (screen_w - nw) / 2, bar_y - 23, 20, boss_enraged ? RED : GOLD);
        DrawRectangle(bar_x, bar_y, bar_w, bar_h, (Color){ 36, 16, 30, 255 });
        DrawRectangle(bar_x, bar_y, (int)((float)bar_w * b_ratio), bar_h,
                      boss_enraged ? (Color){ 235, 35, 65, 255 } : (Color){ 175, 45, 210, 255 });
        DrawRectangleLines(bar_x, bar_y, bar_w, bar_h, boss_enraged ? GOLD : WHITE);

        const char *hp_txt = TextFormat("%d / %d", boss_hp(), boss_max_hp());
        int hw = MeasureText(hp_txt, 16);
        DrawText(hp_txt, (screen_w - hw) / 2, bar_y + 2, 16, WHITE);
    }

    /* Boss awakening banner */
    if (boss_intro_timer() > 0.0f) {
        const char *title = "THE NIGHT SPIRIT AWAKENS!";
        const char *sub   = "Defeat it to end the curse and win!";
        DrawText(title, (screen_w - MeasureText(title, 48)) / 2, 145, 48, (Color){ 255, 65, 105, 255 });
        DrawText(sub,   (screen_w - MeasureText(sub, 24)) / 2,   202, 24, GOLD);
    } else if (zone_cleared) {
        int bob = (int)(6.0f * sinf((float)GetTime() * 6.0f));
        if (gate_banner_timer > 0.0f) {
            const char *title = TextFormat("ZONE %d COMPLETE!", last_opened_gate);
            const char *sub   = TextFormat("Move East into Zone %d ->", last_opened_gate + 1);
            DrawText(title, (screen_w - MeasureText(title, 52)) / 2, 95, 52, GREEN);
            DrawText(sub,   (screen_w - MeasureText(sub, 24)) / 2,   155, 24, WHITE);
        }
        const char *prompt = TextFormat("GO TO ZONE %d ->", active_zone + 1);
        int pw = MeasureText(prompt, 24);
        DrawText(prompt, screen_w - pw - 24 + bob, 440, 24, GREEN);
    } else if (zone_banner_timer > 0.0f) {
        const char *title = ZONE_NAMES[active_zone - 1];
        const char *sub   = (active_zone == 4)
                          ? "Reach 65 kills to summon the Night Spirit!"
                          : "Previous zone sealed — no turning back!";
        DrawText(title, (screen_w - MeasureText(title, 48)) / 2, 95, 48, GOLD);
        DrawText(sub,   (screen_w - MeasureText(sub, 22)) / 2,   152, 22, LIGHTGRAY);
    }

    /* Between waves: countdown banner */
    int wait = waves_countdown();
    if (!zone_cleared && !boss_is_active() && wait > 0) {
        const char *title = TextFormat("WAVE %d", waves_number() + 1);
        const char *sub   = TextFormat("starts in %d", wait);
        DrawText(title, (screen_w - MeasureText(title, 70)) / 2, 230, 70, YELLOW);
        DrawText(sub,   (screen_w - MeasureText(sub, 30)) / 2,   310, 30, LIGHTGRAY);
    }

    weapon_hud_draw(g);
}