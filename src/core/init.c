#include "BridgeEngine.h"
#include "internal/bapi_internal.h"
#include "internal/engine/math.h"
#include "internal/engine/render_context.h"
#include "internal/engine/texture_internal.h"
#include "internal/platform/platform.h"
#include <math.h>
#include <stdlib.h>

struct bapi_window_internal {
	plat_window_t plat_window;
};

struct bapi_renderer_internal {
	plat_renderer_t plat_renderer;
};

static void bapi_event_from_platform(bapi_event_t *dst, const plat_event_t *src)
{
	switch (src->type) {
	case PLAT_EVENT_QUIT:
		dst->type = BAPI_EVENT_QUIT;
		break;
	case PLAT_EVENT_KEY_DOWN:
		dst->type		  = BAPI_EVENT_KEY_DOWN;
		dst->data.key.key = src->data.key.key;
		break;
	case PLAT_EVENT_KEY_UP:
		dst->type		  = BAPI_EVENT_KEY_UP;
		dst->data.key.key = src->data.key.key;
		break;
	case PLAT_EVENT_MOUSE_BUTTON_DOWN:
		dst->type				= BAPI_EVENT_MOUSE_BUTTON_DOWN;
		dst->data.button.x		= src->data.button.x;
		dst->data.button.y		= src->data.button.y;
		dst->data.button.button = src->data.button.button;
		break;
	case PLAT_EVENT_MOUSE_BUTTON_UP:
		dst->type				= BAPI_EVENT_MOUSE_BUTTON_UP;
		dst->data.button.x		= src->data.button.x;
		dst->data.button.y		= src->data.button.y;
		dst->data.button.button = src->data.button.button;
		break;
	case PLAT_EVENT_MOUSE_MOTION:
		dst->type		   = BAPI_EVENT_MOUSE_MOTION;
		dst->data.motion.x = src->data.motion.x;
		dst->data.motion.y = src->data.motion.y;
		break;
	default:
		dst->type = BAPI_EVENT_UNKNOWN;
		break;
	}
}

plat_renderer_t bapi_internal_get_renderer(void)
{
	return bapi_runtime_renderer();
}

bapi_window_t bapi_engine_get_window(void)
{
	bapi_window_t win = malloc(sizeof(struct bapi_window_internal));
	if (win) {
		win->plat_window = bapi_runtime_window();
	}
	return win;
}

bapi_renderer_t bapi_engine_get_renderer(void)
{
	bapi_renderer_t rend = malloc(sizeof(struct bapi_renderer_internal));
	if (rend) {
		rend->plat_renderer = bapi_runtime_renderer();
	}
	return rend;
}

int bapi_engine_init(const char *title, int width, int height)
{
#ifdef USE_BACKEND_XJ380
	const plat_interface_t *platform = plat_xj380_interface();
#else
	const plat_interface_t *platform = plat_sdl3_interface();
#endif
	return bapi_runtime_start(platform, title, width, height);
}

void bapi_engine_quit(void)
{
	bapi_runtime_stop();
	bapi_log_shutdown();
}

int bapi_poll_event(bapi_event_t *event)
{
	const plat_interface_t *plat = plat_get();
	if (event == NULL) {
		return plat->window.poll_event(NULL);
	}
	plat_event_t plat_event;
	int			 has_event = plat->window.poll_event(&plat_event);
	if (has_event) {
		bapi_event_from_platform(event, &plat_event);
	}
	return has_event;
}

int bapi_event_get_type(const bapi_event_t *event)
{
	return event->type;
}

uint8_t bapi_event_get_key_code(const bapi_event_t *event)
{
	return (uint8_t)event->data.key.key;
}

int bapi_event_get_mouse_x(const bapi_event_t *event)
{
	return (int)event->data.button.x;
}

int bapi_event_get_mouse_y(const bapi_event_t *event)
{
	return (int)event->data.button.y;
}

int bapi_event_get_mouse_button(const bapi_event_t *event)
{
	return event->data.button.button;
}

int bapi_event_get_motion_x(const bapi_event_t *event)
{
	return (int)event->data.motion.x;
}

int bapi_event_get_motion_y(const bapi_event_t *event)
{
	return (int)event->data.motion.y;
}

