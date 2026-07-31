#include "BridgeEngine.h"
#include "internal/platform/platform.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

struct plat_io {
	const uint8_t *data;
	size_t		   len;
	size_t		   pos;
};

static plat_io_t *open_read(const char *path)
{
	(void)path;
	plat_io_t *io = (plat_io_t *)malloc(sizeof(plat_io_t));
	if (!io) return NULL;
	io->data = (const uint8_t *)"Hello, BridgeEngine File IO API!";
	io->len	 = 31;
	io->pos	 = 0;
	return io;
}

static void io_close(plat_io_t *io)
{
	free(io);
}

static size_t io_read(plat_io_t *io, void *buf, size_t size)
{
	if (!io || !buf) return 0;
	size_t remaining = io->len - io->pos;
	size_t to_read	 = remaining < size ? remaining : size;
	memcpy(buf, io->data + io->pos, to_read);
	io->pos += to_read;
	return to_read;
}

static int64_t io_seek(plat_io_t *io, int64_t offset, int whence)
{
	if (!io) return -1;
	int64_t target;
	if (whence == 0) {
		target = offset;
	} else if (whence == 1) {
		target = (int64_t)io->pos + offset;
	} else if (whence == 2) {
		target = (int64_t)io->len + offset;
	} else {
		return -1;
	}
	if (target < 0 || target > (int64_t)io->len) return -1;
	io->pos = (size_t)target;
	return target;
}

static int64_t io_tell(plat_io_t *io)
{
	if (!io) return -1;
	return (int64_t)io->pos;
}

static int64_t io_size(plat_io_t *io)
{
	if (!io) return -1;
	return (int64_t)io->len;
}

static int expect(int condition, const char *message)
{
	if (!condition) fprintf(stderr, "%s\n", message);
	return condition;
}

int main(void)
{
	const plat_interface_t platform = {
		.io = {
			.open_read = open_read,
			.close	   = io_close,
			.read	   = io_read,
			.seek	   = io_seek,
			.tell	   = io_tell,
			.size	   = io_size,
		},
	};

	if (plat_init(&platform) != 0) return 1;

	int result = 1;

	bapi_file_t f = bapi_file_open("test.txt");
	result = result && expect(f != NULL, "bapi_file_open succeeds");

	if (f) {
		int64_t sz = bapi_file_size(f);
		result = result && expect(sz == 31, "bapi_file_size returns correct size");

		char buf[64] = {0};
		size_t n	 = bapi_file_read(f, buf, 5);
		result = result && expect(n == 5 && memcmp(buf, "Hello", 5) == 0,
			"bapi_file_read reads first 5 bytes");

		int64_t pos = bapi_file_tell(f);
		result = result && expect(pos == 5, "bapi_file_tell returns 5 after reading 5");

		int64_t seek_ret = bapi_file_seek(f, 7, 0);
		result = result && expect(seek_ret == 7, "bapi_file_seek to offset 7");

		memset(buf, 0, sizeof(buf));
		n = bapi_file_read(f, buf, sizeof(buf) - 1);
		result = result && expect(n > 0, "bapi_file_read after seek");

		seek_ret = bapi_file_seek(f, -3, 2);
		result = result && expect(seek_ret == 28, "bapi_file_seek from end");

		memset(buf, 0, sizeof(buf));
		n = bapi_file_read(f, buf, sizeof(buf) - 1);
		result = result && expect(n == 3 && memcmp(buf, "PI!", 3) == 0,
			"bapi_file_read at end offset");

		bapi_file_close(f);
		result = result && expect(1, "bapi_file_close completes");
	}

	f = bapi_file_open(NULL);
	result = result && expect(f == NULL, "bapi_file_open(NULL) returns NULL");

	plat_shutdown();
	return result ? 0 : 1;
}
