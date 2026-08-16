#include "BridgeEngine.h"
#include "internal/scene/scene_internal.h"
#include <stdlib.h>
#include <string.h>

/* Hash the same truncated form that is stored in ->name, so that names
 * longer than MAX_NAME_LENGTH-1 stay addressable by their full input. */
static uint32_t hash_name(const char *name)
{
	char truncated[MAX_NAME_LENGTH];
	strncpy(truncated, name, MAX_NAME_LENGTH - 1);
	truncated[MAX_NAME_LENGTH - 1] = '\0';
	return bapi_hash_string(truncated);
}

/* Stored names are truncated to MAX_NAME_LENGTH-1; compare that prefix. */
static int name_matches(const char *stored, const char *input)
{
	return strncmp(stored, input, MAX_NAME_LENGTH - 1) == 0;
}

bapi_level_t bapi_level_create(const char *name, int index, bapi_level_callbacks_t callbacks)
{
	if (!name) return NULL;

	bapi_level_t level = (bapi_level_t)malloc(sizeof(struct bapi_level_internal));
	if (!level) return NULL;

	strncpy(level->name, name, MAX_NAME_LENGTH - 1);
	level->name[MAX_NAME_LENGTH - 1] = '\0';
	level->name_hash				 = hash_name(name);
	level->index					 = index;
	level->callbacks				 = callbacks;
	level->user_data				 = callbacks.user_data;
	level->is_loaded				 = 0;

	return level;
}

void bapi_level_destroy(bapi_level_t level)
{
	if (level) {
		free(level);
	}
}

const char *bapi_level_get_name(bapi_level_t level)
{
	return level ? level->name : NULL;
}

int bapi_level_get_index(bapi_level_t level)
{
	return level ? level->index : -1;
}

void *bapi_level_get_user_data(bapi_level_t level)
{
	return level ? level->user_data : NULL;
}

void bapi_level_set_user_data(bapi_level_t level, void *user_data)
{
	if (level) {
		level->user_data = user_data;
	}
}

bapi_level_manager_t bapi_level_manager_create(void)
{
	bapi_level_manager_t manager =
		(bapi_level_manager_t)malloc(sizeof(struct bapi_level_manager_internal));
	if (!manager) return NULL;

	memset(manager->levels, 0, sizeof(manager->levels));
	memset(manager->index_map, 0, sizeof(manager->index_map));
	manager->level_count   = 0;
	manager->current_level = NULL;
	manager->current_index = -1;

	return manager;
}

void bapi_level_manager_destroy(bapi_level_manager_t manager)
{
	if (manager) {
		for (int i = 0; i < manager->level_count; i++) {
			bapi_level_destroy(manager->levels[i]);
		}
		free(manager);
	}
}

int bapi_level_manager_add_level(bapi_level_manager_t manager, bapi_level_t level)
{
	if (!manager || !level) return -1;
	if (manager->level_count >= MAX_LEVELS) return -2;

	manager->levels[manager->level_count++] = level;

	if (level->index >= 0 && level->index < LEVEL_INDEX_MAP_SIZE) {
		manager->index_map[level->index] = level;
	}

	return 0;
}

static int load_level_internal(bapi_level_manager_t manager, bapi_level_t level)
{
	if (!level) return -1;

	if (manager->current_level) {
		manager->current_level->is_loaded = 0;
		if (manager->current_level->callbacks.on_unload) {
			manager->current_level->callbacks.on_unload(manager->current_level);
		}
	}

	level->is_loaded = 1;
	if (level->callbacks.on_load) {
		level->callbacks.on_load(level);
	}

	manager->current_level = level;
	manager->current_index = level->index;
	return 0;
}

int bapi_level_manager_load_level(bapi_level_manager_t manager, const char *name)
{
	if (!manager || !name) return -1;

	uint32_t target_hash = hash_name(name);

	for (int i = 0; i < manager->level_count; i++) {
		if (manager->levels[i]->name_hash == target_hash) {
			if (name_matches(manager->levels[i]->name, name)) {
				return load_level_internal(manager, manager->levels[i]);
			}
		}
	}

	return -2;
}

int bapi_level_manager_load_level_by_index(bapi_level_manager_t manager, int index)
{
	if (!manager) return -1;
	if (index < 0 || index >= LEVEL_INDEX_MAP_SIZE) return -2;

	bapi_level_t level = manager->index_map[index];
	if (!level) return -2;

	return load_level_internal(manager, level);
}

int bapi_level_manager_next_level(bapi_level_manager_t manager)
{
	if (!manager) return -1;

	int next_index = manager->current_index + 1;
	if (next_index >= LEVEL_INDEX_MAP_SIZE) return -2;

	bapi_level_t level = manager->index_map[next_index];
	if (!level) return -2;

	return load_level_internal(manager, level);
}

int bapi_level_manager_previous_level(bapi_level_manager_t manager)
{
	if (!manager) return -1;

	int prev_index = manager->current_index - 1;
	if (prev_index < 0) return -2;

	bapi_level_t level = manager->index_map[prev_index];
	if (!level) return -2;

	return load_level_internal(manager, level);
}

bapi_level_t bapi_level_manager_get_current_level(bapi_level_manager_t manager)
{
	return manager ? manager->current_level : NULL;
}

bapi_level_t bapi_level_manager_get_level(bapi_level_manager_t manager, const char *name)
{
	if (!manager || !name) return NULL;

	uint32_t target_hash = hash_name(name);

	for (int i = 0; i < manager->level_count; i++) {
		if (manager->levels[i]->name_hash == target_hash) {
			if (name_matches(manager->levels[i]->name, name)) {
				return manager->levels[i];
			}
		}
	}

	return NULL;
}

bapi_level_t bapi_level_manager_get_level_by_index(bapi_level_manager_t manager, int index)
{
	if (!manager) return NULL;
	if (index < 0 || index >= LEVEL_INDEX_MAP_SIZE) return NULL;

	return manager->index_map[index];
}

int bapi_level_manager_get_level_count(bapi_level_manager_t manager)
{
	return manager ? manager->level_count : 0;
}

void bapi_level_manager_update(bapi_level_manager_t manager, float delta_time)
{
	if (manager && manager->current_level && manager->current_level->callbacks.on_update) {
		manager->current_level->callbacks.on_update(manager->current_level, delta_time);
	}
}

void bapi_level_manager_render(bapi_level_manager_t manager)
{
	if (manager && manager->current_level && manager->current_level->callbacks.on_render) {
		manager->current_level->callbacks.on_render(manager->current_level);
	}
}

int bapi_level_persistence_count(bapi_level_manager_t manager)
{
	return manager ? manager->level_count : 0;
}

const char *bapi_level_persistence_name(bapi_level_manager_t manager, int index)
{
	if (!manager || index < 0 || index >= manager->level_count) return NULL;
	return bapi_level_get_name(manager->levels[index]);
}

int bapi_level_persistence_index(bapi_level_manager_t manager, int index)
{
	if (!manager || index < 0 || index >= manager->level_count) return -1;
	return bapi_level_get_index(manager->levels[index]);
}
