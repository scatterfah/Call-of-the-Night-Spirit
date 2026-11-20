#ifndef ASSETS_H
#define ASSETS_H

#include "raylib.h"

typedef struct {
    Texture2D bg_moon;
    Texture2D bg_mountains;
    Texture2D bg_graveyard;
    Texture2D bush_large;
    Texture2D bush_small;
    Texture2D statue;
    Texture2D stones[4];
    Texture2D trees[3];
} WorldAssets;

typedef struct {
    Texture2D bg;
    Texture2D far_buildings;
    Texture2D buildings;
    Texture2D foreground;
} Zone2Assets;

typedef struct {
    Texture2D buildings;
    Texture2D front;
    Texture2D foreground;
} Zone3Assets;

typedef struct {
    Texture2D sky;
    Texture2D clouds;
    Texture2D far_mountains;
    Texture2D canyon;
    Texture2D front;
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