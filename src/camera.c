#include "BridgeEngine.h"
#include <math.h>

void bapi_camera_init(bapi_camera_t* cam, float viewport_w, float viewport_h) {
    cam->x = 0;
    cam->y = 0;
    cam->zoom = 1.0f;
    cam->rotation = 0;
    cam->viewport_w = viewport_w;
    cam->viewport_h = viewport_h;
}

void bapi_camera_set_position(bapi_camera_t* cam, float x, float y) {
    cam->x = x;
    cam->y = y;
}

void bapi_camera_move(bapi_camera_t* cam, float dx, float dy) {
    cam->x += dx;
    cam->y += dy;
}

void bapi_camera_set_zoom(bapi_camera_t* cam, float zoom) {
    cam->zoom = zoom;
}

void bapi_camera_set_rotation(bapi_camera_t* cam, float angle_rad) {
    cam->rotation = angle_rad;
}

void bapi_camera_set_viewport(bapi_camera_t* cam, float w, float h) {
    cam->viewport_w = w;
    cam->viewport_h = h;
}

void bapi_camera_world_to_screen(bapi_camera_t* cam, float wx, float wy, float* sx, float* sy) {
    float rx = wx - cam->x;
    float ry = wy - cam->y;

    if (cam->rotation != 0) {
        float c = cosf(-cam->rotation);
        float s = sinf(-cam->rotation);
        float tx = rx * c - ry * s;
        float ty = rx * s + ry * c;
        rx = tx;
        ry = ty;
    }

    if (sx) *sx = rx * cam->zoom + cam->viewport_w * 0.5f;
    if (sy) *sy = ry * cam->zoom + cam->viewport_h * 0.5f;
}

void bapi_camera_screen_to_world(bapi_camera_t* cam, float sx, float sy, float* wx, float* wy) {
    float rx = (sx - cam->viewport_w * 0.5f) / cam->zoom;
    float ry = (sy - cam->viewport_h * 0.5f) / cam->zoom;

    if (cam->rotation != 0) {
        float c = cosf(cam->rotation);
        float s = sinf(cam->rotation);
        float tx = rx * c - ry * s;
        float ty = rx * s + ry * c;
        rx = tx;
        ry = ty;
    }

    if (wx) *wx = rx + cam->x;
    if (wy) *wy = ry + cam->y;
}

bapi_vec2_t bapi_camera_world_to_screen_v(bapi_camera_t* cam, bapi_vec2_t world) {
    bapi_vec2_t screen;
    bapi_camera_world_to_screen(cam, world.x, world.y, &screen.x, &screen.y);
    return screen;
}

bapi_vec2_t bapi_camera_screen_to_world_v(bapi_camera_t* cam, bapi_vec2_t screen) {
    bapi_vec2_t world;
    bapi_camera_screen_to_world(cam, screen.x, screen.y, &world.x, &world.y);
    return world;
}

void bapi_camera_get_view_rect(bapi_camera_t* cam, bapi_rect_t* out_rect) {
    float half_w = cam->viewport_w * 0.5f / cam->zoom;
    float half_h = cam->viewport_h * 0.5f / cam->zoom;
    out_rect->x = cam->x - half_w;
    out_rect->y = cam->y - half_h;
    out_rect->w = half_w * 2;
    out_rect->h = half_h * 2;
}
