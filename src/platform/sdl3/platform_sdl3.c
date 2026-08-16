#include "internal/platform/platform.h"
#include "internal/platform/platform_types.h"
#include "internal/platform/sdl_keycode.h"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct plat_window {
	SDL_Window *window;
};

struct plat_renderer {
	SDL_Renderer *renderer;
};

struct plat_texture {
	SDL_Texture *texture;
};

struct plat_font {
	TTF_Font *font;
	float	  size;
};

struct plat_audio_device {
	SDL_AudioDeviceID device;
};

struct plat_audio_stream {
	SDL_AudioStream *stream;
};

struct plat_mutex {
	SDL_Mutex *mutex;
};

struct plat_io {
	SDL_IOStream *stream;
};

struct plat_surface {
	SDL_Surface *surface;
};

#if 0
static uint8_t sdlkeycode_convert_table[0x80] = {
	' ', ' ', ' ',	' ', ' ', ' ', ' ', ' ', '\b', PLAT_KEY_TAB, ' ', ' ', ' ',			 ' ', ' ',
	' ', ' ', ' ',	' ', ' ', ' ', ' ', ' ', ' ',  ' ',			 ' ', ' ', PLAT_KEY_ESC, ' ', ' ',
	' ', ' ', ' ',	'!', '"', '#', '$', '%', '&',  '\'',		 '(', ')', '*',			 '+', ',',
	'-', '.', '/',	'0', '1', '2', '3', '4', '5',  '6',			 '7', '8', '9',			 ':', ';',
	'<', '=', '>',	'?', '@', ' ', ' ', ' ', ' ',  ' ',			 ' ', ' ', ' ',			 ' ', ' ',
	' ', ' ', ' ',	' ', ' ', ' ', ' ', ' ', ' ',  ' ',			 ' ', ' ', ' ',			 ' ', ' ',
	' ', '[', '\\', ']', '^', '_', '`', 'a', 'b',  'c',			 'd', 'e', 'f',			 'g', 'h',
	'i', 'j', 'k',	'l', 'm', 'n', 'o', 'p', 'q',  'r',			 's', 't', 'u',			 'v', 'w',
	'x', 'y', 'z',	'{', '|', '}', '~', ' ',
};

static uint8_t sdlspkeycode_convert_table[0x90] = {
	PLAT_KEY_CAPS,
	PLAT_KEY_F1,
	PLAT_KEY_F2,
	PLAT_KEY_F3,
	PLAT_KEY_F4,
	PLAT_KEY_F5,
	PLAT_KEY_F6,
	PLAT_KEY_F7,
	PLAT_KEY_F8,
	PLAT_KEY_F9,
	PLAT_KEY_F10,
	PLAT_KEY_F11,
	PLAT_KEY_F12,
	' ',
	PLAT_KEY_SCROLL,
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	PLAT_KEY_NUML,
	'/',
	'*',
	'-',
	'+',
	'\n',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	' ',
	PLAT_KEY_CTRL,
	PLAT_KEY_SHIFT,
	PLAT_KEY_ALT,
	' ',
	PLAT_KEY_CTRL,
	PLAT_KEY_SHIFT,
	PLAT_KEY_ALT,
	' ',
	' ',
	' ',
	' ',
};
#endif

static uint8_t convert_sdl_keycode(SDL_Keycode key)
{
	return bapi_sdl_keycode_convert((uint32_t)key);
}

static int sdl3_init(uint32_t flags)
{
	uint32_t sdl_flags = 0;
	if (flags & PLAT_INIT_VIDEO) sdl_flags |= SDL_INIT_VIDEO;
	if (flags & PLAT_INIT_AUDIO) sdl_flags |= SDL_INIT_AUDIO;
	return SDL_Init(sdl_flags) ? 0 : 1;
}

static void sdl3_quit(void)
{
	SDL_Quit();
}

static plat_window_t sdl3_create_window(const char *title, int width, int height)
{
	plat_window_t w = malloc(sizeof(struct plat_window));
	if (!w) return NULL;
	w->window = SDL_CreateWindow(title, width, height, 0);
	if (!w->window) {
		free(w);
		return NULL;
	}
	return w;
}

static void sdl3_destroy_window(plat_window_t window)
{
	if (window) {
		SDL_DestroyWindow(window->window);
		free(window);
	}
}

