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
    WorldAssets world;
    Zone2Assets zone2;
    Zone3Assets zone3;
    Zone4Assets zone4;
} Assets;

void          assets_load(void);     /* call once after InitWindow() */
void          assets_unload(void);   /* call once before CloseWindow() */
const Assets *assets_get(void);

#endif