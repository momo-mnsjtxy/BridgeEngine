#define BAPI_LOG_ENABLED

#include "BridgeEngine.h"

#include <stdio.h>
#include <string.h>

#define WINDOW_WIDTH  1024
#define WINDOW_HEIGHT 768
#define WORLD_WIDTH	  1600
#define WORLD_HEIGHT  1200
#define CENTER_X	  (WINDOW_WIDTH / 2)
#define CENTER_Y	  (WINDOW_HEIGHT / 2)
#define NUM_BALLS	  5

#define COLOR_WHITE		   bapi_color(255, 255, 255, 255)
#define COLOR_RED		   bapi_color(255, 0, 0, 255)
#define COLOR_GREEN		   bapi_color(0, 255, 0, 255)
#define COLOR_CYAN		   bapi_color(0, 255, 255, 255)
#define COLOR_MAGENTA	   bapi_color(255, 0, 255, 255)
#define COLOR_YELLOW	   bapi_color(255, 255, 0, 255)
#define COLOR_ORANGE	   bapi_color(255, 165, 0, 255)
#define COLOR_GRAY		   bapi_color(128, 128, 128, 255)
#define COLOR_DARK_BG	   bapi_color(30, 30, 40, 255)
#define COLOR_PANEL_BG	   bapi_color(40, 40, 60, 255)
#define COLOR_PANEL_BORDER bapi_color(80, 80, 120, 255)

typedef struct {
	float		  x, y;
	float		  vx, vy;
	float		  radius;
	bapi_color_t  color;
	bapi_circle_t bounds;
} Ball;

static void ball_update(Ball *ball, float w, float h)
{
	ball->x += ball->vx;
	ball->y += ball->vy;

	if (ball->x - ball->radius < 0) {
		ball->x	 = ball->radius;
		ball->vx = -ball->vx;
	}
	if (ball->x + ball->radius > w) {
		ball->x	 = w - ball->radius;
		ball->vx = -ball->vx;
	}
	if (ball->y - ball->radius < 0) {
		ball->y	 = ball->radius;
		ball->vy = -ball->vy;
	}
	if (ball->y + ball->radius > h) {
		ball->y	 = h - ball->radius;
		ball->vy = -ball->vy;
	}
}

static void ball_draw(const Ball *ball)
{
	bapi_fill_circle(ball->x, ball->y, ball->radius, ball->color);
}

static bapi_scene_manager_t g_scene_manager = NULL;
static bapi_level_manager_t g_level_manager = NULL;
static bapi_button_t	   *g_demo_button	= NULL;
static bapi_sound_t			g_test_sound	= NULL;
static bapi_video_t			g_test_video	= NULL;
static bapi_texture_t		g_logo_texture	= NULL;
static bapi_camera_t		g_camera;

static Ball g_balls[NUM_BALLS] = {
	{100, 100, 3, 2, 30, {255, 80, 80, 255}},	   {200, 200, -2, 3, 25, {80, 255, 80, 255}},
	{300, 150, 2.5, -2, 35, {80, 80, 255, 255}},   {400, 300, -3, -1.5, 28, {255, 255, 80, 255}},
	{500, 200, 2.5, 1.5, 32, {255, 80, 255, 255}},
};

static bapi_rect_t g_player	   = {400, 300, 40, 40};
static bapi_vec2_t g_player_vel = {0, 0};

static int	  g_collision_count = 0;
static char	  g_collision_text[64] = "";
static int	  g_fps				   = 0;

static void draw_panel(int x, int y, int w, int h, const char *title)
{
	bapi_fill_rect(x, y, w, h, COLOR_PANEL_BG);
	bapi_draw_rect(x, y, w, h, COLOR_PANEL_BORDER);
	if (title != NULL) {
		float tw, th;
		bapi_get_text_size(title, 20, &tw, &th);
		bapi_draw_text(title, (float)(x + (int)(w - tw) / 2), (float)y + 10, 20, COLOR_WHITE);
	}
}