int bapi_event_is_mouse_button_down(const bapi_event_t *event)
{
	return event->type == BAPI_EVENT_MOUSE_BUTTON_DOWN;
}

int bapi_event_is_mouse_button_up(const bapi_event_t *event)
{
	return event->type == BAPI_EVENT_MOUSE_BUTTON_UP;
}

int bapi_event_is_mouse_motion(const bapi_event_t *event)
{
	return event->type == BAPI_EVENT_MOUSE_MOTION;
}

void bapi_render_clear(void)
{
	const plat_interface_t *plat	 = plat_get();
	plat_renderer_t			renderer = bapi_internal_get_renderer();
	plat->renderer.set_render_draw_color(renderer, 0, 0, 0, 255);
	plat->renderer.render_clear(renderer);
}

void bapi_render_present(void)
{
	const plat_interface_t *plat	 = plat_get();
	plat_renderer_t			renderer = bapi_internal_get_renderer();
	plat->renderer.render_present(renderer);
}

void bapi_set_render_color(bapi_color_t color)
{
	const plat_interface_t *plat	 = plat_get();
	plat_renderer_t			renderer = bapi_internal_get_renderer();
	plat->renderer.set_render_draw_color(renderer, color.r, color.g, color.b, color.a);
}

void bapi_delay(uint32_t ms)
{
	const plat_interface_t *plat = plat_get();
	plat->core.delay(ms);
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
	const plat_interface_t *plat = plat_get();
	return plat->core.get_ticks();
}

void bapi_draw_pixel(float x, float y, bapi_color_t color)
{
	const plat_interface_t *plat	 = plat_get();
	plat_renderer_t			renderer = bapi_internal_get_renderer();
	plat->renderer.set_render_draw_color(renderer, color.r, color.g, color.b, color.a);
	plat->renderer.render_point(renderer, x, y);
}

void bapi_draw_line(float x1, float y1, float x2, float y2, bapi_color_t color)
{
	const plat_interface_t *plat	 = plat_get();
	plat_renderer_t			renderer = bapi_internal_get_renderer();
	plat->renderer.set_render_draw_color(renderer, color.r, color.g, color.b, color.a);
	plat->renderer.render_line(renderer, x1, y1, x2, y2);
}

void bapi_draw_rect(float x, float y, float w, float h, bapi_color_t color)
{
	const plat_interface_t *plat	 = plat_get();
	plat_renderer_t			renderer = bapi_internal_get_renderer();
	plat->renderer.set_render_draw_color(renderer, color.r, color.g, color.b, color.a);
	plat->renderer.render_rect(renderer, x, y, w, h);
}

void bapi_fill_rect(float x, float y, float w, float h, bapi_color_t color)
{
	const plat_interface_t *plat	 = plat_get();
	plat_renderer_t			renderer = bapi_internal_get_renderer();
	plat->renderer.set_render_draw_color(renderer, color.r, color.g, color.b, color.a);
	plat->renderer.render_fill_rect(renderer, x, y, w, h);
}

void bapi_draw_triangle(float x1, float y1, float x2, float y2, float x3, float y3,
						bapi_color_t color)
{
	const plat_interface_t *plat	 = plat_get();
	plat_renderer_t			renderer = bapi_internal_get_renderer();
	plat->renderer.set_render_draw_color(renderer, color.r, color.g, color.b, color.a);
	plat->renderer.render_line(renderer, x1, y1, x2, y2);
	plat->renderer.render_line(renderer, x2, y2, x3, y3);
	plat->renderer.render_line(renderer, x3, y3, x1, y1);
}

void bapi_draw_circle(float cx, float cy, float radius, bapi_color_t color)
{
	const plat_interface_t *plat	 = plat_get();
	plat_renderer_t			renderer = bapi_internal_get_renderer();
	plat->renderer.set_render_draw_color(renderer, color.r, color.g, color.b, color.a);
	const int	segments   = 64;
	const float angle_step = 2.0f * (float)BAPI_PI / (float)segments;

	for (int i = 0; i < segments; i++) {
		float angle1 = (float)i * angle_step;
		float angle2 = (float)(i + 1) * angle_step;

		float x1 = cx + cosf(angle1) * radius;
		float y1 = cy + sinf(angle1) * radius;
		float x2 = cx + cosf(angle2) * radius;
		float y2 = cy + sinf(angle2) * radius;

		plat->renderer.render_line(renderer, x1, y1, x2, y2);
	}
}