static plat_renderer_t sdl3_create_renderer(plat_window_t window)
{
	if (!window) return NULL;
	plat_renderer_t r = malloc(sizeof(struct plat_renderer));
	if (!r) return NULL;
	r->renderer = SDL_CreateRenderer(window->window, NULL);
	if (!r->renderer) {
		free(r);
		return NULL;
	}
	return r;
}

static void sdl3_destroy_renderer(plat_renderer_t renderer)
{
	if (renderer) {
		SDL_DestroyRenderer(renderer->renderer);
		free(renderer);
	}
}

static void sdl3_set_render_draw_color(plat_renderer_t renderer, uint8_t r, uint8_t g, uint8_t b,
									   uint8_t a)
{
	if (renderer) SDL_SetRenderDrawColor(renderer->renderer, r, g, b, a);
}

static void sdl3_set_render_draw_blend_mode(plat_renderer_t renderer, plat_blend_mode_t mode)
{
	if (!renderer) return;
	(void)mode;
	SDL_SetRenderDrawBlendMode(renderer->renderer, SDL_BLENDMODE_BLEND);
}

static void sdl3_render_clear(plat_renderer_t renderer)
{
	if (renderer) SDL_RenderClear(renderer->renderer);
}

static void sdl3_render_present(plat_renderer_t renderer)
{
	if (renderer) SDL_RenderPresent(renderer->renderer);
}

static void sdl3_render_point(plat_renderer_t renderer, float x, float y)
{
	if (renderer) SDL_RenderPoint(renderer->renderer, x, y);
}

static void sdl3_render_line(plat_renderer_t renderer, float x1, float y1, float x2, float y2)
{
	if (renderer) SDL_RenderLine(renderer->renderer, x1, y1, x2, y2);
}

static void sdl3_render_rect(plat_renderer_t renderer, float x, float y, float w, float h)
{
	if (!renderer) return;
	SDL_FRect rect = {x, y, w, h};
	SDL_RenderRect(renderer->renderer, &rect);
}

static void sdl3_render_fill_rect(plat_renderer_t renderer, float x, float y, float w, float h)
{
	if (!renderer) return;
	SDL_FRect rect = {x, y, w, h};
	SDL_RenderFillRect(renderer->renderer, &rect);
}

static void sdl3_render_texture(plat_renderer_t renderer, plat_texture_t texture, float x, float y,
								float w, float h)
{
	if (!renderer || !texture) return;
	SDL_FRect dest = {x, y, w, h};
	SDL_RenderTexture(renderer->renderer, texture->texture, NULL, &dest);
}

static plat_texture_t sdl3_create_texture_from_surface(plat_renderer_t renderer,
													   plat_surface_t *surface)
{
	if (!renderer || !surface) return NULL;
	plat_texture_t t = malloc(sizeof(struct plat_texture));
	if (!t) return NULL;
	t->texture = SDL_CreateTextureFromSurface(renderer->renderer, surface->surface);
	if (!t->texture) {
		free(t);
		return NULL;
	}
	return t;
}

static plat_texture_t sdl3_create_texture(plat_renderer_t renderer, plat_pixel_format_t format,
										  plat_texture_access_t access, int width, int height)
{
	if (!renderer) return NULL;
	SDL_PixelFormat sdl_fmt = SDL_PIXELFORMAT_ARGB8888;
	if (format == PLAT_PIXELFORMAT_ARGB8888) sdl_fmt = SDL_PIXELFORMAT_ARGB8888;

	SDL_TextureAccess sdl_access = SDL_TEXTUREACCESS_STREAMING;
	if (access == PLAT_TEXTUREACCESS_STREAMING) sdl_access = SDL_TEXTUREACCESS_STREAMING;

	plat_texture_t t = malloc(sizeof(struct plat_texture));
	if (!t) return NULL;
	t->texture = SDL_CreateTexture(renderer->renderer, sdl_fmt, sdl_access, width, height);
	if (!t->texture) {
		free(t);
		return NULL;
	}
	return t;
}

static int sdl3_update_texture(plat_texture_t texture, const void *pixels, int pitch)
{
	if (!texture) return -1;
	return SDL_UpdateTexture(texture->texture, NULL, pixels, pitch) ? 0 : -1;
}

