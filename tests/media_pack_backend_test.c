#include "BridgeEngine.h"
#include "internal/engine/render_context.h"
#include "internal/platform/platform.h"
#include "rz_lib.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

/* End-to-end test over the REAL SDL3/FFmpeg backends (headless, dummy
 * drivers): reads the committed example assets into memory with
 * bapi_file_read_alloc, packs them into a runtime-built .rz, then loads
 * audio / image / video through every new entry point:
 * - disk path (streaming for video, two-stage memory reroute for audio/image),
 * - from_memory (video must own a copy: the source buffer is freed before play),
 * - from_pack (whole-entry buffer),
 * - from_pack_stream (streaming pack entry, no full-file buffer). */

static int fails = 0;

static int expect(int condition, const char *message)
{
	if (!condition) {
		fprintf(stderr, "FAIL: %s\n", message);
		fails++;
	}
	return condition;
}

/* Referenced by render_context.c / init.c; not compiled in this test. */
void bapi_text_cleanup(void) {}

void bapi_log_shutdown(void) {}

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

static void check_video(bapi_video_t video, const char *label)
{
	char message[160];
	snprintf(message, sizeof(message), "%s loads", label);
	expect(video != NULL, message);
	if (!video) return;

	snprintf(message, sizeof(message), "%s plays", label);
	expect(bapi_video_play(video) == 0, message);

	bapi_delay(40);
	bapi_video_update();

	int w = 0, h = 0;
	bapi_video_get_size(video, &w, &h);
	snprintf(message, sizeof(message), "%s decodes 1920x1080 (got %dx%d)", label, w, h);
	expect(w == 1920 && h == 1080, message);

	bapi_video_render(video, 0, 0, 320, 200);
	bapi_video_stop(video);
	bapi_video_free(video);
}

