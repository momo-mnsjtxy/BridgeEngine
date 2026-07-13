#pragma once

#include "bapi_types.h"
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct { float x, y; } bapi_vec2_t;

bapi_vec2_t bapi_vec2(float x, float y);
bapi_vec2_t bapi_vec2_add(bapi_vec2_t a, bapi_vec2_t b);
bapi_vec2_t bapi_vec2_sub(bapi_vec2_t a, bapi_vec2_t b);
bapi_vec2_t bapi_vec2_scale(bapi_vec2_t v, float s);
bapi_vec2_t bapi_vec2_negate(bapi_vec2_t v);
float       bapi_vec2_dot(bapi_vec2_t a, bapi_vec2_t b);
float       bapi_vec2_length(bapi_vec2_t v);
float       bapi_vec2_length_sq(bapi_vec2_t v);
bapi_vec2_t bapi_vec2_normalize(bapi_vec2_t v);
float       bapi_vec2_distance(bapi_vec2_t a, bapi_vec2_t b);
bapi_vec2_t bapi_vec2_lerp(bapi_vec2_t a, bapi_vec2_t b, float t);
bapi_vec2_t bapi_vec2_rotate(bapi_vec2_t v, float angle);

int bapi_vec2_equals(bapi_vec2_t a, bapi_vec2_t b);

typedef struct { float x, y, r; } bapi_circle_t;

bapi_circle_t bapi_circle(float x, float y, float r);

int bapi_rect_contains_point(bapi_rect_t r, bapi_vec2_t p);
int bapi_rect_intersects(bapi_rect_t a, bapi_rect_t b);
bapi_rect_t bapi_rect_intersection(bapi_rect_t a, bapi_rect_t b);
bapi_rect_t bapi_rect_union(bapi_rect_t a, bapi_rect_t b);
bapi_vec2_t bapi_rect_center(bapi_rect_t r);

int bapi_circle_contains_point(bapi_circle_t c, bapi_vec2_t p);
int bapi_circle_intersects_circle(bapi_circle_t a, bapi_circle_t b);
int bapi_circle_intersects_rect(bapi_circle_t c, bapi_rect_t r);

int bapi_collision_aabb(bapi_rect_t a, bapi_rect_t b);

float bapi_clamp(float value, float min, float max);
float bapi_lerp(float a, float b, float t);
float bapi_deg_to_rad(float deg);
float bapi_rad_to_deg(float rad);

#ifdef __cplusplus
}
#endif
