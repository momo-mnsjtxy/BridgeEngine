#define BAPI_LOG_ENABLED
#include <BridgeEngine.h>
#include "master_aggregate_bapi_compat_members.h"
#include "master_aggregate_bapi_compat_signatures.h"
#if defined(_MSC_VER) && !defined(__clang__)
#define CHECK_SIGNATURE(symbol, type) typedef char symbol##_exists[(sizeof(&(symbol)) > 0) ? 1 : -1];
#define CHECK_MEMBER(type, member, expected) typedef char type##_##member##_exists[(sizeof(((type *)0)->member) > 0) ? 1 : -1];
#define CHECK_TYPE(type, expected) typedef char type##_exists[(sizeof(type) > 0) ? 1 : -1];
#else
#define CHECK_SIGNATURE(symbol, type) _Static_assert(_Generic(&(symbol), type: 1, default: 0), #symbol);
#define CHECK_MEMBER(type, member, expected) _Static_assert(_Generic(((type *)0)->member, expected: 1, default: 0), #type "." #member);
#define CHECK_TYPE(type, expected) _Static_assert(_Generic((type)0, expected: 1, default: 0), #type);
#endif
#if defined(_MSC_VER) && !defined(__clang__)
#define CHECK_VALUE(name, expression) typedef char name[(expression) ? 1 : -1];
#else
#define CHECK_VALUE(name, expression) _Static_assert(expression, #name);
#endif

MASTER_AGGREGATE_BAPI_SIGNATURES(CHECK_SIGNATURE)
MASTER_AGGREGATE_BAPI_MEMBERS(CHECK_MEMBER)

CHECK_TYPE(bapi_window_t, struct bapi_window_internal *);
CHECK_TYPE(bapi_renderer_t, struct bapi_renderer_internal *);
CHECK_TYPE(bapi_texture_t, struct bapi_texture_internal *);
CHECK_TYPE(bapi_sound_t, struct bapi_sound_internal *);
CHECK_TYPE(bapi_video_t, struct bapi_video_internal *);
CHECK_TYPE(bapi_scene_t, struct bapi_scene_internal *);
CHECK_TYPE(bapi_scene_manager_t, struct bapi_scene_manager_internal *);
CHECK_TYPE(bapi_level_t, struct bapi_level_internal *);
CHECK_TYPE(bapi_level_manager_t, struct bapi_level_manager_internal *);
CHECK_TYPE(bapi_scene_on_enter_fn, void (*)(bapi_scene_t));
CHECK_TYPE(bapi_scene_on_exit_fn, void (*)(bapi_scene_t));
CHECK_TYPE(bapi_scene_on_update_fn, void (*)(bapi_scene_t, float));
CHECK_TYPE(bapi_scene_on_render_fn, void (*)(bapi_scene_t));
CHECK_TYPE(bapi_level_on_load_fn, void (*)(bapi_level_t));
CHECK_TYPE(bapi_level_on_unload_fn, void (*)(bapi_level_t));
CHECK_TYPE(bapi_level_on_update_fn, void (*)(bapi_level_t, float));
CHECK_TYPE(bapi_level_on_render_fn, void (*)(bapi_level_t));

