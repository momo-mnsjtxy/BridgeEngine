#include "BridgeEngine.h"
#include "internal/platform/platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Minimal platform/engine stubs so src/ui.c + src/ui_xml.c link standalone.
struct bapi_texture_internal {
	char path[512];
};
static struct bapi_texture_internal fake_texture;
static const char *g_tex_fail_path = NULL;

static int expect(int condition, const char *message)
{
	if (!condition) fprintf(stderr, "%s\n", message);
	return condition;
}

static void get_mouse_state(float *x, float *y)
{
	*x = 0;
	*y = 0;
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
	if (g_tex_fail_path && strcmp(filepath, g_tex_fail_path) == 0) return NULL;
	strncpy(fake_texture.path, filepath, sizeof(fake_texture.path) - 1);
	fake_texture.path[sizeof(fake_texture.path) - 1] = '\0';
	return &fake_texture;
}

void bapi_texture_destroy(bapi_texture_t texture)
{
	(void)texture;
}

void bapi_texture_render_ex(bapi_texture_t texture, float x, float y, float w, float h)
{
	(void)texture;
	(void)x;
	(void)y;
	(void)w;
	(void)h;
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

struct bapi_video_internal {
	char path[512];
};
static struct bapi_video_internal fake_video;
static int g_video_ok = 0;

bapi_video_t bapi_video_load(const char *filepath)
{
	(void)filepath;
	return g_video_ok ? &fake_video : NULL;
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
	size_t length = strlen(contents);
	if (fwrite(contents, 1, length, file) != length) {
		fclose(file);
		return -1;
	}
	fclose(file);
	return 0;
}

int main(int argc, char **argv)
{
	char saved_path[512];
	if (argc != 2) return 1;
	snprintf(saved_path, sizeof(saved_path), "%s/ui_src_out.xml", argv[1]);

	int	 result = 1;
	bapi_ui_t ui = bapi_ui_create();
	if (!ui) return 1;

	bapi_ui_component_t img	  = bapi_ui_component_create(BAPI_UI_COMPONENT_IMAGE, "logo");
	bapi_ui_component_t rect  = bapi_ui_component_create(BAPI_UI_COMPONENT_RECT, "r");
	bapi_ui_component_t video = bapi_ui_component_create(BAPI_UI_COMPONENT_VIDEO, "clip");
	if (!img || !rect || !video) {
		bapi_ui_component_destroy(img);
		bapi_ui_component_destroy(rect);
		bapi_ui_component_destroy(video);
		bapi_ui_destroy(ui);
		return 1;
	}
	bapi_ui_add_root(ui, img);
	bapi_ui_add_root(ui, rect);
	bapi_ui_add_root(ui, video);

	// --- non-src types rejected ---
	result = expect(bapi_ui_component_set_src(rect, "x.png") == -1, "rect rejects src") &&
			 expect(bapi_ui_component_set_src(NULL, "x.png") == -1, "null component fails") &&
			 expect(bapi_ui_component_set_src(img, NULL) == -1, "null src fails") &&
			 expect(bapi_ui_component_set_src(img, "") == -1, "empty src fails");

	// --- successful load on IMAGE ---
	result = result && expect(bapi_ui_component_set_src(img, "assets/a.png") == 0,
							  "set_src succeeds on image") &&
			 expect(strcmp(bapi_ui_component_get_src(img), "assets/a.png") == 0,
					"get_src returns the src");

	// --- failed load rolls back ---
	const char *old_src = bapi_ui_component_get_src(img);
	g_tex_fail_path		= "assets/missing.png";
	result = result && expect(bapi_ui_component_set_src(img, "assets/missing.png") == -1,
							  "missing texture fails") &&
			 expect(strcmp(bapi_ui_component_get_src(img), old_src) == 0,
					"src preserved on failure");
	g_tex_fail_path = NULL;

	// --- replace with another valid src ---
	result = result && expect(bapi_ui_component_set_src(img, "assets/b.png") == 0,
							  "replace src succeeds") &&
			 expect(strcmp(bapi_ui_component_get_src(img), "assets/b.png") == 0,
					"src updated");

	// --- VIDEO: failed load rolls back, successful load applies ---
	g_video_ok = 0;
	result = result && expect(bapi_ui_component_set_src(video, "assets/mov.mp4") == -1,
							  "video load failure fails") &&
			 expect(bapi_ui_component_get_src(video) == NULL, "video src still empty");
	g_video_ok = 1;
	result = result && expect(bapi_ui_component_set_src(video, "assets/mov.mp4") == 0,
							  "video load success") &&
			 expect(strcmp(bapi_ui_component_get_src(video), "assets/mov.mp4") == 0,
					"video src applied");

	// --- NINE_PATCH / ANIMATION accept src ---
	bapi_ui_component_t np = bapi_ui_component_create(BAPI_UI_COMPONENT_NINE_PATCH, "np");
	bapi_ui_component_t an = bapi_ui_component_create(BAPI_UI_COMPONENT_ANIMATION, "an");
	result = result && expect(np && an, "create nine_patch/animation") &&
			 expect(bapi_ui_component_set_src(np, "assets/np.png") == 0,
					"nine_patch accepts src") &&
			 expect(bapi_ui_component_set_src(an, "assets/an.png") == 0,
					"animation accepts src");

	// --- XML round trip preserves the new src ---
	result = result && expect(bapi_ui_save_to_xml(ui, saved_path) == 0, "save succeeds");
	bapi_ui_t loaded = bapi_ui_load_from_xml(saved_path);
	result = result && expect(loaded != NULL, "src ui reloads");
	if (loaded) {
		bapi_ui_component_t limg = bapi_ui_find(loaded, "logo");
		result = result && expect(limg &&
									  strcmp(bapi_ui_component_get_src(limg), "assets/b.png") == 0,
								  "src survives round trip");
		bapi_ui_destroy(loaded);
	}

	// --- cleanup ---
	bapi_ui_component_destroy(np);
	bapi_ui_component_destroy(an);
	bapi_ui_destroy(ui);
	bapi_ui_destroy(NULL);
	return result ? 0 : 1;
}