static int sdl3_get_texture_size(plat_texture_t texture, int *width, int *height)
{
	if (!texture || !width || !height) return -1;
	float texture_width	 = 0;
	float texture_height = 0;
	if (!SDL_GetTextureSize(texture->texture, &texture_width, &texture_height)) return -1;
	*width	= (int)texture_width;
	*height = (int)texture_height;
	return 0;
}

static void sdl3_destroy_texture(plat_texture_t texture)
{
	if (texture) {
		SDL_DestroyTexture(texture->texture);
		free(texture);
	}
}

static plat_surface_t *sdl3_load_image(const char *filepath)
{
	if (!filepath) return NULL;
	SDL_IOStream *io = SDL_IOFromFile(filepath, "rb");
	if (!io) return NULL;
	plat_surface_t *s = malloc(sizeof(plat_surface_t));
	if (!s) {
		SDL_CloseIO(io);
		return NULL;
	}
	s->surface = IMG_Load_IO(io, true);
	if (!s->surface) {
		free(s);
		return NULL;
	}
	return s;
}

static void sdl3_destroy_surface(plat_surface_t *surface)
{
	if (surface) {
		SDL_DestroySurface(surface->surface);
		free(surface);
	}
}

static int sdl3_poll_event(plat_event_t *event)
{
	if (!event) return SDL_PollEvent(NULL);

	SDL_Event sdl_event;
	while (SDL_PollEvent(&sdl_event)) {
		switch (sdl_event.type) {
		case SDL_EVENT_QUIT:
			event->type = PLAT_EVENT_QUIT;
			return 1;
		case SDL_EVENT_KEY_DOWN:
			event->type			= PLAT_EVENT_KEY_DOWN;
			event->data.key.key = convert_sdl_keycode(sdl_event.key.key);
			return 1;
		case SDL_EVENT_KEY_UP:
			event->type			= PLAT_EVENT_KEY_UP;
			event->data.key.key = convert_sdl_keycode(sdl_event.key.key);
			return 1;
		case SDL_EVENT_MOUSE_BUTTON_DOWN:
			event->type				  = PLAT_EVENT_MOUSE_BUTTON_DOWN;
			event->data.button.x	  = sdl_event.button.x;
			event->data.button.y	  = sdl_event.button.y;
			event->data.button.button = sdl_event.button.button;
			return 1;
		case SDL_EVENT_MOUSE_BUTTON_UP:
			event->type				  = PLAT_EVENT_MOUSE_BUTTON_UP;
			event->data.button.x	  = sdl_event.button.x;
			event->data.button.y	  = sdl_event.button.y;
			event->data.button.button = sdl_event.button.button;
			return 1;
		case SDL_EVENT_MOUSE_MOTION:
			event->type			 = PLAT_EVENT_MOUSE_MOTION;
			event->data.motion.x = sdl_event.motion.x;
			event->data.motion.y = sdl_event.motion.y;
			return 1;
		case SDL_EVENT_MOUSE_WHEEL:
			event->type				= PLAT_EVENT_MOUSE_WHEEL;
			event->data.wheel.x		= sdl_event.wheel.x;
			event->data.wheel.y		= sdl_event.wheel.y;
			event->data.wheel.mouse_x = sdl_event.wheel.mouse_x;
			event->data.wheel.mouse_y = sdl_event.wheel.mouse_y;
			return 1;
		default:

			continue;
		}
	}
	return 0;
}

static void sdl3_get_mouse_state(float *x, float *y)
{
	SDL_GetMouseState(x, y);
}

static int sdl3_init_ttf(void)
{
	return TTF_Init() ? 0 : 1;
}

static void sdl3_quit_ttf(void)
{
	TTF_Quit();
}

static plat_font_t sdl3_open_font(const char *filepath, float size)
{
	if (!filepath) return NULL;
	SDL_IOStream *io = SDL_IOFromFile(filepath, "rb");
	if (!io) return NULL;
	plat_font_t f = malloc(sizeof(struct plat_font));
	if (!f) {
		SDL_CloseIO(io);
		return NULL;
	}
	f->font = TTF_OpenFontIO(io, true, size);
	f->size = size;
	if (!f->font) {
		free(f);
		return NULL;
	}
	return f;
}

