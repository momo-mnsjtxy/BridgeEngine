#pragma once

#include "bridgeengine_version.h"

#include <stddef.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct bapi_window_internal		   *bapi_window_t;
typedef struct bapi_renderer_internal	   *bapi_renderer_t;
typedef struct bapi_texture_internal	   *bapi_texture_t;
typedef struct bapi_sound_internal		   *bapi_sound_t;
typedef struct bapi_video_internal		   *bapi_video_t;
typedef struct bapi_scene_internal		   *bapi_scene_t;
typedef struct bapi_scene_manager_internal *bapi_scene_manager_t;
typedef struct bapi_level_internal		   *bapi_level_t;
typedef struct bapi_level_manager_internal *bapi_level_manager_t;

typedef struct {
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
} bapi_color_t;

typedef struct {
	float x;
	float y;
	float w;
	float h;
} bapi_rect_t;

typedef enum {
	BAPI_EVENT_QUIT = 0,
	BAPI_EVENT_KEY_DOWN = 1,
	BAPI_EVENT_MOUSE_BUTTON_DOWN = 2,
	BAPI_EVENT_MOUSE_BUTTON_UP = 3,
	BAPI_EVENT_MOUSE_MOTION = 4,
	BAPI_EVENT_KEY_UP = 5,
	BAPI_EVENT_UNKNOWN = 6,
} bapi_event_type_t;

typedef struct {
	bapi_event_type_t type;
	union {
		struct {
			uint32_t key;
		} key;
		struct {
			float x;
			float y;
			int	  button;
		} button;
		struct {
			float x;
			float y;
		} motion;
	} data;
} bapi_event_t;

#define BAPI_BUTTON_LEFT		 1
#define BAPI_MAX_CACHED_TEXTURES 64

typedef struct {
	bapi_rect_t	 rect;
	bapi_color_t normal_color;
	bapi_color_t hover_color;
	bapi_color_t click_color;
	const char	*text;
	bapi_color_t text_color;
	float		 text_size;
	float		 text_width;
	float		 text_height;
	int			 is_clicked;
	int			 is_hovered;
} bapi_button_t;

enum special_key_code {
	KEY_ESC = 128,
	KEY_BACKSPACE,
	KEY_TAB,
	KEY_ENTER,
	KEY_CAPS,
	KEY_SHIFT,
	KEY_CTRL,
	KEY_ALT,
	KEY_SPACE,
	KEY_F1,
	KEY_F2,
	KEY_F3,
	KEY_F4,
	KEY_F5,
	KEY_F6,
	KEY_F7,
	KEY_F8,
	KEY_F9,
	KEY_F10,
	KEY_F11,
	KEY_F12,
	KEY_NUML,
	KEY_SCROLL,
};

typedef struct {
	float x;
	float y;
} bapi_vec2_t;

typedef struct {
	float x;
	float y;
	float r;
} bapi_circle_t;

typedef struct {
	float x;
	float y;
	float zoom;
	float viewport_w;
	float viewport_h;
	float rotation;
} bapi_camera_t;

typedef void (*bapi_scene_on_enter_fn)(bapi_scene_t scene);
typedef void (*bapi_scene_on_exit_fn)(bapi_scene_t scene);
typedef void (*bapi_scene_on_update_fn)(bapi_scene_t scene, float delta_time);
typedef void (*bapi_scene_on_render_fn)(bapi_scene_t scene);

typedef struct {
	bapi_scene_on_enter_fn	on_enter;
	bapi_scene_on_exit_fn	on_exit;
	bapi_scene_on_update_fn on_update;
	bapi_scene_on_render_fn on_render;
	void				   *user_data;
} bapi_scene_callbacks_t;

typedef void (*bapi_level_on_load_fn)(bapi_level_t level);
typedef void (*bapi_level_on_unload_fn)(bapi_level_t level);
typedef void (*bapi_level_on_update_fn)(bapi_level_t level, float delta_time);
typedef void (*bapi_level_on_render_fn)(bapi_level_t level);

