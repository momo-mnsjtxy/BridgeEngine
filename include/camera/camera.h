#pragma once

#include "bapi_types.h"
#include "math/math.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float x, y;
    float zoom;
    float viewport_w;
    float viewport_h;
    float rotation;
} bapi_camera_t;

void bapi_camera_init(bapi_camera_t* cam, float viewport_w, float viewport_h);
void bapi_camera_set_position(bapi_camera_t* cam, float x, float y);
void bapi_camera_move(bapi_camera_t* cam, float dx, float dy);
void bapi_camera_set_zoom(bapi_camera_t* cam, float zoom);
void bapi_camera_set_rotation(bapi_camera_t* cam, float angle_rad);
void bapi_camera_set_viewport(bapi_camera_t* cam, float w, float h);

void bapi_camera_world_to_screen(bapi_camera_t* cam, float wx, float wy, float* sx, float* sy);
void bapi_camera_screen_to_world(bapi_camera_t* cam, float sx, float sy, float* wx, float* wy);

bapi_vec2_t bapi_camera_world_to_screen_v(bapi_camera_t* cam, bapi_vec2_t world);
bapi_vec2_t bapi_camera_screen_to_world_v(bapi_camera_t* cam, bapi_vec2_t screen);

void bapi_camera_get_view_rect(bapi_camera_t* cam, bapi_rect_t* out_rect);

#ifdef __cplusplus
}
#endif