static void sdl3_close_font(plat_font_t font)
{
	if (font) {
		TTF_CloseFont(font->font);
		free(font);
	}
}

static plat_surface_t *sdl3_render_text_blended(plat_font_t font, const char *text, int len,
												uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	if (!font) return NULL;
	SDL_Color		color = {r, g, b, a};
	plat_surface_t *s	  = malloc(sizeof(plat_surface_t));
	if (!s) return NULL;
	s->surface = TTF_RenderText_Blended(font->font, text, len, color);
	if (!s->surface) {
		free(s);
		return NULL;
	}
	return s;
}

static int sdl3_get_string_size(plat_font_t font, const char *text, int len, int *w, int *h)
{
	if (!font) return -1;
	return TTF_GetStringSize(font->font, text, len, w, h) ? 0 : -1;
}

static plat_audio_device_t sdl3_open_audio_device(plat_audio_format_t format, int channels,
												  int freq)
{
	SDL_AudioSpec spec;
	SDL_zero(spec);
	if (format == PLAT_AUDIO_F32) spec.format = SDL_AUDIO_F32;
	spec.channels = channels;
	spec.freq	  = freq;

	plat_audio_device_t d = malloc(sizeof(struct plat_audio_device));
	if (!d) return NULL;
	d->device = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec);
	if (d->device == 0) {
		free(d);
		return NULL;
	}
	return d;
}

static void sdl3_close_audio_device(plat_audio_device_t device)
{
	if (device) {
		SDL_CloseAudioDevice(device->device);
		free(device);
	}
}

static plat_audio_stream_t sdl3_create_audio_stream(const plat_audio_spec_t *spec)
{
	if (!spec) return NULL;
	SDL_AudioSpec sdl_spec;
	SDL_zero(sdl_spec);
	if (spec->format == PLAT_AUDIO_F32) sdl_spec.format = SDL_AUDIO_F32;
	sdl_spec.channels = spec->channels;
	sdl_spec.freq	  = spec->freq;

	plat_audio_stream_t s = malloc(sizeof(struct plat_audio_stream));
	if (!s) return NULL;
	s->stream = SDL_CreateAudioStream(&sdl_spec, NULL);
	if (!s->stream) {
		free(s);
		return NULL;
	}
	return s;
}

static plat_audio_stream_t sdl3_open_audio_device_stream(plat_audio_format_t format, int channels,
														 int freq)
{
	SDL_AudioSpec spec;
	SDL_zero(spec);
	if (format == PLAT_AUDIO_F32) spec.format = SDL_AUDIO_F32;
	spec.channels = channels;
	spec.freq	  = freq;

	plat_audio_stream_t s = malloc(sizeof(struct plat_audio_stream));
	if (!s) return NULL;
	s->stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
	if (!s->stream) {
		free(s);
		return NULL;
	}
	return s;
}

static int sdl3_bind_audio_stream(plat_audio_device_t device, plat_audio_stream_t stream)
{
	if (!device || !stream) return -1;
	return SDL_BindAudioStream(device->device, stream->stream) ? 0 : -1;
}

static void sdl3_destroy_audio_stream(plat_audio_stream_t stream)
{
	if (stream) {
		SDL_DestroyAudioStream(stream->stream);
		free(stream);
	}
}

static int sdl3_put_audio_stream_data(plat_audio_stream_t stream, const void *data, int len)
{
	if (!stream) return -1;
	return SDL_PutAudioStreamData(stream->stream, data, len) ? 0 : -1;
}

static int sdl3_flush_audio_stream(plat_audio_stream_t stream)
{
	if (!stream) return -1;
	return SDL_FlushAudioStream(stream->stream) ? 0 : -1;
}

static int sdl3_get_audio_stream_queued(plat_audio_stream_t stream)
{
	if (!stream) return -1;
	return SDL_GetAudioStreamQueued(stream->stream);
}

static void sdl3_clear_audio_stream(plat_audio_stream_t stream)
{
	if (stream) SDL_ClearAudioStream(stream->stream);
}

static int sdl3_set_audio_stream_gain(plat_audio_stream_t stream, float gain)
{
	if (!stream) return -1;
	return SDL_SetAudioStreamGain(stream->stream, gain) ? 0 : -1;
}