typedef struct {
	bapi_level_on_load_fn	on_load;
	bapi_level_on_unload_fn on_unload;
	bapi_level_on_update_fn on_update;
	bapi_level_on_render_fn on_render;
	void				   *user_data;
} bapi_level_callbacks_t;

typedef enum {
	BAPI_LOG_LEVEL_DEBUG = 0,
	BAPI_LOG_LEVEL_INFO,
	BAPI_LOG_LEVEL_WARN,
	BAPI_LOG_LEVEL_ERROR,
	BAPI_LOG_LEVEL_CRITICAL,
	BAPI_LOG_LEVEL_NONE,
} bapi_log_level_t;

typedef struct {
	bapi_log_level_t min_level;
	bool			 use_colors;
	bool			 use_file;
	const char		*log_file_path;
} bapi_log_config_t;

int				bapi_engine_init(const char *title, int width, int height);
/* Releases all engine resources; resource handles are invalid after this call. */
void			bapi_engine_quit(void);
/* Borrowed handles; valid only from bapi_engine_init() through bapi_engine_quit(). */
bapi_window_t	bapi_engine_get_window(void);
bapi_renderer_t bapi_engine_get_renderer(void);

int		bapi_poll_event(bapi_event_t *event);
int		bapi_event_get_type(const bapi_event_t *event);
uint8_t bapi_event_get_key_code(const bapi_event_t *event);
int		bapi_event_get_mouse_x(const bapi_event_t *event);
int		bapi_event_get_mouse_y(const bapi_event_t *event);
int		bapi_event_get_mouse_button(const bapi_event_t *event);
int		bapi_event_get_motion_x(const bapi_event_t *event);
int		bapi_event_get_motion_y(const bapi_event_t *event);
int		bapi_event_is_mouse_button_down(const bapi_event_t *event);
int		bapi_event_is_mouse_button_up(const bapi_event_t *event);
int		bapi_event_is_mouse_motion(const bapi_event_t *event);

void bapi_render_clear(void);
void bapi_render_present(void);
void bapi_set_render_color(bapi_color_t color);
void bapi_delay(uint32_t ms);
void bapi_draw_pixel(float x, float y, bapi_color_t color);
void bapi_draw_line(float x1, float y1, float x2, float y2, bapi_color_t color);
void bapi_draw_rect(float x, float y, float w, float h, bapi_color_t color);
void bapi_fill_rect(float x, float y, float w, float h, bapi_color_t color);
void bapi_draw_triangle(float x1, float y1, float x2, float y2, float x3, float y3,
						bapi_color_t color);
void bapi_draw_circle(float cx, float cy, float radius, bapi_color_t color);
void bapi_fill_circle(float cx, float cy, float radius, bapi_color_t color);
void bapi_draw_polygon(float cx, float cy, float radius, int sides, bapi_color_t color);
void bapi_fill_polygon(float cx, float cy, float radius, int sides, bapi_color_t color);
void bapi_draw_image(const char *filepath, float x, float y, float w, float h);
void bapi_draw_text(const char *text, float x, float y, float size, bapi_color_t color);
void bapi_get_text_size(const char *text, float size, float *width, float *height);
void bapi_text_init(void);
void bapi_text_cleanup(void);

void bapi_mouse_init(void);
void bapi_mouse_handle_event(const bapi_event_t *event);
void bapi_mouse_render(void);
void bapi_mouse_draw_line(float x1, float y1, float x2, float y2, bapi_color_t color);
void bapi_mouse_clear(void);
void bapi_mouse_cleanup(void);

bapi_color_t bapi_color_from_hex(uint32_t hex_color);
bapi_color_t bapi_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
uint32_t	 bapi_get_ticks(void);

int			 bapi_audio_init(void);
void		 bapi_audio_cleanup(void);
bapi_sound_t bapi_sound_load(const char *filepath);
int			 bapi_sound_play(bapi_sound_t sound);
void		 bapi_sound_set_volume(bapi_sound_t sound, float volume);
void		 bapi_sound_set_loop(bapi_sound_t sound, int loop);
void		 bapi_sound_stop(bapi_sound_t sound);
void		 bapi_sound_update(void);
void		 bapi_sound_free(bapi_sound_t sound);

