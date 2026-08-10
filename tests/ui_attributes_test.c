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
	size_t length = strlen(contents);
	if (fwrite(contents, 1, length, file) != length) {
		fclose(file);
		return -1;
	}
	fclose(file);
	return 0;
}

static int float_near(float a, float b)
{
	float diff = a > b ? a - b : b - a;
	return diff < 0.001f;
}

static bapi_ui_t build_ui(void)
{
	bapi_ui_t ui = bapi_ui_create();
	if (!ui) return NULL;
	bapi_ui_component_t slider = bapi_ui_component_create(BAPI_UI_COMPONENT_SLIDER, "vol");
	bapi_ui_component_t grid	  = bapi_ui_component_create(BAPI_UI_COMPONENT_GRID, "grid");
	bapi_ui_component_t orb	  = bapi_ui_component_create(BAPI_UI_COMPONENT_CIRCLE, "orb");
	bapi_ui_component_t tri	  = bapi_ui_component_create(BAPI_UI_COMPONENT_POLYGON, "tri");
	bapi_ui_component_t box	  = bapi_ui_component_create(BAPI_UI_COMPONENT_CHECKBOX, "box");
	if (!slider || !grid || !orb || !tri || !box) {
		bapi_ui_component_destroy(slider);
		bapi_ui_component_destroy(grid);
		bapi_ui_component_destroy(orb);
		bapi_ui_component_destroy(tri);
		bapi_ui_component_destroy(box);
		bapi_ui_destroy(ui);
		return NULL;
	}
	bapi_ui_add_root(ui, slider);
	bapi_ui_add_root(ui, grid);
	bapi_ui_add_root(ui, orb);
	bapi_ui_add_root(ui, tri);
	bapi_ui_add_root(ui, box);
	return ui;
}