static void sdl3_resume_audio_stream_device(plat_audio_stream_t stream)
{
	if (stream) SDL_ResumeAudioStreamDevice(stream->stream);
}

static int sdl3_load_wav(const char *filepath, plat_audio_spec_t *spec, uint8_t **buffer,
						 uint32_t *length)
{
	if (!spec || !filepath) return -1;
	SDL_IOStream *io = SDL_IOFromFile(filepath, "rb");
	if (!io) return -1;
	SDL_AudioSpec sdl_spec;
	SDL_zero(sdl_spec);
	uint8_t *wav_buffer;
	uint32_t wav_length;
	if (!SDL_LoadWAV_IO(io, true, &sdl_spec, &wav_buffer, &wav_length)) {
		return -1;
	}

	if (sdl_spec.format != SDL_AUDIO_F32) {
		SDL_AudioSpec dst_spec;
		SDL_zero(dst_spec);
		dst_spec.format	  = SDL_AUDIO_F32;
		dst_spec.channels = sdl_spec.channels;
		dst_spec.freq	  = sdl_spec.freq;

		uint8_t *converted_buffer;
		int		 converted_length;
		if (!SDL_ConvertAudioSamples(&sdl_spec, wav_buffer, (int)wav_length, &dst_spec,
									 &converted_buffer, &converted_length)) {
			SDL_free(wav_buffer);
			return -1;
		}
		SDL_free(wav_buffer);
		*buffer = converted_buffer;
		*length = (uint32_t)converted_length;
	} else {
		*buffer = wav_buffer;
		*length = wav_length;
	}

	spec->format   = PLAT_AUDIO_F32;
	spec->channels = sdl_spec.channels;
	spec->freq	   = sdl_spec.freq;
	return 0;
}

static void sdl3_mem_free(void *ptr)
{
	SDL_free(ptr);
}

static plat_mutex_t sdl3_create_mutex(void)
{
	plat_mutex_t m = malloc(sizeof(struct plat_mutex));
	if (!m) return NULL;
	m->mutex = SDL_CreateMutex();
	if (!m->mutex) {
		free(m);
		return NULL;
	}
	return m;
}

static void sdl3_destroy_mutex(plat_mutex_t mutex)
{
	if (mutex) {
		SDL_DestroyMutex(mutex->mutex);
		free(mutex);
	}
}

static void sdl3_lock_mutex(plat_mutex_t mutex)
{
	if (mutex) SDL_LockMutex(mutex->mutex);
}

static void sdl3_unlock_mutex(plat_mutex_t mutex)
{
	if (mutex) SDL_UnlockMutex(mutex->mutex);
}

static plat_io_t *sdl3_io_open_read(const char *path)
{
	if (!path) return NULL;
	plat_io_t *io = (plat_io_t *)malloc(sizeof(plat_io_t));
	if (!io) return NULL;
	io->stream = SDL_IOFromFile(path, "rb");
	if (!io->stream) {
		free(io);
		return NULL;
	}
	return io;
}

static void sdl3_io_close(plat_io_t *io)
{
	if (io) {
		SDL_CloseIO(io->stream);
		free(io);
	}
}

static size_t sdl3_io_read(plat_io_t *io, void *buf, size_t size)
{
	if (!io || !buf) return 0;
	return (size_t)SDL_ReadIO(io->stream, buf, size);
}

static int64_t sdl3_io_seek(plat_io_t *io, int64_t offset, int whence)
{
	if (!io) return -1;
	SDL_IOWhence sdl_whence = SDL_IO_SEEK_SET;
	if (whence == SEEK_CUR) sdl_whence = SDL_IO_SEEK_CUR;
	else if (whence == SEEK_END) sdl_whence = SDL_IO_SEEK_END;
	return SDL_SeekIO(io->stream, offset, sdl_whence);
}

static int64_t sdl3_io_tell(plat_io_t *io)
{
	if (!io) return -1;
	return SDL_TellIO(io->stream);
}

static int64_t sdl3_io_size(plat_io_t *io)
{
	if (!io) return -1;
	return SDL_GetIOSize(io->stream);
}

static void sdl3_delay(uint32_t ms)
{
	SDL_Delay(ms);
}

static uint32_t sdl3_get_ticks(void)
{
	return (uint32_t)SDL_GetTicks();
}

