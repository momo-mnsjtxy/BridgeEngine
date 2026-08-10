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

static bapi_ui_t build_ui(bapi_ui_component_t *out_parent, bapi_ui_component_t *out_a,
						  bapi_ui_component_t *out_b, bapi_ui_component_t *out_c)
{
	bapi_ui_t ui = bapi_ui_create();
	if (!ui) return NULL;
	bapi_ui_component_t parent = bapi_ui_component_create(BAPI_UI_COMPONENT_CONTAINER, "p");
	bapi_ui_component_t a		 = bapi_ui_component_create(BAPI_UI_COMPONENT_RECT, "a");
	bapi_ui_component_t b		 = bapi_ui_component_create(BAPI_UI_COMPONENT_RECT, "b");
	bapi_ui_component_t c		 = bapi_ui_component_create(BAPI_UI_COMPONENT_RECT, "c");
	if (!parent || !a || !b || !c) {
		bapi_ui_component_destroy(parent);
		bapi_ui_component_destroy(a);
		bapi_ui_component_destroy(b);
		bapi_ui_component_destroy(c);
		bapi_ui_destroy(ui);
		return NULL;
	}
	bapi_ui_add_root(ui, parent);
	bapi_ui_component_add_child(parent, a);
	bapi_ui_component_add_child(parent, c);
	*out_parent = parent;
	*out_a		 = a;
	*out_b		 = b;
	*out_c		 = c;
	return ui;
}