static void draw_header(void)
{
	bapi_fill_rect(0, 0, (float)WINDOW_WIDTH, 50, COLOR_PANEL_BG);
	bapi_draw_text("BridgeEngine v1.0", 20, 12, 28, COLOR_WHITE);
	bapi_draw_text("Scene & Level Management Demo", (float)CENTER_X - 140, 15, 20, COLOR_CYAN);

	char fps_text[32];
	snprintf(fps_text, sizeof(fps_text), "FPS: %d", g_fps);
	bapi_draw_text(fps_text, (float)WINDOW_WIDTH - 120, 15, 20, COLOR_YELLOW);
}

static void draw_help_panel(void)
{
	draw_panel(WINDOW_WIDTH - 260, 100, 250, 270, "Controls");
	bapi_draw_text("1 - Menu Scene", WINDOW_WIDTH - 245, 140, 18, COLOR_CYAN);
	bapi_draw_text("2 - Game Scene", WINDOW_WIDTH - 245, 165, 18, COLOR_GREEN);
	bapi_draw_text("3 - Settings Scene", WINDOW_WIDTH - 245, 190, 18, COLOR_ORANGE);
	bapi_draw_text("4 - Video Scene", WINDOW_WIDTH - 245, 215, 18, COLOR_MAGENTA);
	bapi_draw_text("5 - Input/Camera Scene", WINDOW_WIDTH - 245, 240, 18, COLOR_YELLOW);
	bapi_draw_text("N - Next Level", WINDOW_WIDTH - 245, 275, 18, COLOR_YELLOW);
	bapi_draw_text("P - Prev Level", WINDOW_WIDTH - 245, 300, 18, COLOR_YELLOW);
	bapi_draw_text("WASD/Arrows - Move", WINDOW_WIDTH - 245, 325, 18, COLOR_CYAN);
	bapi_draw_text("ESC - Exit", WINDOW_WIDTH - 245, 350, 18, COLOR_RED);
}

static void draw_color_palette(void)
{
	const int   size	  = 50;
	const int   gap		  = 5;
	const int   start_x	  = 50;
	const int   y		  = WINDOW_HEIGHT - 80;
	const uint32_t colors[] = {0xFF0000FF, 0x00FF00FF, 0x0000FFFF, 0xFFFF00FF,
							   0xFF00FFFF, 0x00FFFFFF, 0xFFFFFFFF, 0x808080FF};

	for (int i = 0; i < (int)(sizeof(colors) / sizeof(colors[0])); i++) {
		int x = start_x + i * (size + gap);
		bapi_fill_rect((float)x, (float)y, (float)size, (float)size,
					   bapi_color_from_hex(colors[i]));
		bapi_draw_rect((float)x, (float)y, (float)size, (float)size, COLOR_WHITE);
	}
}

static void draw_shapes_demo(void)
{
	draw_panel(50, 450, 400, 150, "Shapes Demo");
	bapi_draw_circle(120, 520, 35, COLOR_RED);
	bapi_fill_circle(200, 520, 30, COLOR_GREEN);
	bapi_draw_polygon(300, 520, 30, 3, COLOR_YELLOW);
	bapi_fill_polygon(380, 520, 30, 4, COLOR_CYAN);
}

static void draw_collision_info(void)
{
	draw_panel(50, 350, 300, 80, "Collision");
	bapi_draw_text(g_collision_text, 60, 390, 18, COLOR_WHITE);
}

static void on_scene_menu_enter(bapi_scene_t scene)
{
	(void)scene;
	BAPI_LOG_INFO("[Scene] Menu entered");
}

static void on_scene_menu_exit(bapi_scene_t scene)
{
	(void)scene;
}

static void on_scene_menu_update(bapi_scene_t scene, float dt)
{
	(void)scene;
	(void)dt;
}

static void on_scene_menu_render(bapi_scene_t scene)
{
	(void)scene;
	bapi_draw_text("MENU", (float)CENTER_X - 50, 150, 48, COLOR_YELLOW);
	bapi_draw_text("Welcome to BridgeEngine Demo!", (float)CENTER_X - 150, 220, 24, COLOR_WHITE);
}

