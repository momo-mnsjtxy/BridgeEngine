#define BAPI_LOG_ENABLED

#include "BridgeEngine.h"

#include <stdio.h>

#define WINDOW_WIDTH  1024
#define WINDOW_HEIGHT 768

#define SCENE_COUNT 3

static bapi_ui_t	  g_ui[SCENE_COUNT];
static const char	 *g_scene_files[SCENE_COUNT] = {"ui/menu.xml", "ui/game.xml", "ui/settings.xml"};
static const char	 *g_scene_roots[SCENE_COUNT] = {"menu", "game", "settings"};
static int			  g_scene = 0;

static void switch_scene(int index)
{
	if (index < 0 || index >= SCENE_COUNT) return;
	g_scene = index;
	for (int i = 0; i < SCENE_COUNT; i++) {
		bapi_ui_component_t root = bapi_ui_find(g_ui[i], g_scene_roots[i]);
		if (root) bapi_ui_component_set_visible(root, i == index);
	}
}

static void handle_key(uint8_t key)
{
	switch (key) {
	case '1': switch_scene(0); break;
	case '2': switch_scene(1); break;
	case '3': switch_scene(2); break;
	default: break;
	}
}

int main(int argc, char *argv[])
{
	(void)argc;
	(void)argv;

	BAPI_LOG_INIT_DEFAULT();

	if (bapi_engine_init("__PROJECT_NAME__", WINDOW_WIDTH, WINDOW_HEIGHT) != 0) {
		BAPI_LOG_ERROR("[Engine] init failed");
		return 1;
	}

	bapi_mouse_init();
	bapi_input_init();
	bapi_text_init();

	for (int i = 0; i < SCENE_COUNT; i++) {
		g_ui[i] = bapi_ui_load_from_xml(g_scene_files[i]);
		if (g_ui[i] == NULL) BAPI_LOG_WARN("[UI] failed to load %s", g_scene_files[i]);
	}
	switch_scene(0);

	bool running = true;
	while (running) {
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
					handle_key(key);
			}
			bapi_mouse_handle_event(&event);
			bapi_ui_update(g_ui[g_scene], &event);
			if (bapi_ui_was_clicked(g_ui[g_scene], "start_button"))
				BAPI_LOG_INFO("[UI] start button clicked");
		}

		bapi_render_clear();
		bapi_set_render_color(bapi_color(30, 30, 40, 255));

		bapi_ui_render(g_ui[g_scene]);

		bapi_mouse_render();
		bapi_render_present();
	}

	for (int i = 0; i < SCENE_COUNT; i++)
		if (g_ui[i]) bapi_ui_destroy(g_ui[i]);
	bapi_text_cleanup();
	bapi_input_cleanup();
	bapi_mouse_cleanup();
	bapi_engine_quit();
	return 0;
}
