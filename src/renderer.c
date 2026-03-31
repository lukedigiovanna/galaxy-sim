#include "renderer.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* Precomputed 256-entry palette */
static unsigned char palette[256][3];
static int palette_built = 0;

/* 5 color stops: t, r, g, b */
static const float stops[5][4] = {
    { 0.00f,   0,   0,   0 },
    { 0.25f,  10,  10,  80 },
    { 0.60f,  60, 120, 220 },
    { 0.85f, 180, 220, 255 },
    { 1.00f, 255, 255, 255 },
};

static void build_palette(void) {
    for (int i = 0; i < 256; i++) {
        float t = (float)i / 255.0f;
        int seg = 0;
        for (int s = 0; s < 4; s++) {
            if (t >= stops[s][0]) seg = s;
        }
        float t0 = stops[seg][0];
        float t1 = stops[seg+1][0];
        float f  = (t - t0) / (t1 - t0);
        palette[i][0] = (unsigned char)(stops[seg][1] + (stops[seg+1][1] - stops[seg][1]) * f);
        palette[i][1] = (unsigned char)(stops[seg][2] + (stops[seg+1][2] - stops[seg][2]) * f);
        palette[i][2] = (unsigned char)(stops[seg][3] + (stops[seg+1][3] - stops[seg][3]) * f);
    }
    palette_built = 1;
}

/* File-scope blur buffer — avoids stack allocation issues */
static float blur_buf[GRID_W * GRID_H];

DensityGrid* renderer_create(void) {
    if (!palette_built) build_palette();

    DensityGrid *grid = (DensityGrid*)malloc(sizeof(DensityGrid));
    grid->width  = GRID_W;
    grid->height = GRID_H;
    grid->cells  = (float*)malloc(GRID_W * GRID_H * sizeof(float));
    grid->pixels = (unsigned char*)malloc(GRID_W * GRID_H * 4);

    Image img = GenImageColor(GRID_W, GRID_H, BLACK);
    grid->texture = LoadTextureFromImage(img);
    UnloadImage(img);
    SetTextureFilter(grid->texture, TEXTURE_FILTER_BILINEAR);

    return grid;
}

void renderer_destroy(DensityGrid *grid) {
    if (!grid) return;
    UnloadTexture(grid->texture);
    free(grid->cells);
    free(grid->pixels);
    free(grid);
}

void renderer_draw(DensityGrid *grid, ParticleSystem *ps, SimCamera *cam) {
    int W = grid->width;
    int H = grid->height;

    /* Step 1: clear */
    memset(grid->cells, 0, W * H * sizeof(float));

    /* Step 2: rasterize particles */
    for (int i = 0; i < ps->count; i++) {
        float sx = ps->x[i] * cam->zoom + cam->pan_x;
        float sy = ps->y[i] * cam->zoom + cam->pan_y;

        int gi = (int)(sx * W / (float)WINDOW_W);
        int gj = (int)(sy * H / (float)WINDOW_H);

        if (gi >= 0 && gi < W && gj >= 0 && gj < H) {
            grid->cells[gj * W + gi] += 1.0f;
        }
    }

    /* Step 3: 3x3 box blur */
    for (int j = 1; j < H - 1; j++) {
        for (int i = 1; i < W - 1; i++) {
            blur_buf[j*W + i] = (
                grid->cells[(j-1)*W + (i-1)] + grid->cells[(j-1)*W + i] + grid->cells[(j-1)*W + (i+1)] +
                grid->cells[ j   *W + (i-1)] + grid->cells[ j   *W + i] + grid->cells[ j   *W + (i+1)] +
                grid->cells[(j+1)*W + (i-1)] + grid->cells[(j+1)*W + i] + grid->cells[(j+1)*W + (i+1)]
            ) / 9.0f;
        }
    }
    /* Copy blurred interior back; leave border as-is */
    for (int j = 1; j < H - 1; j++) {
        for (int i = 1; i < W - 1; i++) {
            grid->cells[j*W + i] = blur_buf[j*W + i];
        }
    }

    /* Step 4: find max density */
    float max_density = 1.0f;
    for (int k = 0; k < W * H; k++) {
        if (grid->cells[k] > max_density) max_density = grid->cells[k];
    }

    /* Step 5: log-scale normalize + color map */
    float log_denom = logf(1.0f + DENSITY_LOG_SCALE);
    for (int k = 0; k < W * H; k++) {
        float norm       = grid->cells[k] / max_density;
        float brightness = logf(1.0f + norm * DENSITY_LOG_SCALE) / log_denom;
        int   idx        = (int)(brightness * 255.0f);
        if (idx > 255) idx = 255;
        grid->pixels[k*4 + 0] = palette[idx][0];
        grid->pixels[k*4 + 1] = palette[idx][1];
        grid->pixels[k*4 + 2] = palette[idx][2];
        grid->pixels[k*4 + 3] = 255;
    }

    /* Step 6: upload to GPU */
    UpdateTexture(grid->texture, grid->pixels);

    /* Step 7: draw upscaled to full window */
    Rectangle src  = { 0, 0, (float)GRID_W, (float)GRID_H };
    Rectangle dest = { 0, 0, (float)WINDOW_W, (float)WINDOW_H };
    DrawTexturePro(grid->texture, src, dest, (Vector2){0, 0}, 0.0f, WHITE);
}