int main(int argc, char **argv)
{
	char dir[1024];
	if (argc < 3) {
		fprintf(stderr, "usage: %s <build-dir> <assets-dir>\n", argv[0]);
		return 1;
	}
	const char *build_dir	= argv[1];
	const char *assets_dir	= argv[2];

	snprintf(dir, sizeof(dir), "%s/packe2e", build_dir);
	test_mkdir(dir);
	snprintf(dir, sizeof(dir), "%s/packe2e/audio", build_dir);
	test_mkdir(dir);
	snprintf(dir, sizeof(dir), "%s/packe2e/image", build_dir);
	test_mkdir(dir);
	snprintf(dir, sizeof(dir), "%s/packe2e/video", build_dir);
	test_mkdir(dir);

	snprintf(dir, sizeof(dir), "%s/packe2e", build_dir);
	if (test_chdir(dir) != 0) {
		fprintf(stderr, "cannot chdir to %s\n", dir);
		return 1;
	}

	SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
	SDL_SetHint(SDL_HINT_AUDIO_DRIVER, "dummy");
	SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");

	if (bapi_runtime_start(plat_sdl3_interface(), "media-pack-e2e", 320, 200) != 0) {
		fprintf(stderr, "bapi_runtime_start failed (headless SDL unavailable?)\n");
		return 1;
	}

	/* Stage 1: read the committed assets into memory (dogfoods the new
	 * bapi_file_read_alloc on the real SDL3 io backend). */
	char wav_path[1024], png_path[1024], mp4_path[1024];
	snprintf(wav_path, sizeof(wav_path), "%s/audio/test.wav", assets_dir);
	snprintf(png_path, sizeof(png_path), "%s/image/XINGJI.png", assets_dir);
	snprintf(mp4_path, sizeof(mp4_path), "%s/video/XINGJILOGE.mp4", assets_dir);

	size_t	 wav_size = 0;
	uint8_t *wav	  = bapi_file_read_alloc(wav_path, &wav_size);
	size_t	 png_size = 0;
	uint8_t *png	  = bapi_file_read_alloc(png_path, &png_size);
	size_t	 mp4_size = 0;
	uint8_t *mp4	  = bapi_file_read_alloc(mp4_path, &mp4_size);
	expect(wav != NULL && wav_size > 0, "wav asset read into memory");
	expect(png != NULL && png_size > 0, "png asset read into memory");
	expect(mp4 != NULL && mp4_size > 0, "mp4 asset read into memory");
	if (!wav || !png || !mp4) {
		bapi_runtime_stop();
		return 1;
	}

	/* Stage 1b: pack the same bytes into a runtime-built .rz. */
	if (write_test_file("audio/test.wav", wav, wav_size) != 0 ||
		write_test_file("image/XINGJI.png", png, png_size) != 0 ||
		write_test_file("video/XINGJILOGE.mp4", mp4, mp4_size) != 0) {
		fprintf(stderr, "cannot write pack source files\n");
		bapi_runtime_stop();
		return 1;
	}

	rz_index_t index	= {NULL, NULL};
	rz_file_t *e1		= make_entry("audio/test.wav");
	rz_file_t *e2		= make_entry("image/XINGJI.png");
	rz_file_t *e3		= make_entry("video/XINGJILOGE.mp4");
	int		   pack_ok	= e1 && e2 && e3 && rz_add_index(e1, &index) &&
					  rz_add_index(e2, &index) && rz_add_index(e3, &index) &&
					  rz_create("media_e2e.rz", &index, 0) == 0;
	index_free_all(&index);
	expect(pack_ok, "runtime .rz built");
	bapi_pack_t pack = pack_ok ? bapi_pack_open("media_e2e.rz") : NULL;
	expect(pack != NULL, "runtime .rz opened");

	/* Audio: disk (two-stage reroute), memory, pack. */
	expect(bapi_audio_init() == 0, "audio device opens (dummy driver)");

	bapi_sound_t sound = bapi_sound_load(wav_path);
	expect(sound != NULL, "disk sound loads through the memory stage");
	if (sound) {
		expect(bapi_sound_play(sound) == 0, "disk sound plays");
		bapi_sound_stop(sound);
		bapi_sound_free(sound);
	}

	sound = bapi_sound_load_from_memory(wav, wav_size);
	expect(sound != NULL, "memory sound loads");
	if (sound) {
		bapi_sound_stop(sound);
		bapi_sound_free(sound);
	}

	sound = bapi_sound_load_from_pack(pack, "audio/test.wav");
	expect(sound != NULL, "pack sound loads");
	if (sound) {
		bapi_sound_stop(sound);
		bapi_sound_free(sound);
	}

	/* Image: disk (two-stage reroute), memory, pack. */
	int w = 0, h = 0;
	bapi_texture_t texture = bapi_texture_load(png_path);
	expect(texture != NULL, "disk texture loads through the memory stage");
	if (texture) {
		bapi_texture_get_size(texture, &w, &h);
		expect(w == 535 && h == 230, "disk texture decodes 535x230");
		bapi_texture_destroy(texture);
	}

	texture = bapi_texture_load_from_memory(png, png_size);
	expect(texture != NULL, "memory texture loads");
	if (texture) {
		bapi_texture_get_size(texture, &w, &h);
		expect(w == 535 && h == 230, "memory texture decodes 535x230");
		bapi_texture_destroy(texture);
	}

	texture = bapi_texture_load_from_pack(pack, "image/XINGJI.png");
	expect(texture != NULL, "pack texture loads");
	if (texture) {
		bapi_texture_get_size(texture, &w, &h);
		expect(w == 535 && h == 230, "pack texture decodes 535x230");
		bapi_texture_destroy(texture);
	}

	/* Video: all four source kinds. The from_memory call must own a copy:
	 * the source buffer is released before playback starts. */
	check_video(bapi_video_load(mp4_path), "disk video (streaming)");
	{
		bapi_video_t video = bapi_video_load_from_memory(mp4, mp4_size);
		free(mp4);
		mp4 = NULL;
		check_video(video, "memory video");
	}
	check_video(bapi_video_load_from_pack(pack, "video/XINGJILOGE.mp4"), "pack video (buffer)");
	check_video(bapi_video_load_from_pack_stream(pack, "video/XINGJILOGE.mp4"),
				"pack video (stream)");

	free(wav);
	free(png);

	if (pack) bapi_pack_close(pack);
	bapi_runtime_stop();
	return fails ? 1 : 0;
}
