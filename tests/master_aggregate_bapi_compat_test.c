#include <BridgeEngine.h>
#include <stdint.h>

static const uintptr_t master_documented_symbols[] = {
	(uintptr_t)&bapi_engine_init, (uintptr_t)&bapi_engine_quit, (uintptr_t)&bapi_engine_get_window,
	(uintptr_t)&bapi_engine_get_renderer, (uintptr_t)&bapi_poll_event, (uintptr_t)&bapi_render_clear,
	(uintptr_t)&bapi_render_present, (uintptr_t)&bapi_set_render_color, (uintptr_t)&bapi_delay,
	(uintptr_t)&bapi_get_ticks, (uintptr_t)&bapi_draw_pixel, (uintptr_t)&bapi_draw_line,
	(uintptr_t)&bapi_draw_rect, (uintptr_t)&bapi_fill_rect, (uintptr_t)&bapi_draw_triangle,
	(uintptr_t)&bapi_draw_circle, (uintptr_t)&bapi_fill_circle, (uintptr_t)&bapi_draw_polygon,
	(uintptr_t)&bapi_fill_polygon, (uintptr_t)&bapi_draw_image, (uintptr_t)&bapi_draw_text,
	(uintptr_t)&bapi_get_text_size, (uintptr_t)&bapi_text_init, (uintptr_t)&bapi_text_cleanup,
	(uintptr_t)&bapi_mouse_init, (uintptr_t)&bapi_mouse_handle_event, (uintptr_t)&bapi_mouse_render,
	(uintptr_t)&bapi_mouse_draw_line, (uintptr_t)&bapi_mouse_clear, (uintptr_t)&bapi_mouse_cleanup,
	(uintptr_t)&bapi_audio_init, (uintptr_t)&bapi_audio_cleanup, (uintptr_t)&bapi_sound_load,
	(uintptr_t)&bapi_sound_play, (uintptr_t)&bapi_sound_set_volume, (uintptr_t)&bapi_sound_set_loop,
	(uintptr_t)&bapi_sound_stop, (uintptr_t)&bapi_sound_update, (uintptr_t)&bapi_sound_free,
	(uintptr_t)&bapi_video_init, (uintptr_t)&bapi_video_cleanup, (uintptr_t)&bapi_video_load,
	(uintptr_t)&bapi_video_free, (uintptr_t)&bapi_video_play, (uintptr_t)&bapi_video_pause,
	(uintptr_t)&bapi_video_stop, (uintptr_t)&bapi_video_render, (uintptr_t)&bapi_video_render_fit,
	(uintptr_t)&bapi_video_render_center, (uintptr_t)&bapi_video_set_loop,
	(uintptr_t)&bapi_video_set_volume, (uintptr_t)&bapi_video_is_playing,
	(uintptr_t)&bapi_video_get_size, (uintptr_t)&bapi_video_update, (uintptr_t)&bapi_log_init,
	(uintptr_t)&bapi_log_shutdown, (uintptr_t)&bapi_log_set_level, (uintptr_t)&bapi_log_message,
	(uintptr_t)&bridgeengine_get_version, (uintptr_t)&bridgeengine_get_version_number,
	(uintptr_t)&bapi_scene_get_name, (uintptr_t)&bapi_scene_get_user_data,
	(uintptr_t)&bapi_scene_set_user_data, (uintptr_t)&bapi_scene_manager_switch_scene,
	(uintptr_t)&bapi_scene_manager_get_current_scene, (uintptr_t)&bapi_scene_manager_get_scene,
	(uintptr_t)&bapi_scene_manager_update, (uintptr_t)&bapi_scene_manager_render,
	(uintptr_t)&bapi_level_get_name, (uintptr_t)&bapi_level_get_index,
	(uintptr_t)&bapi_level_get_user_data, (uintptr_t)&bapi_level_set_user_data,
	(uintptr_t)&bapi_level_manager_load_level, (uintptr_t)&bapi_level_manager_load_level_by_index,
	(uintptr_t)&bapi_level_manager_next_level, (uintptr_t)&bapi_level_manager_previous_level,
	(uintptr_t)&bapi_level_manager_get_current_level, (uintptr_t)&bapi_level_manager_get_level,
	(uintptr_t)&bapi_level_manager_get_level_by_index, (uintptr_t)&bapi_level_manager_get_level_count,
	(uintptr_t)&bapi_level_manager_update, (uintptr_t)&bapi_level_manager_render,
};

#define CHECK_SIGNATURE(symbol, type) _Static_assert(_Generic(&(symbol), type: 1, default: 0), #symbol)

