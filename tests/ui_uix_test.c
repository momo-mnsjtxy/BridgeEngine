// Round-trip test for encrypted .uix UI documents (bapi_ui_save_to_file /
// bapi_ui_load_from_file). Verifies:
//   - save with a key then load with the same key round-trips the components
//   - loading with a wrong key fails
//   - loading an encrypted file with an empty key fails
//   - saving with an empty key produces plain XML that loads via load_from_xml
//   - load_from_file with an empty key still reads plain XML
//   - tampering (a modified payload) is rejected by the checksum

#include "BridgeEngine.h"
#include "internal/platform/platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct bapi_texture_internal {
	char path[512];
};

static struct bapi_texture_internal fake_texture;
static char							texture_path[512];
static int							texture_destroy_count;
static int							texture_render_count;
static float						mouse_x;
static float						mouse_y;

static int expect(int condition, const char *message)
{
	if (!condition) fprintf(stderr, "%s\n", message);
	return condition;
}

static void get_mouse_state(float *x, float *y)
{
	*x = mouse_x;
	*y = mouse_y;
}

const plat_interface_t *plat_get(void)
{
	static const plat_interface_t platform = {
		.window = {.get_mouse_state = get_mouse_state},
	};
	return &platform;
}

bapi_color_t bapi_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	bapi_color_t color = {r, g, b, a};
	return color;
}

void bapi_get_text_size(const char *text, float size, float *width, float *height)
{
	(void)text;
	(void)size;
	if (width) *width = 20;
	if (height) *height = 10;
}

int bapi_event_is_mouse_button_down(const bapi_event_t *event)
{
	return event && event->type == BAPI_EVENT_MOUSE_BUTTON_DOWN;
}

int bapi_event_is_mouse_button_up(const bapi_event_t *event)
{
	return event && event->type == BAPI_EVENT_MOUSE_BUTTON_UP;
}

void bapi_fill_rect(float x, float y, float w, float h, bapi_color_t color)
{
	(void)x;
	(void)y;
	(void)w;
	(void)h;
	(void)color;
}

void bapi_draw_rect(float x, float y, float w, float h, bapi_color_t color)
{
	(void)x;
	(void)y;
	(void)w;
	(void)h;
	(void)color;
}

void bapi_draw_text(const char *text, float x, float y, float size, bapi_color_t color)
{
	(void)text;
	(void)x;
	(void)y;
	(void)size;
	(void)color;
}

bapi_texture_t bapi_texture_from_file(const char *filepath, int *out_w, int *out_h)
{
	if (out_w) *out_w = 32;
	if (out_h) *out_h = 16;
	strncpy(texture_path, filepath, sizeof(texture_path) - 1);
	texture_path[sizeof(texture_path) - 1] = '\0';
	strncpy(fake_texture.path, filepath, sizeof(fake_texture.path) - 1);
	fake_texture.path[sizeof(fake_texture.path) - 1] = '\0';
	return &fake_texture;
}

void bapi_texture_destroy(bapi_texture_t texture)
{
	if (texture) texture_destroy_count++;
}

void bapi_texture_render_ex(bapi_texture_t texture, float x, float y, float w, float h)
{
	(void)x;
	(void)y;
	(void)w;
	(void)h;
	if (texture) texture_render_count++;
}

void bapi_draw_line(float x1, float y1, float x2, float y2, bapi_color_t color)
{
	(void)x1;
	(void)y1;
	(void)x2;
	(void)y2;
	(void)color;
}

void bapi_draw_circle(float cx, float cy, float radius, bapi_color_t color)
{
	(void)cx;
	(void)cy;
	(void)radius;
	(void)color;
}

void bapi_fill_circle(float cx, float cy, float radius, bapi_color_t color)
{
	(void)cx;
	(void)cy;
	(void)radius;
	(void)color;
}

void bapi_draw_polygon(float cx, float cy, float radius, int sides, bapi_color_t color)
{
	(void)cx;
	(void)cy;
	(void)radius;
	(void)sides;
	(void)color;
}

bapi_video_t bapi_video_load(const char *filepath)
{
	(void)filepath;
	return NULL;
}

void bapi_video_free(bapi_video_t video)
{
	(void)video;
}

void bapi_video_render(bapi_video_t video, int x, int y, int w, int h)
{
	(void)video;
	(void)x;
	(void)y;
	(void)w;
	(void)h;
}

static int write_text_file(const char *path, const char *contents)
{
	FILE *file = fopen(path, "wb");
	if (!file) return -1;
	if (fwrite(contents, 1, strlen(contents), file) != strlen(contents)) {
		fclose(file);
		return -1;
	}
	fclose(file);
	return 0;
}

// Read a file's first bytes; returns 1 on success.
static int read_file_prefix(const char *path, unsigned char *out, size_t max)
{
	FILE *file = fopen(path, "rb");
	if (!file) return 0;
	size_t n = fread(out, 1, max, file);
	fclose(file);
	return n > 0;
}

