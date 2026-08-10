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
static int							fill_rect_count;
static int							draw_rect_count;
static int							draw_text_count;
static int							texture_render_count;

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
	fill_rect_count++;
}

void bapi_draw_rect(float x, float y, float w, float h, bapi_color_t color)
{
	(void)x;
	(void)y;
	(void)w;
	(void)h;
	(void)color;
	draw_rect_count++;
}

void bapi_draw_text(const char *text, float x, float y, float size, bapi_color_t color)
{
	(void)text;
	(void)x;
	(void)y;
	(void)size;
	(void)color;
	draw_text_count++;
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
	size_t length = strlen(contents);
	if (fwrite(contents, 1, length, file) != length) {
		fclose(file);
		return -1;
	}
	fclose(file);
	return 0;
}

static char *read_text_file(const char *path)
{
	FILE *file = fopen(path, "rb");
	if (!file) return NULL;
	fseek(file, 0, SEEK_END);
	long size = ftell(file);
	fseek(file, 0, SEEK_SET);
	if (size < 0) {
		fclose(file);
		return NULL;
	}
	char *buffer = malloc((size_t)size + 1);
	if (!buffer) {
		fclose(file);
		return NULL;
	}
	if (fread(buffer, 1, (size_t)size, file) != (size_t)size) {
		fclose(file);
		free(buffer);
		return NULL;
	}
	buffer[size] = '\0';
	fclose(file);
	return buffer;
}

static float abs_diff(float a, float b)
{
	return a > b ? a - b : b - a;
}

static int rect_near(bapi_rect_t a, bapi_rect_t b)
{
	return abs_diff(a.x, b.x) < 0.01f && abs_diff(a.y, b.y) < 0.01f &&
		   abs_diff(a.w, b.w) < 0.01f && abs_diff(a.h, b.h) < 0.01f;
}

