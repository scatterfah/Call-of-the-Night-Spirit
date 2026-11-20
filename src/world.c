#include <math.h>
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
    "ZONE 1: COURTYARD",
    "ZONE 2: INDUSTRIAL RUINS",
    "ZONE 3: NEON DISTRICT",
    "ZONE 4: CRIMSON CANYON"
};

static int   active_zone       = 1;     /* advances 1 -> 2 -> 3 -> 4 and never goes back */
static int   open_gates        = 0;
static int   last_opened_gate  = 0;
static float gate_banner_timer = 0.0f;
static float zone_banner_timer = 0.0f;

typedef enum {
    PROP_STONE_1,
    PROP_STONE_2,
    PROP_STONE_3,
    PROP_STONE_4,
    PROP_BUSH_LARGE,
    PROP_BUSH_SMALL,
    PROP_STATUE,
    PROP_TREE_1,
    PROP_TREE_2,
    PROP_TREE_3,
    PROP_IND_CRATE,
    PROP_IND_BARREL,
    PROP_IND_BARRIER,
    PROP_CITY_CAR_RED,
    PROP_CITY_CAR_TAXI,
    PROP_CITY_CAR_POLICE,
    PROP_CITY_DUMPSTER,
    PROP_CITY_KIOSK,
    PROP_CANYON_ROCK,
    PROP_CANYON_CACTUS
} PropKind;

typedef struct {
    float    x;         /* world x centre */
    float    y;         /* world y bottom anchor */
    PropKind kind;
    float    height;    /* target pixel height */
    Color    tint;
} WorldProp;

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
    /* Zone 1: Blood-Moon Courtyard */
    {  190.0f, 455.0f, 11.0f, 0.72f, PROP_TREE_1,          130.0f, {225, 205, 245, 255} },
    {  275.0f, 610.0f, 11.0f, 0.72f, PROP_TREE_2,          136.0f, {225, 205, 245, 255} },
    {  135.0f, 555.0f,  9.0f, 0.00f, PROP_STONE_1,          48.0f, {235, 220, 245, 255} },
    {  340.0f, 395.0f,  0.0f, 0.00f, PROP_BUSH_LARGE,       48.0f, {195, 215, 205, 235} },
    {  640.0f, 455.0f, 12.0f, 0.72f, PROP_TREE_3,          138.0f, {225, 205, 245, 255} },
    {  590.0f, 610.0f, 11.0f, 0.65f, PROP_STATUE,           92.0f, {240, 220, 250, 255} },
    {  760.0f, 395.0f,  9.0f, 0.00f, PROP_STONE_2,          48.0f, {235, 220, 245, 255} },
    {  840.0f, 535.0f, 11.0f, 0.72f, PROP_TREE_1,          132.0f, {225, 205, 245, 255} },
    { 1010.0f, 460.0f, 11.0f, 0.72f, PROP_TREE_2,          134.0f, {225, 205, 245, 255} },
    { 1045.0f, 605.0f, 12.0f, 0.72f, PROP_TREE_3,          136.0f, {225, 205, 245, 255} },
    {  935.0f, 635.0f,  0.0f, 0.00f, PROP_BUSH_LARGE,       48.0f, {195, 215, 205, 235} },

    /* Zone 2: Industrial Ruins (crates, toxic barrels & concrete barricades) */
    { 1420.0f, 440.0f, 16.0f, 0.00f, PROP_IND_CRATE,        46.0f, WHITE },
    { 1475.0f, 452.0f, 11.0f, 0.00f, PROP_IND_BARREL,       38.0f, WHITE },
    { 1515.0f, 605.0f, 14.0f, 0.00f, PROP_IND_BARRIER,      30.0f, WHITE },
    { 1670.0f, 515.0f, 16.0f, 0.00f, PROP_IND_CRATE,        48.0f, WHITE },
    { 1795.0f, 395.0f, 11.0f, 0.00f, PROP_IND_BARREL,       38.0f, WHITE },
    { 1875.0f, 605.0f, 16.0f, 0.00f, PROP_IND_CRATE,        46.0f, WHITE },
    { 2035.0f, 450.0f, 14.0f, 0.00f, PROP_IND_BARRIER,      30.0f, WHITE },
    { 2170.0f, 595.0f, 16.0f, 0.00f, PROP_IND_CRATE,        48.0f, WHITE },
    { 2225.0f, 608.0f, 11.0f, 0.00f, PROP_IND_BARREL,       38.0f, WHITE },
    { 2215.0f, 415.0f, 11.0f, 0.00f, PROP_IND_BARREL,       38.0f, WHITE },

    /* Zone 3: Neon District — cars on the road, kiosks & dumpsters on the sidewalk */
    { 2555.0f, 326.0f, 13.0f, 0.00f, PROP_CITY_KIOSK,       54.0f, WHITE },
    { 2665.0f, 430.0f, 15.0f, 0.00f, PROP_CITY_CAR_RED,     44.0f, WHITE },
    { 2845.0f, 588.0f, 15.0f, 0.00f, PROP_CITY_CAR_TAXI,    44.0f, WHITE },
    { 2945.0f, 326.0f, 14.0f, 0.00f, PROP_CITY_DUMPSTER,    40.0f, WHITE },
    { 3065.0f, 434.0f, 15.0f, 0.00f, PROP_CITY_CAR_POLICE,  44.0f, WHITE },
    { 3195.0f, 326.0f, 13.0f, 0.00f, PROP_CITY_KIOSK,       54.0f, WHITE },
    { 3270.0f, 584.0f, 15.0f, 0.00f, PROP_CITY_CAR_RED,     44.0f, WHITE },
    { 3455.0f, 430.0f, 15.0f, 0.00f, PROP_CITY_CAR_TAXI,    44.0f, WHITE },
    { 3530.0f, 326.0f, 14.0f, 0.00f, PROP_CITY_DUMPSTER,    40.0f, WHITE },

    /* Zone 4: Crimson Canyon — red sandstone boulders & desert saguaro cacti */
    { 3825.0f, 445.0f, 15.0f, 0.00f, PROP_CANYON_ROCK,      48.0f, WHITE },
    { 3915.0f, 610.0f, 10.0f, 0.00f, PROP_CANYON_CACTUS,    64.0f, WHITE },
    { 4045.0f, 395.0f, 10.0f, 0.00f, PROP_CANYON_CACTUS,    58.0f, WHITE },
    { 4140.0f, 525.0f, 16.0f, 0.00f, PROP_CANYON_ROCK,      54.0f, WHITE },
    { 4305.0f, 435.0f, 10.0f, 0.00f, PROP_CANYON_CACTUS,    66.0f, WHITE },
    { 4385.0f, 615.0f, 15.0f, 0.00f, PROP_CANYON_ROCK,      50.0f, WHITE },
    { 4555.0f, 455.0f, 16.0f, 0.00f, PROP_CANYON_ROCK,      52.0f, WHITE },
    { 4645.0f, 595.0f, 10.0f, 0.00f, PROP_CANYON_CACTUS,    62.0f, WHITE },
};

