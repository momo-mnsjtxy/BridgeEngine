#include "BridgeEngine.h"
#include "internal/engine/render_context.h"
#include "internal/platform/platform.h"
#include <stdio.h>

struct plat_window {
	int unused;
};

struct plat_renderer {
	int unused;
};

static struct plat_window	fake_window;
static struct plat_renderer fake_renderer;
static int					poll_count;
static int					render_count;
static int					delay_count;
static int					ticks_count;

static const plat_interface_t test_platform;

void bapi_video_cleanup(void) {}
void bapi_audio_cleanup(void) {}
void bapi_texture_cleanup(void) {}
void bapi_text_cleanup(void) {}
void bapi_log_shutdown(void) {}

const plat_interface_t *plat_sdl3_interface(void)
{
	return &test_platform;
}

static int core_init(uint32_t flags)
{
	(void)flags;
	return 0;
}

static void core_quit(void) {}

static void delay(uint32_t ms)
{
	(void)ms;
	delay_count++;
}

static uint32_t get_ticks(void)
{
	ticks_count++;
	return 42;
}

static plat_window_t create_window(const char *title, int width, int height)
{
	(void)title;
	(void)width;
	(void)height;
	return &fake_window;
}

static void destroy_window(plat_window_t window)
{
	(void)window;
}

static int poll_event(plat_event_t *event)
{
	(void)event;
	poll_count++;
	return 0;
}

static plat_renderer_t create_renderer(plat_window_t window)
{
	return window != NULL ? &fake_renderer : NULL;
}

static void destroy_renderer(plat_renderer_t renderer)
{
	(void)renderer;
}

static void render_noop(plat_renderer_t renderer)
{
	(void)renderer;
	render_count++;
}

static void set_render_draw_color(plat_renderer_t renderer, uint8_t red, uint8_t green,
								  uint8_t blue, uint8_t alpha)
{
	(void)renderer;
	(void)red;
	(void)green;
	(void)blue;
	(void)alpha;
	render_count++;
}

static void render_point(plat_renderer_t renderer, float x, float y)
{
	(void)renderer;
	(void)x;
	(void)y;
	render_count++;
}

static void set_render_draw_blend_mode(plat_renderer_t renderer, plat_blend_mode_t mode)
{
	(void)renderer;
	(void)mode;
}

static int init_ttf(void)
{
	return 0;
}

static void quit_ttf(void) {}

static int expect(int condition, const char *message)
{
	if (!condition) fprintf(stderr, "%s\n", message);
	return condition;
}

static const plat_interface_t test_platform = {
	.core = {.init = core_init, .quit = core_quit, .delay = delay, .get_ticks = get_ticks},
	.window = {.create_window = create_window, .destroy_window = destroy_window, .poll_event = poll_event},
	.renderer = {
		.create_renderer = create_renderer,
		.destroy_renderer = destroy_renderer,
		.set_render_draw_color = set_render_draw_color,
		.set_render_draw_blend_mode = set_render_draw_blend_mode,
		.render_clear = render_noop,
		.render_present = render_noop,
		.render_point = render_point,
	},
	.text = {.init_ttf = init_ttf, .quit_ttf = quit_ttf},
};

static void exercise_public_runtime_apis(bapi_event_t *event)
{
	bapi_poll_event(event);
	bapi_render_clear();
	bapi_render_present();
	bapi_set_render_color(bapi_color(1, 2, 3, 4));
	bapi_draw_pixel(1, 2, bapi_color(1, 2, 3, 4));
	bapi_delay(1);
	(void)bapi_get_ticks();
}

int main(void)
{
	bapi_event_t event = {.type = BAPI_EVENT_QUIT};
	exercise_public_runtime_apis(&event);
	int result = expect(event.type == BAPI_EVENT_UNKNOWN, "pre-init polling returns unknown event") &&
				 expect(poll_count == 0 && render_count == 0 && delay_count == 0 && ticks_count == 0,
						"pre-init APIs do not call the backend") &&
				 expect(bapi_engine_get_window() == NULL && bapi_engine_get_renderer() == NULL,
						"pre-init handles are null");

	result = result && expect(bapi_engine_init("test", 1, 1) == 0,
						"runtime starts") &&
				 expect(bapi_engine_get_window() == (bapi_window_t)&fake_window &&
							bapi_engine_get_renderer() == (bapi_renderer_t)&fake_renderer,
						"runtime handles are borrowed platform handles");

	exercise_public_runtime_apis(&event);
	result = result && expect(poll_count == 1 && render_count > 0 && delay_count == 1 && ticks_count == 1,
						"active APIs call the backend");
	bapi_engine_quit();

	int poll_before = poll_count;
	int render_before = render_count;
	int delay_before = delay_count;
	int ticks_before = ticks_count;
	event.type = BAPI_EVENT_QUIT;
	exercise_public_runtime_apis(&event);
	return result && expect(event.type == BAPI_EVENT_UNKNOWN, "post-quit polling returns unknown event") &&
			   expect(poll_count == poll_before && render_count == render_before &&
						  delay_count == delay_before && ticks_count == ticks_before,
					  "post-quit APIs do not call the backend") &&
			   expect(bapi_engine_get_window() == NULL && bapi_engine_get_renderer() == NULL,
					  "post-quit handles are null")
			   ? 0
			   : 1;
}