CHECK_SIGNATURE(bapi_engine_init, int (*)(const char *, int, int));
CHECK_SIGNATURE(bapi_engine_quit, void (*)(void));
CHECK_SIGNATURE(bapi_engine_get_window, bapi_window_t (*)(void));
CHECK_SIGNATURE(bapi_engine_get_renderer, bapi_renderer_t (*)(void));
CHECK_SIGNATURE(bapi_poll_event, int (*)(bapi_event_t *));
CHECK_SIGNATURE(bapi_render_clear, void (*)(void));
CHECK_SIGNATURE(bapi_render_present, void (*)(void));
CHECK_SIGNATURE(bapi_set_render_color, void (*)(bapi_color_t));
CHECK_SIGNATURE(bapi_delay, void (*)(uint32_t));
CHECK_SIGNATURE(bapi_draw_pixel, void (*)(float, float, bapi_color_t));
CHECK_SIGNATURE(bapi_draw_line, void (*)(float, float, float, float, bapi_color_t));
CHECK_SIGNATURE(bapi_draw_rect, void (*)(float, float, float, float, bapi_color_t));
CHECK_SIGNATURE(bapi_fill_rect, void (*)(float, float, float, float, bapi_color_t));
CHECK_SIGNATURE(bapi_draw_image, void (*)(const char *, float, float, float, float));
CHECK_SIGNATURE(bapi_draw_text, void (*)(const char *, float, float, float, bapi_color_t));
CHECK_SIGNATURE(bapi_text_init, void (*)(void));
CHECK_SIGNATURE(bapi_text_cleanup, void (*)(void));
CHECK_SIGNATURE(bapi_mouse_init, void (*)(void));
CHECK_SIGNATURE(bapi_mouse_handle_event, void (*)(const bapi_event_t *));
CHECK_SIGNATURE(bapi_mouse_render, void (*)(void));
CHECK_SIGNATURE(bapi_mouse_clear, void (*)(void));
CHECK_SIGNATURE(bapi_mouse_cleanup, void (*)(void));
CHECK_SIGNATURE(bapi_color, bapi_color_t (*)(uint8_t, uint8_t, uint8_t, uint8_t));
CHECK_SIGNATURE(bapi_get_ticks, uint32_t (*)(void));
CHECK_SIGNATURE(bapi_audio_init, int (*)(void));
CHECK_SIGNATURE(bapi_audio_cleanup, void (*)(void));
CHECK_SIGNATURE(bapi_sound_load, bapi_sound_t (*)(const char *));
CHECK_SIGNATURE(bapi_sound_play, int (*)(bapi_sound_t));
CHECK_SIGNATURE(bapi_sound_stop, void (*)(bapi_sound_t));
CHECK_SIGNATURE(bapi_sound_update, void (*)(void));
CHECK_SIGNATURE(bapi_sound_free, void (*)(bapi_sound_t));
CHECK_SIGNATURE(bapi_video_init, int (*)(void));
CHECK_SIGNATURE(bapi_video_cleanup, void (*)(void));
CHECK_SIGNATURE(bapi_video_load, bapi_video_t (*)(const char *));
CHECK_SIGNATURE(bapi_video_play, int (*)(bapi_video_t));
CHECK_SIGNATURE(bapi_video_update, void (*)(void));
CHECK_SIGNATURE(bapi_log_init, bool (*)(const bapi_log_config_t *));
CHECK_SIGNATURE(bapi_log_shutdown, void (*)(void));
CHECK_SIGNATURE(bapi_log_set_level, void (*)(bapi_log_level_t));
CHECK_SIGNATURE(bridgeengine_get_version, const char *(*)(void));
CHECK_SIGNATURE(bridgeengine_get_version_number, int (*)(void));

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

CHECK_VALUE(bapi_event_values, BAPI_EVENT_QUIT == 0 && BAPI_EVENT_KEY_DOWN == 1 && BAPI_EVENT_MOUSE_BUTTON_DOWN == 2 &&
			BAPI_EVENT_MOUSE_BUTTON_UP == 3 && BAPI_EVENT_MOUSE_MOTION == 4)
CHECK_VALUE(bapi_special_key_values, KEY_ESC == 128 && KEY_BACKSPACE == 129 && KEY_TAB == 130 && KEY_ENTER == 131 &&
			KEY_CAPS == 132 && KEY_SHIFT == 133 && KEY_CTRL == 134 && KEY_ALT == 135 && KEY_SPACE == 136 &&
			KEY_F1 == 137 && KEY_F12 == 148 && KEY_NUML == 149 && KEY_SCROLL == 150)
CHECK_VALUE(bapi_button_left_value, BAPI_BUTTON_LEFT == 1)
CHECK_VALUE(bapi_log_level_values, BAPI_LOG_LEVEL_DEBUG == 0 && BAPI_LOG_LEVEL_INFO == 1 && BAPI_LOG_LEVEL_WARN == 2 &&
			BAPI_LOG_LEVEL_ERROR == 3 && BAPI_LOG_LEVEL_CRITICAL == 4 && BAPI_LOG_LEVEL_NONE == 5)

int main(void)
{
	bapi_color_t color = bapi_color(1, 2, 3, 4);
	BAPI_LOG_DEBUG("master aggregate compatibility");
	BAPI_LOG_INFO("master aggregate compatibility");
	BAPI_LOG_WARN("master aggregate compatibility");
	BAPI_LOG_ERROR("master aggregate compatibility");
	BAPI_LOG_CRITICAL("master aggregate compatibility");
	BAPI_LOG_ASSERT(1, "master aggregate compatibility");
	if (0)
		BAPI_LOG_INIT_DEFAULT();
	return BRIDGEENGINE_MAJOR < 0 || BRIDGEENGINE_MINOR < 0 || BRIDGEENGINE_PATCH < 0 ||
			   BRIDGEENGINE_VERSION[0] == '\0' || bridgeengine_get_version()[0] == '\0' ||
			   bridgeengine_get_version_number() < 0 || color.a != 4;
}