static const WorldProp FOREGROUND_PROPS[] = {
    {  180.0f, 718.0f, PROP_BUSH_LARGE,    50.0f, {145, 155, 165, 210} },
    {  720.0f, 718.0f, PROP_STONE_1,       44.0f, {165, 155, 180, 210} },
    { 1520.0f, 718.0f, PROP_IND_BARREL,    38.0f, WHITE },
    { 2080.0f, 718.0f, PROP_IND_CRATE,     44.0f, WHITE },
    { 2760.0f, 718.0f, PROP_CITY_DUMPSTER, 38.0f, WHITE },
    { 3340.0f, 718.0f, PROP_CITY_KIOSK,    46.0f, WHITE },
    { 3980.0f, 718.0f, PROP_CANYON_ROCK,   44.0f, WHITE },
    { 4520.0f, 718.0f, PROP_CANYON_CACTUS, 56.0f, WHITE },
};

static bool is_car_prop(PropKind kind) {
    return kind == PROP_CITY_CAR_RED || kind == PROP_CITY_CAR_TAXI || kind == PROP_CITY_CAR_POLICE;
}

static bool is_procedural_prop(PropKind kind) {
    return kind == PROP_IND_CRATE || kind == PROP_IND_BARREL || kind == PROP_IND_BARRIER ||
           is_car_prop(kind) || kind == PROP_CITY_DUMPSTER || kind == PROP_CITY_KIOSK ||
           kind == PROP_CANYON_ROCK || kind == PROP_CANYON_CACTUS;
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

static Texture2D prop_texture(const WorldAssets *w, PropKind kind) {
    switch (kind) {
        case PROP_STONE_1:    return w->stones[0];
        case PROP_STONE_2:    return w->stones[1];
        case PROP_STONE_3:    return w->stones[2];
        case PROP_STONE_4:    return w->stones[3];
        case PROP_BUSH_LARGE: return w->bush_large;
        case PROP_BUSH_SMALL: return w->bush_small;
        case PROP_STATUE:     return w->statue;
        case PROP_TREE_1:     return w->trees[0];
        case PROP_TREE_2:     return w->trees[1];
        case PROP_TREE_3:     return w->trees[2];
        default:              return (Texture2D){ 0 };
    }
}

/* Detailed cyberpunk city car renderer (sports coupe, yellow taxi, flashing police cruiser) */
static void draw_city_car(PropKind kind, float cx, float bottom_y, float h) {
    int x = (int)cx;
    int by = (int)bottom_y;
    int w = 96;
    int hh = (int)h;
    int left = x - w / 2;
    int top  = by - hh;

    Color body_main = { 168, 28, 52, 255 };
    Color body_hi   = { 228, 58, 86, 255 };
    Color roof_col  = { 118, 20, 42, 255 };
    Color underglow = { 245, 55, 130, 52 };

    if (kind == PROP_CITY_CAR_TAXI) {
        body_main = (Color){ 204, 150, 28, 255 };
        body_hi   = (Color){ 245, 198, 62, 255 };
        roof_col  = (Color){ 164, 118, 22, 255 };
        underglow = (Color){ 245, 185, 55, 42 };
    } else if (kind == PROP_CITY_CAR_POLICE) {
        body_main = (Color){ 26, 34, 62, 255 };
        body_hi   = (Color){ 52, 66, 108, 255 };
        roof_col  = (Color){ 195, 205, 225, 255 };
        underglow = (Color){ 65, 175, 255, 48 };
    }

    DrawEllipse(x, by - 2, 52.0f, 12.0f, underglow);
    DrawTriangle((Vector2){ (float)(left + w), (float)(top + 24) },
                 (Vector2){ (float)(left + w + 52), (float)(top + 10) },
                 (Vector2){ (float)(left + w + 52), (float)(by - 2) },
                 (Color){ 210, 245, 255, 28 });

    DrawRectangle(left + 10, by - 10, 18, 10, (Color){ 10, 10, 16, 255 });
    DrawRectangle(left + 15, by - 7, 8, 5, (Color){ 120, 135, 155, 255 });
    DrawRectangle(left + w - 28, by - 10, 18, 10, (Color){ 10, 10, 16, 255 });
    DrawRectangle(left + w - 23, by - 7, 8, 5, (Color){ 120, 135, 155, 255 });

    DrawRectangle(left, top + 16, w, hh - 22, body_main);
    DrawRectangle(left + 2, top + 17, w - 4, 6, body_hi);
    DrawRectangle(left - 2, by - 12, 6, 5, (Color){ 32, 34, 46, 255 });
    DrawRectangle(left + w - 4, by - 12, 6, 5, (Color){ 32, 34, 46, 255 });

    if (kind == PROP_CITY_CAR_POLICE) {
        DrawRectangle(left + 28, top + 18, 40, hh - 26, (Color){ 215, 225, 240, 255 });
        DrawRectangle(left + 42, top + 24, 12, 4, (Color){ 40, 95, 210, 255 });
    } else if (kind == PROP_CITY_CAR_TAXI) {
        for (int sx = left + 22; sx < left + w - 24; sx += 10) {
            DrawRectangle(sx, top + 24, 5, 4, (Color){ 20, 20, 28, 255 });
        }
    }

    DrawRectangle(left + 18, top + 3, w - 38, 14, roof_col);
    DrawRectangle(left + 22, top + 6, 22, 10, (Color){ 48, 155, 195, 195 });
    DrawRectangle(left + 48, top + 6, 22, 10, (Color){ 68, 195, 235, 215 });
    DrawLine(left + 26, top + 7, left + 33, top + 14, (Color){ 200, 245, 255, 150 });
    DrawLine(left + 54, top + 7, left + 61, top + 14, (Color){ 200, 245, 255, 150 });

    DrawRectangleLines(left, top + 16, w, hh - 22, (Color){ 12, 10, 22, 255 });
    DrawRectangleLines(left + 18, top + 3, w - 38, 14, (Color){ 12, 10, 22, 255 });

    DrawRectangle(left, top + 21, 6, 6, (Color){ 255, 45, 85, 255 });
    DrawRectangle(left + w - 6, top + 21, 6, 6, (Color){ 185, 250, 255, 255 });

    if (kind == PROP_CITY_CAR_TAXI) {
        DrawRectangle(x - 10, top - 2, 18, 5, (Color){ 255, 235, 120, 255 });
    } else if (kind == PROP_CITY_CAR_POLICE) {
        bool flash = ((int)(GetTime() * 6.0) % 2) == 0;
        Color left_l  = flash ? (Color){ 255, 45, 80, 255 } : (Color){ 90, 20, 35, 255 };
        Color right_l = flash ? (Color){ 25, 65, 120, 255 } : (Color){ 65, 185, 255, 255 };
        DrawRectangle(x - 10, top - 2, 10, 5, left_l);
        DrawRectangle(x,      top - 2, 10, 5, right_l);
        Color glow = flash ? (Color){ 255, 45, 80, 50 } : (Color){ 65, 185, 255, 50 };
        DrawCircle(flash ? (x - 5) : (x + 5), top, 16.0f, glow);
    }
}

static void draw_procedural_prop(PropKind kind, float cx, float bottom_y, float h) {
    if (is_car_prop(kind)) {
        draw_city_car(kind, cx, bottom_y, h);
        return;
    }

    int x = (int)cx;
    int by = (int)bottom_y;

    if (kind == PROP_IND_CRATE) {
        int s = (int)h;
        int left = x - s / 2;
        int top  = by - s;
        DrawRectangle(left, top, s, s, (Color){ 24, 48, 38, 255 });
        DrawRectangle(left, top, s, 8, (Color){ 46, 84, 66, 255 });
        DrawRectangleLines(left, top, s, s, (Color){ 10, 22, 18, 255 });
        DrawRectangleLines(left + 5, top + 10, s - 10, s - 15, (Color){ 42, 78, 62, 255 });
        DrawLine(left + 6, top + 11, left + s - 6, by - 6, (Color){ 36, 68, 54, 255 });
        DrawLine(left + s - 6, top + 11, left + 6, by - 6, (Color){ 36, 68, 54, 255 });
        DrawRectangle(left + s - 12, top + 3, 5, 3, (Color){ 135, 245, 215, 255 });
    } else if (kind == PROP_IND_BARREL) {
        int w = 26;
        int hh = (int)h;
        int left = x - w / 2;
        int top  = by - hh;
        DrawRectangle(left, top + 4, w, hh - 6, (Color){ 22, 52, 40, 255 });
        DrawEllipse(x, top + 4, (float)w * 0.5f, 4.5f, (Color){ 48, 92, 72, 255 });
        DrawRectangle(left, top + hh / 2 - 4, w, 8, (Color){ 95, 225, 175, 230 });
        DrawRectangle(left + 4, top + hh / 2 - 1, w - 8, 2, (Color){ 20, 48, 36, 255 });
        DrawRectangleLines(left, top + 4, w, hh - 6, (Color){ 10, 24, 18, 255 });
    } else if (kind == PROP_IND_BARRIER) {
        int w = 66;
        int hh = (int)h;
        int left = x - w / 2;
        int top  = by - hh;
        DrawRectangle(left, top, w, hh, (Color){ 34, 58, 48, 255 });
        DrawRectangle(left, top, w, 6, (Color){ 58, 96, 78, 255 });
        for (int sx = left + 6; sx < left + w - 8; sx += 14) {
            DrawRectangle(sx, top + 9, 7, 10, (Color){ 110, 235, 185, 200 });
        }
        DrawRectangleLines(left, top, w, hh, (Color){ 12, 24, 20, 255 });
    } else if (kind == PROP_CITY_DUMPSTER) {
        int w = 52;
        int hh = (int)h;
        int left = x - w / 2;
        int top  = by - hh;
        DrawRectangle(left, top + 6, w, hh - 6, (Color){ 36, 40, 58, 255 });
        DrawRectangle(left - 2, top, w + 4, 8, (Color){ 56, 62, 86, 255 });
        DrawRectangle(left + 6, top + 14, w - 12, 5, (Color){ 24, 26, 40, 255 });
        DrawRectangle(left + 6, top + 24, w - 12, 5, (Color){ 24, 26, 40, 255 });
        DrawRectangleLines(left, top + 6, w, hh - 6, (Color){ 14, 14, 24, 255 });
        DrawLine(left + 2, top + 9, left + w - 2, top + 9, (Color){ 240, 75, 165, 190 });
    } else if (kind == PROP_CITY_KIOSK) {
        int w = 34;
        int hh = (int)h;
        int left = x - w / 2;
        int top  = by - hh;
        DrawRectangle(left, top, w, hh, (Color){ 26, 22, 46, 255 });
        DrawRectangle(left + 4, top + 6, w - 8, 22, (Color){ 55, 205, 245, 215 });
        DrawRectangle(left + 7, top + 10, w - 14, 5, (Color){ 255, 95, 185, 235 });
        DrawRectangle(left + 7, top + 18, w - 18, 4, WHITE);
        DrawRectangle(left + 5, top + 34, w - 10, 12, (Color){ 14, 12, 26, 255 });
        DrawRectangleLines(left, top, w, hh, (Color){ 235, 80, 175, 220 });
    } else if (kind == PROP_CANYON_ROCK) {
        /* Layered red-orange & deep violet sandstone boulder matching front_2.png */
        int w = (int)(h * 1.15f);
        int hh = (int)h;
        int left = x - w / 2;
        int top  = by - hh;
        /* Dark shadowed base */
        DrawRectangle(left, by - hh / 2, w, hh / 2, (Color){ 46, 22, 44, 255 });
        /* Mid burnt-red sandstone strata */
        DrawRectangle(left + 4, top + hh / 4, w - 8, hh / 2 + 4, (Color){ 138, 38, 28, 255 });
        DrawRectangle(left + 10, top + 4, w - 20, hh / 2, (Color){ 186, 62, 24, 255 });
        /* Sunlit orange cap & ridge highlights */
        DrawRectangle(left + 14, top, w - 28, 8, (Color){ 232, 108, 34, 255 });
        DrawRectangle(left + 6, top + hh / 2 - 4, w / 2 - 4, 5, (Color){ 214, 86, 28, 255 });
        /* Deep violet cliff shadow on right side */
        DrawRectangle(x + 4, top + 12, w / 2 - 8, hh - 14, (Color){ 64, 24, 48, 210 });
        DrawRectangleLines(left + 4, top + hh / 4, w - 8, hh - hh / 4, (Color){ 26, 10, 24, 255 });
    } else if (kind == PROP_CANYON_CACTUS) {
        /* Olive-green & dark-shaded desert saguaro cactus matching front_2.png */
        int hh = (int)h;
        int top = by - hh;
        Color dark_c = { 34, 42, 18, 255 };
        Color mid_c  = { 72, 86, 26, 255 };
        Color hi_c   = { 118, 126, 38, 255 };
        /* Main vertical trunk */
        DrawRectangle(x - 6, top + 4, 12, hh - 4, mid_c);
        DrawRectangle(x - 4, top, 8, 6, mid_c);
        DrawRectangle(x - 5, top + 4, 4, hh - 6, hi_c);
        DrawRectangle(x + 2, top + 4, 4, hh - 4, dark_c);
        /* Left cactus arm */
        int la_y = top + (int)(hh * 0.42f);
        DrawRectangle(x - 18, la_y, 12, 8, mid_c);
        DrawRectangle(x - 18, la_y - 16, 8, 22, mid_c);
        DrawRectangle(x - 17, la_y - 16, 3, 20, hi_c);
        /* Right cactus arm */
        int ra_y = top + (int)(hh * 0.30f);
        DrawRectangle(x + 6, ra_y, 12, 8, dark_c);
        DrawRectangle(x + 11, ra_y - 14, 7, 20, mid_c);
        DrawRectangle(x + 15, ra_y - 14, 3, 20, dark_c);
    }
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

static void draw_prop_split(Texture2D tex, float cx, float bottom_y,
                            float target_h, float split_frac, bool draw_top, Color tint) {
    if (tex.id == 0 || tex.height <= 0) return;

    int split_px = (int)((float)tex.height * split_frac);
    if (split_px < 1) split_px = 1;
    if (split_px >= tex.height) split_px = tex.height - 1;

    float scale = target_h / (float)tex.height;
    float draw_w = (float)tex.width * scale;

    if (draw_top) {
        float dst_h = (float)split_px * scale;
        Rectangle src = { 0.0f, 0.0f, (float)tex.width, (float)split_px };
        Rectangle dst = { cx - draw_w * 0.5f, bottom_y - target_h, draw_w, dst_h };
        DrawTexturePro(tex, src, dst, (Vector2){ 0.0f, 0.0f }, 0.0f, tint);
    } else {
        int bot_px = tex.height - split_px;
        float dst_h = (float)bot_px * scale;
        Rectangle src = { 0.0f, (float)split_px, (float)tex.width, (float)bot_px };
        Rectangle dst = { cx - draw_w * 0.5f, bottom_y - dst_h, draw_w, dst_h };
        DrawTexturePro(tex, src, dst, (Vector2){ 0.0f, 0.0f }, 0.0f, tint);
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
    /* Everything west of the active zone is permanently sealed off */
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

/* Moon-only background for the Start Menu (and dimmed on High Scores / Game Over) */
void world_draw_menu_bg(bool dim) {
    const WorldAssets *w = &assets_get()->world;
    float sw = (float)GetScreenWidth();
    float sh = (float)GetScreenHeight();

    if (w->bg_moon.id != 0) {
        float sx = sw / (float)w->bg_moon.width;
        float sy = sh / (float)w->bg_moon.height;
        float scale = (sx > sy) ? sx : sy;
        float draw_w = (float)w->bg_moon.width * scale;
        float draw_h = (float)w->bg_moon.height * scale;
        Rectangle src = { 0.0f, 0.0f, (float)w->bg_moon.width, (float)w->bg_moon.height };
        Rectangle dst = { (sw - draw_w) * 0.5f, (sh - draw_h) * 0.5f, draw_w, draw_h };
        DrawTexturePro(w->bg_moon, src, dst, (Vector2){ 0.0f, 0.0f }, 0.0f, WHITE);
    } else {
        ClearBackground((Color){ 16, 10, 24, 255 });
    }

    if (dim) {
        DrawRectangle(0, 0, (int)sw, (int)sh, (Color){ 10, 4, 18, 130 });
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

    bool has_z4 = (z4->sky.id != 0 || z4->canyon.id != 0 || z4->front.id != 0);

    /* Never render a previous sealed-off zone's skyline */
    float z2_t = (z2->bg.id != 0 && active_zone <= 2)
               ? zone_blend(g->camera_x, z2_start, z3_start) : 0.0f;
    float z3_t = ((z3->foreground.id != 0 || z3->buildings.id != 0) && active_zone <= 3)
               ? zone_blend(g->camera_x, z3_start, z4_start) : 0.0f;
    float z4_t = has_z4
               ? zone_blend(g->camera_x, z4_start, world_width() + 500.0f) : 0.0f;
    float z1_t = (active_zone == 1 || (!has_z4 && active_zone >= 3))
               ? (1.0f - z2_t - z3_t - z4_t) : 0.0f;
    if (z1_t < 0.0f) z1_t = 0.0f;
    if (z1_t + z2_t + z3_t + z4_t < 0.01f) z1_t = 1.0f;

    /* 1. Zone 1 Gothic Graveyard horizon */
    if (z1_t > 0.01f) {
        if (w->bg_moon.id != 0) {
            float extra_w = 180.0f;
            float draw_w = sw + extra_w;
            float draw_h = draw_w * ((float)w->bg_moon.height / (float)w->bg_moon.width);
            if (draw_h < ground_y + 120.0f) draw_h = ground_y + 120.0f;

            float max_scroll = world_width() - sw;
            float progress = (max_scroll > 1.0f) ? ((g->camera_x - sw * 0.5f) / max_scroll) : 0.0f;
            if (progress < 0.0f) progress = 0.0f;
            if (progress > 1.0f) progress = 1.0f;

            Rectangle src = { 0.0f, 0.0f, (float)w->bg_moon.width, (float)w->bg_moon.height };
            Rectangle dst = { -extra_w * progress, -36.0f, draw_w, draw_h };
            DrawTexturePro(w->bg_moon, src, dst, (Vector2){ 0.0f, 0.0f }, 0.0f,
                           fade_tint(WHITE, z1_t));
        } else {
            DrawRectangle(0, 0, (int)sw, (int)ground_y, fade_tint((Color){ 18, 8, 30, 255 }, z1_t));
        }

        draw_parallax_layer(w->bg_mountains, 155.0f, ground_y + 20.0f,
                            g->camera_x * 0.16f, fade_tint((Color){ 210, 190, 230, 235 }, z1_t));
        draw_parallax_layer(w->bg_graveyard, 138.0f, ground_y + 12.0f,
                            g->camera_x * 0.34f, fade_tint(WHITE, z1_t));
    }

    /* 2. Zone 2 Industrial Ruins parallax skyline */
    if (z2_t > 0.01f) {
        if (z2->bg.id != 0) {
            Rectangle src = { 0.0f, 0.0f, (float)z2->bg.width, (float)z2->bg.height };
            Rectangle dst = { 0.0f, 0.0f, sw, ground_y + 24.0f };
            DrawTexturePro(z2->bg, src, dst, (Vector2){ 0.0f, 0.0f }, 0.0f,
                           fade_tint(WHITE, z2_t));
        }
        draw_parallax_layer(z2->far_buildings, 235.0f, ground_y + 10.0f,
                            g->camera_x * 0.14f, fade_tint((Color){ 215, 245, 230, 220 }, z2_t));
        draw_parallax_layer(z2->buildings, 185.0f, ground_y + 16.0f,
                            g->camera_x * 0.28f, fade_tint((Color){ 225, 245, 235, 240 }, z2_t));
        draw_parallax_layer(z2->foreground, 158.0f, ground_y + 12.0f,
                            g->camera_x * 0.44f, fade_tint(WHITE, z2_t));
    }

    /* 3. Zone 3 Neon District skyline & glowing street with cars */
    if (z3_t > 0.01f) {
        DrawRectangleGradientV(0, 0, (int)sw, (int)ground_y + 24,
                               fade_tint((Color){ 14, 10, 36, 255 }, z3_t),
                               fade_tint((Color){ 58, 20, 74, 255 }, z3_t));

        draw_parallax_layer(z3->buildings, 235.0f, ground_y + 8.0f,
                            g->camera_x * 0.14f, fade_tint((Color){ 235, 225, 255, 240 }, z3_t));
        draw_parallax_layer(z3->front, 215.0f, ground_y + 14.0f,
                            g->camera_x * 0.26f, fade_tint(WHITE, z3_t));
        draw_parallax_layer(z3->foreground, 245.0f, ground_y + 18.0f,
                            g->camera_x * 0.44f, fade_tint(WHITE, z3_t));
    }

    /* 4. Zone 4 Crimson Canyon 5-layer sunset desert parallax */
    if (z4_t > 0.01f) {
        if (z4->sky.id != 0) {
            Rectangle src = { 0.0f, 0.0f, (float)z4->sky.width, (float)z4->sky.height };
            Rectangle dst = { 0.0f, 0.0f, sw, ground_y + 24.0f };
            DrawTexturePro(z4->sky, src, dst, (Vector2){ 0.0f, 0.0f }, 0.0f,
                           fade_tint(WHITE, z4_t));
        } else {
            DrawRectangleGradientV(0, 0, (int)sw, (int)ground_y + 24,
                                   fade_tint((Color){ 242, 102, 102, 255 }, z4_t),
                                   fade_tint((Color){ 246, 208, 104, 255 }, z4_t));
        }

        draw_parallax_layer(z4->clouds, 240.0f, ground_y + 16.0f,
                            g->camera_x * 0.06f + (float)GetTime() * 12.0f,
                            fade_tint(WHITE, z4_t));
        draw_parallax_layer(z4->far_mountains, 235.0f, ground_y + 18.0f,
                            g->camera_x * 0.14f, fade_tint(WHITE, z4_t));
        draw_parallax_layer(z4->canyon, 240.0f, ground_y + 24.0f,
                            g->camera_x * 0.28f, fade_tint(WHITE, z4_t));
        draw_parallax_layer(z4->front, 248.0f, ground_y + 18.0f,
                            g->camera_x * 0.44f, fade_tint(WHITE, z4_t));
    }

    /* 5. Ground gradient blending seamlessly across all 4 zone themes */
    Color z1_top_col = { 12,  9, 26, 255 };
    Color z1_bot_col = { 22, 16, 36, 255 };
    Color z2_top_col = { 12, 24, 19, 255 };
    Color z2_bot_col = { 20, 36, 28, 255 };
    Color z3_top_col = { 24, 29, 42, 255 };
    Color z3_bot_col = { 18, 22, 34, 255 };
    Color z4_top_col = { 33, 13, 28, 255 };
    Color z4_bot_col = { 56, 24, 34, 255 };

    Color ground_top_col = blend4_color(z1_top_col, z1_t, z2_top_col, z2_t,
                                        z3_top_col, z3_t, z4_top_col, z4_t);
    Color ground_bot_col = blend4_color(z1_bot_col, z1_t, z2_bot_col, z2_t,
                                        z3_bot_col, z3_t, z4_bot_col, z4_t);

    DrawRectangleGradientV(0, (int)ground_y, (int)sw, (int)(sh - ground_y),
                           ground_top_col, ground_bot_col);

    /* 6. Soft horizon blend in Zones 1 & 2 (kept crisp in Zone 3 street & Zone 4 canyon) */
    float fog_scale = z1_t + z2_t;
    if (fog_scale > 0.05f) {
        Color mist_glow = blend4_color((Color){ 140,  60, 150, 0 }, z1_t,
                                       (Color){  60, 180, 130, 0 }, z2_t,
                                       (Color){ 195,  55, 165, 0 }, z3_t,
                                       (Color){ 210,  95,  55, 0 }, z4_t);
        Color mist_edge = blend4_color((Color){ 12,  8, 24, 150 }, z1_t,
                                       (Color){ 10, 24, 18, 165 }, z2_t,
                                       (Color){ 24, 29, 42,  40 }, z3_t,
                                       (Color){ 33, 13, 28,  40 }, z4_t);
        mist_edge = fade_tint(mist_edge, fog_scale);
        DrawRectangleGradientV(0, (int)ground_y - 20, (int)sw, 26, mist_glow, mist_edge);
        DrawRectangleGradientV(0, (int)ground_y, (int)sw, 24,
                               mist_edge, (Color){ mist_edge.r, mist_edge.g, mist_edge.b, 0 });
    }
}

void world_reset(void) {
    active_zone       = 1;
    open_gates        = 0;
    last_opened_gate  = 0;
    gate_banner_timer = 0.0f;
    zone_banner_timer = 0.0f;
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

    /* 1. Unlock only the seal belonging to the current active zone */
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

    /* 2. Once the player crosses into the next zone, lock the previous zone permanently */
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

    /* 3. Smooth camera clamped strictly between active zone's left edge and frontier seal */
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

    if (is_car_prop(p->kind)) {
        float cy = p->y - 16.0f;
        float min_d = r + p->radius;
        float min_d2 = min_d * min_d;

        float dx1 = x - (p->x - 20.0f);
        float dy1 = y - cy;
        if (dx1 * dx1 + dy1 * dy1 < min_d2) return true;

        float dx2 = x - (p->x + 20.0f);
        float dy2 = y - cy;
        if (dx2 * dx2 + dy2 * dy2 < min_d2) return true;

        return false;
    }

    float dx = x - p->x;
    float dy = y - (p->y - 10.0f);
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

/* Draw the complete Zone 3 multi-lane neon avenue: sidewalks, curbs, asphalt, lane lines & crosswalks */
static void draw_zone3_road(float top_y, float bot_y, float min_limit, float lock_limit) {
    float z3_left  = (float)(ZONE_STRIDE_COLS * 2 * TILE_SIZE);
    float z3_right = (float)(ZONE_STRIDE_COLS * 3 * TILE_SIZE);
    if (z3_left < min_limit)   z3_left  = min_limit;
    if (z3_right > lock_limit) z3_right = lock_limit;
    if (z3_left >= z3_right) return;

    int rx = (int)z3_left;
    int rw = (int)(z3_right - z3_left);
    int top_sidewalk_h = 46;
    int bot_sidewalk_h = 44;
    int road_top = (int)top_y + top_sidewalk_h;
    int road_bot = (int)bot_y - bot_sidewalk_h;
    int road_h   = road_bot - road_top;
    int lane_mid = road_top + road_h / 2;

    DrawRectangle(rx, (int)top_y, rw, top_sidewalk_h, (Color){ 44, 52, 70, 255 });
    DrawRectangle(rx, road_top - 6, rw, 6, (Color){ 78, 90, 115, 255 });
    DrawRectangle(rx, road_top, rw, 4, (Color){ 12, 14, 24, 210 });

    DrawRectangle(rx, road_bot, rw, bot_sidewalk_h, (Color){ 40, 48, 66, 255 });
    DrawRectangle(rx, road_bot, rw, 6, (Color){ 74, 86, 110, 255 });

    for (int sx = rx; sx < rx + rw; sx += 48) {
        DrawLine(sx, (int)top_y, sx, road_top - 6, (Color){ 26, 32, 46, 175 });
        DrawLine(sx, road_bot + 6, sx, (int)bot_y, (Color){ 24, 28, 42, 175 });
    }
    for (int lx = rx + 60; lx < rx + rw - 40; lx += 170) {
        Color spill = ((lx / 170) % 3 == 0) ? (Color){ 245, 55, 160, 42 }
                    : ((lx / 170) % 3 == 1) ? (Color){  55, 215, 255, 42 }
                                            : (Color){ 255, 170,  65, 36 };
        DrawEllipse(lx, (int)top_y + 22, 68.0f, 18.0f, spill);
    }

    DrawRectangle(rx, road_top + 4, rw, road_h - 4, (Color){ 23, 27, 40, 255 });
    DrawRectangle(rx, road_top + 42, rw, 26, (Color){ 18, 22, 34, 140 });
    DrawRectangle(rx, road_top + 92, rw, 26, (Color){ 18, 22, 34, 140 });
    DrawRectangle(rx, lane_mid + 40, rw, 26, (Color){ 18, 22, 34, 140 });
    DrawRectangle(rx, lane_mid + 90, rw, 26, (Color){ 18, 22, 34, 140 });

    DrawRectangle(rx, road_top + 10, rw, 3, (Color){ 185, 195, 215, 120 });
    DrawRectangle(rx, road_bot - 12, rw, 3, (Color){ 185, 195, 215, 120 });

    for (int dx = rx + 20; dx < rx + rw - 36; dx += 64) {
        DrawRectangle(dx, lane_mid - 5, 34, 3, (Color){ 245, 195, 70, 175 });
        DrawRectangle(dx, lane_mid + 2, 34, 3, (Color){ 245, 195, 70, 175 });
    }

    static const int CROSSWALKS[2] = { 2750, 3355 };
    for (int c = 0; c < 2; c++) {
        int cx = CROSSWALKS[c];
        if (cx < rx || cx + 52 > (int)z3_right) continue;
        for (int cy = road_top + 18; cy < road_bot - 22; cy += 26) {
            DrawRectangle(cx, cy, 48, 14, (Color){ 195, 205, 225, 105 });
        }
    }

    for (int px = rx + 95; px < rx + rw - 60; px += 155) {
        int py = (px % 2 == 0) ? (road_top + 52) : (lane_mid + 72);
        Color glow = ((px / 155) % 2 == 0) ? (Color){ 245, 65, 165, 42 }
                                           : (Color){  65, 215, 255, 40 };
        DrawEllipse(px, py, 28.0f, 10.0f, glow);
        DrawEllipse(px, py, 16.0f, 5.0f,  (Color){ glow.r, glow.g, glow.b, 65 });
    }
    for (int mx = rx + 180; mx < rx + rw - 80; mx += 320) {
        DrawCircle(mx, lane_mid, 12.0f, (Color){ 14, 18, 28, 210 });
        DrawCircleLines(mx, lane_mid, 12.0f, (Color){ 64, 74, 102, 160 });
    }
}

void world_draw(void) {
    const WorldAssets *w = &assets_get()->world;
    float top_y      = world_ground_top();
    float bot_y      = world_ground_bottom();
    float time       = (float)GetTime();
    float min_limit  = (active_zone > 1) ? world_unlocked_min_x() : 0.0f;
    float lock_limit = (open_gates < WORLD_GATE_COUNT) ? world_unlocked_max_x() : world_width();
    int   min_col    = (active_zone > 1) ? ((active_zone - 1) * ZONE_STRIDE_COLS + 1) : 1;
    int   max_col    = (open_gates < WORLD_GATE_COUNT)
                     ? ((open_gates + 1) * ZONE_STRIDE_COLS)
                     : (WORLD_COLS - 1);

    /* 1. Ground pathways and zone-themed surface details (only active & newly opened zone) */
    for (int col = min_col; col < max_col; col++) {
        int x = col * TILE_SIZE;
        bool in_zone2 = (col > ZONE_STRIDE_COLS && col < ZONE_STRIDE_COLS * 2);
        bool in_zone3 = (col >= ZONE_STRIDE_COLS * 2 && col < ZONE_STRIDE_COLS * 3);
        bool in_zone4 = (col >= ZONE_STRIDE_COLS * 3);
        if (in_zone3) continue;

        if (in_zone2) {
            DrawRectangle(x, (int)top_y + 135, TILE_SIZE, 125, (Color){ 34, 62, 48, 42 });
            if (col % 2 == 0) {
                DrawRectangle(x + 6, (int)top_y + 195, 24, 4, (Color){ 100, 190, 145, 55 });
            }
        } else if (in_zone4) {
            /* Dusty red-ochre canyon trail cutting through the badlands */
            DrawRectangle(x, (int)top_y + 128, TILE_SIZE, 140, (Color){ 134, 52, 36, 52 });
            DrawRectangle(x, (int)top_y + 150, TILE_SIZE, 96,  (Color){ 168, 70, 38, 34 });
        } else {
            DrawRectangle(x, (int)top_y + 135, TILE_SIZE, 125, (Color){ 62, 42, 82, 32 });
        }

        for (int row = GROUND_TOP_ROW; row < GROUND_BOTTOM_ROW; row++) {
            int y = row * TILE_SIZE;
            int hash = (row * 53 + col * 97) & 31;

            if (in_zone2) {
                if ((row % 3 == 0) && (col % 3 == 0)) {
                    DrawRectangleLines(x, y, TILE_SIZE * 3, TILE_SIZE * 3, (Color){ 10, 22, 16, 70 });
                }
                if (hash == 5 || hash == 21) {
                    DrawRectangle(x + 8, y + 10, 24, 18, (Color){ 12, 26, 20, 175 });
                    DrawLine(x + 14, y + 10, x + 14, y + 28, (Color){ 36, 72, 56, 150 });
                    DrawLine(x + 20, y + 10, x + 20, y + 28, (Color){ 36, 72, 56, 150 });
                    DrawLine(x + 26, y + 10, x + 26, y + 28, (Color){ 36, 72, 56, 150 });
                } else if (hash == 13) {
                    DrawEllipse(x + 20, y + 22, 18.0f, 8.0f, (Color){ 65, 205, 145, 42 });
                }
            } else if (in_zone4) {
                /* Dry desert sandstone pebbles, parched earth cracks & warm dust patches */
                if (hash == 3 || hash == 19) {
                    DrawEllipse(x + 18, y + 20, 14.0f, 6.0f, (Color){ 196, 86, 42, 48 });
                } else if (hash == 9) {
                    DrawLine(x + 6,  y + 14, x + 18, y + 22, (Color){ 24, 8, 22, 150 });
                    DrawLine(x + 18, y + 22, x + 30, y + 18, (Color){ 24, 8, 22, 150 });
                    DrawLine(x + 18, y + 22, x + 22, y + 32, (Color){ 24, 8, 22, 130 });
                } else if (hash == 24) {
                    DrawRectangle(x + 10, y + 24, 6, 4, (Color){ 165, 68, 38, 130 });
                    DrawRectangle(x + 22, y + 16, 4, 3, (Color){ 115, 44, 52, 140 });
                }
            } else {
                if (row >= 10 && row <= 13 && (hash == 2 || hash == 17)) {
                    DrawEllipse(x + 20, y + 20, 15.0f, 8.0f, (Color){ 105, 82, 130, 36 });
                } else if (hash == 6) {
                    DrawEllipse(x + 18, y + 22, 22.0f, 10.0f, (Color){ 6, 4, 14, 95 });
                }

                if (w->bush_small.id != 0 && (hash == 11 || hash == 27) &&
                    (col % ZONE_STRIDE_COLS) != 0) {
                    draw_prop_anchored(w->bush_small,
                                       (float)(x + 20),
                                       (float)(y + 30),
                                       24.0f,
                                       (Color){ 150, 165, 175, 155 });
                }
            }
        }
    }

    draw_zone3_road(top_y, bot_y, min_limit, lock_limit);

    /* Subtle zone titles etched into the floor */
    for (int z = active_zone - 1; z <= open_gates && z < 4; z++) {
        int cx = (z * ZONE_STRIDE_COLS + ZONE_STRIDE_COLS / 2) * TILE_SIZE;
        int cy = (int)((top_y + bot_y) * 0.5f);
        int tw = MeasureText(ZONE_NAMES[z], 24);
        Color label_col = (z == 1) ? (Color){ 150, 245, 205, 36 }
                        : (z == 2) ? (Color){ 255, 120, 210, 48 }
                        : (z == 3) ? (Color){ 255, 185, 110, 48 }
                                   : (Color){ 235, 200, 245, 28 };
        DrawText(ZONE_NAMES[z], cx - tw / 2, cy - 12, 24, label_col);
    }

    /* 2. Arena props (only inside active/unlocked range — previous zones are gone) */
    int arena_count = (int)(sizeof(ARENA_PROPS) / sizeof(ARENA_PROPS[0]));
    for (int i = 0; i < arena_count; i++) {
        const ArenaProp *p = &ARENA_PROPS[i];
        if (p->x < min_limit || p->x > lock_limit) continue;

        if (!is_car_prop(p->kind)) {
            float shadow_rx = (p->radius > 0.0f) ? (p->radius * 2.1f) : (p->height * 0.42f);
            float shadow_ry = (p->radius > 0.0f) ? (p->radius * 0.75f) : (p->height * 0.16f);
            DrawEllipse((int)p->x, (int)(p->y - 3.0f), shadow_rx, shadow_ry, (Color){ 0, 0, 0, 135 });
        }

        if (is_procedural_prop(p->kind)) {
            draw_procedural_prop(p->kind, p->x, p->y, p->height);
            continue;
        }

        Texture2D tex = prop_texture(w, p->kind);
        if (tex.id == 0) continue;

        if (p->split_frac > 0.0f) {
            draw_prop_split(tex, p->x, p->y, p->height, p->split_frac, false, p->tint);
        } else {
            draw_prop_anchored(tex, p->x, p->y, p->height, p->tint);
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

        draw_prop_anchored(w->statue, gx, top_y + 78.0f, 76.0f,
                           (g >= open_gates) ? (Color){ 255, 175, 185, 255 }
                                             : (Color){ 175, 240, 200, 235 });
    }
}

void world_draw_over(const GameState *g) {
    const WorldAssets *w = &assets_get()->world;
    float time       = (float)GetTime();
    float bot_y      = world_ground_bottom();
    float min_limit  = (active_zone > 1) ? world_unlocked_min_x() : 0.0f;
    float lock_limit = (open_gates < WORLD_GATE_COUNT) ? world_unlocked_max_x() : world_width();

    /* 1. Upper canopies of arena trees and statues drawn OVER entities for 2.5D depth */
    int arena_count = (int)(sizeof(ARENA_PROPS) / sizeof(ARENA_PROPS[0]));
    for (int i = 0; i < arena_count; i++) {
        const ArenaProp *p = &ARENA_PROPS[i];
        if (p->split_frac <= 0.0f || p->x < min_limit || p->x > lock_limit) continue;

        Texture2D tex = prop_texture(w, p->kind);
        if (tex.id == 0) continue;

        Color tint = p->tint;
        float half_w = p->height * 0.45f;
        if (g->player_y < p->y - 16.0f && g->player_y > p->y - p->height - 16.0f &&
            fabsf(g->player_x - p->x) < half_w) {
            tint.a = 135;
        }

        draw_prop_split(tex, p->x, p->y, p->height, p->split_frac, true, tint);
    }

    /* 2. Bottom guardian statues and foreground silhouettes in active/unlocked range */
    for (int gate = active_zone - 1; gate <= open_gates && gate < WORLD_GATE_COUNT; gate++) {
        float gx = (float)((gate + 1) * ZONE_STRIDE_COLS * TILE_SIZE + TILE_SIZE / 2);
        draw_prop_anchored(w->statue, gx, bot_y + 34.0f, 84.0f,
                           (gate >= open_gates) ? (Color){ 220, 155, 165, 225 }
                                                : (Color){ 155, 215, 180, 200 });
    }

    int fg_count = (int)(sizeof(FOREGROUND_PROPS) / sizeof(FOREGROUND_PROPS[0]));
    for (int i = 0; i < fg_count; i++) {
        const WorldProp *p = &FOREGROUND_PROPS[i];
        if (p->x < min_limit || p->x > lock_limit) continue;

        if (is_procedural_prop(p->kind)) {
            draw_procedural_prop(p->kind, p->x, p->y, p->height);
            continue;
        }

        Texture2D tex = prop_texture(w, p->kind);
        if (tex.id == 0) continue;
        draw_prop_anchored(tex, p->x, p->y, p->height, p->tint);
    }

    /* 3. Light drifting ground mist in Zones 1 & 2 only (Zones 3 & 4 stay crisp) */
    float span = lock_limit - min_limit;
    if (span < 100.0f) span = 100.0f;
    for (int i = 0; i < 10; i++) {
        float mx = min_limit + fmodf((float)i * 310.0f + time * (9.0f + (float)(i % 3) * 3.0f),
                                     span);
        int z = world_zone_at(mx);
        if (z >= 3) continue;

        float my = 330.0f + (float)((i * 79) % 320);
        float rx = 115.0f + (float)(i % 4) * 26.0f;
        float ry = 20.0f + (float)(i % 3) * 7.0f;
        Color mist = (z == 2) ? (Color){  90, 215, 165, 18 }
                              : (Color){ 145,  95, 175, 16 };
        DrawEllipse((int)mx, (int)my, rx, ry, mist);
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