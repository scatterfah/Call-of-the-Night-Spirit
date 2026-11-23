#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include "assets.h"

static Assets g_assets = { 0 };

typedef enum {
    WHITE_KEEP = 0,
    WHITE_STRIP_FLOOD,    /* border flood-fill (protects interior details like flames & bandages) */
    WHITE_STRIP_NEUTRAL   /* strips neutral near-white (clears gaps in iron fences & branches) */
} WhiteMode;

static const char *find_asset_path(const char *png_path, const char *jpg_path) {
    if (png_path != NULL && FileExists(png_path)) return png_path;
    if (jpg_path != NULL && FileExists(jpg_path)) return jpg_path;
    return NULL;
}

static bool is_near_white(Color c, int min_v, int max_diff) {
    int hi = c.r;
    if (c.g > hi) hi = c.g;
    if (c.b > hi) hi = c.b;
    int lo = c.r;
    if (c.g < lo) lo = c.g;
    if (c.b < lo) lo = c.b;
    return (lo >= min_v) && ((hi - lo) <= max_diff);
}

/* Flood-fills outer white pixels starting from every grid cell boundary (cols x rows) */
static void strip_white_flood_grid(Image *img, int cols, int rows, int min_v) {
    int w = img->width;
    int h = img->height;
    int total = w * h;
    if (total <= 0 || cols <= 0 || rows <= 0) return;

    Color *px = (Color *)img->data;
    unsigned char *vis = calloc((size_t)total, 1);
    int *queue = malloc((size_t)total * sizeof(int));
    if (vis == NULL || queue == NULL) {
        free(vis);
        free(queue);
        return;
    }

    int head = 0;
    int tail = 0;

    for (int r = 0; r <= rows; r++) {
        int y = (r == rows) ? (h - 1) : ((r * h) / rows);
        for (int x = 0; x < w; x++) {
            int idx = y * w + x;
            if (!vis[idx] && is_near_white(px[idx], min_v, 22)) {
                vis[idx] = 1;
                queue[tail++] = idx;
            }
        }
    }

    for (int c = 0; c <= cols; c++) {
        int x = (c == cols) ? (w - 1) : ((c * w) / cols);
        for (int y = 0; y < h; y++) {
            int idx = y * w + x;
            if (!vis[idx] && is_near_white(px[idx], min_v, 22)) {
                vis[idx] = 1;
                queue[tail++] = idx;
            }
        }
    }

    while (head < tail) {
        int idx = queue[head++];
        px[idx].a = 0;

        int x = idx % w;
        int y = idx / w;
        int nb[4] = {
            (x > 0)     ? (idx - 1) : -1,
            (x < w - 1) ? (idx + 1) : -1,
            (y > 0)     ? (idx - w) : -1,
            (y < h - 1) ? (idx + w) : -1
        };
        for (int k = 0; k < 4; k++) {
            int n = nb[k];
            if (n >= 0 && !vis[n] && is_near_white(px[n], min_v, 26)) {
                vis[n] = 1;
                queue[tail++] = n;
            }
        }
    }

    free(vis);
    free(queue);
}

static void strip_white_neutral_rect(Image *img, int x0, int y0, int x1, int y1,
                                     int min_v, int max_diff) {
    Color *px = (Color *)img->data;
    for (int y = y0; y < y1; y++) {
        for (int x = x0; x < x1; x++) {
            int idx = y * img->width + x;
            if (is_near_white(px[idx], min_v, max_diff)) {
                px[idx].a = 0;
            }
        }
    }
}

static Texture2D load_texture_asset(const char *png_path, const char *jpg_path,
                                    WhiteMode white_mode, bool trim_vertical, bool smooth) {
    Texture2D tex = { 0 };
    const char *path = find_asset_path(png_path, jpg_path);
    if (path == NULL) return tex;

    if (white_mode != WHITE_KEEP || trim_vertical) {
        Image img = LoadImage(path);
        if (img.data != NULL) {
            ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
            if (white_mode == WHITE_STRIP_FLOOD) {
                strip_white_flood_grid(&img, 1, 1, 232);
            } else if (white_mode == WHITE_STRIP_NEUTRAL) {
                strip_white_flood_grid(&img, 1, 1, 228);
                strip_white_neutral_rect(&img, 0, 0, img.width, img.height, 238, 12);
            }
            if (trim_vertical) {
                Color *px = (Color *)img.data;
                int top_row = img.height;
                int bot_row = -1;
                for (int y = 0; y < img.height; y++) {
                    for (int x = 0; x < img.width; x++) {
                        if (px[y * img.width + x].a > 24) {
                            if (y < top_row) top_row = y;
                            if (y > bot_row) bot_row = y;
                            break;
                        }
                    }
                }
                if (bot_row >= top_row) {
                    if (bot_row - top_row > 8) bot_row -= 2;
                    ImageCrop(&img, (Rectangle){ 0.0f, (float)top_row,
                                                 (float)img.width, (float)(bot_row - top_row + 1) });
                }
            }
            tex = LoadTextureFromImage(img);
            UnloadImage(img);
        }
    } else {
        tex = LoadTexture(path);
    }

    if (tex.id != 0) {
        SetTextureFilter(tex, smooth ? TEXTURE_FILTER_BILINEAR : TEXTURE_FILTER_POINT);
    }
    return tex;
}

