#include "camera.h"
#include "raylib.h"

void camera_init(SimCamera *cam, float galaxy_radius) {
    cam->zoom        = (WINDOW_H * 0.4f) / galaxy_radius;
    cam->pan_x       = WINDOW_W * 0.5f;
    cam->pan_y       = WINDOW_H * 0.5f;
    cam->is_dragging = 0;
}

void world_to_screen(SimCamera *cam, float wx, float wy, float *sx, float *sy) {
    *sx = wx * cam->zoom + cam->pan_x;
    *sy = wy * cam->zoom + cam->pan_y;
}

void screen_to_world(SimCamera *cam, float sx, float sy, float *wx, float *wy) {
    *wx = (sx - cam->pan_x) / cam->zoom;
    *wy = (sy - cam->pan_y) / cam->zoom;
}

int camera_update(SimCamera *cam, float galaxy_radius) {
    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f) {
        Vector2 mouse = GetMousePosition();
        float wx, wy;
        screen_to_world(cam, mouse.x, mouse.y, &wx, &wy);
        cam->zoom *= (wheel > 0.0f) ? 1.15f : (1.0f / 1.15f);
        cam->pan_x = mouse.x - wx * cam->zoom;
        cam->pan_y = mouse.y - wy * cam->zoom;
    }

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        cam->is_dragging  = 1;
        cam->drag_start_x = GetMousePosition().x;
        cam->drag_start_y = GetMousePosition().y;
        cam->drag_pan_x0  = cam->pan_x;
        cam->drag_pan_y0  = cam->pan_y;
    }
    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        cam->is_dragging = 0;
    }
    if (cam->is_dragging) {
        Vector2 mouse = GetMousePosition();
        cam->pan_x = cam->drag_pan_x0 + (mouse.x - cam->drag_start_x);
        cam->pan_y = cam->drag_pan_y0 + (mouse.y - cam->drag_start_y);
    }

    if (IsKeyPressed(KEY_R)) {
        camera_init(cam, galaxy_radius);
        return 1;
    }
    return 0;
}
