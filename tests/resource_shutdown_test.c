#include "BridgeEngine.h"
#include "internal/bapi_internal.h"
#include "internal/engine/render_context.h"
#include "internal/platform/platform.h"
#include <stdio.h>
#include <stdlib.h>

struct plat_window {
	int unused;
};

struct plat_renderer {
	int unused;
};

struct plat_texture {
	int unused;
};

struct plat_surface {
	int unused;
};

struct plat_audio_device {
	int unused;
};

struct plat_audio_stream {
	int unused;
};

static struct plat_window		fake_window;
static struct plat_renderer		fake_renderer;
static struct plat_audio_device fake_audio_device;
static int						destroyed_texture_count;
static int						destroyed_stream_count;
static int						freed_sound_buffer_count;
static int						closed_audio_device_count;
static int						unsupported_video_warning_count;

plat_renderer_t bapi_internal_get_renderer(void)
{
	return &fake_renderer;
}

void bapi_text_cleanup(void) {}

static int core_init(uint32_t flags)
{
	(void)flags;
	return 0;
}

static void core_quit(void) {}

static void log_warn(const char *fmt, ...)
{
	(void)fmt;
	unsupported_video_warning_count++;
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
}

static void render_texture(plat_renderer_t renderer, plat_texture_t texture, float x, float y,
						   float width, float height)
{
	(void)renderer;
	(void)texture;
	(void)x;
	(void)y;
	(void)width;
	(void)height;
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

static plat_surface_t *load_image(const char *filepath)
{
	return filepath != NULL ? malloc(sizeof(struct plat_surface)) : NULL;
}

static void destroy_surface(plat_surface_t *surface)
{
	free(surface);
}

static plat_texture_t create_texture_from_surface(plat_renderer_t renderer, plat_surface_t *surface)
{
	if (renderer == NULL || surface == NULL) return NULL;
	return malloc(sizeof(struct plat_texture));
}

static int get_texture_size(plat_texture_t texture, int *width, int *height)
{
	if (texture == NULL || width == NULL || height == NULL) return 1;
	*width	= 1;
	*height = 1;
	return 0;
}

static void destroy_texture(plat_texture_t texture)
{
	if (texture != NULL) destroyed_texture_count++;
	free(texture);
}

static plat_audio_device_t open_audio_device(plat_audio_format_t format, int channels, int freq)
{
	(void)format;
	(void)channels;
	(void)freq;
	return &fake_audio_device;
}

static void close_audio_device(plat_audio_device_t device)
{
	if (device != NULL) closed_audio_device_count++;
}

static plat_audio_stream_t create_audio_stream(const plat_audio_spec_t *spec)
{
	(void)spec;
	return malloc(sizeof(struct plat_audio_stream));
}

static plat_audio_stream_t open_audio_device_stream(plat_audio_format_t format, int channels,
													int freq)
{
	(void)format;
	(void)channels;
	(void)freq;
	return malloc(sizeof(struct plat_audio_stream));
}

static int bind_audio_stream(plat_audio_device_t device, plat_audio_stream_t stream)
{
	(void)device;
	(void)stream;
	return 0;
}

static int set_audio_stream_gain(plat_audio_stream_t stream, float gain)
{
	(void)stream;
	(void)gain;
	return 0;
}

static void resume_audio_stream_device(plat_audio_stream_t stream)
{
	(void)stream;
}

static void destroy_audio_stream(plat_audio_stream_t stream)
{
	if (stream != NULL) destroyed_stream_count++;
	free(stream);
}

static int load_wav(const char *filepath, plat_audio_spec_t *spec, uint8_t **buffer,
					uint32_t *length)
{
	if (filepath == NULL || spec == NULL || buffer == NULL || length == NULL) return 1;
	*buffer = malloc(1);
	if (*buffer == NULL) return 1;
	spec->channels = 2;
	spec->freq	   = 44100;
	*length		   = 1;
	return 0;
}

static void mem_free(void *buffer)
{
	if (buffer != NULL) freed_sound_buffer_count++;
	free(buffer);
}

static int expect(int condition, const char *message)
{
	if (!condition) fprintf(stderr, "%s\n", message);
	return condition;
}

static const plat_interface_t test_platform = {
	.capabilities = PLAT_CAPABILITY_AUDIO | PLAT_CAPABILITY_VIDEO,
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
			.render_texture				= render_texture,
		},
	.texture =
		{
			.create_texture_from_surface = create_texture_from_surface,
			.get_texture_size			 = get_texture_size,
			.destroy_texture			 = destroy_texture,
			.load_image					 = load_image,
			.destroy_surface			 = destroy_surface,
		},
	.text = {.init_ttf = init_ttf, .quit_ttf = quit_ttf},
	.audio =
		{
			.open_audio_device			= open_audio_device,
			.close_audio_device			= close_audio_device,
			.create_audio_stream		= create_audio_stream,
			.open_audio_device_stream	= open_audio_device_stream,
			.bind_audio_stream			= bind_audio_stream,
			.destroy_audio_stream		= destroy_audio_stream,
			.set_audio_stream_gain		= set_audio_stream_gain,
			.resume_audio_stream_device = resume_audio_stream_device,
			.load_wav					= load_wav,
			.mem_free					= mem_free,
		},
};

static const plat_interface_t unsupported_video_platform = {
	.core = {.log_warn = log_warn},
};

int main(int argc, char **argv)
{
	if (argc != 2) return 1;
	if (bapi_runtime_start(&test_platform, "test", 1, 1) != 0) return 1;

	bapi_texture_t texture = bapi_texture_load("texture");
	if (bapi_audio_init() != 0) return 1;
	bapi_sound_t sound	= bapi_sound_load("sound");
	bapi_video_t video	= bapi_video_load(argv[1]);
	int			 result = expect(texture != NULL && sound != NULL && video != NULL,
								 "resources load through their production modules");

	bapi_runtime_stop();
	result = result && expect(destroyed_texture_count == 1, "shutdown destroys tracked textures") &&
			   expect(destroyed_stream_count == 1, "shutdown destroys tracked video streams") &&
			   expect(freed_sound_buffer_count == 1,
						  "shutdown releases tracked sound buffers") &&
			   expect(closed_audio_device_count == 1, "shutdown closes the audio device");
	if (plat_init(&unsupported_video_platform) != 0) return 1;
	result = result && expect(bapi_video_init() != 0 && bapi_video_load(argv[1]) == NULL,
						"video capability failure is explicit in the FFmpeg implementation") &&
				expect(unsupported_video_warning_count == 1,
						"FFmpeg implementation logs unsupported video once");
	plat_shutdown();
	return result ? 0 : 1;
}
