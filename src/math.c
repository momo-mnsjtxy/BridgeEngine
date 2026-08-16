#include "BridgeEngine.h"
#include <math.h>

static float bapi_math_abs(float value)
{
	return value < 0.0f ? -value : value;
}

static float bapi_math_min(float left, float right)
{
	return left < right ? left : right;
}

static float bapi_math_max(float left, float right)
{
	return left > right ? left : right;
}

bapi_vec2_t bapi_vec2(float x, float y)
{
	bapi_vec2_t v = {x, y};
	return v;
}

bapi_vec2_t bapi_vec2_add(bapi_vec2_t a, bapi_vec2_t b)
{
	bapi_vec2_t v = {a.x + b.x, a.y + b.y};
	return v;
}

bapi_vec2_t bapi_vec2_sub(bapi_vec2_t a, bapi_vec2_t b)
{
	bapi_vec2_t v = {a.x - b.x, a.y - b.y};
	return v;
}

bapi_vec2_t bapi_vec2_scale(bapi_vec2_t v, float s)
{
	bapi_vec2_t r = {v.x * s, v.y * s};
	return r;
}

bapi_vec2_t bapi_vec2_negate(bapi_vec2_t v)
{
	bapi_vec2_t r = {-v.x, -v.y};
	return r;
}

float bapi_vec2_dot(bapi_vec2_t a, bapi_vec2_t b)
{
	return a.x * b.x + a.y * b.y;
}

float bapi_vec2_length(bapi_vec2_t v)
{
	return sqrtf(v.x * v.x + v.y * v.y);
}

float bapi_vec2_length_sq(bapi_vec2_t v)
{
	return v.x * v.x + v.y * v.y;
}

bapi_vec2_t bapi_vec2_normalize(bapi_vec2_t v)
{
	float len = bapi_vec2_length(v);
	if (!(len > 0.00001f)) {
		bapi_vec2_t zero = {0, 0};
		return zero;
	}
	bapi_vec2_t r = {v.x / len, v.y / len};
	return r;
}

float bapi_vec2_distance(bapi_vec2_t a, bapi_vec2_t b)
{
	float dx = a.x - b.x;
	float dy = a.y - b.y;
	return sqrtf(dx * dx + dy * dy);
}

bapi_vec2_t bapi_vec2_lerp(bapi_vec2_t a, bapi_vec2_t b, float t)
{
	bapi_vec2_t v = {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t};
	return v;
}

bapi_vec2_t bapi_vec2_rotate(bapi_vec2_t v, float angle)
{
	float		c = cosf(angle);
	float		s = sinf(angle);
	bapi_vec2_t r = {v.x * c - v.y * s, v.x * s + v.y * c};
	return r;
}

int bapi_vec2_equals(bapi_vec2_t a, bapi_vec2_t b)
{
	const float eps = 0.00001f;
	return bapi_math_abs(a.x - b.x) < eps && bapi_math_abs(a.y - b.y) < eps;
}

bapi_circle_t bapi_circle(float x, float y, float r)
{
	bapi_circle_t c = {x, y, r};
	return c;
}

int bapi_rect_contains_point(bapi_rect_t r, bapi_vec2_t p)
{
	return p.x >= r.x && p.x <= r.x + r.w && p.y >= r.y && p.y <= r.y + r.h;
}

int bapi_rect_intersects(bapi_rect_t a, bapi_rect_t b)
{
	return a.x < b.x + b.w && a.x + a.w > b.x && a.y < b.y + b.h && a.y + a.h > b.y;
}

bapi_rect_t bapi_rect_intersection(bapi_rect_t a, bapi_rect_t b)
{
	bapi_rect_t r;
	r.x = bapi_math_max(a.x, b.x);
	r.y = bapi_math_max(a.y, b.y);
	r.w = bapi_math_min(a.x + a.w, b.x + b.w) - r.x;
	r.h = bapi_math_min(a.y + a.h, b.y + b.h) - r.y;
	if (r.w < 0) r.w = 0;
	if (r.h < 0) r.h = 0;
	return r;
}

bapi_rect_t bapi_rect_union(bapi_rect_t a, bapi_rect_t b)
{
	bapi_rect_t r;
	r.x = bapi_math_min(a.x, b.x);
	r.y = bapi_math_min(a.y, b.y);
	r.w = bapi_math_max(a.x + a.w, b.x + b.w) - r.x;
	r.h = bapi_math_max(a.y + a.h, b.y + b.h) - r.y;
	return r;
}

bapi_vec2_t bapi_rect_center(bapi_rect_t r)
{
	bapi_vec2_t v = {r.x + r.w * 0.5f, r.y + r.h * 0.5f};
	return v;
}

int bapi_circle_contains_point(bapi_circle_t c, bapi_vec2_t p)
{
	float dx = p.x - c.x;
	float dy = p.y - c.y;
	return (dx * dx + dy * dy) <= (c.r * c.r);
}

int bapi_circle_intersects_circle(bapi_circle_t a, bapi_circle_t b)
{
	float dx	  = a.x - b.x;
	float dy	  = a.y - b.y;
	float dist_sq = dx * dx + dy * dy;
	float rad_sum = a.r + b.r;
	return dist_sq <= rad_sum * rad_sum;
}

int bapi_circle_intersects_rect(bapi_circle_t c, bapi_rect_t r)
{
	float near_x = bapi_math_max(r.x, bapi_math_min(c.x, r.x + r.w));
	float near_y = bapi_math_max(r.y, bapi_math_min(c.y, r.y + r.h));
	float dx	 = c.x - near_x;
	float dy	 = c.y - near_y;
	return (dx * dx + dy * dy) <= (c.r * c.r);
}

int bapi_collision_aabb(bapi_rect_t a, bapi_rect_t b)
{
	return bapi_rect_intersects(a, b);
}

float bapi_clamp(float value, float min, float max)
{
	if (min > max) {
		float tmp = min;
		min = max;
		max = tmp;
	}
	if (value < min) return min;
	if (value > max) return max;
	return value;
}

float bapi_lerp(float a, float b, float t)
{
	return a + (b - a) * t;
}

float bapi_deg_to_rad(float deg)
{
	return deg * 0.01745329252f;
}

float bapi_rad_to_deg(float rad)
{
	return rad * 57.295779513f;
}
