#pragma once

#include "BridgeEngine.h"

int			bapi_scene_persistence_count(bapi_scene_manager_t manager);
const char *bapi_scene_persistence_name(bapi_scene_manager_t manager, int index);

int			bapi_level_persistence_count(bapi_level_manager_t manager);
const char *bapi_level_persistence_name(bapi_level_manager_t manager, int index);
int			bapi_level_persistence_index(bapi_level_manager_t manager, int index);
