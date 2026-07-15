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

struct plat_audio_device {
	int unused;
};

static struct plat_window		fake_window;
static struct plat_renderer		fake_renderer;
static struct plat_audio_device	fake_audio_device;
static uint32_t					init_flags;
static int						open_audio_count;
static int						poll_count;
static int						draw_count;
static int						warning_count;

static const plat_interface_t desktop_platform;

void bapi_texture_cleanup(void) {}
void bapi_text_cleanup(void) {}
void bapi_log_shutdown(void) {}

const plat_interface_t *plat_sdl3_interface(void)
{
	return &desktop_platform;
}

static int core_init(uint32_t flags)
{
	init_flags = flags;
	return 0;
}

static void core_quit(void) {}

static void log_warn(const char *fmt, ...)
{
	(void)fmt;
	warning_count++;
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

static void set_render_draw_color(plat_renderer_t renderer, uint8_t red, uint8_t green,
								  uint8_t blue, uint8_t alpha)
{
	(void)renderer;
	(void)red;
	(void)green;
	(void)blue;
	(void)alpha;
	draw_count++;
}

static void render_noop(plat_renderer_t renderer)
{
	(void)renderer;
	draw_count++;
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

static plat_audio_device_t open_audio_device(plat_audio_format_t format, int channels, int freq)
{
	(void)format;
	(void)channels;
	(void)freq;
	open_audio_count++;
	return &fake_audio_device;
}

static void close_audio_device(plat_audio_device_t device)
{
	(void)device;
}

static int expect(int condition, const char *message)
{
	if (!condition) fprintf(stderr, "%s\n", message);
	return condition;
}

static const plat_interface_t desktop_platform = {
	.capabilities = PLAT_CAPABILITY_AUDIO | PLAT_CAPABILITY_VIDEO,
	.core = {.init = core_init, .quit = core_quit, .log_warn = log_warn},
	.window = {.create_window = create_window, .destroy_window = destroy_window, .poll_event = poll_event},
	.renderer = {
		.create_renderer = create_renderer,
		.destroy_renderer = destroy_renderer,
		.set_render_draw_color = set_render_draw_color,
		.set_render_draw_blend_mode = set_render_draw_blend_mode,
		.render_clear = render_noop,
		.render_present = render_noop,
	},
	.text = {.init_ttf = init_ttf, .quit_ttf = quit_ttf},
	.audio = {.open_audio_device = open_audio_device, .close_audio_device = close_audio_device},
};

static const plat_interface_t xj380_platform = {
	.core = {.init = core_init, .quit = core_quit, .log_warn = log_warn},
	.window = {.create_window = create_window, .destroy_window = destroy_window, .poll_event = poll_event},
	.renderer = {
		.create_renderer = create_renderer,
		.destroy_renderer = destroy_renderer,
		.set_render_draw_color = set_render_draw_color,
		.set_render_draw_blend_mode = set_render_draw_blend_mode,
		.render_clear = render_noop,
		.render_present = render_noop,
	},
	.text = {.init_ttf = init_ttf, .quit_ttf = quit_ttf},
};

int main(void)
{
	bapi_event_t event = {.type = BAPI_EVENT_UNKNOWN};
	int result = expect(bapi_runtime_start(&desktop_platform, "desktop", 1, 1) == 0,
						"desktop profile starts") &&
				expect(plat_supports(PLAT_CAPABILITY_AUDIO) && plat_supports(PLAT_CAPABILITY_VIDEO),
						"desktop profile exposes media capabilities") &&
				expect(init_flags == (PLAT_INIT_VIDEO | PLAT_INIT_AUDIO),
						"desktop profile requests audio initialization") &&
				expect(bapi_audio_init() == 0 && open_audio_count == 1,
						"desktop profile keeps audio available");
	bapi_runtime_stop();

	init_flags = 0;
	poll_count = 0;
	draw_count = 0;
	warning_count = 0;
	result = result && expect(bapi_runtime_start(&xj380_platform, "xj380", 1, 1) == 0,
						"XJ380 profile starts") &&
				expect(!plat_supports(PLAT_CAPABILITY_AUDIO) && !plat_supports(PLAT_CAPABILITY_VIDEO),
						"XJ380 profile omits media capabilities") &&
				expect(plat_get()->audio.open_audio_device == NULL && plat_get()->audio.load_wav == NULL,
						"XJ380 profile leaves audio vtable slots null") &&
				expect(init_flags == PLAT_INIT_VIDEO, "XJ380 profile does not request audio initialization");
	bapi_poll_event(&event);
	bapi_render_clear();
	bapi_render_present();
	result = result && expect(poll_count == 1 && draw_count > 0,
						"XJ380 profile still polls and draws") &&
				expect(bapi_audio_init() != 0 && bapi_sound_load("sound.wav") == NULL &&
						   open_audio_count == 1,
						"unsupported audio fails without an audio vtable") &&
				expect(bapi_video_init() != 0 && bapi_video_load("video.mp4") == NULL,
						"unsupported video fails explicitly") &&
				expect(warning_count == 2, "unsupported media logs one warning per media type");
	bapi_sound_stop(NULL);
	bapi_sound_free(NULL);
	bapi_video_pause(NULL);
	bapi_video_stop(NULL);
	bapi_video_render(NULL, 0, 0, 0, 0);
	bapi_runtime_stop();
	return result ? 0 : 1;
}
