#ifndef CAMERA_H
#define CAMERA_H

#define WINDOW_W 1280
#define WINDOW_H 720

typedef struct {
    float pan_x, pan_y;
    float zoom;
    int   is_dragging;
    float drag_start_x, drag_start_y;
    float drag_pan_x0, drag_pan_y0;
} SimCamera;

void camera_init(SimCamera *cam, float galaxy_radius);
int  camera_update(SimCamera *cam, float galaxy_radius);
void world_to_screen(SimCamera *cam, float wx, float wy, float *sx, float *sy);
void screen_to_world(SimCamera *cam, float sx, float sy, float *wx, float *wy);

#endif