static void on_scene_game_enter(bapi_scene_t scene)
{
	(void)scene;
	BAPI_LOG_INFO("[Scene] Game entered");
}

static void on_scene_game_exit(bapi_scene_t scene)
{
	(void)scene;
}

static void on_scene_game_update(bapi_scene_t scene, float dt)
{
	(void)scene;
	(void)dt;
	for (int i = 0; i < NUM_BALLS; i++) ball_update(&g_balls[i], WORLD_WIDTH, WORLD_HEIGHT);
}

static void on_scene_game_render(bapi_scene_t scene)
{
	(void)scene;
	bapi_draw_text("GAME", (float)CENTER_X - 40, 150, 48, COLOR_GREEN);
	for (int i = 0; i < NUM_BALLS; i++) ball_draw(&g_balls[i]);
	draw_panel(600, 450, 200, 150, "Circles");
	bapi_draw_circle(680, 520, 35, COLOR_RED);
	bapi_fill_circle(750, 520, 30, COLOR_GREEN);
}

static void on_scene_settings_enter(bapi_scene_t scene)
{
	(void)scene;
	BAPI_LOG_INFO("[Scene] Settings entered");
}

static void on_scene_settings_exit(bapi_scene_t scene)
{
	(void)scene;
}

static void on_scene_settings_update(bapi_scene_t scene, float dt)
{
	(void)scene;
	(void)dt;
}

static void on_scene_settings_render(bapi_scene_t scene)
{
	(void)scene;
	bapi_draw_text("SETTINGS", (float)CENTER_X - 70, 150, 48, COLOR_ORANGE);
	draw_panel(CENTER_X - 200, 220, 400, 250, "Configuration");
	bapi_draw_text("Audio Volume: 80%", (float)CENTER_X - 80, 280, 20, COLOR_WHITE);
	bapi_fill_rect((float)CENTER_X - 150, 310, 240, 20, COLOR_GRAY);
	bapi_fill_rect((float)CENTER_X - 150, 310, 192, 20, COLOR_GREEN);
	bapi_draw_text("Graphics: High", (float)CENTER_X - 60, 360, 20, COLOR_WHITE);
	bapi_draw_text("Fullscreen: Off", (float)CENTER_X - 70, 400, 20, COLOR_WHITE);
}

static void on_scene_video_enter(bapi_scene_t scene)
{
	(void)scene;
	BAPI_LOG_INFO("[Scene] Video entered");
	if (g_test_video != NULL) bapi_video_play(g_test_video);
}

static void on_scene_video_exit(bapi_scene_t scene)
{
	(void)scene;
	if (g_test_video != NULL) bapi_video_stop(g_test_video);
}

static void on_scene_video_update(bapi_scene_t scene, float dt)
{
	(void)scene;
	(void)dt;
}

static void on_scene_video_render(bapi_scene_t scene)
{
	(void)scene;
	bapi_draw_text("VIDEO", (float)CENTER_X - 40, 50, 48, COLOR_MAGENTA);

	if (g_test_video == NULL) {
		bapi_draw_text("Video not loaded!", (float)CENTER_X - 80, (float)CENTER_Y, 24, COLOR_RED);
		return;
	}

	int vw, vh;
	bapi_video_get_size(g_test_video, &vw, &vh);
	bapi_video_render_fit(g_test_video, 0, 100, WINDOW_WIDTH, WINDOW_HEIGHT - 250);

	draw_panel(50, WINDOW_HEIGHT - 150, 300, 100, "Video Info");
	bapi_draw_text("File: XINGJILOGE.mp4", 70, (float)WINDOW_HEIGHT - 115, 16, COLOR_WHITE);
	char size_text[64];
	snprintf(size_text, sizeof(size_text), "Size: %dx%d", vw, vh);
	bapi_draw_text(size_text, 70, (float)WINDOW_HEIGHT - 90, 16, COLOR_WHITE);
	bapi_draw_text("Status: Playing", 70, (float)WINDOW_HEIGHT - 65, 16, COLOR_GREEN);
}

