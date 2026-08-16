#include "BridgeEngine.h"
#include "internal/platform/platform.h"
#include "rz_lib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

/* Two-stage loading pipeline test with fake platform backends:
 * - memory-backed plat_io keyed by virtual file path,
 * - recording load_wav_mem/load_image_mem that decode anything,
 * - a runtime-built RZip pack for the pack-side API,
 * - video covered through the XJ380-style stub (returns NULL). */

static int fails = 0;

static int expect(int condition, const char *message)
{
	if (!condition) {
		fprintf(stderr, "FAIL: %s\n", message);
		fails++;
	}
	return condition;
}

static int test_mkdir(const char *dir)
{
#ifdef _WIN32
	return _mkdir(dir);
#else
	return mkdir(dir, 0755);
#endif
}

static int test_chdir(const char *dir)
{
#ifdef _WIN32
	return _chdir(dir);
#else
	return chdir(dir);
#endif
}

static char *test_strdup(const char *text)
{
	size_t len  = strlen(text) + 1;
	char  *copy = (char *)malloc(len);
	if (copy) memcpy(copy, text, len);
	return copy;
}

/* ------------------------------------------------------------------ */
/* Fake platform: io over virtual files, recording media decode slots. */

struct plat_io {
	const uint8_t *data;
	size_t		   len;
	size_t		   pos;
};

struct plat_renderer {
	int unused;
};

struct plat_texture {
	int width;
	int height;
};

struct plat_surface {
	int width;
	int height;
};

static struct plat_renderer fake_renderer;

plat_renderer_t bapi_internal_get_renderer(void)
{
	return &fake_renderer;
}

static const uint8_t g_disk_wav[] = "RIFF\x24\x00\x00\x00WAVE fake wav bytes for routing";
static const uint8_t g_disk_png[] = "\x89PNG\r\n\x1a\n fake png bytes for routing";
static const uint8_t g_pack_wav[] = "pack-wav-entry-bytes";
static const uint8_t g_pack_png[] = "pack-png-entry-bytes";
static const uint8_t g_pack_video[] = "pack-video-entry-bytes";
static const uint8_t g_pack_sub[] = "sub/entry/bytes 0123456789";

static const struct vfile {
	const char	   *name;
	const uint8_t *data;
	size_t			len;
} g_files[] = {
	{"disk.wav", g_disk_wav, sizeof(g_disk_wav)},
	{"disk.png", g_disk_png, sizeof(g_disk_png)},
	{"empty.bin", NULL, 0},
};

