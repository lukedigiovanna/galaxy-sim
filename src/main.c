#include "raylib.h"
#include "simulation.h"
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

    DensityGrid *grid = renderer_create();

    SimCamera cam;
    float galaxy_radius = 4.0f * GALAXY_R_SCALE;
    camera_init(&cam, galaxy_radius);

    int paused = 0;

    while (!WindowShouldClose()) {
        int reset = camera_update(&cam, galaxy_radius);
        (void)reset;

        if (IsKeyPressed(KEY_SPACE)) paused = !paused;

        BeginDrawing();
        ClearBackground(BLACK);

        renderer_draw(grid, ps, &cam);

        DrawFPS(10, 10);
        DrawText(paused ? "PAUSED" : "RUNNING", 10, 30, 16, GRAY);

        EndDrawing();
    }

    renderer_destroy(grid);
    particles_destroy(ps);
    CloseWindow();
    return 0;
}