CHECK_SIGNATURE(bapi_event_get_type, int (*)(const bapi_event_t *));
CHECK_SIGNATURE(bapi_event_get_key_code, uint8_t (*)(const bapi_event_t *));
CHECK_SIGNATURE(bapi_event_get_mouse_x, int (*)(const bapi_event_t *));
CHECK_SIGNATURE(bapi_event_get_mouse_y, int (*)(const bapi_event_t *));
CHECK_SIGNATURE(bapi_event_get_mouse_button, int (*)(const bapi_event_t *));
CHECK_SIGNATURE(bapi_event_get_motion_x, int (*)(const bapi_event_t *));
CHECK_SIGNATURE(bapi_event_get_motion_y, int (*)(const bapi_event_t *));
CHECK_SIGNATURE(bapi_event_is_mouse_button_down, int (*)(const bapi_event_t *));
CHECK_SIGNATURE(bapi_event_is_mouse_button_up, int (*)(const bapi_event_t *));
CHECK_SIGNATURE(bapi_event_is_mouse_motion, int (*)(const bapi_event_t *));
CHECK_SIGNATURE(bapi_color_from_hex, bapi_color_t (*)(uint32_t));
CHECK_SIGNATURE(bapi_create_button, bapi_button_t *(*)(float, float, float, float, const char *, bapi_color_t, bapi_color_t, bapi_color_t, bapi_color_t, float));
CHECK_SIGNATURE(bapi_destroy_button, void (*)(bapi_button_t *));
CHECK_SIGNATURE(bapi_button_update, int (*)(bapi_button_t *, const bapi_event_t *));
CHECK_SIGNATURE(bapi_button_render, void (*)(bapi_button_t *));
CHECK_SIGNATURE(bapi_button_is_clicked, int (*)(bapi_button_t *));
CHECK_SIGNATURE(bapi_button_is_hovered, int (*)(bapi_button_t *));
CHECK_SIGNATURE(bapi_scene_create, bapi_scene_t (*)(const char *, bapi_scene_callbacks_t));
CHECK_SIGNATURE(bapi_scene_destroy, void (*)(bapi_scene_t));
CHECK_SIGNATURE(bapi_scene_manager_create, bapi_scene_manager_t (*)(void));
CHECK_SIGNATURE(bapi_scene_manager_destroy, void (*)(bapi_scene_manager_t));
CHECK_SIGNATURE(bapi_scene_manager_add_scene, int (*)(bapi_scene_manager_t, bapi_scene_t));
CHECK_SIGNATURE(bapi_scene_get_name, const char *(*)(bapi_scene_t));
CHECK_SIGNATURE(bapi_scene_get_user_data, void *(*)(bapi_scene_t));
CHECK_SIGNATURE(bapi_scene_set_user_data, void (*)(bapi_scene_t, void *));
CHECK_SIGNATURE(bapi_scene_manager_switch_scene, int (*)(bapi_scene_manager_t, const char *));
CHECK_SIGNATURE(bapi_scene_manager_get_current_scene, bapi_scene_t (*)(bapi_scene_manager_t));
CHECK_SIGNATURE(bapi_scene_manager_get_scene, bapi_scene_t (*)(bapi_scene_manager_t, const char *));
CHECK_SIGNATURE(bapi_scene_manager_update, void (*)(bapi_scene_manager_t, float));
CHECK_SIGNATURE(bapi_scene_manager_render, void (*)(bapi_scene_manager_t));
CHECK_SIGNATURE(bapi_level_create, bapi_level_t (*)(const char *, int, bapi_level_callbacks_t));
CHECK_SIGNATURE(bapi_level_destroy, void (*)(bapi_level_t));
CHECK_SIGNATURE(bapi_level_manager_create, bapi_level_manager_t (*)(void));
CHECK_SIGNATURE(bapi_level_manager_destroy, void (*)(bapi_level_manager_t));
CHECK_SIGNATURE(bapi_level_manager_add_level, int (*)(bapi_level_manager_t, bapi_level_t));
CHECK_SIGNATURE(bapi_level_get_name, const char *(*)(bapi_level_t));
CHECK_SIGNATURE(bapi_level_get_index, int (*)(bapi_level_t));
CHECK_SIGNATURE(bapi_level_get_user_data, void *(*)(bapi_level_t));
CHECK_SIGNATURE(bapi_level_set_user_data, void (*)(bapi_level_t, void *));
CHECK_SIGNATURE(bapi_level_manager_load_level, int (*)(bapi_level_manager_t, const char *));
CHECK_SIGNATURE(bapi_level_manager_load_level_by_index, int (*)(bapi_level_manager_t, int));
CHECK_SIGNATURE(bapi_level_manager_next_level, int (*)(bapi_level_manager_t));
CHECK_SIGNATURE(bapi_level_manager_previous_level, int (*)(bapi_level_manager_t));
CHECK_SIGNATURE(bapi_level_manager_get_current_level, bapi_level_t (*)(bapi_level_manager_t));
CHECK_SIGNATURE(bapi_level_manager_get_level, bapi_level_t (*)(bapi_level_manager_t, const char *));
CHECK_SIGNATURE(bapi_level_manager_get_level_by_index, bapi_level_t (*)(bapi_level_manager_t, int));
CHECK_SIGNATURE(bapi_level_manager_get_level_count, int (*)(bapi_level_manager_t));
CHECK_SIGNATURE(bapi_level_manager_update, void (*)(bapi_level_manager_t, float));
CHECK_SIGNATURE(bapi_level_manager_render, void (*)(bapi_level_manager_t));
CHECK_SIGNATURE(bapi_scene_manager_load_from_xml, bapi_scene_manager_t (*)(const char *));
CHECK_SIGNATURE(bapi_level_manager_load_from_xml, bapi_level_manager_t (*)(const char *));
CHECK_SIGNATURE(bapi_scene_manager_save_to_xml, int (*)(bapi_scene_manager_t, const char *));
CHECK_SIGNATURE(bapi_level_manager_save_to_xml, int (*)(bapi_level_manager_t, const char *));

int main(void)
{
	bapi_color_t color = bapi_color(1, 2, 3, 4);
	bapi_vec2_t vector = bapi_vec2(1.0f, 2.0f);
	return BRIDGEENGINE_MAJOR < 0 || BRIDGEENGINE_MINOR < 0 || BRIDGEENGINE_PATCH < 0 ||
			   BRIDGEENGINE_VERSION[0] == '\0' || bridgeengine_get_version()[0] == '\0' ||
			   bridgeengine_get_version_number() < 0 || color.a != 4 || vector.x != 1.0f ||
			   master_documented_symbols[0] == 0;
}