static void on_scene_input_enter(bapi_scene_t scene)
{
	(void)scene;
	BAPI_LOG_INFO("[Scene] Input/Camera demo entered");
}

static void on_scene_input_exit(bapi_scene_t scene)
{
	(void)scene;
}

static void on_scene_input_update(bapi_scene_t scene, float dt)
{
	(void)scene;

	float speed	   = 300.0f * dt;
	g_player_vel.x = 0;
	g_player_vel.y = 0;

	if (bapi_is_key_down(KEY_SHIFT)) speed *= 2.5f;
	if (bapi_is_key_down('w') || bapi_is_key_down('W')) g_player_vel.y = -speed;
	if (bapi_is_key_down('s') || bapi_is_key_down('S')) g_player_vel.y = speed;
	if (bapi_is_key_down('a') || bapi_is_key_down('A')) g_player_vel.x = -speed;
	if (bapi_is_key_down('d') || bapi_is_key_down('D')) g_player_vel.x = speed;

	g_player.x += g_player_vel.x;
	g_player.y += g_player_vel.y;

	g_player.x = bapi_clamp(g_player.x, 0, WORLD_WIDTH - g_player.w);
	g_player.y = bapi_clamp(g_player.y, 0, WORLD_HEIGHT - g_player.h);

	float target_cam_x = g_player.x + g_player.w * 0.5f;
	float target_cam_y = g_player.y + g_player.h * 0.5f;
	g_camera.x		   = bapi_lerp(g_camera.x, target_cam_x, 0.08f);
	g_camera.y		   = bapi_lerp(g_camera.y, target_cam_y, 0.08f);

	if (bapi_is_key_pressed('z') || bapi_is_key_pressed('Z')) {
		g_camera.zoom = bapi_clamp(g_camera.zoom + 0.2f, 0.3f, 3.0f);
	}
	if (bapi_is_key_pressed('x') || bapi_is_key_pressed('X')) {
		g_camera.zoom = bapi_clamp(g_camera.zoom - 0.2f, 0.3f, 3.0f);
	}

	bapi_rect_t player_rect = {g_player.x + 5, g_player.y + 5, g_player.w - 10, g_player.h - 10};
	g_collision_count		= 0;
	for (int i = 0; i < NUM_BALLS; i++) {
		bapi_circle_t ball_c = bapi_circle(g_balls[i].x, g_balls[i].y, g_balls[i].radius);
		if (bapi_circle_intersects_rect(ball_c, player_rect)) g_collision_count++;
	}
	snprintf(g_collision_text, sizeof(g_collision_text), "Collisions: %d  |  Zoom: %.1f",
			 g_collision_count, g_camera.zoom);
}

