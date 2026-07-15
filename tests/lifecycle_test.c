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

static int fail_core_init;
static int fail_window_create;
static int fail_renderer_create;
static int fail_ttf_init;
static int core_quit_count;
static int window_destroy_count;
static int renderer_destroy_count;
static int renderer_destroy_order;
static int window_destroy_order;
static int core_quit_order;
static int ttf_quit_count;
static int text_cleanup_count;
static int video_cleanup_count;
static int audio_cleanup_count;
static int texture_cleanup_count;
static int video_cleanup_order;
static int audio_cleanup_order;
static int texture_cleanup_order;
static int cleanup_order;
static int ttf_quit_order;
static int lifecycle_order;

void bapi_text_cleanup(void)
{
	text_cleanup_count++;
	cleanup_order = ++lifecycle_order;
}

void bapi_video_cleanup(void)
{
	video_cleanup_count++;
	video_cleanup_order = ++lifecycle_order;
}

void bapi_audio_cleanup(void)
{
	audio_cleanup_count++;
	audio_cleanup_order = ++lifecycle_order;
}

void bapi_texture_cleanup(void)
{
	texture_cleanup_count++;
	texture_cleanup_order = ++lifecycle_order;
}

static int core_init(uint32_t flags)
{
	(void)flags;
	return fail_core_init;
}

static void core_quit(void)
{
	core_quit_count++;
	core_quit_order = ++lifecycle_order;
}

static plat_window_t create_window(const char *title, int width, int height)
{
	(void)title;
	(void)width;
	(void)height;
	return fail_window_create ? NULL : &fake_window;
}

static void destroy_window(plat_window_t window)
{
	if (window) {
		window_destroy_count++;
		window_destroy_order = ++lifecycle_order;
	}
}

static plat_renderer_t create_renderer(plat_window_t window)
{
	return fail_renderer_create || !window ? NULL : &fake_renderer;
}

static void destroy_renderer(plat_renderer_t renderer)
{
	if (renderer) {
		renderer_destroy_count++;
		renderer_destroy_order = ++lifecycle_order;
	}
}

static void set_render_draw_color(plat_renderer_t renderer, uint8_t red, uint8_t green,
								  uint8_t blue, uint8_t alpha)
{
	(void)renderer;
	(void)red;
	(void)green;
	(void)blue;
	(void)alpha;
}

static void render_noop(plat_renderer_t renderer)
{
	(void)renderer;
}

static void set_render_draw_blend_mode(plat_renderer_t renderer, plat_blend_mode_t mode)
{
	(void)renderer;
	(void)mode;
}

static int init_ttf(void)
{
	return fail_ttf_init;
}

static void quit_ttf(void)
{
	ttf_quit_count++;
	ttf_quit_order = ++lifecycle_order;
}

static void reset_state(void)
{
	fail_core_init		   = 0;
	fail_window_create	   = 0;
	fail_renderer_create   = 0;
	fail_ttf_init		   = 0;
	core_quit_count		   = 0;
	window_destroy_count   = 0;
	renderer_destroy_count = 0;
	renderer_destroy_order = 0;
	window_destroy_order   = 0;
	core_quit_order		   = 0;
	ttf_quit_count		   = 0;
	text_cleanup_count	   = 0;
	video_cleanup_count	  = 0;
	audio_cleanup_count	  = 0;
	texture_cleanup_count  = 0;
	video_cleanup_order	  = 0;
	audio_cleanup_order	  = 0;
	texture_cleanup_order  = 0;
	cleanup_order		   = 0;
	ttf_quit_order		   = 0;
	lifecycle_order		   = 0;
}

static int expect(int condition, const char *message)
{
	if (!condition) fprintf(stderr, "%s\n", message);
	return condition;
}

static const plat_interface_t test_platform = {
	.core	= {.init = core_init, .quit = core_quit},
	.window = {.create_window = create_window, .destroy_window = destroy_window},
	.renderer =
		{
			.create_renderer			= create_renderer,
			.destroy_renderer			= destroy_renderer,
			.set_render_draw_color		= set_render_draw_color,
			.set_render_draw_blend_mode = set_render_draw_blend_mode,
			.render_clear				= render_noop,
			.render_present				= render_noop,
		},
	.text = {.init_ttf = init_ttf, .quit_ttf = quit_ttf},
};

static int test_failure_cleanup(void)
{
	int result = 1;

	reset_state();
	fail_core_init = 1;
	result = result &&
			 expect(bapi_runtime_start(&test_platform, "test", 1, 1) != 0,
					"core initialization fails") &&
			 expect(core_quit_count == 0 && plat_get() == NULL, "core failure releases platform");

	reset_state();
	fail_window_create = 1;
	result =
		result &&
		expect(bapi_runtime_start(&test_platform, "test", 1, 1) != 0, "window creation fails") &&
		expect(core_quit_count == 1 && window_destroy_count == 0 && plat_get() == NULL,
			   "window failure releases core");

	reset_state();
	fail_renderer_create = 1;
	result =
		result &&
		expect(bapi_runtime_start(&test_platform, "test", 1, 1) != 0, "renderer creation fails") &&
		expect(core_quit_count == 1 && window_destroy_count == 1 && renderer_destroy_count == 0 &&
				   plat_get() == NULL,
			   "renderer failure releases window and core");

	reset_state();
	fail_ttf_init = 1;
	result =
		result &&
		expect(bapi_runtime_start(&test_platform, "test", 1, 1) != 0, "TTF initialization fails") &&
		expect(core_quit_count == 1 && window_destroy_count == 1 && renderer_destroy_count == 1 &&
				   ttf_quit_count == 0 && plat_get() == NULL,
			   "TTF failure releases renderer, window, and core");

	return result;
}

static int test_successful_stop(void)
{
	reset_state();
	int result = expect(bapi_runtime_start(&test_platform, "test", 1, 1) == 0, "runtime starts") &&
				 expect(bapi_runtime_is_initialized(), "runtime reports initialized");
	bapi_runtime_stop();
	result =
		result &&
		expect(video_cleanup_count == 1 && audio_cleanup_count == 1 && texture_cleanup_count == 1 &&
				   video_cleanup_order < audio_cleanup_order &&
				   audio_cleanup_order < texture_cleanup_order && texture_cleanup_order < cleanup_order,
			   "stop cleans media resources before text") &&
		expect(text_cleanup_count == 1 && ttf_quit_count == 1 && cleanup_order < ttf_quit_order,
			   "stop cleans text before closing TTF") &&
		expect(renderer_destroy_count == 1 && window_destroy_count == 1 && core_quit_count == 1,
			   "stop releases renderer, window, and core") &&
		expect(ttf_quit_order < renderer_destroy_order &&
				   renderer_destroy_order < window_destroy_order &&
				   window_destroy_order < core_quit_order,
			   "stop releases backend resources in dependency order") &&
		expect(!bapi_runtime_is_initialized() && plat_get() == NULL, "stop resets runtime");
	bapi_runtime_stop();
	return result && expect(text_cleanup_count == 1, "repeated stop is idempotent");
}

int main(void)
{
	return test_failure_cleanup() && test_successful_stop() ? 0 : 1;
}
