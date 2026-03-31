#ifndef RENDERER_H
#define RENDERER_H

#include "raylib.h"
#include "simulation.h"
#include "camera.h"

#define GRID_W            640
#define GRID_H            360
#define DENSITY_LOG_SCALE 20.0f

typedef struct {
    float         *cells;
    unsigned char *pixels;
    int            width, height;
    Texture2D      texture;
} DensityGrid;

DensityGrid* renderer_create(void);
void         renderer_destroy(DensityGrid *grid);
void         renderer_draw(DensityGrid *grid, ParticleSystem *ps, SimCamera *cam);

#endif
