#define MASTER_AGGREGATE_BAPI_MEMBERS(X) \
	X(bapi_color_t, r, uint8_t) \
	X(bapi_color_t, g, uint8_t) \
	X(bapi_color_t, b, uint8_t) \
	X(bapi_color_t, a, uint8_t) \
	X(bapi_rect_t, x, float) \
	X(bapi_rect_t, y, float) \
	X(bapi_rect_t, w, float) \
	X(bapi_rect_t, h, float) \
	X(bapi_button_t, rect, bapi_rect_t) \
	X(bapi_button_t, normal_color, bapi_color_t) \
	X(bapi_button_t, hover_color, bapi_color_t) \
	X(bapi_button_t, click_color, bapi_color_t) \
	X(bapi_button_t, text, const char *) \
	X(bapi_button_t, text_color, bapi_color_t) \
	X(bapi_button_t, text_size, float) \
	X(bapi_button_t, text_width, float) \
	X(bapi_button_t, text_height, float) \
	X(bapi_button_t, is_clicked, int) \
	X(bapi_button_t, is_hovered, int) \
	X(bapi_scene_callbacks_t, on_enter, bapi_scene_on_enter_fn) \
	X(bapi_scene_callbacks_t, on_exit, bapi_scene_on_exit_fn) \
	X(bapi_scene_callbacks_t, on_update, bapi_scene_on_update_fn) \
	X(bapi_scene_callbacks_t, on_render, bapi_scene_on_render_fn) \
	X(bapi_scene_callbacks_t, user_data, void *) \
	X(bapi_level_callbacks_t, on_load, bapi_level_on_load_fn) \
	X(bapi_level_callbacks_t, on_unload, bapi_level_on_unload_fn) \
	X(bapi_level_callbacks_t, on_update, bapi_level_on_update_fn) \
	X(bapi_level_callbacks_t, on_render, bapi_level_on_render_fn) \
	X(bapi_level_callbacks_t, user_data, void *) \
	X(bapi_log_config_t, min_level, bapi_log_level_t) \
	X(bapi_log_config_t, use_colors, bool) \
	X(bapi_log_config_t, use_file, bool) \
	X(bapi_log_config_t, log_file_path, const char *)