static void load_grid_sheet(const char *png_path, const char *jpg_path,
                            int cols, int rows, int inner_white_cells,
                            Texture2D *out_tex, Rectangle *out_rects) {
    const char *path = find_asset_path(png_path, jpg_path);
    if (path == NULL || cols <= 0 || rows <= 0) return;

    Image img = LoadImage(path);
    if (img.data == NULL) return;

    ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    strip_white_flood_grid(&img, cols, rows, 230);

    Color *px = (Color *)img.data;
    int total_cells = cols * rows;

    for (int i = 0; i < total_cells; i++) {
        int c = i % cols;
        int r = i / cols;
        int qx    = (c * img.width) / cols;
        int qy    = (r * img.height) / rows;
        int x_end = ((c + 1) * img.width) / cols;
        int y_end = ((r + 1) * img.height) / rows;

        if (i < inner_white_cells) {
            strip_white_neutral_rect(&img, qx, qy, x_end, y_end, 234, 14);
        }

        int min_x = x_end, max_x = qx;
        int min_y = y_end, max_y = qy;

        for (int y = qy; y < y_end; y++) {
            for (int x = qx; x < x_end; x++) {
                if (px[y * img.width + x].a > 24) {
                    if (x < min_x) min_x = x;
                    if (x > max_x) max_x = x;
                    if (y < min_y) min_y = y;
                    if (y > max_y) max_y = y;
                }
            }
        }

        if (max_x > min_x && max_y > min_y) {
            out_rects[i] = (Rectangle){
                (float)min_x, (float)min_y,
                (float)(max_x - min_x + 1), (float)(max_y - min_y + 1)
            };
        } else {
            out_rects[i] = (Rectangle){
                (float)qx, (float)qy,
                (float)(x_end - qx), (float)(y_end - qy)
            };
        }
    }

    *out_tex = LoadTextureFromImage(img);
    UnloadImage(img);
    if (out_tex->id != 0) {
        SetTextureFilter(*out_tex, TEXTURE_FILTER_BILINEAR);
    }
}

static void unload_if_loaded(Texture2D *tex) {
    if (tex->id != 0) {
        UnloadTexture(*tex);
        *tex = (Texture2D){ 0 };
    }
}

