#include "BridgeEngine.h"
#include "internal/platform/platform.h"
#include <stdlib.h>

/*
 * XJ380 stub: the RZip reader (thirdparty/rzip/rz_lib.c) depends on POSIX
 * facilities (sys/stat.h, fseeko/ftello, mkdir, time) that do not exist under
 * the XJ380 freestanding toolchain, so resource packs are unsupported there.
 * Every call fails loudly once, per the backend-stub convention.
 */

static int pack_unsupported_warning_logged;

static void warn_pack_unsupported_once(const plat_interface_t *plat)
{
	if (!pack_unsupported_warning_logged && plat != NULL && plat->core.log_warn != NULL) {
		plat->core.log_warn("Resource packs are not supported by this platform");
		pack_unsupported_warning_logged = 1;
	}
}

bapi_pack_t bapi_pack_open(const char *path)
{
	(void)path;
	const plat_interface_t *plat = plat_get();
	warn_pack_unsupported_once(plat);
	return NULL;
}

void bapi_pack_close(bapi_pack_t pack)
{
	(void)pack;
}

int bapi_pack_file_count(bapi_pack_t pack)
{
	(void)pack;
	const plat_interface_t *plat = plat_get();
	warn_pack_unsupported_once(plat);
	return -1;
}

const char *bapi_pack_file_name(bapi_pack_t pack, int index)
{
	(void)pack;
	(void)index;
	const plat_interface_t *plat = plat_get();
	warn_pack_unsupported_once(plat);
	return NULL;
}

int bapi_pack_find_file(bapi_pack_t pack, const char *name)
{
	(void)pack;
	(void)name;
	const plat_interface_t *plat = plat_get();
	warn_pack_unsupported_once(plat);
	return -1;
}

int64_t bapi_pack_file_size(bapi_pack_t pack, const char *name)
{
	(void)pack;
	(void)name;
	const plat_interface_t *plat = plat_get();
	warn_pack_unsupported_once(plat);
	return -1;
}

size_t bapi_pack_read_file(bapi_pack_t pack, const char *name, void *buffer, size_t buffer_size)
{
	(void)pack;
	(void)name;
	(void)buffer;
	(void)buffer_size;
	const plat_interface_t *plat = plat_get();
	warn_pack_unsupported_once(plat);
	return 0;
}

uint8_t *bapi_pack_read_file_alloc(bapi_pack_t pack, const char *name, size_t *out_size)
{
	(void)pack;
	(void)name;
	(void)out_size;
	const plat_interface_t *plat = plat_get();
	warn_pack_unsupported_once(plat);
	return NULL;
}

bapi_pack_stream_t bapi_pack_stream_open(bapi_pack_t pack, const char *name)
{
	(void)pack;
	(void)name;
	const plat_interface_t *plat = plat_get();
	warn_pack_unsupported_once(plat);
	return NULL;
}

size_t bapi_pack_stream_read(bapi_pack_stream_t stream, void *buffer, size_t size)
{
	(void)stream;
	(void)buffer;
	(void)size;
	const plat_interface_t *plat = plat_get();
	warn_pack_unsupported_once(plat);
	return 0;
}

int64_t bapi_pack_stream_seek(bapi_pack_stream_t stream, int64_t offset, int whence)
{
	(void)stream;
	(void)offset;
	(void)whence;
	const plat_interface_t *plat = plat_get();
	warn_pack_unsupported_once(plat);
	return -1;
}

int64_t bapi_pack_stream_tell(bapi_pack_stream_t stream)
{
	(void)stream;
	const plat_interface_t *plat = plat_get();
	warn_pack_unsupported_once(plat);
	return -1;
}

int64_t bapi_pack_stream_size(bapi_pack_stream_t stream)
{
	(void)stream;
	const plat_interface_t *plat = plat_get();
	warn_pack_unsupported_once(plat);
	return -1;
}

void bapi_pack_stream_close(bapi_pack_stream_t stream)
{
	(void)stream;
}