static void on_scene_input_render(bapi_scene_t scene)
{
	(void)scene;

	bapi_rect_t view;
	bapi_camera_get_view_rect(&g_camera, &view);

	bapi_vec2_t player_screen =
		bapi_camera_world_to_screen_v(&g_camera, bapi_vec2(g_player.x, g_player.y));
	bapi_vec2_t player_size = bapi_camera_world_to_screen_v(
		&g_camera, bapi_vec2(g_player.x + g_player.w, g_player.y + g_player.h));
	float ps_w = player_size.x - player_screen.x;
	float ps_h = player_size.y - player_screen.y;

	for (int i = 0; i < NUM_BALLS; i++) {
		bapi_vec2_t bs =
			bapi_camera_world_to_screen_v(&g_camera, bapi_vec2(g_balls[i].x, g_balls[i].y));
		bapi_fill_circle(bs.x, bs.y, g_balls[i].radius * g_camera.zoom, g_balls[i].color);
	}

	bapi_fill_rect(player_screen.x, player_screen.y, ps_w, ps_h, COLOR_CYAN);
	bapi_draw_rect(player_screen.x, player_screen.y, ps_w, ps_h, COLOR_WHITE);

	bapi_texture_render(g_logo_texture, 10, 10);

	bapi_fill_rect(0, 550, WINDOW_WIDTH, 218, COLOR_DARK_BG);
	bapi_draw_text("Input & Camera Demo", (float)CENTER_X - 120, 560, 28, COLOR_YELLOW);
	bapi_draw_text("WASD/Arrows: Move  |  SHIFT: Sprint  |  Z/X: Zoom  |  1-4: Switch scenes",
				   (float)CENTER_X - 300, 600, 18, COLOR_CYAN);

	bapi_vec2_t mouse_world;
	bapi_camera_screen_to_world(&g_camera, bapi_get_mouse_x(), bapi_get_mouse_y(), &mouse_world.x,
								&mouse_world.y);
	char info[128];
	snprintf(info, sizeof(info), "Screen: (%.0f, %.0f)  World: (%.0f, %.0f)", bapi_get_mouse_x(),
			 bapi_get_mouse_y(), mouse_world.x, mouse_world.y);
	bapi_draw_text(info, (float)CENTER_X - 180, 630, 16, COLOR_WHITE);

	snprintf(info, sizeof(info), "View: (%.0f, %.0f, %.0f, %.0f)", view.x, view.y, view.w, view.h);
	bapi_draw_text(info, (float)CENTER_X - 120, 655, 16, COLOR_GRAY);

	bapi_draw_text(g_collision_text, (float)CENTER_X - 120, 680, 18, COLOR_GREEN);

	int mx = (int)bapi_get_mouse_x();
	int my = (int)bapi_get_mouse_y();
	bapi_draw_line((float)mx - 8, (float)my, (float)mx + 8, (float)my, COLOR_WHITE);
	bapi_draw_line((float)mx, (float)my - 8, (float)mx, (float)my + 8, COLOR_WHITE);
}

static void on_level_load(bapi_level_t level)
{
	BAPI_LOG_INFO("[Level] Loaded: %s", bapi_level_get_name(level));
}

static void on_level_unload(bapi_level_t level)
{
	BAPI_LOG_INFO("[Level] Unloaded: %s", bapi_level_get_name(level));
}

static void on_level_update(bapi_level_t level, float dt)
{
	(void)level;
	(void)dt;
}

static void on_level_render(bapi_level_t level)
{
	const char	*name	  = bapi_level_get_name(level);
	int			 idx	  = bapi_level_get_index(level);
	bapi_color_t colors[] = {bapi_color(34, 139, 34, 255), bapi_color(210, 180, 140, 255),
							 bapi_color(0, 105, 148, 255)};
	draw_panel(50, 100, 200, 80, "Current Level");
	bapi_fill_rect(70, 145, 160, 25, colors[(idx - 1) % 3]);
	bapi_draw_text(name, 90, 148, 18, COLOR_WHITE);
}

static void handle_keyboard(uint8_t key)
{
	switch (key) {
	case '1':
		bapi_scene_manager_switch_scene(g_scene_manager, "menu");
		break;
	case '2':
		bapi_scene_manager_switch_scene(g_scene_manager, "game");
		break;
	case '3':
		bapi_scene_manager_switch_scene(g_scene_manager, "settings");
		break;
	case '4':
		bapi_scene_manager_switch_scene(g_scene_manager, "video");
		break;
	case '5':
		bapi_scene_manager_switch_scene(g_scene_manager, "input");
		break;
	case 'n':
	case 'N':
		bapi_level_manager_next_level(g_level_manager);
		break;
	case 'p':
	case 'P':
		bapi_level_manager_previous_level(g_level_manager);
		break;
	case ' ':
		if (g_test_sound != NULL) {
			bapi_sound_play(g_test_sound);
			BAPI_LOG_INFO("[Audio] Playing test sound");
		}
		break;
	default:
		break;
	}
}

