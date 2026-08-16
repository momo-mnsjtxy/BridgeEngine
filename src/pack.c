#include "BridgeEngine.h"
#include "rz_lib.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

/*
 * Resource pack reader built on the vendored RZip library (thirdparty/rzip).
 *
 * Safety notes:
 * - rz_read_file() (rz_lib.c) does not validate offset/count against the
 *   entry's comp_size, so every read here is clamped to the entry's stored
 *   size before calling into the library.
 * - RZip keeps one shared FILE* per archive and a process-global registry of
 *   open archives, so this API is single-threaded by contract: one handle
 *   per pack, handles are not copied.
 * - The library is store-only today (comp_size == dcom_size); when real
 *   compression lands, read_file/read_file_alloc keep returning the logical
 *   (decompressed) bytes and this wrapper must switch to the decompressor.
 */

struct bapi_pack_internal {
	rz_t rz;
};

/* rz_read_file()'s count parameter is an int; read large entries in chunks. */
#define PACK_READ_CHUNK 65536u

static rz_file_t *pack_find_node(bapi_pack_t pack, const char *name)
{
	if (!pack || !pack->rz.header || !pack->rz.index || !name) return NULL;
	for (rz_file_t *node = pack->rz.index->head; node != NULL; node = node->next) {
		if (strcmp(rz_get_file_name(node), name) == 0) {
			return node;
		}
	}
	return NULL;
}

/* Copy up to `capacity` bytes of the entry's stored data into `dest` in
 * bounded chunks; returns the number of bytes copied (partial on I/O error). */
static size_t pack_copy_file(rz_file_t *node, uint8_t *dest, size_t capacity)
{
	uint64_t total = rz_get_file_comp_size(node);
	uint64_t want	= total < (uint64_t)capacity ? total : (uint64_t)capacity;
	size_t	 done	= 0;

	while (done < want) {
		/* rz_read_file() takes a `long` offset; on LLP64 platforms entries
		 * past LONG_MAX bytes are not addressable through it, so stop there. */
		if ((uint64_t)done > (uint64_t)LONG_MAX) break;

		size_t chunk = (size_t)(want - done);
		if (chunk > PACK_READ_CHUNK) chunk = PACK_READ_CHUNK;

		uint8_t *part = rz_read_file(node, (long)done, (int)chunk);
		if (!part) break;
		memcpy(dest + done, part, chunk);
		free(part);
		done += chunk;
	}

	return done;
}

bapi_pack_t bapi_pack_open(const char *path)
{
	if (!path || !path[0]) return NULL;

	rz_t rz = rz_open(path);
	if (rz.header == NULL) {
		return NULL;
	}

	bapi_pack_t pack = (bapi_pack_t)malloc(sizeof(struct bapi_pack_internal));
	if (!pack) {
		rz_close(&rz);
		return NULL;
	}
	pack->rz = rz;
	return pack;
}

void bapi_pack_close(bapi_pack_t pack)
{
	if (!pack) return;
	rz_close(&pack->rz);
	free(pack);
}

int bapi_pack_file_count(bapi_pack_t pack)
{
	if (!pack || !pack->rz.header) return -1;
	uint32_t count = pack->rz.header->file_count;
	return count > (uint32_t)INT_MAX ? INT_MAX : (int)count;
}

const char *bapi_pack_file_name(bapi_pack_t pack, int index)
{
	if (!pack || !pack->rz.index || index < 0) return NULL;
	rz_file_t *node = pack->rz.index->head;
	for (int i = 0; node != NULL; node = node->next, i++) {
		if (i == index) {
			return rz_get_file_name(node);
		}
	}
	return NULL;
}

int bapi_pack_find_file(bapi_pack_t pack, const char *name)
{
	if (!pack || !pack->rz.index || !name) return -1;
	int		   index = 0;
	rz_file_t *node  = pack->rz.index->head;
	for (; node != NULL; node = node->next, index++) {
		if (strcmp(rz_get_file_name(node), name) == 0) {
			return index;
		}
	}
	return -1;
}

int64_t bapi_pack_file_size(bapi_pack_t pack, const char *name)
{
	rz_file_t *node = pack_find_node(pack, name);
	if (!node) return -1;
	/* Logical (decompressed) size; equals the stored size until compression
	 * is implemented. Sizes above INT64_MAX would wrap; unreachable in
	 * practice given the format's 4 GiB index_offset cap. */
	return (int64_t)rz_get_file_decomp_size(node);
}

size_t bapi_pack_read_file(bapi_pack_t pack, const char *name, void *buffer, size_t buffer_size)
{
	if (!pack || !name || !buffer || buffer_size == 0) return 0;
	rz_file_t *node = pack_find_node(pack, name);
	if (!node) return 0;
	return pack_copy_file(node, (uint8_t *)buffer, buffer_size);
}

uint8_t *bapi_pack_read_file_alloc(bapi_pack_t pack, const char *name, size_t *out_size)
{
	rz_file_t *node = pack_find_node(pack, name);
	if (!node) return NULL;

	uint64_t size = rz_get_file_comp_size(node);
	if (size > (uint64_t)SIZE_MAX) return NULL;

	/* malloc(1) for empty entries: malloc(0) is not portable. */
	uint8_t *buffer = (uint8_t *)malloc(size > 0 ? (size_t)size : 1);
	if (!buffer) return NULL;

	size_t got = pack_copy_file(node, buffer, (size_t)size);
	if (got != (size_t)size) {
		free(buffer);
		return NULL;
	}

	if (out_size) *out_size = (size_t)size;
	return buffer;
}
