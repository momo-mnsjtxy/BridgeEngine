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
static char							render_order[16];
static int							render_order_count;
static float						mouse_x;
static float						mouse_y;

static int expect(int condition, const char *message)
{
	if (!condition) fprintf(stderr, "%s\n", message);
	return condition;
}

static void append_render(char code)
{
	if (render_order_count < (int)sizeof(render_order) - 1) {
		render_order[render_order_count++] = code;
		render_order[render_order_count]   = '\0';
	}
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
	fill_rect_count++;
	append_render('r');
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
	(void)x;
	(void)y;
	(void)size;
	(void)color;
	if (strcmp(text, "Main & Menu") == 0)
		append_render('l');
	else if (strcmp(text, "Start") == 0)
		append_render('b');
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
	if (texture) {
		texture_render_count++;
		append_render('i');
	}
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

int main(int argc, char **argv)
{
	char valid_path[512];
	char invalid_path[512];
	char duplicate_path[512];
	char components_path[512];
	char interaction_path[512];
	if (argc != 2) return 1;
	snprintf(valid_path, sizeof(valid_path), "%s/ui.xml", argv[1]);
	snprintf(invalid_path, sizeof(invalid_path), "%s/ui_invalid.xml", argv[1]);
	snprintf(duplicate_path, sizeof(duplicate_path), "%s/ui_duplicate.xml", argv[1]);
	snprintf(components_path, sizeof(components_path), "%s/ui_components.xml", argv[1]);
	snprintf(interaction_path, sizeof(interaction_path), "%s/ui_interaction.xml", argv[1]);

	const char *valid_xml =
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
		"<ui>\n"
		"  <rect id=\"panel\" x=\"1\" y=\"2\" w=\"3\" h=\"4\" color=\"#01020304\" />\n"
		"  <label\n"
		"      id=\"title\" x=\"10\" y=\"20\" text=\"Main &amp; Menu\" size=\"18\"\n"
		"      color=\"#FFFFFFFF\" />\n"
		"  <button id=\"start\" x=\"30\" y=\"40\" w=\"100\" h=\"50\" text=\"Start\"\n"
		"      text_size=\"16\" normal=\"#111111FF\" hover=\"#222222FF\"\n"
		"      click=\"#333333FF\" text_color=\"#FFFFFFFF\" />\n"
		"  <image id=\"logo\" x=\"5\" y=\"6\" w=\"7\" h=\"8\" src=\"assets/logo.png\" />\n"
		"</ui>\n";
	const char *invalid_xml = "<ui><unknown id=\"bad\" /></ui>\n";
	const char *duplicate_xml =
		"<ui>\n"
		"  <rect id=\"same\" x=\"0\" y=\"0\" w=\"1\" h=\"1\" color=\"#FFFFFF\" />\n"
		"  <label id=\"same\" x=\"0\" y=\"0\" text=\"dup\" size=\"1\" color=\"#FFFFFF\" />\n"
		"</ui>\n";
	const char *components_xml =
		"<ui>\n"
		"  <panel id=\"panel\" x=\"10\" y=\"10\" w=\"500\" h=\"400\" color=\"#101010\">\n"
		"    <row id=\"row\" x=\"20\" y=\"20\" w=\"300\" h=\"40\" step=\"5\">\n"
		"      <checkbox id=\"check\" x=\"0\" y=\"0\" w=\"20\" h=\"20\" />\n"
		"      <toggle id=\"toggle\" x=\"0\" y=\"0\" w=\"20\" h=\"20\" />\n"
		"      <radio id=\"radio\" x=\"0\" y=\"0\" w=\"20\" h=\"20\" />\n"
		"    </row>\n"
		"    <line id=\"line\" x=\"1\" y=\"2\" w=\"3\" h=\"4\" color=\"#FFFFFF\" />\n"
		"    <circle id=\"circle\" x=\"30\" y=\"30\" radius=\"10\" color=\"#FFFFFF\" />\n"
		"    <polygon id=\"polygon\" x=\"50\" y=\"50\" radius=\"10\" sides=\"5\" color=\"#FFFFFF\" "
		"/>\n"
		"    <border id=\"border\" x=\"1\" y=\"1\" w=\"10\" h=\"10\" color=\"#FFFFFF\" />\n"
		"    <progress id=\"progress\" x=\"1\" y=\"1\" w=\"100\" h=\"10\" value=\"0.5\" "
		"color=\"#00FF00\" />\n"
		"    <slider id=\"slider\" x=\"1\" y=\"20\" w=\"100\" h=\"10\" value=\"0.25\" />\n"
		"    <separator id=\"separator\" x=\"1\" y=\"1\" w=\"10\" h=\"1\" color=\"#FFFFFF\" />\n"
		"    <grid id=\"grid\" x=\"20\" y=\"100\" w=\"200\" h=\"100\" columns=\"2\" step=\"4\">\n"
		"      <label id=\"grid_a\" x=\"0\" y=\"0\" text=\"A\" size=\"12\" />\n"
		"      <label id=\"grid_b\" x=\"0\" y=\"0\" text=\"B\" size=\"12\" />\n"
		"    </grid>\n"
		"    <input id=\"input\" x=\"20\" y=\"220\" w=\"200\" h=\"30\" text=\"edit\" />\n"
		"    <select id=\"select\" x=\"20\" y=\"260\" w=\"100\" h=\"30\" text=\"choice\" />\n"
		"    <list id=\"list\" x=\"20\" y=\"300\" w=\"100\" h=\"30\" text=\"items\" />\n"
		"    <scroll id=\"scroll\" x=\"20\" y=\"340\" w=\"100\" h=\"30\" />\n"
		"    <tab id=\"tab\" x=\"130\" y=\"300\" w=\"100\" h=\"30\" text=\"tab\" />\n"
		"  </panel>\n"
		"</ui>\n";
	const char *interaction_xml =
		"<ui>\n"
		"  <panel id=\"interaction_panel\" x=\"0\" y=\"0\" w=\"400\" h=\"200\">\n"
		"    <button id=\"bottom\" x=\"10\" y=\"10\" w=\"100\" h=\"30\" text=\"Bottom\" />\n"
		"    <button id=\"top\" x=\"10\" y=\"10\" w=\"100\" h=\"30\" text=\"Top\" />\n"
		"    <radio id=\"radio_one\" x=\"140\" y=\"10\" w=\"20\" h=\"20\" />\n"
		"    <radio id=\"radio_two\" x=\"170\" y=\"10\" w=\"20\" h=\"20\" />\n"
		"    <button id=\"disabled\" x=\"10\" y=\"45\" w=\"100\" h=\"10\" text=\"Disabled\" "
		"enabled=\"false\" />\n"
		"    <input id=\"entry\" x=\"10\" y=\"60\" w=\"120\" h=\"30\" text=\"a\" max_length=\"2\" "
		"/>\n"
		"    <slider id=\"range\" x=\"10\" y=\"100\" w=\"100\" h=\"10\" min=\"20\" max=\"100\" "
		"value=\"20\" />\n"
		"    <scroll id=\"scrollbox\" x=\"200\" y=\"10\" w=\"100\" h=\"60\">\n"
		"      <button id=\"scroll_button\" relative=\"true\" x=\"0\" y=\"20\" w=\"80\" h=\"20\" "
		"text=\"Scroll\" />\n"
		"    </scroll>\n"
		"  </panel>\n"
		"</ui>\n";

	if (write_text_file(valid_path, valid_xml) != 0 ||
		write_text_file(invalid_path, invalid_xml) != 0 ||
		write_text_file(duplicate_path, duplicate_xml) != 0 ||
		write_text_file(components_path, components_xml) != 0 ||
		write_text_file(interaction_path, interaction_xml) != 0) {
		return 1;
	}

	bapi_ui_t ui	 = bapi_ui_load_from_xml(valid_path);
	int		  result = expect(ui != NULL, "valid ui xml loads") &&
				 expect(strstr(texture_path, "assets/logo.png") != NULL, "image path is resolved");

	bapi_event_t down = {.type		  = BAPI_EVENT_MOUSE_BUTTON_DOWN,
						 .data.button = {50, 60, BAPI_BUTTON_LEFT}};
	bapi_event_t up	  = {.type		  = BAPI_EVENT_MOUSE_BUTTON_UP,
						 .data.button = {50, 60, BAPI_BUTTON_LEFT}};
	mouse_x			  = 50;
	mouse_y			  = 60;
	bapi_ui_update(ui, &down);
	bapi_ui_update(ui, &up);
	result = result && expect(bapi_ui_was_clicked(ui, "start") == 1, "button click is reported") &&
			 expect(bapi_ui_was_clicked(ui, "title") == 0, "non-button id is not clicked") &&
			 expect(bapi_ui_was_clicked(ui, "missing") == 0, "missing id is not clicked");

	bapi_ui_render(ui);
	result = result && expect(strcmp(render_order, "rlrbi") == 0, "nodes render in xml order") &&
			 expect(fill_rect_count == 2, "rect and button fill render") &&
			 expect(draw_rect_count == 1, "button border renders") &&
			 expect(draw_text_count == 2, "label and button text render") &&
			 expect(texture_render_count == 1, "image renders");
	bapi_ui_destroy(ui);
	result = result && expect(texture_destroy_count == 1, "image texture is destroyed");

	result = result && expect(bapi_ui_load_from_xml(invalid_path) == NULL, "unknown tag fails") &&
			 expect(bapi_ui_load_from_xml(duplicate_path) == NULL, "duplicate id fails") &&
			 expect(bapi_ui_load_from_xml(NULL) == NULL, "null path fails");

	bapi_ui_t components = bapi_ui_load_from_xml(components_path);
	result				 = result && expect(components != NULL, "all UI components load") &&
			 expect(bapi_ui_component_get_type(bapi_ui_find(components, "panel")) ==
						BAPI_UI_COMPONENT_PANEL,
					"panel type is available") &&
			 expect(bapi_ui_component_get_type(bapi_ui_find(components, "slider")) ==
						BAPI_UI_COMPONENT_SLIDER,
					"slider type is available") &&
			 expect(bapi_ui_component_get_type(bapi_ui_find(components, "grid")) ==
						BAPI_UI_COMPONENT_GRID,
					"grid type is available");
	bapi_rect_t child_rect;
	bapi_ui_component_get_rect(bapi_ui_find(components, "toggle"), &child_rect);
	result = result && expect(child_rect.x > 20, "row layout positions children") &&
			 expect(bapi_ui_component_set_text(bapi_ui_find(components, "input"), "changed") == 0,
					"component text can be changed") &&
			 expect(strcmp(bapi_ui_component_get_text(bapi_ui_find(components, "input")),
						   "changed") == 0,
					"component text is readable") &&
			 expect(bapi_ui_component_set_value(bapi_ui_find(components, "progress"), 0.75f) == 0,
					"component value can be changed") &&
			 expect(bapi_ui_component_get_value(bapi_ui_find(components, "progress")) == 0.75f,
					"component value is readable");
	bapi_ui_component_set_visible(bapi_ui_find(components, "panel"), 0);
	result = result && expect(!bapi_ui_component_is_visible(bapi_ui_find(components, "panel")),
							  "component visibility can be changed");
	bapi_ui_destroy(components);

	bapi_ui_t interaction	  = bapi_ui_load_from_xml(interaction_path);
	result					  = result && expect(interaction != NULL, "interaction ui loads");
	bapi_event_t overlap_down = {.type		  = BAPI_EVENT_MOUSE_BUTTON_DOWN,
								 .data.button = {20, 20, BAPI_BUTTON_LEFT}};
	bapi_event_t overlap_up	  = {.type		  = BAPI_EVENT_MOUSE_BUTTON_UP,
								 .data.button = {20, 20, BAPI_BUTTON_LEFT}};
	bapi_ui_update(interaction, &overlap_down);
	bapi_ui_update(interaction, &overlap_up);
	result = result &&
			 expect(bapi_ui_was_clicked(interaction, "top"), "topmost button consumes click") &&
			 expect(!bapi_ui_was_clicked(interaction, "bottom"), "covered button does not click");
	bapi_event_t tab   = {.type = BAPI_EVENT_KEY_DOWN, .data.key = {KEY_TAB}};
	bapi_event_t enter = {.type = BAPI_EVENT_KEY_DOWN, .data.key = {KEY_ENTER}};
	bapi_ui_update(interaction, &tab);
	bapi_ui_update(interaction, &tab);
	bapi_ui_update(interaction, &enter);
	result = result &&
			 expect(bapi_ui_component_is_focused(bapi_ui_find(interaction, "radio_two")),
					"tab moves focus between enabled controls") &&
			 expect(bapi_ui_component_is_checked(bapi_ui_find(interaction, "radio_two")),
					"enter activates focused control");
	bapi_event_t radio_one_down = {.type		= BAPI_EVENT_MOUSE_BUTTON_DOWN,
								   .data.button = {145, 15, BAPI_BUTTON_LEFT}};
	bapi_event_t radio_one_up	= {.type		= BAPI_EVENT_MOUSE_BUTTON_UP,
								   .data.button = {145, 15, BAPI_BUTTON_LEFT}};
	bapi_event_t radio_two_down = {.type		= BAPI_EVENT_MOUSE_BUTTON_DOWN,
								   .data.button = {175, 15, BAPI_BUTTON_LEFT}};
	bapi_event_t radio_two_up	= {.type		= BAPI_EVENT_MOUSE_BUTTON_UP,
								   .data.button = {175, 15, BAPI_BUTTON_LEFT}};
	bapi_ui_update(interaction, &radio_one_down);
	bapi_ui_update(interaction, &radio_one_up);
	bapi_ui_update(interaction, &radio_two_down);
	bapi_ui_update(interaction, &radio_two_up);
	result =
		result && expect(!bapi_ui_component_is_checked(bapi_ui_find(interaction, "radio_one")) &&
							 bapi_ui_component_is_checked(bapi_ui_find(interaction, "radio_two")),
						 "radio siblings are mutually exclusive");
	bapi_event_t input_down	 = {.type		 = BAPI_EVENT_MOUSE_BUTTON_DOWN,
								.data.button = {20, 70, BAPI_BUTTON_LEFT}};
	bapi_event_t input_up	 = {.type		 = BAPI_EVENT_MOUSE_BUTTON_UP,
								.data.button = {20, 70, BAPI_BUTTON_LEFT}};
	bapi_event_t input_key	 = {.type = BAPI_EVENT_KEY_DOWN, .data.key = {'b'}};
	bapi_event_t input_extra = {.type = BAPI_EVENT_KEY_DOWN, .data.key = {'c'}};
	bapi_ui_update(interaction, &input_down);
	bapi_ui_update(interaction, &input_up);
	bapi_ui_update(interaction, &input_key);
	bapi_ui_update(interaction, &input_extra);
	result =
		result &&
		expect(bapi_ui_component_is_focused(bapi_ui_find(interaction, "entry")),
			   "input gains focus") &&
		expect(strcmp(bapi_ui_component_get_text(bapi_ui_find(interaction, "entry")), "ab") == 0,
			   "input honors max length");
	bapi_event_t slider_down = {.type		 = BAPI_EVENT_MOUSE_BUTTON_DOWN,
								.data.button = {60, 105, BAPI_BUTTON_LEFT}};
	bapi_event_t slider_up	 = {.type		 = BAPI_EVENT_MOUSE_BUTTON_UP,
								.data.button = {60, 105, BAPI_BUTTON_LEFT}};
	bapi_ui_update(interaction, &slider_down);
	bapi_ui_update(interaction, &slider_up);
	result =
		result && expect(bapi_ui_component_get_value(bapi_ui_find(interaction, "range")) == 60.0f,
						 "slider maps pointer to min max range");
	bapi_ui_component_set_scroll_offset(bapi_ui_find(interaction, "scrollbox"), 10.0f);
	bapi_ui_layout(interaction);
	bapi_ui_component_get_rect(bapi_ui_find(interaction, "scroll_button"), &child_rect);
	result =
		result && expect(child_rect.y == 20.0f, "scroll layout applies offset") &&
		expect(bapi_ui_component_get_scroll_offset(bapi_ui_find(interaction, "scrollbox")) == 10.0f,
			   "scroll offset is queryable");
	bapi_ui_destroy(interaction);
	bapi_ui_destroy(NULL);
	bapi_ui_update(NULL, NULL);
	bapi_ui_render(NULL);
	return result ? 0 : 1;
}