static void setup_audio(void)
{
	if (bapi_audio_init() != 0) {
		BAPI_LOG_ERROR("[Audio] init failed");
		return;
	}
	g_test_sound = bapi_sound_load("assets/audio/test.wav");
	if (g_test_sound == NULL) {
		BAPI_LOG_ERROR("[Audio] failed to load assets/audio/test.wav");
		return;
	}
	bapi_sound_set_loop(g_test_sound, 1);
	bapi_sound_play(g_test_sound);
	BAPI_LOG_INFO("[Audio] initialized; press SPACE to play sound");
}

static void setup_video(void)
{
	if (bapi_video_init() != 0) {
		BAPI_LOG_ERROR("[Video] init failed");
		return;
	}
	g_test_video = bapi_video_load("assets/video/XINGJILOGE.mp4");
	if (g_test_video == NULL) {
		BAPI_LOG_ERROR("[Video] failed to load assets/video/XINGJILOGE.mp4");
		return;
	}
	bapi_video_set_loop(g_test_video, 1);
	BAPI_LOG_INFO("[Video] initialized; press 4 for the video scene");
}

static void setup_logo_texture(void)
{
	g_logo_texture = bapi_texture_load("assets/image/XINGJI.png");
	if (g_logo_texture != NULL) {
		BAPI_LOG_INFO("[Texture] logo loaded");
	} else {
		BAPI_LOG_WARN("[Texture] logo not found; continuing without it");
	}
}

static void setup_button(void)
{
	g_demo_button =
		bapi_create_button((float)CENTER_X - 80, (float)WINDOW_HEIGHT - 160, 160, 50, "Click Me!",
						   bapi_color(60, 130, 60, 255), bapi_color(80, 160, 80, 255),
						   bapi_color(40, 100, 40, 255), COLOR_WHITE, 22);
}

static void register_scenes(void)
{
	bapi_scene_callbacks_t menu_cb = {on_scene_menu_enter, on_scene_menu_exit, on_scene_menu_update,
									  on_scene_menu_render, NULL};
	bapi_scene_callbacks_t game_cb = {on_scene_game_enter, on_scene_game_exit, on_scene_game_update,
									  on_scene_game_render, NULL};
	bapi_scene_callbacks_t settings_cb = {on_scene_settings_enter, on_scene_settings_exit,
										  on_scene_settings_update, on_scene_settings_render, NULL};
	bapi_scene_callbacks_t video_cb	   = {on_scene_video_enter, on_scene_video_exit,
										  on_scene_video_update, on_scene_video_render, NULL};
	bapi_scene_callbacks_t input_cb	   = {on_scene_input_enter, on_scene_input_exit,
										  on_scene_input_update, on_scene_input_render, NULL};

	g_scene_manager = bapi_scene_manager_create();
	bapi_scene_manager_add_scene(g_scene_manager, bapi_scene_create("menu", menu_cb));
	bapi_scene_manager_add_scene(g_scene_manager, bapi_scene_create("game", game_cb));
	bapi_scene_manager_add_scene(g_scene_manager, bapi_scene_create("settings", settings_cb));
	bapi_scene_manager_add_scene(g_scene_manager, bapi_scene_create("video", video_cb));
	bapi_scene_manager_add_scene(g_scene_manager, bapi_scene_create("input", input_cb));
	bapi_scene_manager_switch_scene(g_scene_manager, "menu");
}

static void register_levels(void)
{
	bapi_level_callbacks_t level_cb = {on_level_load, on_level_unload, on_level_update,
									   on_level_render, NULL};

	g_level_manager = bapi_level_manager_create();
	bapi_level_manager_add_level(g_level_manager, bapi_level_create("Forest", 1, level_cb));
	bapi_level_manager_add_level(g_level_manager, bapi_level_create("Desert", 2, level_cb));
	bapi_level_manager_add_level(g_level_manager, bapi_level_create("Ocean", 3, level_cb));
	bapi_level_manager_load_level_by_index(g_level_manager, 1);
}