bapi_button_t *bapi_create_button(float x, float y, float w, float h, const char *text,
								  bapi_color_t normal_color, bapi_color_t hover_color,
								  bapi_color_t click_color, bapi_color_t text_color,
								  float text_size);
void		   bapi_destroy_button(bapi_button_t *button);
int			   bapi_button_update(bapi_button_t *button, const bapi_event_t *event);
void		   bapi_button_render(bapi_button_t *button);
int			   bapi_button_is_clicked(bapi_button_t *button);
int			   bapi_button_is_hovered(bapi_button_t *button);

void bapi_camera_init(bapi_camera_t *cam, float viewport_w, float viewport_h);
void bapi_camera_set_position(bapi_camera_t *cam, float x, float y);
void bapi_camera_move(bapi_camera_t *cam, float dx, float dy);
void bapi_camera_set_zoom(bapi_camera_t *cam, float zoom);
void bapi_camera_set_rotation(bapi_camera_t *cam, float angle_rad);
void bapi_camera_set_viewport(bapi_camera_t *cam, float w, float h);
void bapi_camera_world_to_screen(bapi_camera_t *cam, float wx, float wy, float *sx, float *sy);
void bapi_camera_screen_to_world(bapi_camera_t *cam, float sx, float sy, float *wx, float *wy);
bapi_vec2_t bapi_camera_world_to_screen_v(bapi_camera_t *cam, bapi_vec2_t world);
bapi_vec2_t bapi_camera_screen_to_world_v(bapi_camera_t *cam, bapi_vec2_t screen);
void		bapi_camera_get_view_rect(bapi_camera_t *cam, bapi_rect_t *out_rect);

void  bapi_input_init(void);
void  bapi_input_cleanup(void);
void  bapi_input_update(void);
void  bapi_input_handle_event(const bapi_event_t *event);
int	  bapi_is_key_down(uint8_t key);
int	  bapi_is_key_pressed(uint8_t key);
int	  bapi_is_key_released(uint8_t key);
int	  bapi_is_mouse_button_down(int button);
int	  bapi_is_mouse_button_pressed(int button);
int	  bapi_is_mouse_button_released(int button);
float bapi_get_mouse_x(void);
float bapi_get_mouse_y(void);
void  bapi_get_mouse_position(float *x, float *y);

bapi_level_t bapi_level_create(const char *name, int index, bapi_level_callbacks_t callbacks);
void		 bapi_level_destroy(bapi_level_t level);
const char	*bapi_level_get_name(bapi_level_t level);
int			 bapi_level_get_index(bapi_level_t level);
void		*bapi_level_get_user_data(bapi_level_t level);
void		 bapi_level_set_user_data(bapi_level_t level, void *user_data);
bapi_level_manager_t bapi_level_manager_create(void);
void				 bapi_level_manager_destroy(bapi_level_manager_t manager);
int					 bapi_level_manager_add_level(bapi_level_manager_t manager, bapi_level_t level);
int					 bapi_level_manager_load_level(bapi_level_manager_t manager, const char *name);
int			 bapi_level_manager_load_level_by_index(bapi_level_manager_t manager, int index);
int			 bapi_level_manager_next_level(bapi_level_manager_t manager);
int			 bapi_level_manager_previous_level(bapi_level_manager_t manager);
bapi_level_t bapi_level_manager_get_current_level(bapi_level_manager_t manager);
bapi_level_t bapi_level_manager_get_level(bapi_level_manager_t manager, const char *name);
bapi_level_t bapi_level_manager_get_level_by_index(bapi_level_manager_t manager, int index);
int			 bapi_level_manager_get_level_count(bapi_level_manager_t manager);
void		 bapi_level_manager_update(bapi_level_manager_t manager, float delta_time);
void		 bapi_level_manager_render(bapi_level_manager_t manager);