void assets_load(void) {
    /* Zone 1 (Blood-Moon Courtyard) */
    WorldAssets *w = &g_assets.world;
    w->bg              = load_texture_asset("assets/zone_1/bg.png",    "assets/zone_1/bg.jpg",    WHITE_KEEP,          false, true);
    w->buildings       = load_texture_asset("assets/zone_1/front.png", "assets/zone_1/front.jpg", WHITE_STRIP_NEUTRAL, true,  true);
    w->floor           = load_texture_asset("assets/zone_1/floor.png", "assets/zone_1/floor.jpg", WHITE_KEEP,          false, true);
    load_grid_sheet("assets/zone_1/props.png", "assets/zone_1/props.jpg", 2, 2, 4, &w->props, w->prop_rects);
    w->fallback_moon   = load_texture_asset("assets/zone_1/background.png", NULL, WHITE_KEEP, false, true);
    w->fallback_statue = load_texture_asset("assets/zone_1/statue.png",     NULL, WHITE_KEEP, false, false);

    /* Zone 2 (Cursed Catacombs) */
    Zone2Assets *z2 = &g_assets.zone2;
    z2->bg        = load_texture_asset("assets/zone_2/bg.png",    "assets/zone_2/bg.jpg",    WHITE_KEEP,        false, true);
    z2->buildings = load_texture_asset("assets/zone_2/front.png", "assets/zone_2/front.jpg", WHITE_STRIP_FLOOD, true,  true);
    z2->floor     = load_texture_asset("assets/zone_2/floor.png", "assets/zone_2/floor.jpg", WHITE_KEEP,        false, true);
    load_grid_sheet("assets/zone_2/props.png", "assets/zone_2/props.jpg", 2, 2, 0, &z2->props, z2->prop_rects);

    /* Zone 3 (Dead Streets) */
    Zone3Assets *z3 = &g_assets.zone3;
    z3->bg        = load_texture_asset("assets/zone_3/bg.png",    "assets/zone_3/bg.jpg",    WHITE_KEEP,        false, true);
    z3->buildings = load_texture_asset("assets/zone_3/front.png", "assets/zone_3/front.jpg", WHITE_STRIP_FLOOD, true,  true);
    z3->road      = load_texture_asset("assets/zone_3/road.png",  "assets/zone_3/road.jpg",  WHITE_KEEP,        false, true);
    load_grid_sheet("assets/zone_3/props.png", "assets/zone_3/props.jpg", 2, 2, 0, &z3->props, z3->prop_rects);

    /* Zone 4 (Crimson Canyon) */
    Zone4Assets *z4 = &g_assets.zone4;
    z4->bg        = load_texture_asset("assets/zone_4/bg.png",    "assets/zone_4/bg.jpg",    WHITE_KEEP,          false, true);
    z4->buildings = load_texture_asset("assets/zone_4/front.png", "assets/zone_4/front.jpg", WHITE_STRIP_NEUTRAL, true,  true);
    z4->floor     = load_texture_asset("assets/zone_4/floor.png", "assets/zone_4/floor.jpg", WHITE_KEEP,          false, true);
    load_grid_sheet("assets/zone_4/props.png", "assets/zone_4/props.jpg", 2, 2, 4, &z4->props, z4->prop_rects);

    /* Hero, Modern Tactical Weapons & Pickups */
    HeroAssets *h = &g_assets.hero;
    load_grid_sheet("assets/hero/hero.png",    "assets/hero/hero.jpg",    4, 2, 0, &h->hero,    h->hero_rects);
    load_grid_sheet("assets/hero/weapons.png", "assets/hero/weapons.jpg", 3, 2, 3, &h->weapons, h->weapon_rects);
    load_grid_sheet("assets/hero/pickups.png", "assets/hero/pickups.jpg", 2, 2, 0, &h->pickups, h->pickup_rects);

    /* Zombies & Final Boss (The Night Spirit) */
    EnemyAssets *e = &g_assets.enemies;
    load_grid_sheet("assets/enemies/zombies.png", "assets/enemies/zombies.jpg", 4, 3, 0,
                    &e->zombies, e->zombie_rects);
    load_grid_sheet("assets/enemies/boss.png",    "assets/enemies/boss.jpg",    4, 2, 0,
                    &e->boss,    e->boss_rects);
}

void assets_unload(void) {
    WorldAssets *w = &g_assets.world;
    unload_if_loaded(&w->bg);
    unload_if_loaded(&w->buildings);
    unload_if_loaded(&w->floor);
    unload_if_loaded(&w->props);
    unload_if_loaded(&w->fallback_moon);
    unload_if_loaded(&w->fallback_statue);

    Zone2Assets *z2 = &g_assets.zone2;
    unload_if_loaded(&z2->bg);
    unload_if_loaded(&z2->buildings);
    unload_if_loaded(&z2->floor);
    unload_if_loaded(&z2->props);

    Zone3Assets *z3 = &g_assets.zone3;
    unload_if_loaded(&z3->bg);
    unload_if_loaded(&z3->buildings);
    unload_if_loaded(&z3->road);
    unload_if_loaded(&z3->props);

    Zone4Assets *z4 = &g_assets.zone4;
    unload_if_loaded(&z4->bg);
    unload_if_loaded(&z4->buildings);
    unload_if_loaded(&z4->floor);
    unload_if_loaded(&z4->props);

    HeroAssets *h = &g_assets.hero;
    unload_if_loaded(&h->hero);
    unload_if_loaded(&h->weapons);
    unload_if_loaded(&h->pickups);

    EnemyAssets *e = &g_assets.enemies;
    unload_if_loaded(&e->zombies);
    unload_if_loaded(&e->boss);
}

const Assets *assets_get(void) {
    return &g_assets;
}