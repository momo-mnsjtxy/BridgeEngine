#pragma once

#include "scene/scene.h"
#include "level/level.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


bapi_scene_manager_t bapi_scene_manager_load_from_xml(const char* filepath);
bapi_level_manager_t bapi_level_manager_load_from_xml(const char* filepath);

int bapi_scene_manager_save_to_xml(bapi_scene_manager_t manager, const char* filepath);
int bapi_level_manager_save_to_xml(bapi_level_manager_t manager, const char* filepath);

#ifdef __cplusplus
}
#endif