bool bapi_log_init(const bapi_log_config_t *config);
void bapi_log_shutdown(void);
void bapi_log_set_level(bapi_log_level_t level);
void bapi_log_message(bapi_log_level_t level, const char *file, int line, const char *func,
					  const char *format, ...);

bapi_vec2_t	  bapi_vec2(float x, float y);
bapi_vec2_t	  bapi_vec2_add(bapi_vec2_t a, bapi_vec2_t b);
bapi_vec2_t	  bapi_vec2_sub(bapi_vec2_t a, bapi_vec2_t b);
bapi_vec2_t	  bapi_vec2_scale(bapi_vec2_t vector, float scalar);
bapi_vec2_t	  bapi_vec2_negate(bapi_vec2_t vector);
float		  bapi_vec2_dot(bapi_vec2_t a, bapi_vec2_t b);
float		  bapi_vec2_length(bapi_vec2_t vector);
float		  bapi_vec2_length_sq(bapi_vec2_t vector);
bapi_vec2_t	  bapi_vec2_normalize(bapi_vec2_t vector);
float		  bapi_vec2_distance(bapi_vec2_t a, bapi_vec2_t b);
bapi_vec2_t	  bapi_vec2_lerp(bapi_vec2_t a, bapi_vec2_t b, float t);
bapi_vec2_t	  bapi_vec2_rotate(bapi_vec2_t vector, float angle);
int			  bapi_vec2_equals(bapi_vec2_t a, bapi_vec2_t b);
bapi_circle_t bapi_circle(float x, float y, float radius);
/*
 * Geometry queries require finite coordinates, non-negative circle radii, and
 * non-negative rectangle widths and heights. Results outside these preconditions
 * are unspecified. Rectangle/AABB queries require positive-area overlap, while
 * circle queries count touching as an intersection.
 */
int			  bapi_rect_contains_point(bapi_rect_t rect, bapi_vec2_t point);
int			  bapi_rect_intersects(bapi_rect_t a, bapi_rect_t b);
bapi_rect_t	  bapi_rect_intersection(bapi_rect_t a, bapi_rect_t b);
bapi_rect_t	  bapi_rect_union(bapi_rect_t a, bapi_rect_t b);
bapi_vec2_t	  bapi_rect_center(bapi_rect_t rect);
int			  bapi_circle_contains_point(bapi_circle_t circle, bapi_vec2_t point);
int			  bapi_circle_intersects_circle(bapi_circle_t a, bapi_circle_t b);
int			  bapi_circle_intersects_rect(bapi_circle_t circle, bapi_rect_t rect);
/* Alias of bapi_rect_intersects with identical positive-area semantics. */
int			  bapi_collision_aabb(bapi_rect_t a, bapi_rect_t b);
float		  bapi_clamp(float value, float min, float max);
float		  bapi_lerp(float a, float b, float t);
float		  bapi_deg_to_rad(float degrees);
float		  bapi_rad_to_deg(float radians);

bapi_scene_t		 bapi_scene_create(const char *name, bapi_scene_callbacks_t callbacks);
void				 bapi_scene_destroy(bapi_scene_t scene);
const char			*bapi_scene_get_name(bapi_scene_t scene);
void				*bapi_scene_get_user_data(bapi_scene_t scene);
void				 bapi_scene_set_user_data(bapi_scene_t scene, void *user_data);
bapi_scene_manager_t bapi_scene_manager_create(void);
void				 bapi_scene_manager_destroy(bapi_scene_manager_t manager);
int					 bapi_scene_manager_add_scene(bapi_scene_manager_t manager, bapi_scene_t scene);
int			 bapi_scene_manager_switch_scene(bapi_scene_manager_t manager, const char *name);
bapi_scene_t bapi_scene_manager_get_current_scene(bapi_scene_manager_t manager);
bapi_scene_t bapi_scene_manager_get_scene(bapi_scene_manager_t manager, const char *name);
void		 bapi_scene_manager_update(bapi_scene_manager_t manager, float delta_time);
void		 bapi_scene_manager_render(bapi_scene_manager_t manager);

