#include "BridgeEngine.h"
#include "internal/platform/platform.h"
#include <stdlib.h>

struct bapi_file_internal {
	plat_io_t *io;
};

bapi_file_t bapi_file_open(const char *path)
{
	const plat_interface_t *plat = plat_get();
	if (!plat || !path) return NULL;
	plat_io_t *io = plat->io.open_read(path);
	if (!io) return NULL;
	bapi_file_t file = (bapi_file_t)malloc(sizeof(struct bapi_file_internal));
	if (!file) {
		plat->io.close(io);
		return NULL;
	}
	file->io = io;
	return file;
}

size_t bapi_file_read(bapi_file_t file, void *buffer, size_t size)
{
	const plat_interface_t *plat = plat_get();
	if (!plat || !file || !buffer) return 0;
	return plat->io.read(file->io, buffer, size);
}

int64_t bapi_file_seek(bapi_file_t file, int64_t offset, int origin)
{
	const plat_interface_t *plat = plat_get();
	if (!plat || !file) return -1;
	return plat->io.seek(file->io, offset, origin);
}

int64_t bapi_file_tell(bapi_file_t file)
{
	const plat_interface_t *plat = plat_get();
	if (!plat || !file) return -1;
	return plat->io.tell(file->io);
}

int64_t bapi_file_size(bapi_file_t file)
{
	const plat_interface_t *plat = plat_get();
	if (!plat || !file) return -1;
	return plat->io.size(file->io);
}

void bapi_file_close(bapi_file_t file)
{
	const plat_interface_t *plat = plat_get();
	if (plat && file) {
		plat->io.close(file->io);
		free(file);
	}
}