void bapi_fill_circle(float cx, float cy, float radius, bapi_color_t color)
{
	const plat_interface_t *plat	 = plat_get();
	plat_renderer_t			renderer = bapi_internal_get_renderer();
	plat->renderer.set_render_draw_color(renderer, color.r, color.g, color.b, color.a);
	const int	segments   = 64;
	const float angle_step = 2.0f * (float)BAPI_PI / (float)segments;

	for (int i = 0; i < segments; i++) {
		float angle1 = (float)i * angle_step;
		float angle2 = (float)(i + 1) * angle_step;

		float x1 = cx + cosf(angle1) * radius;
		float y1 = cy + sinf(angle1) * radius;
		float x2 = cx + cosf(angle2) * radius;
		float y2 = cy + sinf(angle2) * radius;

		plat->renderer.render_line(renderer, cx, cy, x1, y1);
		plat->renderer.render_line(renderer, x1, y1, x2, y2);
		plat->renderer.render_line(renderer, x2, y2, cx, cy);
	}
}

void bapi_draw_polygon(float cx, float cy, float radius, int sides, bapi_color_t color)
{
	if (sides < 3) {
		return;
	}

	const plat_interface_t *plat	 = plat_get();
	plat_renderer_t			renderer = bapi_internal_get_renderer();
	plat->renderer.set_render_draw_color(renderer, color.r, color.g, color.b, color.a);
	float angle_step = 2.0f * (float)BAPI_PI / (float)sides;

	for (int i = 0; i < sides; i++) {
		float angle1 = (float)i * angle_step - (float)BAPI_PI / 2;
		float angle2 = (float)(i + 1) * angle_step - (float)BAPI_PI / 2;

		float x1 = cx + cosf(angle1) * radius;
		float y1 = cy + sinf(angle1) * radius;
		float x2 = cx + cosf(angle2) * radius;
		float y2 = cy + sinf(angle2) * radius;

		plat->renderer.render_line(renderer, x1, y1, x2, y2);
	}
}

void bapi_fill_polygon(float cx, float cy, float radius, int sides, bapi_color_t color)
{
	if (sides < 3) {
		return;
	}

	const plat_interface_t *plat	 = plat_get();
	plat_renderer_t			renderer = bapi_internal_get_renderer();
	plat->renderer.set_render_draw_color(renderer, color.r, color.g, color.b, color.a);
	float angle_step = 2.0f * (float)BAPI_PI / (float)sides;

	for (int i = 0; i < sides; i++) {
		float angle1 = (float)i * angle_step - (float)BAPI_PI / 2;
		float angle2 = (float)(i + 1) * angle_step - (float)BAPI_PI / 2;

		float x1 = cx + cosf(angle1) * radius;
		float y1 = cy + sinf(angle1) * radius;
		float x2 = cx + cosf(angle2) * radius;
		float y2 = cy + sinf(angle2) * radius;

		plat->renderer.render_line(renderer, cx, cy, x1, y1);
		plat->renderer.render_line(renderer, x1, y1, x2, y2);
		plat->renderer.render_line(renderer, x2, y2, cx, cy);
	}
}

void bapi_draw_image(const char *filepath, float x, float y, float w, float h)
{
	const plat_interface_t *plat	 = plat_get();
	plat_renderer_t			renderer = bapi_internal_get_renderer();

	plat_surface_t *surface = plat->texture.load_image(filepath);
	if (surface == NULL) {
		return;
	}

	bapi_texture_t texture = malloc(sizeof(struct bapi_texture_internal));
	if (texture == NULL) {
		plat->texture.destroy_surface(surface);
		return;
	}

	texture->platform_texture = plat->texture.create_texture_from_surface(renderer, surface);
	plat->texture.destroy_surface(surface);

	if (texture->platform_texture == NULL) {
		free(texture);
		return;
	}

	plat->renderer.render_texture(renderer, texture->platform_texture, x, y, w, h);

	plat->texture.destroy_texture(texture->platform_texture);
	free(texture);
}
