#include "raylib.h"

int main(void) {
    InitWindow(1280, 720, "Riftwalker");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);
        DrawText("Riftwalker", 520, 340, 40, RAYWHITE);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
