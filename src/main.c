#include "raylib.h"
#include "simulation.h"
#include "quadtree.h"
#include "galaxy_init.h"
#include "renderer.h"
#include "camera.h"
#include <stdlib.h>

int main(void) {
    InitWindow(WINDOW_W, WINDOW_H, "Riftwalker - Galaxy Sim");
    SetTargetFPS(60);

    srand(42);

    ParticleSystem *ps = particles_create(GALAXY_N);
    galaxy_init(ps);

    QuadTree *tree = quadtree_create(GALAXY_N);

    /* Initial force computation — leapfrog needs valid ax/ay before first step */
    quadtree_build(tree, ps);
    quadtree_compute_forces(tree, ps);

    DensityGrid *grid = renderer_create();

    SimCamera cam;
    float galaxy_radius = 2.0f * GALAXY_R_SCALE;
    camera_init(&cam, galaxy_radius);

    int paused = 0;
    float dt = 0.01f;

    while (!WindowShouldClose()) {
        int reset = camera_update(&cam, galaxy_radius);

        if (reset) {
            srand(42);
            galaxy_init(ps);
            quadtree_build(tree, ps);
            quadtree_compute_forces(tree, ps);
        }

        if (IsKeyPressed(KEY_SPACE)) paused = !paused;

        if (!paused) {
            simulation_step(ps, tree, dt);
        }

        BeginDrawing();
        ClearBackground(BLACK);
        renderer_draw(grid, ps, &cam);
        DrawFPS(10, 10);
        DrawText(paused ? "PAUSED" : "RUNNING", 10, 30, 16, GRAY);
        EndDrawing();
    }

    quadtree_destroy(tree);
    renderer_destroy(grid);
    particles_destroy(ps);
    CloseWindow();
    return 0;
}
