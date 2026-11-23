#ifndef ASSETS_H
#define ASSETS_H

#include "raylib.h"

typedef struct {
    Texture2D bg;              /* colossal blood-moon cathedral skyline (also menu bg) */
    Texture2D buildings;       /* wrought-iron fences, mausoleums & lanterns */
    Texture2D floor;           /* seamless mossy cobblestone courtyard */
    Texture2D props;           /* 2x2 sheet: gnarled oak, angel statue, gravestones, lamp */
    Rectangle prop_rects[4];   /* tight bounding box for each of the 4 props */
    Texture2D fallback_moon;   /* fallback to legacy background.png if bg.png is absent */
    Texture2D fallback_statue; /* fallback to legacy statue.png if props.png is absent */
} WorldAssets;

typedef struct {
    Texture2D bg;              /* vaulted gothic catacombs cavern */
    Texture2D buildings;       /* skull-lined crypt walls trimmed flush to floor */
    Texture2D floor;           /* seamless ancient stone & blood-rune floor */
    Texture2D props;           /* 2x2 sheet: coffin, brazier, altar, spiked gate */
    Rectangle prop_rects[4];   /* tight bounding box for each of the 4 props */
} Zone2Assets;

typedef struct {
    Texture2D bg;              /* moonlit ruined city skyline & storm clouds */
    Texture2D buildings;       /* brick storefronts trimmed flush to sidewalk */
    Texture2D road;            /* seamless illustrated street & sidewalks */
    Texture2D props;           /* 2x2 sheet: taxi, police car, dumpster, barricade */
    Rectangle prop_rects[4];   /* tight bounding box for each of the 4 props */
} Zone3Assets;

typedef struct {
    Texture2D bg;              /* eclipsed crimson-violet canyon boss sky */
    Texture2D buildings;       /* obsidian ritual arches, canyon rim & rune monoliths */
    Texture2D floor;           /* seamless cracked volcanic sandstone & fissure floor */
    Texture2D props;           /* 2x2 sheet: magma boulder, rune obelisk, beast bones, charred tree */
    Rectangle prop_rects[4];   /* tight bounding box for each of the 4 props */
} Zone4Assets;

typedef struct {
    Texture2D hero;            /* 4x2 sheet: 0..3 run, 4..5 idle, 6 bomb wind-up, 7 throw follow-through */
    Rectangle hero_rects[8];
    Texture2D weapons;         /* 3x2 sheet: 0 pistol, 1 shotgun, 2 SMG, 3 canister, 4 muzzle, 5 blast */
    Rectangle weapon_rects[6];
    Texture2D pickups;         /* 2x2 sheet: 0 medkit, 1 ammo tin, 2 bomb bundle, 3 Night Spirit crate */
    Rectangle pickup_rects[4];
} HeroAssets;

typedef struct {
    Texture2D zombies;         /* 4x3 sheet: row 0 walker, row 1 runner, row 2 tank (4 frames each) */
    Rectangle zombie_rects[12];
    Texture2D boss;            /* 4x2 sheet: 0..3 hover, 4 telegraph, 5 dash, 6 summon, 7 enraged */
    Rectangle boss_rects[8];
} EnemyAssets;

typedef struct {
    WorldAssets world;
    Zone2Assets zone2;
    Zone3Assets zone3;
    Zone4Assets zone4;
    HeroAssets  hero;
    EnemyAssets enemies;
} Assets;

void          assets_load(void);     /* call once after InitWindow() */
void          assets_unload(void);   /* call once before CloseWindow() */
const Assets *assets_get(void);

#endif