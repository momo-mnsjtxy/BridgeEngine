#include "BridgeEngine.h"
#include <stdint.h>
#include <stdlib.h>

/*
 * Whole-entry pack loaders (the full-buffer API set): read the entry into a
 * malloc'd buffer, decode from that buffer, release the buffer. The decoded
 * resource owns its own storage (see bapi_*_load_from_memory), so the entry
 * buffer is freed right after the decode.
 *
 * Pure composition over the public API: on platforms without pack support
 * (pack stub) or without the memory decode slot, the underlying calls fail
 * and these functions return NULL.
 */

bapi_sound_t bapi_sound_load_from_pack(bapi_pack_t pack, const char *name)
{
	size_t	 size = 0;
	uint8_t *data = bapi_pack_read_file_alloc(pack, name, &size);
	if (!data) return NULL;

	bapi_sound_t sound = bapi_sound_load_from_memory(data, size);
	free(data);
	return sound;
}

bapi_texture_t bapi_texture_load_from_pack(bapi_pack_t pack, const char *name)
{
	size_t	 size = 0;
	uint8_t *data = bapi_pack_read_file_alloc(pack, name, &size);
	if (!data) return NULL;

	bapi_texture_t texture = bapi_texture_load_from_memory(data, size);
	free(data);
	return texture;
}

bapi_video_t bapi_video_load_from_pack(bapi_pack_t pack, const char *name)
{
	size_t	 size = 0;
	uint8_t *data = bapi_pack_read_file_alloc(pack, name, &size);
	if (!data) return NULL;

	bapi_video_t video = bapi_video_load_from_memory(data, size);
	free(data);
	return video;
}