static void shutdown(void)
{
	BAPI_LOG_INFO("Shutting down...");

	bapi_destroy_button(g_demo_button);
	bapi_scene_manager_destroy(g_scene_manager);
	bapi_level_manager_destroy(g_level_manager);
	bapi_text_cleanup();

	if (g_test_sound != NULL) {
		bapi_sound_stop(g_test_sound);
		bapi_sound_free(g_test_sound);
	}
	bapi_audio_cleanup();

	if (g_test_video != NULL) {
		bapi_video_stop(g_test_video);
		bapi_video_free(g_test_video);
	}
	bapi_video_cleanup();

	if (g_logo_texture != NULL) bapi_texture_destroy(g_logo_texture);

	bapi_input_cleanup();
	bapi_mouse_cleanup();
	bapi_engine_quit();

	BAPI_LOG_INFO("Cleanup complete. Goodbye!");
}

static int is_menu_scene_active(void)
{
	bapi_scene_t scene = bapi_scene_manager_get_current_scene(g_scene_manager);
	const char	*name  = bapi_scene_get_name(scene);
	return name != NULL && strcmp(name, "menu") == 0;
}

int main(int argc, char *argv[])
{
	(void)argc;
	(void)argv;

	BAPI_LOG_INIT_DEFAULT();

	BAPI_LOG_INFO("========================================");
	BAPI_LOG_INFO("   BridgeEngine Feature Demo");
	BAPI_LOG_INFO("========================================");

	if (bapi_engine_init("BridgeEngine Demo", WINDOW_WIDTH, WINDOW_HEIGHT) != 0) {
		BAPI_LOG_ERROR("[Engine] init failed");
		return 1;
	}

	bapi_mouse_init();
	bapi_input_init();
	bapi_text_init();

	register_scenes();
	register_levels();
	setup_audio();
	setup_video();
	setup_logo_texture();
	setup_button();

	bapi_camera_init(&g_camera, (float)WINDOW_WIDTH, (float)WINDOW_HEIGHT);
	g_camera.x = WINDOW_WIDTH * 0.5f;
	g_camera.y = WINDOW_HEIGHT * 0.5f;

	BAPI_LOG_INFO("Demo running. Press ESC to exit.");

	bool	 running = true;
	uint32_t last_time = bapi_get_ticks();
	int		 frames	= 0;
	uint32_t fps_time = last_time;

	while (running) {
		uint32_t now = bapi_get_ticks();
		float	 dt	 = (float)(now - last_time) / 1000.0f;
		last_time	 = now;
		if (dt > 0.05f) dt = 0.05f;

		bapi_input_update();

		bapi_event_t event;
		while (bapi_poll_event(&event)) {
			int type = bapi_event_get_type(&event);
			bapi_input_handle_event(&event);
			if (type == BAPI_EVENT_QUIT) {
				running = false;
			} else if (type == BAPI_EVENT_KEY_DOWN) {
				uint8_t key = bapi_event_get_key_code(&event);
				if (key == KEY_ESC)
					running = false;
				else
					handle_keyboard(key);
			}
			bapi_mouse_handle_event(&event);
			if (bapi_button_update(g_demo_button, &event)) {
				BAPI_LOG_INFO("[Event] Button clicked!");
			}
		}

		bapi_render_clear();
		bapi_set_render_color(COLOR_DARK_BG);

		bapi_scene_manager_update(g_scene_manager, dt);
		bapi_scene_manager_render(g_scene_manager);

		bapi_level_manager_update(g_level_manager, dt);

		bapi_sound_update();
		bapi_video_update();

		draw_header();
		if (is_menu_scene_active()) {
			bapi_level_manager_render(g_level_manager);
			draw_help_panel();
			draw_color_palette();
			draw_shapes_demo();
			draw_collision_info();
			bapi_button_render(g_demo_button);
		}

		bapi_mouse_render();

		frames++;
		if (now - fps_time >= 1000) {
			g_fps	 = frames;
			frames	 = 0;
			fps_time = now;
		}

		bapi_render_present();
	}

	shutdown();
	return 0;
}
