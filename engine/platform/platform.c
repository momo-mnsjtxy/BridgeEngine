#include "platform/platform.h"
#include <stddef.h>

static plat_interface_t g_plat;
static int				g_plat_initialized = 0;

static void plat_expand_grouped_api(plat_interface_t *api)
{
	if (api->init == NULL) api->init = api->core.init;
	if (api->quit == NULL) api->quit = api->core.quit;
	if (api->delay == NULL) api->delay = api->core.delay;
	if (api->get_ticks == NULL) api->get_ticks = api->core.get_ticks;
	if (api->log_debug == NULL) api->log_debug = api->core.log_debug;
	if (api->log_info == NULL) api->log_info = api->core.log_info;
	if (api->log_warn == NULL) api->log_warn = api->core.log_warn;
	if (api->log_error == NULL) api->log_error = api->core.log_error;
	if (api->log_critical == NULL) api->log_critical = api->core.log_critical;

	if (api->create_window == NULL) api->create_window = api->window.create_window;
	if (api->destroy_window == NULL) api->destroy_window = api->window.destroy_window;
	if (api->poll_event == NULL) api->poll_event = api->window.poll_event;
	if (api->get_mouse_state == NULL) api->get_mouse_state = api->window.get_mouse_state;

	if (api->create_renderer == NULL) api->create_renderer = api->renderer.create_renderer;
	if (api->destroy_renderer == NULL) api->destroy_renderer = api->renderer.destroy_renderer;
	if (api->set_render_draw_color == NULL)
		api->set_render_draw_color = api->renderer.set_render_draw_color;
	if (api->set_render_draw_blend_mode == NULL)
		api->set_render_draw_blend_mode = api->renderer.set_render_draw_blend_mode;
	if (api->render_clear == NULL) api->render_clear = api->renderer.render_clear;
	if (api->render_present == NULL) api->render_present = api->renderer.render_present;
	if (api->render_point == NULL) api->render_point = api->renderer.render_point;
	if (api->render_line == NULL) api->render_line = api->renderer.render_line;
	if (api->render_rect == NULL) api->render_rect = api->renderer.render_rect;
	if (api->render_fill_rect == NULL) api->render_fill_rect = api->renderer.render_fill_rect;
	if (api->render_texture == NULL) api->render_texture = api->renderer.render_texture;

	if (api->create_texture_from_surface == NULL)
		api->create_texture_from_surface = api->texture.create_texture_from_surface;
	if (api->create_texture == NULL) api->create_texture = api->texture.create_texture;
	if (api->update_texture == NULL) api->update_texture = api->texture.update_texture;
	if (api->destroy_texture == NULL) api->destroy_texture = api->texture.destroy_texture;
	if (api->load_image == NULL) api->load_image = api->texture.load_image;
	if (api->destroy_surface == NULL) api->destroy_surface = api->texture.destroy_surface;

	if (api->init_ttf == NULL) api->init_ttf = api->text.init_ttf;
	if (api->quit_ttf == NULL) api->quit_ttf = api->text.quit_ttf;
	if (api->open_font == NULL) api->open_font = api->text.open_font;
	if (api->close_font == NULL) api->close_font = api->text.close_font;
	if (api->render_text_blended == NULL) api->render_text_blended = api->text.render_text_blended;
	if (api->get_string_size == NULL) api->get_string_size = api->text.get_string_size;

	if (api->open_audio_device == NULL) api->open_audio_device = api->audio.open_audio_device;
	if (api->close_audio_device == NULL) api->close_audio_device = api->audio.close_audio_device;
	if (api->create_audio_stream == NULL) api->create_audio_stream = api->audio.create_audio_stream;
	if (api->open_audio_device_stream == NULL)
		api->open_audio_device_stream = api->audio.open_audio_device_stream;
	if (api->bind_audio_stream == NULL) api->bind_audio_stream = api->audio.bind_audio_stream;
	if (api->destroy_audio_stream == NULL)
		api->destroy_audio_stream = api->audio.destroy_audio_stream;
	if (api->put_audio_stream_data == NULL)
		api->put_audio_stream_data = api->audio.put_audio_stream_data;
	if (api->flush_audio_stream == NULL) api->flush_audio_stream = api->audio.flush_audio_stream;
	if (api->get_audio_stream_queued == NULL)
		api->get_audio_stream_queued = api->audio.get_audio_stream_queued;
	if (api->clear_audio_stream == NULL) api->clear_audio_stream = api->audio.clear_audio_stream;
	if (api->set_audio_stream_gain == NULL)
		api->set_audio_stream_gain = api->audio.set_audio_stream_gain;
	if (api->resume_audio_stream_device == NULL)
		api->resume_audio_stream_device = api->audio.resume_audio_stream_device;
	if (api->load_wav == NULL) api->load_wav = api->audio.load_wav;
	if (api->mem_free == NULL) api->mem_free = api->audio.mem_free;

	if (api->create_mutex == NULL) api->create_mutex = api->sync.create_mutex;
	if (api->destroy_mutex == NULL) api->destroy_mutex = api->sync.destroy_mutex;
	if (api->lock_mutex == NULL) api->lock_mutex = api->sync.lock_mutex;
	if (api->unlock_mutex == NULL) api->unlock_mutex = api->sync.unlock_mutex;
}

int plat_init(const plat_interface_t *interface)
{
	if (interface == NULL) {
		return 1;
	}
	g_plat = *interface;
	plat_expand_grouped_api(&g_plat);
	g_plat_initialized = 1;
	return 0;
}

const plat_interface_t *plat_get(void)
{
	if (!g_plat_initialized) {
		return NULL;
	}
	return &g_plat;
}

void plat_shutdown(void)
{
	g_plat_initialized = 0;
}