int main(int argc, char **argv)
{
	char encrypted_path[512];
	char plain_path[512];
	if (argc != 2) return 1;
	snprintf(encrypted_path, sizeof(encrypted_path), "%s/ui_enc.uix", argv[1]);
	snprintf(plain_path, sizeof(plain_path), "%s/ui_plain.xml", argv[1]);

	const char *key = "s3cret-key";

	// Build a UI with two components (no image, so no texture dependency).
	bapi_ui_t ui = bapi_ui_create();
	if (!ui) return 1;
	bapi_ui_component_t panel = bapi_ui_component_create(BAPI_UI_COMPONENT_PANEL, "root");
	bapi_ui_component_t label = bapi_ui_component_create(BAPI_UI_COMPONENT_LABEL, "title");
	if (!panel || !label) return 1;
	bapi_rect_t pr = {10, 20, 300, 200};
	bapi_ui_component_set_rect(panel, pr);
	bapi_ui_component_set_text(label, "Hello");
	bapi_ui_add_root(ui, panel);
	bapi_ui_component_add_child(panel, label);

	int result = 1;

	// 1. encrypted save + correct-key load round-trips
	if (bapi_ui_save_to_file(ui, encrypted_path, key) != 0) {
		fprintf(stderr, "encrypted save failed\n");
		result = 0;
	}
	unsigned char header[4] = {0};
	if (read_file_prefix(encrypted_path, header, 4)) {
		result = result && expect(memcmp(header, "BUIX", 4) == 0, "encrypted file has BUIX magic");
	}
	bapi_ui_t loaded = bapi_ui_load_from_file(encrypted_path, key);
	result = result && expect(loaded != NULL, "encrypted file loads with correct key");
	if (loaded) {
		result = result &&
				 expect(bapi_ui_find(loaded, "root") != NULL, "panel id survives round-trip") &&
				 expect(bapi_ui_find(loaded, "title") != NULL, "label id survives round-trip");
		bapi_rect_t rr;
		bapi_ui_component_get_rect(bapi_ui_find(loaded, "root"), &rr);
		result = result && expect(rr.x == 10 && rr.y == 20 && rr.w == 300 && rr.h == 200,
								  "rect values survive round-trip");
		bapi_ui_destroy(loaded);
	}

	// 2. wrong key fails
	result = result && expect(bapi_ui_load_from_file(encrypted_path, "wrong") == NULL,
							  "wrong key is rejected");
	// 3. empty key on encrypted file fails
	result = result && expect(bapi_ui_load_from_file(encrypted_path, NULL) == NULL,
							  "encrypted file without key is rejected");
	result = result && expect(bapi_ui_load_from_file(encrypted_path, "") == NULL,
							  "encrypted file with empty key is rejected");

	// 4. plain XML round-trip via the new API (empty key)
	bapi_rect_t cr = {5, 6, 7, 8};
	bapi_ui_component_set_rect(label, cr);
	if (bapi_ui_save_to_file(ui, plain_path, "") != 0) {
		fprintf(stderr, "plain save failed\n");
		result = 0;
	}
	// the plain file should contain literal XML
	unsigned char plain_header[5] = {0};
	if (read_file_prefix(plain_path, plain_header, 5)) {
		result = result && expect(memcmp(plain_header, "<?xml", 5) == 0,
								  "empty-key save writes plain XML");
	}
	bapi_ui_t plain_loaded = bapi_ui_load_from_file(plain_path, "");
	result = result && expect(plain_loaded != NULL, "plain XML loads via load_from_file");
	if (plain_loaded) {
		bapi_rect_t lr;
		bapi_ui_component_get_rect(bapi_ui_find(plain_loaded, "title"), &lr);
		result = result && expect(lr.x == 5 && lr.y == 6, "plain round-trip keeps rect");
		bapi_ui_destroy(plain_loaded);
	}
	// load_from_file with a key should refuse a plain XML file
	result = result && expect(bapi_ui_load_from_file(plain_path, "k") == NULL,
							  "plain XML with a key is rejected");

	// 5. tamper detection: flip a payload byte, checksum must catch it
	{
		FILE *file = fopen(encrypted_path, "r+b");
		if (file) {
			fseek(file, 0, SEEK_END);
			long size = ftell(file);
			if (size > 12) {
				fseek(file, size - 1, SEEK_SET);
				unsigned char byte = 0;
				fread(&byte, 1, 1, file);
				fseek(file, size - 1, SEEK_SET);
				byte ^= 0x01;
				fwrite(&byte, 1, 1, file);
			}
			fclose(file);
		}
		result = result && expect(bapi_ui_load_from_file(encrypted_path, key) == NULL,
								  "tampered payload is rejected");
	}

	bapi_ui_destroy(ui);
	return result ? 0 : 1;
}