static plat_io_t *io_open_read(const char *path)
{
	if (!path) return NULL;
	for (size_t i = 0; i < sizeof(g_files) / sizeof(g_files[0]); i++) {
		if (strcmp(g_files[i].name, path) == 0) {
			plat_io_t *io = (plat_io_t *)malloc(sizeof(plat_io_t));
			if (!io) return NULL;
			io->data = g_files[i].data;
			io->len	 = g_files[i].len;
			io->pos	 = 0;
			return io;
		}
	}
	return NULL;
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
	if (to_read == 0) return 0;
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

/* --- recording audio slots --- */

static int		 g_wav_mem_calls;
static int		 g_wav_path_calls;
static uint8_t *g_last_wav_input;
static size_t	 g_last_wav_size;
static int		 g_mem_free_calls;

static int record_wav_input(const void *data, size_t size)
{
	free(g_last_wav_input);
	g_last_wav_input = NULL;
	g_last_wav_size	 = 0;
	if (size == 0) return 1;
	g_last_wav_input = (uint8_t *)malloc(size);
	if (!g_last_wav_input) return 0;
	memcpy(g_last_wav_input, data, size);
	g_last_wav_size = size;
	return 1;
}

static int fake_load_wav_mem(const void *data, size_t size, plat_audio_spec_t *spec,
							 uint8_t **buffer, uint32_t *length)
{
	g_wav_mem_calls++;
	if (!spec || !data || size == 0 || !record_wav_input(data, size)) return -1;
	spec->format   = PLAT_AUDIO_F32;
	spec->channels = 2;
	spec->freq	   = 22050;
	*buffer		   = (uint8_t *)malloc(size);
	if (!*buffer) return -1;
	memcpy(*buffer, data, size);
	*length = (uint32_t)size;
	return 0;
}

static int fake_load_wav(const char *filepath, plat_audio_spec_t *spec, uint8_t **buffer,
						 uint32_t *length)
{
	g_wav_path_calls++;
	if (!spec || !filepath) return -1;
	spec->format   = PLAT_AUDIO_F32;
	spec->channels = 1;
	spec->freq	   = 11025;
	*buffer		   = (uint8_t *)malloc(4);
	if (!*buffer) return -1;
	memset(*buffer, 0xAB, 4);
	*length = 4;
	return 0;
}

static void fake_mem_free(void *ptr)
{
	g_mem_free_calls++;
	free(ptr);
}

/* --- recording texture slots --- */

static int		 g_image_mem_calls;
static int		 g_image_path_calls;
static uint8_t *g_last_image_input;
static size_t	 g_last_image_size;
static int		 g_destroyed_texture_count;

static int record_image_input(const void *data, size_t size)
{
	free(g_last_image_input);
	g_last_image_input = NULL;
	g_last_image_size	= 0;
	if (size == 0) return 1;
	g_last_image_input = (uint8_t *)malloc(size);
	if (!g_last_image_input) return 0;
	memcpy(g_last_image_input, data, size);
	g_last_image_size = size;
	return 1;
}

static plat_surface_t *fake_load_image_mem(const void *data, size_t size)
{
	g_image_mem_calls++;
	if (!data || size == 0 || !record_image_input(data, size)) return NULL;
	plat_surface_t *surface = (plat_surface_t *)malloc(sizeof(*surface));
	if (surface) {
		surface->width	= 16;
		surface->height = 9;
	}
	return surface;
}

static plat_surface_t *fake_load_image(const char *filepath)
{
	g_image_path_calls++;
	if (!filepath) return NULL;
	plat_surface_t *surface = (plat_surface_t *)malloc(sizeof(*surface));
	if (surface) {
		surface->width	= 16;
		surface->height = 9;
	}
	return surface;
}

static void fake_destroy_surface(plat_surface_t *surface)
{
	free(surface);
}

static plat_texture_t fake_create_texture_from_surface(plat_renderer_t renderer,
														plat_surface_t *surface)
{
	if (!renderer || !surface) return NULL;
	plat_texture_t texture = (plat_texture_t)malloc(sizeof(*texture));
	if (texture) {
		texture->width	= surface->width;
		texture->height = surface->height;
	}
	return texture;
}

static void fake_destroy_texture(plat_texture_t texture)
{
	g_destroyed_texture_count++;
	free(texture);
}

static int fake_get_texture_size(plat_texture_t texture, int *width, int *height)
{
	if (!texture || !width || !height) return -1;
	*width	= texture->width;
	*height = texture->height;
	return 0;
}

/* --- platform vtable: memory stage enabled --- */

static const plat_interface_t mem_platform = {
	.capabilities = PLAT_CAPABILITY_AUDIO,
	.io =
		{
			.open_read = io_open_read,
			.close	   = io_close,
			.read	   = io_read,
			.seek	   = io_seek,
			.tell	   = io_tell,
			.size	   = io_size,
		},
	.audio =
		{
			.load_wav	 = fake_load_wav,
			.load_wav_mem = fake_load_wav_mem,
			.mem_free	 = fake_mem_free,
		},
	.texture =
		{
			.load_image				 = fake_load_image,
			.load_image_mem			 = fake_load_image_mem,
			.create_texture_from_surface = fake_create_texture_from_surface,
			.get_texture_size		 = fake_get_texture_size,
			.destroy_surface		 = fake_destroy_surface,
			.destroy_texture		 = fake_destroy_texture,
		},
};

/* --- platform vtable: path-only backend (fallback phase) --- */

static const plat_interface_t path_platform = {
	.capabilities = PLAT_CAPABILITY_AUDIO,
	.audio =
		{
			.load_wav = fake_load_wav,
			.mem_free = fake_mem_free,
		},
	.texture =
		{
			.load_image				 = fake_load_image,
			.create_texture_from_surface = fake_create_texture_from_surface,
			.get_texture_size		 = fake_get_texture_size,
			.destroy_surface		 = fake_destroy_surface,
			.destroy_texture		 = fake_destroy_texture,
		},
};

/* ------------------------------------------------------------------ */
/* RZip pack helpers (mirrors tests/pack_test.c). */

static int write_test_file(const char *name, const unsigned char *data, size_t len)
{
	FILE *f = fopen(name, "wb");
	if (!f) return -1;
	if (len > 0 && fwrite(data, 1, len, f) != len) {
		fclose(f);
		return -1;
	}
	fclose(f);
	return 0;
}

static void index_free_all(rz_index_t *index)
{
	rz_file_t *node = index->head;
	while (node) {
		rz_file_t *next = node->next;
		free(node->filename);
		free(node);
		node = next;
	}
	index->head = NULL;
	index->tail = NULL;
}

static rz_file_t *make_entry(const char *name)
{
	rz_file_t *file = (rz_file_t *)calloc(1, sizeof(*file));
	if (!file) return NULL;
	file->filename = test_strdup(name);
	if (!file->filename) {
		free(file);
		return NULL;
	}
	return file;
}

static int build_media_pack(void)
{
	if (write_test_file("sound.wav", g_pack_wav, sizeof(g_pack_wav)) != 0 ||
		write_test_file("image.png", g_pack_png, sizeof(g_pack_png)) != 0 ||
		write_test_file("video.mp4", g_pack_video, sizeof(g_pack_video)) != 0 ||
		write_test_file("empty.dat", NULL, 0) != 0 ||
		write_test_file("data/sub.bin", g_pack_sub, sizeof(g_pack_sub)) != 0) {
		return -1;
	}

	rz_index_t index = {NULL, NULL};
	rz_file_t *e1 = make_entry("sound.wav");
	rz_file_t *e2 = make_entry("image.png");
	rz_file_t *e3 = make_entry("video.mp4");
	rz_file_t *e4 = make_entry("empty.dat");
	rz_file_t *e5 = make_entry("data/sub.bin");
	if (!e1 || !e2 || !e3 || !e4 || !e5 || !rz_add_index(e1, &index) ||
		!rz_add_index(e2, &index) || !rz_add_index(e3, &index) || !rz_add_index(e4, &index) ||
		!rz_add_index(e5, &index)) {
		index_free_all(&index);
		return -1;
	}
	int created = rz_create("media.rz", &index, 0);
	index_free_all(&index);
	return created;
}

/* ------------------------------------------------------------------ */

static void test_file_read_alloc(void)
{
	size_t	 size = 0;
	uint8_t *data = bapi_file_read_alloc("disk.wav", &size);
	expect(data != NULL && size == sizeof(g_disk_wav), "file_read_alloc disk.wav size");
	expect(data && memcmp(data, g_disk_wav, sizeof(g_disk_wav)) == 0,
		   "file_read_alloc disk.wav content");
	free(data);

	data = bapi_file_read_alloc("empty.bin", &size);
	expect(data != NULL && size == 0, "file_read_alloc empty file is non-NULL with size 0");
	free(data);

	size = 0xDEAD;
	expect(bapi_file_read_alloc("missing.bin", &size) == NULL, "file_read_alloc missing fails");
	expect(size == 0xDEAD, "file_read_alloc out_size untouched on failure");

	expect(bapi_file_read_alloc(NULL, &size) == NULL, "file_read_alloc NULL path fails");
	data = bapi_file_read_alloc("disk.wav", NULL);
	expect(data != NULL, "file_read_alloc NULL out_size works");
	free(data);
}

static void test_sound_routing(void)
{
	bapi_sound_t sound = bapi_sound_load("disk.wav");
	expect(sound != NULL, "bapi_sound_load via memory stage succeeds");
	expect(g_wav_mem_calls == 1 && g_wav_path_calls == 0,
		   "bapi_sound_load routed through load_wav_mem");
	expect(g_last_wav_size == sizeof(g_disk_wav) &&
			   memcmp(g_last_wav_input, g_disk_wav, sizeof(g_disk_wav)) == 0,
		   "bapi_sound_load decoded exact file bytes");
	if (sound) {
		bapi_sound_free(sound);
	}

	expect(bapi_sound_load("missing.bin") == NULL, "bapi_sound_load missing file fails");
	expect(g_wav_mem_calls == 1, "failed read does not reach the decoder");

	sound = bapi_sound_load_from_memory(g_disk_wav, sizeof(g_disk_wav));
	expect(sound != NULL, "bapi_sound_load_from_memory succeeds");
	expect(g_wav_mem_calls == 2, "load_from_memory reached the decoder");
	expect(g_last_wav_size == sizeof(g_disk_wav) &&
			   memcmp(g_last_wav_input, g_disk_wav, sizeof(g_disk_wav)) == 0,
		   "load_from_memory passed exact bytes");
	if (sound) bapi_sound_free(sound);

	expect(bapi_sound_load_from_memory(NULL, 16) == NULL, "sound from NULL data fails");
	expect(bapi_sound_load_from_memory(g_disk_wav, 0) == NULL, "sound from size 0 fails");
	expect(g_mem_free_calls == 2, "decoded PCM buffers freed via mem_free");
}

static void test_texture_routing(void)
{
	bapi_texture_t texture = bapi_texture_load("disk.png");
	expect(texture != NULL, "bapi_texture_load via memory stage succeeds");
	expect(g_image_mem_calls == 1 && g_image_path_calls == 0,
		   "bapi_texture_load routed through load_image_mem");
	expect(g_last_image_size == sizeof(g_disk_png) &&
			   memcmp(g_last_image_input, g_disk_png, sizeof(g_disk_png)) == 0,
		   "bapi_texture_load decoded exact file bytes");
	if (texture) {
		int w = 0, h = 0;
		bapi_texture_get_size(texture, &w, &h);
		expect(w == 16 && h == 9, "texture size comes from the fake surface");
		bapi_texture_destroy(texture);
	}

	expect(bapi_texture_load("missing.png") == NULL, "bapi_texture_load missing file fails");
	expect(g_image_mem_calls == 1, "failed read does not reach the decoder");

	texture = bapi_texture_load_from_memory(g_disk_png, sizeof(g_disk_png));
	expect(texture != NULL, "bapi_texture_load_from_memory succeeds");
	expect(g_image_mem_calls == 2, "load_from_memory reached the decoder");
	if (texture) bapi_texture_destroy(texture);

	expect(bapi_texture_load_from_memory(NULL, 16) == NULL, "texture from NULL data fails");
	expect(bapi_texture_load_from_memory(g_disk_png, 0) == NULL, "texture from size 0 fails");
}

static void test_pack_composition(bapi_pack_t pack)
{
	bapi_sound_t sound = bapi_sound_load_from_pack(pack, "sound.wav");
	expect(sound != NULL, "bapi_sound_load_from_pack succeeds");
	expect(g_last_wav_size == sizeof(g_pack_wav) &&
			   memcmp(g_last_wav_input, g_pack_wav, sizeof(g_pack_wav)) == 0,
		   "pack sound decoded exact entry bytes");
	if (sound) bapi_sound_free(sound);

	expect(bapi_sound_load_from_pack(pack, "nope.wav") == NULL, "pack sound missing entry fails");
	expect(bapi_sound_load_from_pack(NULL, "sound.wav") == NULL, "pack sound NULL pack fails");

	bapi_texture_t texture = bapi_texture_load_from_pack(pack, "image.png");
	expect(texture != NULL, "bapi_texture_load_from_pack succeeds");
	expect(g_last_image_size == sizeof(g_pack_png) &&
			   memcmp(g_last_image_input, g_pack_png, sizeof(g_pack_png)) == 0,
		   "pack texture decoded exact entry bytes");
	if (texture) bapi_texture_destroy(texture);

	expect(bapi_texture_load_from_pack(pack, NULL) == NULL, "pack texture NULL name fails");

	/* video is the stub in this test: every entry point fails cleanly. */
	expect(bapi_video_load_from_pack(pack, "video.mp4") == NULL, "video stub from_pack fails");
	expect(bapi_video_load_from_pack_stream(pack, "video.mp4") == NULL,
		   "video stub from_pack_stream fails");
	expect(bapi_video_load_from_pack_stream(NULL, "video.mp4") == NULL,
		   "video stub NULL pack fails");
}

static void test_pack_streams(bapi_pack_t pack)
{
	bapi_pack_stream_t stream = bapi_pack_stream_open(pack, "data/sub.bin");
	expect(stream != NULL, "pack stream open succeeds");
	if (!stream) return;

	expect(bapi_pack_stream_size(stream) == (int64_t)sizeof(g_pack_sub), "stream size");
	expect(bapi_pack_stream_tell(stream) == 0, "stream starts at 0");

	uint8_t buffer[64];
	size_t	got = bapi_pack_stream_read(stream, buffer, 3);
	expect(got == 3 && memcmp(buffer, g_pack_sub, 3) == 0, "stream partial read");
	expect(bapi_pack_stream_tell(stream) == 3, "stream tell after read");

	expect(bapi_pack_stream_seek(stream, 1, SEEK_CUR) == 4, "stream seek CUR");
	got = bapi_pack_stream_read(stream, buffer, sizeof(buffer));
	expect(got == sizeof(g_pack_sub) - 4 && memcmp(buffer, g_pack_sub + 4, got) == 0,
		   "stream read to end");

	expect(bapi_pack_stream_seek(stream, -1, SEEK_END) == (int64_t)sizeof(g_pack_sub) - 1,
		   "stream seek END");
	got = bapi_pack_stream_read(stream, buffer, sizeof(buffer));
	expect(got == 1 && buffer[0] == g_pack_sub[sizeof(g_pack_sub) - 1], "stream read last byte");

	expect(bapi_pack_stream_seek(stream, 1000, SEEK_SET) == (int64_t)sizeof(g_pack_sub),
		   "stream seek past end clamps to size");
	expect(bapi_pack_stream_read(stream, buffer, sizeof(buffer)) == 0, "stream read at EOF is 0");

	expect(bapi_pack_stream_seek(stream, -1000, SEEK_SET) == 0, "stream negative seek clamps to 0");
	got = bapi_pack_stream_read(stream, buffer, sizeof(buffer));
	expect(got == sizeof(g_pack_sub) && memcmp(buffer, g_pack_sub, sizeof(g_pack_sub)) == 0,
		   "stream full read from 0");

	expect(bapi_pack_stream_seek(stream, 5, SEEK_CUR) == (int64_t)sizeof(g_pack_sub),
		   "stream seek CUR past end clamps");
	expect(bapi_pack_stream_seek(stream, -99, SEEK_CUR) == 0,
		   "stream seek CUR before start clamps");
	expect(bapi_pack_stream_seek(stream, 2, 99) == -1, "stream seek bad whence fails");
	expect(bapi_pack_stream_read(stream, NULL, sizeof(buffer)) == 0, "stream read NULL buffer");

	bapi_pack_stream_close(stream);

	stream = bapi_pack_stream_open(pack, "empty.dat");
	expect(stream != NULL, "pack stream open empty entry succeeds");
	if (stream) {
		expect(bapi_pack_stream_size(stream) == 0, "empty entry stream size 0");
		expect(bapi_pack_stream_read(stream, buffer, sizeof(buffer)) == 0, "empty entry read 0");
		expect(bapi_pack_stream_seek(stream, 5, SEEK_SET) == 0, "empty entry seek clamps to 0");
		bapi_pack_stream_close(stream);
	}

	expect(bapi_pack_stream_open(pack, "nope.bin") == NULL, "stream open missing entry fails");
	expect(bapi_pack_stream_open(NULL, "data/sub.bin") == NULL, "stream open NULL pack fails");
	expect(bapi_pack_stream_open(pack, NULL) == NULL, "stream open NULL name fails");
	bapi_pack_stream_close(NULL);

	expect(bapi_pack_stream_read(NULL, buffer, sizeof(buffer)) == 0, "stream read NULL fails");
	expect(bapi_pack_stream_seek(NULL, 0, SEEK_SET) == -1, "stream seek NULL fails");
	expect(bapi_pack_stream_tell(NULL) == -1, "stream tell NULL fails");
	expect(bapi_pack_stream_size(NULL) == -1, "stream size NULL fails");
}

static void test_path_fallback(void)
{
	int wav_mem_before   = g_wav_mem_calls;
	int wav_path_before  = g_wav_path_calls;
	int image_mem_before = g_image_mem_calls;
	int image_path_before = g_image_path_calls;

	size_t size = 0;
	expect(bapi_file_read_alloc("disk.wav", &size) == NULL, "fallback: no io, no file buffer");

	bapi_sound_t sound = bapi_sound_load("disk.wav");
	expect(sound != NULL, "fallback: bapi_sound_load still works");
	expect(g_wav_mem_calls == wav_mem_before && g_wav_path_calls == wav_path_before + 1,
		   "fallback: bapi_sound_load used load_wav");
	if (sound) bapi_sound_free(sound);

	bapi_texture_t texture = bapi_texture_load("disk.png");
	expect(texture != NULL, "fallback: bapi_texture_load still works");
	expect(g_image_mem_calls == image_mem_before && g_image_path_calls == image_path_before + 1,
		   "fallback: bapi_texture_load used load_image");
	if (texture) bapi_texture_destroy(texture);

	expect(bapi_sound_load_from_memory(g_disk_wav, sizeof(g_disk_wav)) == NULL,
		   "fallback: sound from memory unsupported");
	expect(bapi_texture_load_from_memory(g_disk_png, sizeof(g_disk_png)) == NULL,
		   "fallback: texture from memory unsupported");
}

int main(int argc, char **argv)
{
	char dir[1024];
	if (argc < 2) {
		fprintf(stderr, "usage: %s <build-dir>\n", argv[0]);
		return 1;
	}
	snprintf(dir, sizeof(dir), "%s/resmemtest", argv[1]);
	test_mkdir(dir);
	snprintf(dir, sizeof(dir), "%s/resmemtest/data", argv[1]);
	test_mkdir(dir);

	snprintf(dir, sizeof(dir), "%s/resmemtest", argv[1]);
	if (test_chdir(dir) != 0) {
		fprintf(stderr, "cannot chdir to %s\n", dir);
		return 1;
	}

	if (build_media_pack() != 0) {
		fprintf(stderr, "cannot build media.rz\n");
		return 1;
	}

	if (plat_init(&mem_platform) != 0) return 1;

	test_file_read_alloc();
	test_sound_routing();
	test_texture_routing();

	bapi_pack_t pack = bapi_pack_open("media.rz");
	expect(pack != NULL, "open media.rz");
	if (pack) {
		test_pack_composition(pack);
		test_pack_streams(pack);
		bapi_pack_close(pack);
	}

	/* Re-init with a path-only backend: the two-stage path must fall back
	 * cleanly when either io or the memory decode slot is missing. */
	if (plat_init(&path_platform) != 0) return 1;
	test_path_fallback();

	plat_shutdown();
	return fails ? 1 : 0;
}
