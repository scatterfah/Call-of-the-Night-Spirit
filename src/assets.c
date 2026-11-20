#include "assets.h"

static Assets g_assets = { 0 };

static Texture2D load_pixel_texture(const char *path) {
    Texture2D tex = { 0 };
    if (FileExists(path)) {
        tex = LoadTexture(path);
        if (tex.id != 0) {
            SetTextureFilter(tex, TEXTURE_FILTER_POINT);
        }
    }
    return tex;
}

static void unload_if_loaded(Texture2D *tex) {
    if (tex->id != 0) {
        UnloadTexture(*tex);
        *tex = (Texture2D){ 0 };
    }
}

void assets_load(void) {
    WorldAssets *w = &g_assets.world;

    /* Zone 1 (Gothic Graveyard) */
    w->bg_moon      = load_pixel_texture("assets/zone_1/background.png");
    w->bg_mountains = load_pixel_texture("assets/zone_1/mountains.png");
    w->bg_graveyard = load_pixel_texture("assets/zone_1/graveyard.png");
    w->bush_large   = load_pixel_texture("assets/zone_1/bush-large.png");
    w->bush_small   = load_pixel_texture("assets/zone_1/bush-small.png");
    w->statue       = load_pixel_texture("assets/zone_1/statue.png");

    w->stones[0]    = load_pixel_texture("assets/zone_1/stone-1.png");
    w->stones[1]    = load_pixel_texture("assets/zone_1/stone-2.png");
    w->stones[2]    = load_pixel_texture("assets/zone_1/stone-3.png");
    w->stones[3]    = load_pixel_texture("assets/zone_1/stone-4.png");

    w->trees[0]     = load_pixel_texture("assets/zone_1/tree-1.png");
    w->trees[1]     = load_pixel_texture("assets/zone_1/tree-2.png");
    w->trees[2]     = load_pixel_texture("assets/zone_1/tree-3.png");

    /* Zone 2 (Industrial Ruins) */
    Zone2Assets *z2 = &g_assets.zone2;
    z2->bg            = load_pixel_texture("assets/zone_2/bg.png");
    z2->far_buildings = load_pixel_texture("assets/zone_2/far-buildings.png");
    z2->buildings     = load_pixel_texture("assets/zone_2/buildings.png");
    z2->foreground    = load_pixel_texture("assets/zone_2/skill-foreground.png");

    /* Zone 3 (Neon District) */
    Zone3Assets *z3 = &g_assets.zone3;
    z3->buildings  = load_pixel_texture("assets/zone_3/buildings.png");
    z3->front      = load_pixel_texture("assets/zone_3/front.png");
    z3->foreground = load_pixel_texture("assets/zone_3/foreground.png");
    if (z3->foreground.id == 0) {
        z3->foreground = load_pixel_texture("assets/zone_3/foreground-empty.png");
    }

    /* Zone 4 (Crimson Canyon) */
    Zone4Assets *z4 = &g_assets.zone4;
    z4->sky           = load_pixel_texture("assets/zone_4/sky.png");
    z4->clouds        = load_pixel_texture("assets/zone_4/clouds.png");
    z4->far_mountains = load_pixel_texture("assets/zone_4/far-mountains.png");
    z4->canyon        = load_pixel_texture("assets/zone_4/canyon.png");
    z4->front         = load_pixel_texture("assets/zone_4/front.png");
    if (z4->front.id == 0) {
        z4->front = load_pixel_texture("assets/zone_4/front_2.png");
    }
}

void assets_unload(void) {
    WorldAssets *w = &g_assets.world;

    unload_if_loaded(&w->bg_moon);
    unload_if_loaded(&w->bg_mountains);
    unload_if_loaded(&w->bg_graveyard);
    unload_if_loaded(&w->bush_large);
    unload_if_loaded(&w->bush_small);
    unload_if_loaded(&w->statue);

    for (int i = 0; i < 4; i++) {
        unload_if_loaded(&w->stones[i]);
    }
    for (int i = 0; i < 3; i++) {
        unload_if_loaded(&w->trees[i]);
    }

    Zone2Assets *z2 = &g_assets.zone2;
    unload_if_loaded(&z2->bg);
    unload_if_loaded(&z2->far_buildings);
    unload_if_loaded(&z2->buildings);
    unload_if_loaded(&z2->foreground);

    Zone3Assets *z3 = &g_assets.zone3;
    unload_if_loaded(&z3->buildings);
    unload_if_loaded(&z3->front);
    unload_if_loaded(&z3->foreground);

    Zone4Assets *z4 = &g_assets.zone4;
    unload_if_loaded(&z4->sky);
    unload_if_loaded(&z4->clouds);
    unload_if_loaded(&z4->far_mountains);
    unload_if_loaded(&z4->canyon);
    unload_if_loaded(&z4->front);
}

const Assets *assets_get(void) {
    return &g_assets;
}