static void sdl3_log_debug(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_DEBUG, fmt, args);
	va_end(args);
}

static void sdl3_log_info(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO, fmt, args);
	va_end(args);
}

static void sdl3_log_warn(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_WARN, fmt, args);
	va_end(args);
}

static void sdl3_log_error(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_ERROR, fmt, args);
	va_end(args);
}

static void sdl3_log_critical(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_CRITICAL, fmt, args);
	va_end(args);
}

static const plat_interface_t sdl3_interface = {
	.capabilities = PLAT_CAPABILITY_AUDIO | PLAT_CAPABILITY_VIDEO,
	.core =
		{
			.init		  = sdl3_init,
			.quit		  = sdl3_quit,
			.delay		  = sdl3_delay,
			.get_ticks	  = sdl3_get_ticks,
			.log_debug	  = sdl3_log_debug,
			.log_info	  = sdl3_log_info,
			.log_warn	  = sdl3_log_warn,
			.log_error	  = sdl3_log_error,
			.log_critical = sdl3_log_critical,
		},
	.window =
		{
			.create_window	 = sdl3_create_window,
			.destroy_window	 = sdl3_destroy_window,
			.poll_event		 = sdl3_poll_event,
			.get_mouse_state = sdl3_get_mouse_state,
		},
	.renderer =
		{
			.create_renderer			= sdl3_create_renderer,
			.destroy_renderer			= sdl3_destroy_renderer,
			.set_render_draw_color		= sdl3_set_render_draw_color,
			.set_render_draw_blend_mode = sdl3_set_render_draw_blend_mode,
			.render_clear				= sdl3_render_clear,
			.render_present				= sdl3_render_present,
			.render_point				= sdl3_render_point,
			.render_line				= sdl3_render_line,
			.render_rect				= sdl3_render_rect,
			.render_fill_rect			= sdl3_render_fill_rect,
			.render_texture				= sdl3_render_texture,
		},
	.texture =
		{
			.create_texture_from_surface = sdl3_create_texture_from_surface,
			.create_texture				 = sdl3_create_texture,
			.update_texture				 = sdl3_update_texture,
			.get_texture_size			 = sdl3_get_texture_size,
			.destroy_texture			 = sdl3_destroy_texture,
			.load_image					 = sdl3_load_image,
			.destroy_surface			 = sdl3_destroy_surface,
		},
	.text =
		{
			.init_ttf			 = sdl3_init_ttf,
			.quit_ttf			 = sdl3_quit_ttf,
			.open_font			 = sdl3_open_font,
			.close_font			 = sdl3_close_font,
			.render_text_blended = sdl3_render_text_blended,
			.get_string_size	 = sdl3_get_string_size,
		},
	.audio =
		{
			.open_audio_device			= sdl3_open_audio_device,
			.close_audio_device			= sdl3_close_audio_device,
			.create_audio_stream		= sdl3_create_audio_stream,
			.open_audio_device_stream	= sdl3_open_audio_device_stream,
			.bind_audio_stream			= sdl3_bind_audio_stream,
			.destroy_audio_stream		= sdl3_destroy_audio_stream,
			.put_audio_stream_data		= sdl3_put_audio_stream_data,
			.flush_audio_stream			= sdl3_flush_audio_stream,
			.get_audio_stream_queued	= sdl3_get_audio_stream_queued,
			.clear_audio_stream			= sdl3_clear_audio_stream,
			.set_audio_stream_gain		= sdl3_set_audio_stream_gain,
			.resume_audio_stream_device = sdl3_resume_audio_stream_device,
			.load_wav					= sdl3_load_wav,
			.mem_free					= sdl3_mem_free,
		},
	.sync =
		{
			.create_mutex  = sdl3_create_mutex,
			.destroy_mutex = sdl3_destroy_mutex,
			.lock_mutex	   = sdl3_lock_mutex,
			.unlock_mutex  = sdl3_unlock_mutex,
		},
	.io =
		{
			.open_read = sdl3_io_open_read,
			.close	   = sdl3_io_close,
			.read	   = sdl3_io_read,
			.seek	   = sdl3_io_seek,
			.tell	   = sdl3_io_tell,
			.size	   = sdl3_io_size,
		},
};

const plat_interface_t *plat_sdl3_interface(void)
{
	return &sdl3_interface;
}
