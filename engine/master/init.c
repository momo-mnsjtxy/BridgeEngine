#include "platform/platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include "bapi_types.h"
#include "bapi_internal.h"
#include "log/log.h"

struct bapi_window_internal {
	plat_window_t plat_window;
};

struct bapi_renderer_internal {
	plat_renderer_t plat_renderer;
};

static plat_window_t window = NULL;
static plat_renderer_t renderer = NULL;
static bool initialized = false;

plat_renderer_t bapi_internal_renderer = NULL;

bapi_window_t bapi_engine_get_window(void)
{
	bapi_window_t win = malloc(sizeof(struct bapi_window_internal));
	if (win) {
		win->plat_window = window;
	}
	return win;
}

bapi_renderer_t bapi_engine_get_renderer(void)
{
	bapi_renderer_t rend = malloc(sizeof(struct bapi_renderer_internal));
	if (rend) {
		rend->plat_renderer = renderer;
	}
	return rend;
}

int bapi_engine_init(const char* title, int width, int height)
{
	if (initialized) {
		return 0;
	}

	
#ifdef USE_BACKEND_XJ380
	if (plat_init(plat_xj380_interface()) != 0) {
#else
	if (plat_init(plat_sdl3_interface()) != 0) {
#endif
		return 1;
	}

	const plat_interface_t* plat = plat_get();

	if (plat->init(PLAT_INIT_VIDEO | PLAT_INIT_AUDIO) != 0) {
		BAPI_LOG_INIT_DEFAULT();
		BAPI_LOG_CRITICAL("Failed to initialize platform backend");
		return 1;
	}

	window = plat->create_window(title, width, height);
	if (window == NULL) {
		BAPI_LOG_INIT_DEFAULT();
		BAPI_LOG_CRITICAL("Failed to create window");
		plat->quit();
		return 1;
	}

	renderer = plat->create_renderer(window);
	bapi_internal_renderer = renderer;

	plat->set_render_draw_color(renderer, 0, 0, 0, 255);
	plat->render_clear(renderer);
	plat->set_render_draw_blend_mode(renderer, PLAT_BLENDMODE_BLEND);
	plat->render_present(renderer);

	if (plat->init_ttf() != 0) {
		BAPI_LOG_INIT_DEFAULT();
		BAPI_LOG_WARN("Failed to initialize TTF");
		return 0;
	}

	initialized = true;
	return 0;
}

void bapi_engine_quit(void)
{
	const plat_interface_t* plat = plat_get();

	plat->quit_ttf();
	if (renderer) {
		plat->destroy_renderer(renderer);
		renderer = NULL;
		bapi_internal_renderer = NULL;
	}
	if (window) {
		plat->destroy_window(window);
		window = NULL;
	}
	plat->quit();
	initialized = false;
	bapi_log_shutdown();
	plat_shutdown();
}

int bapi_poll_event(bapi_event_t* event)
{
	const plat_interface_t* plat = plat_get();
	if (event == NULL) {
		return plat->poll_event(NULL);
	}
	return plat->poll_event(&event->event);
}

int bapi_event_get_type(const bapi_event_t* event)
{
	return event->event.type;
}

uint8_t bapi_event_get_key_code(const bapi_event_t* event)
{
	return (uint8_t)event->event.data.key.key;
}

int bapi_event_get_mouse_x(const bapi_event_t* event)
{
	return (int)event->event.data.button.x;
}

int bapi_event_get_mouse_y(const bapi_event_t* event)
{
	return (int)event->event.data.button.y;
}

int bapi_event_get_mouse_button(const bapi_event_t* event)
{
	return event->event.data.button.button;
}

int bapi_event_get_motion_x(const bapi_event_t* event)
{
	return (int)event->event.data.motion.x;
}

int bapi_event_get_motion_y(const bapi_event_t* event)
{
	return (int)event->event.data.motion.y;
}

int bapi_event_is_mouse_button_down(const bapi_event_t* event)
{
	return event->event.type == PLAT_EVENT_MOUSE_BUTTON_DOWN;
}

int bapi_event_is_mouse_button_up(const bapi_event_t* event)
{
	return event->event.type == PLAT_EVENT_MOUSE_BUTTON_UP;
}

int bapi_event_is_mouse_motion(const bapi_event_t* event)
{
	return event->event.type == PLAT_EVENT_MOUSE_MOTION;
}

void bapi_render_clear(void)
{
	const plat_interface_t* plat = plat_get();
	plat->set_render_draw_color(renderer, 0, 0, 0, 255);
	plat->render_clear(renderer);
}

void bapi_render_present(void)
{
	const plat_interface_t* plat = plat_get();
	plat->render_present(renderer);
}

void bapi_set_render_color(bapi_color_t color)
{
	const plat_interface_t* plat = plat_get();
	plat->set_render_draw_color(renderer, color.r, color.g, color.b, color.a);
}

void bapi_delay(uint32_t ms)
{
	const plat_interface_t* plat = plat_get();
	plat->delay(ms);
}

bapi_color_t bapi_color_from_hex(uint32_t hex_color)
{
	bapi_color_t color;
	color.r = (hex_color >> 24) & 0xFF;
	color.g = (hex_color >> 16) & 0xFF;
	color.b = (hex_color >> 8) & 0xFF;
	color.a = hex_color & 0xFF;
	return color;
}

bapi_color_t bapi_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	bapi_color_t color = {r, g, b, a};
	return color;
}

uint32_t bapi_get_ticks(void)
{
	const plat_interface_t* plat = plat_get();
	return plat->get_ticks();
}