int main(int argc, char **argv)
{
	char saved_path[512];
	if (argc != 2) return 1;
	snprintf(saved_path, sizeof(saved_path), "%s/ui_attributes_out.xml", argv[1]);

	int		result = 1;
	bapi_ui_t ui	   = build_ui();
	if (!ui) {
		fprintf(stderr, "ui failed to build\n");
		return 1;
	}

	bapi_ui_component_t slider = bapi_ui_find(ui, "vol");
	bapi_ui_component_t grid	 = bapi_ui_find(ui, "grid");
	bapi_ui_component_t orb	 = bapi_ui_find(ui, "orb");
	bapi_ui_component_t tri	 = bapi_ui_find(ui, "tri");
	bapi_ui_component_t box	 = bapi_ui_find(ui, "box");

	// --- min/max/step getters default to engine defaults ---
	result = expect(float_near(bapi_ui_component_get_min_value(slider), 0.0f),
					"slider default min is 0") &&
			 expect(float_near(bapi_ui_component_get_max_value(slider), 1.0f),
					"slider default max is 1") &&
			 expect(float_near(bapi_ui_component_get_step(slider), 1.0f),
					"slider default step is 1");

	// --- setters ---
	result = result &&
			 expect(bapi_ui_component_set_min_value(slider, 10.0f) == 0,
					"set_min_value succeeds") &&
			 expect(float_near(bapi_ui_component_get_min_value(slider), 10.0f),
					"min value round trip") &&
			 expect(bapi_ui_component_set_max_value(slider, 90.0f) == 0,
					"set_max_value succeeds") &&
			 expect(float_near(bapi_ui_component_get_max_value(slider), 90.0f),
					"max value round trip") &&
			 expect(bapi_ui_component_set_step(slider, 2.5f) == 0, "set_step succeeds") &&
			 expect(float_near(bapi_ui_component_get_step(slider), 2.5f), "step round trip");

	// --- value is clamped when range shrinks ---
	bapi_ui_component_set_value(slider, 50.0f);
	result = result && expect(float_near(bapi_ui_component_get_value(slider), 50.0f),
							  "value set inside range") &&
			 expect(bapi_ui_component_set_min_value(slider, 60.0f) == 0,
					"raise min above value") &&
			 expect(float_near(bapi_ui_component_get_value(slider), 60.0f),
					"value clamps up to new min") &&
			 expect(bapi_ui_component_set_max_value(slider, 30.0f) == 0,
					"lower max below value") &&
			 expect(float_near(bapi_ui_component_get_value(slider), 30.0f),
					"value clamps down to new max");

	// --- columns / radius / sides ---
	result = result &&
			 expect(bapi_ui_component_get_columns(grid) == 0, "grid default columns is 0") &&
			 expect(bapi_ui_component_set_columns(grid, 4) == 0, "set_columns succeeds") &&
			 expect(bapi_ui_component_get_columns(grid) == 4, "columns round trip") &&
			 expect(bapi_ui_component_set_columns(grid, -1) == -1, "negative columns fails") &&
			 expect(bapi_ui_component_set_radius(orb, 18.5f) == 0, "set_radius succeeds") &&
			 expect(float_near(bapi_ui_component_get_radius(orb), 18.5f), "radius round trip") &&
			 expect(bapi_ui_component_set_radius(orb, -1.0f) == -1, "negative radius fails") &&
			 expect(bapi_ui_component_set_sides(tri, 5) == 0, "set_sides succeeds") &&
			 expect(bapi_ui_component_get_sides(tri) == 5, "sides round trip") &&
			 expect(bapi_ui_component_set_sides(tri, -1) == -1, "negative sides fails");

	// --- checked / relative ---
	result = result &&
			 expect(bapi_ui_component_is_checked(box) == 0, "checkbox default unchecked") &&
			 expect(bapi_ui_component_set_checked(box, 1) == 0, "set_checked succeeds") &&
			 expect(bapi_ui_component_is_checked(box) == 1, "checked round trip") &&
			 expect(bapi_ui_component_set_checked(box, 0) == 0, "uncheck succeeds") &&
			 expect(bapi_ui_component_get_relative(grid) == 0, "grid default absolute") &&
			 expect(bapi_ui_component_set_relative(grid, 1) == 0, "set_relative succeeds") &&
			 expect(bapi_ui_component_get_relative(grid) == 1, "relative round trip") &&
			 expect(bapi_ui_component_set_relative(grid, 0) == 0, "unset relative succeeds");

	// --- null / invalid guards ---
	result = result && expect(bapi_ui_component_get_min_value(NULL) == 0.0f, "null min is 0") &&
			 expect(bapi_ui_component_get_max_value(NULL) == 0.0f, "null max is 0") &&
			 expect(bapi_ui_component_get_step(NULL) == 0.0f, "null step is 0") &&
			 expect(bapi_ui_component_get_columns(NULL) == 0, "null columns is 0") &&
			 expect(bapi_ui_component_get_radius(NULL) == 0.0f, "null radius is 0") &&
			 expect(bapi_ui_component_get_sides(NULL) == 0, "null sides is 0") &&
			 expect(bapi_ui_component_get_relative(NULL) == 0, "null relative is 0") &&
			 expect(bapi_ui_component_set_min_value(NULL, 1.0f) == -1, "null set min fails") &&
			 expect(bapi_ui_component_set_max_value(NULL, 1.0f) == -1, "null set max fails") &&
			 expect(bapi_ui_component_set_step(NULL, 1.0f) == -1, "null set step fails") &&
			 expect(bapi_ui_component_set_columns(NULL, 1) == -1, "null set columns fails") &&
			 expect(bapi_ui_component_set_radius(NULL, 1.0f) == -1, "null set radius fails") &&
			 expect(bapi_ui_component_set_sides(NULL, 1) == -1, "null set sides fails") &&
			 expect(bapi_ui_component_set_checked(NULL, 1) == -1, "null set checked fails") &&
			 expect(bapi_ui_component_set_relative(NULL, 1) == -1, "null set relative fails");

	// --- XML round trip preserves the new attributes ---
	result = result && expect(bapi_ui_save_to_xml(ui, saved_path) == 0, "save succeeds");
	bapi_ui_t loaded = bapi_ui_load_from_xml(saved_path);
	result = result && expect(loaded != NULL, "saved ui reloads");

	bapi_ui_component_t slider_loaded = bapi_ui_find(loaded, "vol");
	bapi_ui_component_t grid_loaded	= bapi_ui_find(loaded, "grid");
	bapi_ui_component_t orb_loaded	= bapi_ui_find(loaded, "orb");
	bapi_ui_component_t tri_loaded	= bapi_ui_find(loaded, "tri");
	bapi_ui_component_t box_loaded	= bapi_ui_find(loaded, "box");

	result = result && expect(slider_loaded && grid_loaded && orb_loaded && tri_loaded && box_loaded,
							  "all components reload") &&
			 expect(slider_loaded &&
						float_near(bapi_ui_component_get_min_value(slider_loaded), 60.0f),
					"min value survives round trip") &&
			 expect(slider_loaded &&
						float_near(bapi_ui_component_get_max_value(slider_loaded), 30.0f),
					"max value survives round trip") &&
			 expect(slider_loaded &&
						float_near(bapi_ui_component_get_step(slider_loaded), 2.5f),
					"step survives round trip") &&
			 expect(grid_loaded && bapi_ui_component_get_columns(grid_loaded) == 4,
					"columns survives round trip") &&
			 expect(orb_loaded &&
						float_near(bapi_ui_component_get_radius(orb_loaded), 18.5f),
					"radius survives round trip") &&
			 expect(tri_loaded && bapi_ui_component_get_sides(tri_loaded) == 5,
					"sides survives round trip") &&
			 expect(box_loaded && bapi_ui_component_is_checked(box_loaded) == 0,
					"unchecked state survives round trip");

	bapi_ui_destroy(loaded);
	bapi_ui_destroy(ui);
	bapi_ui_destroy(NULL);
	return result ? 0 : 1;
}