bapi_texture_t bapi_texture_load(const char *filepath);
void		   bapi_texture_destroy(bapi_texture_t texture);
void		   bapi_texture_render(bapi_texture_t texture, float x, float y);
void		   bapi_texture_render_ex(bapi_texture_t texture, float x, float y, float w, float h);
void		   bapi_texture_get_size(bapi_texture_t texture, int *w, int *h);
void		   bapi_texture_cache_clear(void);
bapi_texture_t bapi_texture_from_file(const char *filepath, int *out_w, int *out_h);

int			 bapi_video_init(void);
void		 bapi_video_cleanup(void);
bapi_video_t bapi_video_load(const char *filepath);
void		 bapi_video_free(bapi_video_t video);
int			 bapi_video_play(bapi_video_t video);
void		 bapi_video_pause(bapi_video_t video);
void		 bapi_video_stop(bapi_video_t video);
void		 bapi_video_render(bapi_video_t video, int x, int y, int w, int h);
void bapi_video_render_fit(bapi_video_t video, int area_x, int area_y, int area_w, int area_h);
void bapi_video_render_center(bapi_video_t video, int window_w, int window_h);
void bapi_video_set_loop(bapi_video_t video, int loop);
void bapi_video_set_volume(bapi_video_t video, float volume);
int	 bapi_video_is_playing(bapi_video_t video);
void bapi_video_get_size(bapi_video_t video, int *w, int *h);
void bapi_video_update(void);

bapi_scene_manager_t bapi_scene_manager_load_from_xml(const char *filepath);
bapi_level_manager_t bapi_level_manager_load_from_xml(const char *filepath);
int bapi_scene_manager_save_to_xml(bapi_scene_manager_t manager, const char *filepath);
int bapi_level_manager_save_to_xml(bapi_level_manager_t manager, const char *filepath);

const char *bridgeengine_get_version(void);
int			bridgeengine_get_version_number(void);

#ifdef __cplusplus
}
#endif

#if defined(BAPI_LOG_ENABLED)
#define BAPI_LOG_DEBUG(fmt, ...)                                                                   \
	bapi_log_message(BAPI_LOG_LEVEL_DEBUG, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define BAPI_LOG_INFO(fmt, ...)                                                                    \
	bapi_log_message(BAPI_LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define BAPI_LOG_WARN(fmt, ...)                                                                    \
	bapi_log_message(BAPI_LOG_LEVEL_WARN, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define BAPI_LOG_ERROR(fmt, ...)                                                                   \
	bapi_log_message(BAPI_LOG_LEVEL_ERROR, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define BAPI_LOG_CRITICAL(fmt, ...)                                                                \
	bapi_log_message(BAPI_LOG_LEVEL_CRITICAL, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define BAPI_LOG_ASSERT(condition, fmt, ...)                                                       \
	do {                                                                                           \
		if (!(condition))                                                                          \
			bapi_log_message(BAPI_LOG_LEVEL_CRITICAL, __FILE__, __LINE__, __func__,                \
							 "ASSERTION FAILED: " fmt, ##__VA_ARGS__);                             \
	} while (0)
#else
#define BAPI_LOG_DEBUG(fmt, ...)			 ((void)0)
#define BAPI_LOG_INFO(fmt, ...)				 ((void)0)
#define BAPI_LOG_WARN(fmt, ...)				 ((void)0)
#define BAPI_LOG_ERROR(fmt, ...)			 ((void)0)
#define BAPI_LOG_CRITICAL(fmt, ...)			 ((void)0)
#define BAPI_LOG_ASSERT(condition, fmt, ...) ((void)(condition))
#endif

#define BAPI_LOG_INIT_DEFAULT()                                                                    \
	do {                                                                                           \
		bapi_log_config_t config = {BAPI_LOG_LEVEL_INFO, true, false, NULL};                       \
		bapi_log_init(&config);                                                                    \
	} while (0)