void bapi_draw_pixel(float x, float y, bapi_color_t color)
{
	const plat_interface_t* plat = plat_get();
	plat->set_render_draw_color(renderer, color.r, color.g, color.b, color.a);
	plat->render_point(renderer, x, y);
}

void bapi_draw_line(float x1, float y1, float x2, float y2, bapi_color_t color)
{
	const plat_interface_t* plat = plat_get();
	plat->set_render_draw_color(renderer, color.r, color.g, color.b, color.a);
	plat->render_line(renderer, x1, y1, x2, y2);
}

void bapi_draw_rect(float x, float y, float w, float h, bapi_color_t color)
{
	const plat_interface_t* plat = plat_get();
	plat->set_render_draw_color(renderer, color.r, color.g, color.b, color.a);
	plat->render_rect(renderer, x, y, w, h);
}

void bapi_fill_rect(float x, float y, float w, float h, bapi_color_t color)
{
	const plat_interface_t* plat = plat_get();
	plat->set_render_draw_color(renderer, color.r, color.g, color.b, color.a);
	plat->render_fill_rect(renderer, x, y, w, h);
}

void bapi_draw_triangle(float x1, float y1, float x2, float y2, float x3, float y3, bapi_color_t color)
{
	const plat_interface_t* plat = plat_get();
	plat->set_render_draw_color(renderer, color.r, color.g, color.b, color.a);
	plat->render_line(renderer, x1, y1, x2, y2);
	plat->render_line(renderer, x2, y2, x3, y3);
	plat->render_line(renderer, x3, y3, x1, y1);
}

void bapi_draw_circle(float cx, float cy, float radius, bapi_color_t color)
{
	const plat_interface_t* plat = plat_get();
	plat->set_render_draw_color(renderer, color.r, color.g, color.b, color.a);
	int segments = 64;
	float angle_step = 2.0f * (float)M_PI / segments;

	for (int i = 0; i < segments; i++) {
		float angle1 = i * angle_step;
		float angle2 = (i + 1) * angle_step;

		float x1 = cx + cosf(angle1) * radius;
		float y1 = cy + sinf(angle1) * radius;
		float x2 = cx + cosf(angle2) * radius;
		float y2 = cy + sinf(angle2) * radius;

		plat->render_line(renderer, x1, y1, x2, y2);
	}
}

void bapi_fill_circle(float cx, float cy, float radius, bapi_color_t color)
{
	const plat_interface_t* plat = plat_get();
	plat->set_render_draw_color(renderer, color.r, color.g, color.b, color.a);
	int segments = 64;
	float angle_step = 2.0f * (float)M_PI / segments;

	for (int i = 0; i < segments; i++) {
		float angle1 = i * angle_step;
		float angle2 = (i + 1) * angle_step;

		float x1 = cx + cosf(angle1) * radius;
		float y1 = cy + sinf(angle1) * radius;
		float x2 = cx + cosf(angle2) * radius;
		float y2 = cy + sinf(angle2) * radius;

		plat->render_line(renderer, cx, cy, x1, y1);
		plat->render_line(renderer, x1, y1, x2, y2);
		plat->render_line(renderer, x2, y2, cx, cy);
	}
}

void bapi_draw_polygon(float cx, float cy, float radius, int sides, bapi_color_t color)
{
	if (sides < 3) {
		return;
	}

	const plat_interface_t* plat = plat_get();
	plat->set_render_draw_color(renderer, color.r, color.g, color.b, color.a);
	float angle_step = 2.0f * (float)M_PI / sides;

	for (int i = 0; i < sides; i++) {
		float angle1 = i * angle_step - (float)M_PI / 2;
		float angle2 = (i + 1) * angle_step - (float)M_PI / 2;

		float x1 = cx + cosf(angle1) * radius;
		float y1 = cy + sinf(angle1) * radius;
		float x2 = cx + cosf(angle2) * radius;
		float y2 = cy + sinf(angle2) * radius;

		plat->render_line(renderer, x1, y1, x2, y2);
	}
}

void bapi_fill_polygon(float cx, float cy, float radius, int sides, bapi_color_t color)
{
	if (sides < 3) {
		return;
	}

	const plat_interface_t* plat = plat_get();
	plat->set_render_draw_color(renderer, color.r, color.g, color.b, color.a);
	float angle_step = 2.0f * (float)M_PI / sides;

	for (int i = 0; i < sides; i++) {
		float angle1 = i * angle_step - (float)M_PI / 2;
		float angle2 = (i + 1) * angle_step - (float)M_PI / 2;

		float x1 = cx + cosf(angle1) * radius;
		float y1 = cy + sinf(angle1) * radius;
		float x2 = cx + cosf(angle2) * radius;
		float y2 = cy + sinf(angle2) * radius;

		plat->render_line(renderer, cx, cy, x1, y1);
		plat->render_line(renderer, x1, y1, x2, y2);
		plat->render_line(renderer, x2, y2, cx, cy);
	}
}

void bapi_draw_image(const char* filepath, float x, float y, float w, float h)
{
	const plat_interface_t* plat = plat_get();

	plat_surface_t* surface = plat->load_image(filepath);
	if (surface == NULL) {
		return;
	}

	bapi_texture_t texture = malloc(sizeof(struct bapi_texture_internal));
	if (texture == NULL) {
		plat->destroy_surface(surface);
		return;
	}

	texture->plat_texture = plat->create_texture_from_surface(renderer, surface);
	plat->destroy_surface(surface);

	if (texture->plat_texture == NULL) {
		free(texture);
		return;
	}

	plat->render_texture(renderer, texture->plat_texture, x, y, w, h);

	plat->destroy_texture(texture->plat_texture);
	free(texture);
}