static int color_eq(bapi_color_t a, bapi_color_t b)
{
	return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

static int compare_node(bapi_ui_component_t a, bapi_ui_component_t b)
{
	if (!a || !b) return 0;
	if (bapi_ui_component_get_type(a) != bapi_ui_component_get_type(b)) return 0;
	bapi_rect_t ra;
	bapi_rect_t rb;
	bapi_ui_component_get_rect(a, &ra);
	bapi_ui_component_get_rect(b, &rb);
	if (!rect_near(ra, rb)) return 0;
	if (bapi_ui_component_is_visible(a) != bapi_ui_component_is_visible(b)) return 0;
	if (bapi_ui_component_is_enabled(a) != bapi_ui_component_is_enabled(b)) return 0;
	int ca = bapi_ui_component_get_child_count(a);
	int cb = bapi_ui_component_get_child_count(b);
	if (ca != cb) return 0;
	for (int i = 0; i < ca; i++) {
		if (!compare_node(bapi_ui_component_get_child(a, i), bapi_ui_component_get_child(b, i)))
			return 0;
	}
	return 1;
}

static bapi_ui_component_t find_component(bapi_ui_t ui, const char *id)
{
	return bapi_ui_find(ui, id);
}

int main(int argc, char **argv)
{
	char source_path[512];
	char saved_path[512];
	if (argc != 2) return 1;
	snprintf(source_path, sizeof(source_path), "%s/ui_save_source.xml", argv[1]);
	snprintf(saved_path, sizeof(saved_path), "%s/ui_save_out.xml", argv[1]);

	const char *source_xml =
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
		"<ui>\n"
		"  <panel id=\"panel\" x=\"10\" y=\"10\" w=\"500\" h=\"320\" color=\"#20242CFF\">\n"
		"    <label id=\"title\" x=\"20\" y=\"24\" text=\"A &amp; B &lt;Menu&gt;\" size=\"28\" "
		"color=\"#FFFFFFFF\" />\n"
		"    <button id=\"start\" x=\"40\" y=\"80\" w=\"180\" h=\"48\" text=\"Start\"\n"
		"            text_size=\"20\" normal=\"#2F80EDFF\" hover=\"#56CCF2FF\"\n"
		"            click=\"#1C5DB8FF\" text_color=\"#FFFFFFFF\" />\n"
		"    <progress id=\"level\" x=\"70\" y=\"145\" w=\"160\" h=\"25\" value=\"0.33\" "
		"color=\"#228B22FF\" color2=\"#808080FF\" />\n"
		"    <slider id=\"vol\" x=\"70\" y=\"180\" w=\"160\" h=\"20\" value=\"0.5\" />\n"
		"    <image id=\"logo\" x=\"300\" y=\"24\" w=\"128\" h=\"128\" src=\"assets/logo.png\" />\n"
		"    <grid id=\"grid\" x=\"20\" y=\"210\" w=\"200\" h=\"60\" columns=\"2\" step=\"4\">\n"
		"      <label id=\"cell_a\" x=\"0\" y=\"0\" text=\"A\" size=\"12\" />\n"
		"      <label id=\"cell_b\" x=\"0\" y=\"0\" text=\"B\" size=\"12\" />\n"
		"    </grid>\n"
		"    <row id=\"bar\" x=\"20\" y=\"280\" w=\"300\" h=\"30\" step=\"5\">\n"
		"      <checkbox id=\"opt_a\" x=\"0\" y=\"0\" w=\"20\" h=\"20\" />\n"
		"      <checkbox id=\"opt_b\" x=\"0\" y=\"0\" w=\"20\" h=\"20\" checked=\"true\" />\n"
		"    </row>\n"
		"    <scroll id=\"box\" x=\"340\" y=\"24\" w=\"120\" h=\"100\">\n"
		"      <button id=\"scrolled\" relative=\"true\" x=\"0\" y=\"20\" w=\"80\" h=\"20\" "
		"text=\"Scroll\" />\n"
		"    </scroll>\n"
		"    <circle id=\"orb\" x=\"300\" y=\"160\" radius=\"24\" color=\"#FF0000FF\" />\n"
		"    <polygon id=\"tri\" x=\"400\" y=\"220\" radius=\"30\" sides=\"3\" color=\"#00FF00FF\" "
		"/>\n"
		"    <label id=\"hidden\" x=\"20\" y=\"300\" text=\"Hidden\" size=\"14\" visible=\"false\" "
		"/>\n"
		"  </panel>\n"
		"</ui>\n";

	if (write_text_file(source_path, source_xml) != 0) return 1;

	bapi_ui_t ui = bapi_ui_load_from_xml(source_path);
	if (!ui) {
		fprintf(stderr, "source ui failed to load\n");
		return 1;
	}
	char source_texture_path[512];
	strncpy(source_texture_path, texture_path, sizeof(source_texture_path) - 1);
	source_texture_path[sizeof(source_texture_path) - 1] = '\0';

	int result = 1;

	result = expect(bapi_ui_save_to_xml(ui, saved_path) == 0, "save to xml succeeds") &&
			 expect(bapi_ui_save_to_xml(NULL, saved_path) == -1, "null ui save fails") &&
			 expect(bapi_ui_save_to_xml(ui, NULL) == -1, "null path save fails");

	bapi_ui_t saved = bapi_ui_load_from_xml(saved_path);
	result = result && expect(saved != NULL, "saved ui loads");

	bapi_ui_component_t panel_orig	 = find_component(ui, "panel");
	bapi_ui_component_t panel_saved	 = find_component(saved, "panel");
	bapi_ui_component_t title_saved	 = find_component(saved, "title");
	bapi_ui_component_t button_saved = find_component(saved, "start");
	bapi_ui_component_t progress_saved = find_component(saved, "level");
	bapi_ui_component_t slider_saved	= find_component(saved, "vol");
	bapi_ui_component_t opt_b_saved	 = find_component(saved, "opt_b");
	bapi_ui_component_t hidden_saved	= find_component(saved, "hidden");

	result = result &&
			 expect(compare_node(panel_orig, panel_saved), "panel structure survives round trip") &&
			 expect(strcmp(bapi_ui_component_get_text(title_saved), "A & B <Menu>") == 0,
					"entity text survives round trip") &&
			 expect(bapi_ui_component_get_text_size(title_saved) == 28.0f,
					"label size survives round trip") &&
			 expect(bapi_ui_component_get_text_size(button_saved) == 20.0f,
					"button text size survives round trip") &&
			 expect(bapi_ui_component_get_value(progress_saved) == 0.33f,
					"progress value survives round trip") &&
			 expect(bapi_ui_component_get_value(slider_saved) == 0.5f,
					"slider value survives round trip") &&
			 expect(bapi_ui_component_is_checked(opt_b_saved) == 1,
					"checked state survives round trip") &&
			 expect(bapi_ui_component_is_visible(hidden_saved) == 0,
					"visible=false survives round trip");

	bapi_color_t normal;
	bapi_color_t hover;
	bapi_color_t click;
	bapi_color_t text_color;
	bapi_color_t progress_color;
	bapi_color_t progress_color2;
	bapi_color_t orb_color;
	result =
		result &&
		expect(bapi_ui_component_get_color(button_saved, BAPI_UI_COLOR_NORMAL, &normal) == 0 &&
				   color_eq(normal, bapi_color(0x2F, 0x80, 0xED, 0xFF)),
			   "button normal color survives") &&
		expect(bapi_ui_component_get_color(button_saved, BAPI_UI_COLOR_HOVER, &hover) == 0 &&
				   color_eq(hover, bapi_color(0x56, 0xCC, 0xF2, 0xFF)),
			   "button hover color survives") &&
		expect(bapi_ui_component_get_color(button_saved, BAPI_UI_COLOR_CLICK, &click) == 0 &&
				   color_eq(click, bapi_color(0x1C, 0x5D, 0xB8, 0xFF)),
			   "button click color survives") &&
		expect(bapi_ui_component_get_color(button_saved, BAPI_UI_COLOR_TEXT, &text_color) == 0 &&
				   color_eq(text_color, bapi_color(255, 255, 255, 255)),
			   "button text color survives") &&
		expect(bapi_ui_component_get_color(progress_saved, BAPI_UI_COLOR_NORMAL, &progress_color) ==
					   0 &&
				   color_eq(progress_color, bapi_color(0x22, 0x8B, 0x22, 0xFF)),
			   "progress color survives") &&
		expect(bapi_ui_component_get_color(progress_saved, BAPI_UI_COLOR_HOVER, &progress_color2) ==
					   0 &&
				   color_eq(progress_color2, bapi_color(0x80, 0x80, 0x80, 0xFF)),
			   "progress color2 survives") &&
		expect(bapi_ui_component_get_color(find_component(saved, "orb"), BAPI_UI_COLOR_NORMAL,
										   &orb_color) == 0 &&
				   color_eq(orb_color, bapi_color(0xFF, 0x00, 0x00, 0xFF)),
			   "circle color survives");

	bapi_rect_t scrolled_orig;
	bapi_rect_t scrolled_saved;
	bapi_ui_component_get_rect(find_component(ui, "scrolled"), &scrolled_orig);
	bapi_ui_component_get_rect(find_component(saved, "scrolled"), &scrolled_saved);
	result = result && expect(rect_near(scrolled_orig, scrolled_saved),
							  "relative child layout survives round trip") &&
			 expect(bapi_ui_component_get_text_size(find_component(saved, "grid")) == 18.0f,
					"grid keeps default text size");

	result = result && expect(strcmp(texture_path, source_texture_path) == 0,
							  "image src path survives round trip");

	char *saved_contents = read_text_file(saved_path);
	result = result && expect(saved_contents != NULL, "saved file is readable") &&
			 expect(saved_contents && strstr(saved_contents, "radius=\"24\"") != NULL,
					"radius attribute is serialized") &&
			 expect(saved_contents && strstr(saved_contents, "sides=\"3\"") != NULL,
					"sides attribute is serialized") &&
			 expect(saved_contents && strstr(saved_contents, "columns=\"2\"") != NULL,
					"columns attribute is serialized") &&
			 expect(saved_contents && strstr(saved_contents, "relative=\"true\"") != NULL,
					"relative attribute is serialized") &&
			 expect(saved_contents && strstr(saved_contents, "checked=\"true\"") != NULL,
					"checked attribute is serialized") &&
			 expect(saved_contents && strstr(saved_contents, "&amp;") != NULL,
					"text entities are escaped on save") &&
			 expect(saved_contents && strstr(saved_contents, "src=\"assets/logo.png\"") != NULL,
					"relative src is serialized verbatim");
	free(saved_contents);

	int render_count = 0;
	fill_rect_count	 = 0;
	bapi_ui_render(ui);
	render_count = fill_rect_count;
	fill_rect_count = 0;
	bapi_ui_render_ex(ui, 10.0f, 20.0f, 2.0f);
	result = result && expect(fill_rect_count == render_count,
							  "render_ex draws the same primitives as render");
	bapi_ui_render_ex(NULL, 0.0f, 0.0f, 1.0f);

	bapi_ui_component_t drawn =
		bapi_ui_component_create(BAPI_UI_COMPONENT_RECT, "drawn");
	result = result && expect(drawn != NULL, "component can be created") &&
			 expect(bapi_ui_component_create(BAPI_UI_COMPONENT_RECT, "") == NULL,
					"empty id component fails") &&
			 expect(bapi_ui_component_create(BAPI_UI_COMPONENT_RECT, NULL) == NULL,
					"null id component fails");
	bapi_ui_component_set_rect(drawn, (bapi_rect_t){50.0f, 60.0f, 100.0f, 20.0f});
	bapi_rect_t drawn_rect;
	bapi_ui_component_get_rect(drawn, &drawn_rect);
	result = result && expect(rect_near(drawn_rect, (bapi_rect_t){50.0f, 60.0f, 100.0f, 20.0f}),
							  "set_rect is applied");

	bapi_ui_component_t panel = find_component(ui, "panel");
	int panel_children_before = bapi_ui_component_get_child_count(panel);
	result =
		result && expect(bapi_ui_component_add_child(panel, drawn) == 0,
						  "child can be added to a container") &&
		expect(bapi_ui_component_get_child_count(panel) == panel_children_before + 1,
			   "child count grows") &&
		expect(bapi_ui_component_get_parent(drawn) == panel, "parent pointer is set") &&
		expect(bapi_ui_component_get_child(panel, panel_children_before) == drawn,
			   "child is appended at the end") &&
		expect(bapi_ui_component_add_child(NULL, drawn) == -1, "null parent add fails") &&
		expect(bapi_ui_component_add_child(panel, NULL) == -1, "null child add fails");

	result = result && expect(bapi_ui_component_set_id(drawn, "renamed") == 0,
							  "component id can be renamed") &&
			 expect(strcmp(bapi_ui_component_get_id(drawn), "renamed") == 0,
					"get_id returns renamed id") &&
			 expect(bapi_ui_component_get_id(NULL) == NULL, "null component get_id is null") &&
			 expect(bapi_ui_find(ui, "renamed") == drawn, "renamed id is findable") &&
			 expect(bapi_ui_find(ui, "drawn") == NULL, "old id no longer resolves") &&
			 expect(bapi_ui_component_set_id(drawn, "") == -1, "empty id rename fails");

	result = result &&
			 expect(bapi_ui_component_set_color(drawn, BAPI_UI_COLOR_NORMAL,
												bapi_color(1, 2, 3, 4)) == 0 &&
					 bapi_ui_component_get_color(drawn, BAPI_UI_COLOR_NORMAL, &normal) == 0 &&
					 color_eq(normal, bapi_color(1, 2, 3, 4)),
				 "set_color/get_color round trip") &&
			 expect(bapi_ui_component_set_color(drawn, (bapi_ui_color_role_t)99,
												bapi_color(0, 0, 0, 0)) == -1,
					"invalid color role fails") &&
			 expect(bapi_ui_component_get_color(NULL, BAPI_UI_COLOR_NORMAL, &normal) == -1,
					"null component color get fails");

	bapi_ui_component_set_text_size(drawn, 22.0f);
	result = result && expect(bapi_ui_component_get_text_size(drawn) == 22.0f,
							  "text size setter/getter round trip");

	result = result && expect(bapi_ui_save_to_xml(ui, saved_path) == 0,
							  "save with edited tree succeeds");
	bapi_ui_t edited = bapi_ui_load_from_xml(saved_path);
	result = result && expect(edited != NULL, "edited tree reloads") &&
			 expect(bapi_ui_find(edited, "renamed") != NULL, "edited id survives save/load");

	result = result && expect(bapi_ui_component_remove(drawn) == 0, "child can be removed") &&
			 expect(bapi_ui_component_get_parent(drawn) == NULL, "removed child is detached") &&
			 expect(bapi_ui_component_get_child_count(panel) == panel_children_before,
					"child count shrinks after remove") &&
			 expect(bapi_ui_component_remove(drawn) == -1, "double remove fails");
	bapi_ui_component_destroy(drawn);

	bapi_ui_component_t extra = bapi_ui_component_create(BAPI_UI_COMPONENT_LABEL, "extra");
	result = result && expect(bapi_ui_add_root(ui, extra) == 0, "component can become a root") &&
			 expect(bapi_ui_remove_root(ui, extra) == 0, "root can be removed") &&
			 expect(bapi_ui_remove_root(ui, extra) == -1, "double root remove fails") &&
			 expect(bapi_ui_remove_root(NULL, extra) == -1, "null ui root remove fails");
	bapi_ui_component_destroy(extra);

	bapi_ui_destroy(edited);
	bapi_ui_destroy(saved);
	bapi_ui_destroy(ui);
	bapi_ui_destroy(NULL);
	bapi_ui_save_to_xml(NULL, NULL);
	return result ? 0 : 1;
}