int main(int argc, char **argv)
{
	char saved_path[512];
	if (argc != 2) return 1;
	snprintf(saved_path, sizeof(saved_path), "%s/ui_tree_edit_out.xml", argv[1]);

	int	 result = 1;
	bapi_ui_component_t parent, a, b, c;
	bapi_ui_t ui = build_ui(&parent, &a, &b, &c);
	if (!ui) {
		fprintf(stderr, "ui failed to build\n");
		return 1;
	}

	// initial order: a, c
	result = expect(bapi_ui_component_get_child(parent, 0) == a, "first child is a") &&
			 expect(bapi_ui_component_get_child(parent, 1) == c, "second child is c");

	// --- insert at index 1: a, b, c ---
	result = result && expect(bapi_ui_component_insert_child(parent, b, 1) == 0,
							  "insert child at index succeeds") &&
			 expect(bapi_ui_component_get_child(parent, 0) == a, "child 0 is a") &&
			 expect(bapi_ui_component_get_child(parent, 1) == b, "child 1 is b") &&
			 expect(bapi_ui_component_get_child(parent, 2) == c, "child 2 is c") &&
			 expect(bapi_ui_component_get_parent(b) == parent, "inserted child reparented");

	// --- append at index == child_count ---
	bapi_ui_component_t d = bapi_ui_component_create(BAPI_UI_COMPONENT_RECT, "d");
	result = result && expect(d != NULL, "create d") &&
			 expect(bapi_ui_component_insert_child(parent, d, 3) == 0, "append via index") &&
			 expect(bapi_ui_component_get_child(parent, 3) == d, "child 3 is d");

	// --- invalid indices ---
	result = result &&
			 expect(bapi_ui_component_insert_child(parent, b, -1) == -1, "negative index fails") &&
			 expect(bapi_ui_component_insert_child(parent, b, 999) == -1, "too-large index fails") &&
			 expect(bapi_ui_component_insert_child(NULL, b, 0) == -1, "null parent fails");

	// --- already-attached child is rejected ---
	bapi_ui_component_t detached = bapi_ui_component_create(BAPI_UI_COMPONENT_RECT, "detached");
	result = result && expect(detached != NULL, "create detached") &&
			 expect(bapi_ui_component_insert_child(parent, a, 0) == -1,
					"attached child rejected") &&
			 expect(bapi_ui_component_insert_child(parent, a, 0) == -1,
					"attached child still rejected");

	// --- self insert / cycle rejection ---
	result = result && expect(bapi_ui_component_insert_child(parent, parent, 0) == -1,
							  "self insert rejected");
	bapi_ui_component_t inner = bapi_ui_component_create(BAPI_UI_COMPONENT_RECT, "inner");
	bapi_ui_component_add_child(parent, inner);
	result = result &&
			 expect(bapi_ui_component_insert_child(inner, parent, 0) == -1,
					"cycle into own parent rejected");

	// --- move a to the end via detach + insert ---
	result = result && expect(bapi_ui_component_remove(a) == 0, "detach a") &&
			 expect(bapi_ui_component_insert_child(parent, a, 4) == 0, "reinsert a at end") &&
			 expect(bapi_ui_component_get_child(parent, 4) == a, "a now last");

	// --- clone: deep copy with fresh id, fields, children, detached ---
	bapi_ui_component_t source = bapi_ui_component_create(BAPI_UI_COMPONENT_PANEL, "src_panel");
	bapi_ui_component_t kid	= bapi_ui_component_create(BAPI_UI_COMPONENT_SLIDER, "kid");
	bapi_rect_t			src_rect = {11.0f, 22.0f, 33.0f, 44.0f};
	bapi_ui_component_set_rect(source, src_rect);
	bapi_ui_component_set_radius(source, 7.5f);
	bapi_ui_component_set_text(source, "hello");
	bapi_ui_component_add_child(source, kid);
	bapi_ui_component_set_value(kid, 0.42f);
	bapi_ui_component_set_max_value(kid, 100.0f);

	bapi_ui_component_t clone = bapi_ui_component_clone(source);
	result = result && expect(clone != NULL, "clone succeeds") &&
			 expect(bapi_ui_component_get_type(clone) == BAPI_UI_COMPONENT_PANEL,
					"clone keeps type") &&
			 expect(strcmp(bapi_ui_component_get_id(clone), "src_panel") == 0,
					"clone keeps id") &&
			 expect(bapi_ui_component_get_parent(clone) == NULL, "clone is detached");

	bapi_rect_t cloned_rect;
	bapi_ui_component_get_rect(clone, &cloned_rect);
	result = result && expect(cloned_rect.x == 11.0f && cloned_rect.y == 22.0f &&
								  cloned_rect.w == 33.0f && cloned_rect.h == 44.0f,
							  "clone copies rect") &&
			 expect(bapi_ui_component_get_radius(clone) == 7.5f, "clone copies radius") &&
			 expect(strcmp(bapi_ui_component_get_text(clone), "hello") == 0,
					"clone copies text") &&
			 expect(bapi_ui_component_get_child_count(clone) == 1, "clone copies children") &&
			 expect(bapi_ui_component_get_parent(bapi_ui_component_get_child(clone, 0)) == clone,
					"cloned child reparented") &&
			 expect(bapi_ui_component_get_value(bapi_ui_component_get_child(clone, 0)) == 0.42f,
					"cloned child copies value") &&
			 expect(bapi_ui_component_get_max_value(bapi_ui_component_get_child(clone, 0)) ==
						100.0f,
					"cloned child copies range");

	// mutating the clone must not affect the source
	bapi_ui_component_set_rect(clone, (bapi_rect_t){0, 0, 5, 5});
	bapi_rect_t src_rect_check;
	bapi_ui_component_get_rect(source, &src_rect_check);
	result = result && expect(src_rect_check.x == 11.0f, "source unaffected by clone mutation");

	// --- XML round trip keeps the reordered tree ---
	result = result && expect(bapi_ui_save_to_xml(ui, saved_path) == 0, "save succeeds");
	bapi_ui_t loaded = bapi_ui_load_from_xml(saved_path);
	result = result && expect(loaded != NULL, "tree reloads");
	if (loaded) {
		bapi_ui_component_t lp = bapi_ui_find(loaded, "p");
		result = result &&
				 expect(lp != NULL, "parent reloads") &&
				 expect(bapi_ui_component_get_child_count(lp) == 5, "child count preserved") &&
				 expect(strcmp(bapi_ui_component_get_id(bapi_ui_component_get_child(lp, 4)),
							   "a") == 0,
						"insert order preserved");
		bapi_ui_destroy(loaded);
	}

	// --- insert_root: roots reorder ---
	bapi_ui_t ui2 = bapi_ui_create();
	bapi_ui_component_t r1 = bapi_ui_component_create(BAPI_UI_COMPONENT_RECT, "r1");
	bapi_ui_component_t r2 = bapi_ui_component_create(BAPI_UI_COMPONENT_RECT, "r2");
	result = result && expect(ui2 && r1 && r2, "create root ui") &&
			 expect(bapi_ui_add_root(ui2, r1) == 0, "add root r1") &&
			 expect(bapi_ui_add_root(ui2, r2) == 0, "add root r2") &&
			 expect(bapi_ui_get_root(ui2, 0) == r1, "root 0 is r1") &&
			 expect(bapi_ui_remove_root(ui2, r1) == 0, "detach r1") &&
			 expect(bapi_ui_insert_root(ui2, r1, 1) == 0, "insert root r1 at 1") &&
			 expect(bapi_ui_get_root(ui2, 0) == r2, "root 0 is r2 after move") &&
			 expect(bapi_ui_get_root(ui2, 1) == r1, "root 1 is r1 after move") &&
			 expect(bapi_ui_insert_root(ui2, r1, -1) == -1, "negative root index fails") &&
			 expect(bapi_ui_insert_root(ui2, r1, 99) == -1, "large root index fails") &&
			 expect(bapi_ui_insert_root(ui2, r1, 0) == -1, "attached root rejected") &&
			 expect(bapi_ui_component_get_parent(r1) == NULL, "root has no parent");
	bapi_ui_destroy(ui2);

	// --- cleanup ---
	bapi_ui_component_destroy(detached);
	bapi_ui_component_destroy(clone);
	bapi_ui_component_destroy(source);
	bapi_ui_destroy(ui);
	return result ? 0 : 1;
}
