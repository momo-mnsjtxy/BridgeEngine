#define BAPI_LOG_ENABLED

#include "BridgeEngine.h"

#include <stdio.h>
#include <stdlib.h>

#define WINDOW_WIDTH  1024
#define WINDOW_HEIGHT 768

#define MAX_SCENES 16

// Encryption key for .uix UI documents. Supplied by the build via
// BRIDGEENGINE_UI_KEY (see CMakeLists.txt); empty when the project does not
// encrypt its UI. bapi_ui_load_from_file treats an empty key as plain XML.
#ifndef BRIDGEENGINE_UI_KEY
#define BRIDGEENGINE_UI_KEY ""
#endif

static bapi_ui_t		g_ui[MAX_SCENES];
static const char	   *g_scene_files[MAX_SCENES];
static int				 g_scene_count = 0;
static int				 g_scene = 0;

// Load every document listed in the .bep project file (the editor writes the
// [documents] section when the project is saved). Adding or removing scenes in
// the editor therefore needs no code change in the game.
static int load_project_scenes(void)
{
	bapi_project_t project = bapi_project_load_from_bep("__PROJECT_NAME__.bep");
	if (!project) {
		BAPI_LOG_ERROR("[Scene] no __PROJECT_NAME__.bep found; falling back to ui/menu.xml");
		g_scene_files[g_scene_count] = "ui/menu.xml";
		g_ui[g_scene_count]			 = bapi_ui_load_from_file(g_scene_files[g_scene_count],
															  BRIDGEENGINE_UI_KEY);
		if (g_ui[g_scene_count]) g_scene_count++;
		return g_scene_count > 0 ? 0 : -1;
	}

	int documents = bapi_project_get_document_count(project);
	if (documents > MAX_SCENES) documents = MAX_SCENES;
	for (int i = 0; i < documents; i++) {
		const char *path = bapi_project_get_document_path(project, i);
		if (!path) continue;
		g_scene_files[g_scene_count] = path;
		g_ui[g_scene_count]			 = bapi_ui_load_from_file(path, BRIDGEENGINE_UI_KEY);
		if (g_ui[g_scene_count]) {
			g_scene_count++;
		} else {
			BAPI_LOG_WARN("[Scene] failed to load %s", path);
		}
	}
	bapi_project_destroy(project);

	if (g_scene_count == 0) BAPI_LOG_ERROR("[Scene] no documents could be loaded");
	return g_scene_count > 0 ? 0 : -1;
}

// A scene is "the scene" whenever any of its root components is visible; keep a
// single always-on root list for the switch: we toggle all roots' visibility.
static void switch_scene(int index)
{
	if (index < 0 || index >= g_scene_count) return;
	g_scene = index;
	for (int i = 0; i < g_scene_count; i++) {
		int visible = (i == index);
		int roots	= bapi_ui_get_root_count(g_ui[i]);
		for (int r = 0; r < roots; r++)
			bapi_ui_component_set_visible(bapi_ui_get_root(g_ui[i], r), visible);
	}
}

static void handle_key(uint8_t key)
{
	if (key >= '1' && key <= '9') {
		int index = key - '1';
		if (index < g_scene_count) switch_scene(index);
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

	if (load_project_scenes() != 0) {
		BAPI_LOG_ERROR("[Engine] no scenes available");
		bapi_text_cleanup();
		bapi_input_cleanup();
		bapi_mouse_cleanup();
		bapi_engine_quit();
		return 1;
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

	for (int i = 0; i < g_scene_count; i++)
		if (g_ui[i]) bapi_ui_destroy(g_ui[i]);
	bapi_text_cleanup();
	bapi_input_cleanup();
	bapi_mouse_cleanup();
	bapi_engine_quit();
	return 0;
}
