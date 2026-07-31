#include "BridgeEngine.h"
#include <stdio.h>
#include <string.h>

static int expect(int condition, const char *message)
{
	if (!condition) fprintf(stderr, "%s\n", message);
	return condition;
}

int main(int argc, char **argv)
{
	char				   scene_path[512];
	char				   level_path[512];
	bapi_scene_callbacks_t scene_callbacks = {0};
	bapi_level_callbacks_t level_callbacks = {0};
	if (argc != 2) return 1;
	snprintf(scene_path, sizeof(scene_path), "%s/scenes.xml", argv[1]);
	snprintf(level_path, sizeof(level_path), "%s/levels.xml", argv[1]);

	bapi_scene_manager_t scenes = bapi_scene_manager_create();
	bapi_level_manager_t levels = bapi_level_manager_create();
	if (!expect(scenes != NULL && levels != NULL, "managers are created")) return 1;
	bapi_scene_manager_add_scene(scenes, bapi_scene_create("menu", scene_callbacks));
	bapi_scene_manager_add_scene(scenes, bapi_scene_create("game", scene_callbacks));
	bapi_level_manager_add_level(levels, bapi_level_create("forest", 3, level_callbacks));
	if (!expect(bapi_scene_manager_save_to_xml(scenes, scene_path) == 0, "scenes save")) return 1;
	if (!expect(bapi_level_manager_save_to_xml(levels, level_path) == 0, "levels save")) return 1;
	bapi_scene_manager_destroy(scenes);
	bapi_level_manager_destroy(levels);

	scenes = bapi_scene_manager_load_from_xml(scene_path);
	levels = bapi_level_manager_load_from_xml(level_path);
	int result =
		expect(bapi_scene_manager_get_scene(scenes, "menu") != NULL, "scene round trip") &&
		expect(bapi_level_manager_get_level_by_index(levels, 3) != NULL,
			   "level index round trip") &&
		expect(strcmp(bapi_level_get_name(bapi_level_manager_get_level_by_index(levels, 3)),
					  "forest") == 0,
			   "level name round trip");
	bapi_scene_manager_destroy(scenes);
	bapi_level_manager_destroy(levels);
	return result ? 0 : 1;
}
