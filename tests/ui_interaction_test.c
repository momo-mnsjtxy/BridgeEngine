#include "BridgeEngine.h"
#include "internal/platform/platform.h"
#include "internal/scene/ui_internal.h"
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

// ---- helpers -------------------------------------------------------------

static void update(bapi_ui_t ui, bapi_event_type_t type, float x, float y)
{
	bapi_event_t event;
	memset(&event, 0, sizeof(event));
	event.type = type;
	switch (type) {
	case BAPI_EVENT_MOUSE_MOTION:
		event.data.motion.x = x;
		event.data.motion.y = y;
		break;
	case BAPI_EVENT_MOUSE_BUTTON_DOWN:
	case BAPI_EVENT_MOUSE_BUTTON_UP:
		event.data.button.x = x;
		event.data.button.y = y;
		event.data.button.button = BAPI_BUTTON_LEFT;
		break;
	default:
		break;
	}
	bapi_ui_update(ui, &event);
}

static void key_down(bapi_ui_t ui, uint32_t key)
{
	bapi_event_t event;
	memset(&event, 0, sizeof(event));
	event.type		  = BAPI_EVENT_KEY_DOWN;
	event.data.key.key = key;
	bapi_ui_update(ui, &event);
}

static void text_input(bapi_ui_t ui, const char *text)
{
	bapi_event_t event;
	memset(&event, 0, sizeof(event));
	event.type			 = BAPI_EVENT_TEXT_INPUT;
	strncpy(event.data.text.text, text, sizeof(event.data.text.text) - 1);
	event.data.text.text[sizeof(event.data.text.text) - 1] = '\0';
	bapi_ui_update(ui, &event);
}

static void wheel(bapi_ui_t ui, float x, float y, float delta)
{
	bapi_event_t event;
	memset(&event, 0, sizeof(event));
	event.type				= BAPI_EVENT_MOUSE_WHEEL;
	event.data.wheel.x		= x;
	event.data.wheel.y		= delta;
	event.data.wheel.mouse_x = x;
	event.data.wheel.mouse_y = y;
	bapi_ui_update(ui, &event);
}

static void click(bapi_ui_t ui, float x, float y)
{
	update(ui, BAPI_EVENT_MOUSE_MOTION, x, y);
	update(ui, BAPI_EVENT_MOUSE_BUTTON_DOWN, x, y);
	update(ui, BAPI_EVENT_MOUSE_BUTTON_UP, x, y);
}

// ---- scene ---------------------------------------------------------------

static bapi_ui_t build_ui(bapi_ui_component_t *out_cb, bapi_ui_component_t *out_slider,
						  bapi_ui_component_t *out_input, bapi_ui_component_t *out_btn,
						  bapi_ui_component_t *out_scroll)
{
	bapi_ui_t ui = bapi_ui_create();
	if (!ui) return NULL;

	bapi_ui_component_t root = bapi_ui_component_create(BAPI_UI_COMPONENT_CONTAINER, "root");
	bapi_ui_component_t cb	 = bapi_ui_component_create(BAPI_UI_COMPONENT_CHECKBOX, "cb");
	bapi_ui_component_t sl	 = bapi_ui_component_create(BAPI_UI_COMPONENT_SLIDER, "sl");
	bapi_ui_component_t in	 = bapi_ui_component_create(BAPI_UI_COMPONENT_INPUT, "in");
	bapi_ui_component_t btn	 = bapi_ui_component_create(BAPI_UI_COMPONENT_BUTTON, "btn");
	bapi_ui_component_t sc	 = bapi_ui_component_create(BAPI_UI_COMPONENT_SCROLL, "sc");
	bapi_ui_component_t panel = bapi_ui_component_create(BAPI_UI_COMPONENT_PANEL, "sc_panel");
	if (!root || !cb || !sl || !in || !btn || !sc || !panel) {
		bapi_ui_component_destroy(root);
		bapi_ui_component_destroy(cb);
		bapi_ui_component_destroy(sl);
		bapi_ui_component_destroy(in);
		bapi_ui_component_destroy(btn);
		bapi_ui_component_destroy(sc);
		bapi_ui_component_destroy(panel);
		bapi_ui_destroy(ui);
		return NULL;
	}

	bapi_ui_component_set_rect(cb, (bapi_rect_t){10, 10, 80, 24});
	bapi_ui_component_set_rect(sl, (bapi_rect_t){10, 50, 160, 24});
	bapi_ui_component_set_rect(in, (bapi_rect_t){10, 90, 120, 24});
	bapi_ui_component_set_rect(btn, (bapi_rect_t){10, 130, 80, 32});
	bapi_ui_component_set_rect(sc, (bapi_rect_t){10, 180, 200, 200});
	bapi_ui_component_set_rect(panel, (bapi_rect_t){0, 0, 180, 400});
	bapi_ui_component_set_min_value(sl, 0.0f);
	bapi_ui_component_set_max_value(sl, 100.0f);

	bapi_ui_add_root(ui, root);
	bapi_ui_component_add_child(root, cb);
	bapi_ui_component_add_child(root, sl);
	bapi_ui_component_add_child(root, in);
	bapi_ui_component_add_child(root, btn);
	bapi_ui_component_add_child(root, sc);
	bapi_ui_component_add_child(sc, panel);

	*out_cb = cb;
	*out_slider = sl;
	*out_input = in;
	*out_btn = btn;
	*out_scroll = sc;
	return ui;
}

static float approx(float a, float b)
{
	float d = a - b;
	return d < 0 ? -d : d;
}

int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	int result = 1;
	bapi_ui_component_t cb, sl, in, btn, sc;
	bapi_ui_t ui = build_ui(&cb, &sl, &in, &btn, &sc);
	if (!ui) {
		fprintf(stderr, "ui failed to build\n");
		return 1;
	}

	struct bapi_ui_component_internal *icb = (struct bapi_ui_component_internal *)cb;
	struct bapi_ui_component_internal *iin = (struct bapi_ui_component_internal *)in;

	// --- hover: motion over the checkbox sets its hovered transient ---
	update(ui, BAPI_EVENT_MOUSE_MOTION, 50, 20);
	result = expect(icb->hovered == 1, "hover sets checkbox hovered");
	update(ui, BAPI_EVENT_MOUSE_MOTION, 500, 500);
	result = result && expect(icb->hovered == 0, "motion elsewhere clears hover");

	// --- click toggles checkbox ---
	click(ui, 50, 20);
	result = result && expect(bapi_ui_component_is_checked(cb) == 1, "click checks checkbox");
	click(ui, 50, 20);
	result = result && expect(bapi_ui_component_is_checked(cb) == 0, "second click unchecks");

	// --- click slider sets value from pointer x ---
	// rect x=10 w=160, click x=85 => ratio (85-10)/160 = 0.46875 => value 46.875
	click(ui, 85, 62);
	result = result &&
			 expect(approx(bapi_ui_component_get_value(sl), 46.875f) < 0.01f,
					"slider value follows click x");

	// --- click button registers a click ---
	click(ui, 50, 145);
	result = result && expect(bapi_ui_was_clicked(ui, "btn") == 1, "button was clicked");

	// --- TAB focus cycling, ENTER activates focused ---
	// focus order follows child order: cb, sl, in, btn, sc (panel not interactive)
	key_down(ui, KEY_TAB);
	result = result && expect(ui->focused == cb, "TAB focuses first interactive node");
	key_down(ui, KEY_ENTER);
	result = result && expect(bapi_ui_component_is_checked(cb) == 1, "ENTER toggles focused checkbox");
	key_down(ui, KEY_TAB);
	result = result && expect(ui->focused == sl, "second TAB reaches slider");

	// --- click an input to focus it, type ASCII + UTF-8 ---
	click(ui, 50, 100);
	result = result && expect(ui->focused == in, "click focuses input");
	key_down(ui, 'a');
	key_down(ui, 'b');
	result = result && expect(strcmp(bapi_ui_component_get_text(in), "ab") == 0,
							  "ASCII keys append to input");
	key_down(ui, KEY_BACKSPACE);
	result = result && expect(strcmp(bapi_ui_component_get_text(in), "a") == 0,
							  "backspace removes an ASCII byte");
	text_input(ui, "\xe4\xbd\xa0"); // UTF-8 "你"
	result = result && expect(strcmp(bapi_ui_component_get_text(in), "a\xe4\xbd\xa0") == 0,
							  "text input appends UTF-8 bytes");

	// --- input focus: clicking elsewhere then typing does not corrupt ---
	click(ui, 50, 20);
	key_down(ui, 'x');
	result = result && expect(strcmp(bapi_ui_component_get_text(in), "a\xe4\xbd\xa0") == 0,
							  "typing with focus elsewhere ignores input");

	// --- text input with no focused input is a safe no-op ---
	key_down(ui, KEY_ESC);
	text_input(ui, "abc");
	result = result && expect(bapi_ui_component_is_focused(in) == 0, "ESC clears focus");

	// --- wheel scrolls the SCROLL ancestor and clamps ---
	wheel(ui, 50, 200, -1.0f);
	result = result && expect(approx(bapi_ui_component_get_scroll_offset(sc), 20.0f) < 0.01f,
							  "wheel down scrolls by one step");
	wheel(ui, 50, 200, 1.0f);
	result = result && expect(approx(bapi_ui_component_get_scroll_offset(sc), 0.0f) < 0.01f,
							  "wheel up scrolls back");
	// content 400 - view 200 = 200 max, step 20 => 10 notches to clamp
	for (int i = 0; i < 15; i++) wheel(ui, 50, 200, -1.0f);
	result = result &&
			 expect(approx(bapi_ui_component_get_scroll_offset(sc), 200.0f) < 0.01f,
					"scroll clamps at content extent");

	// --- wheel over non-scroll area does not scroll ---
	bapi_ui_component_set_scroll_offset(sc, 50.0f);
	wheel(ui, 500, 500, -1.0f);
	result = result && expect(approx(bapi_ui_component_get_scroll_offset(sc), 50.0f) < 0.01f,
							  "wheel off the scroll area is a no-op");

	// --- press persists during drag, cleared on release ---
	update(ui, BAPI_EVENT_MOUSE_BUTTON_DOWN, 50, 145);
	result = result && expect(((struct bapi_ui_component_internal *)btn)->pressed == 1,
							  "press sets pressed");
	update(ui, BAPI_EVENT_MOUSE_MOTION, 500, 500);
	result = result && expect(((struct bapi_ui_component_internal *)btn)->pressed == 1,
							  "motion elsewhere keeps pressed while held");
	update(ui, BAPI_EVENT_MOUSE_BUTTON_UP, 500, 500);
	result = result && expect(((struct bapi_ui_component_internal *)btn)->pressed == 0,
							  "release clears pressed");

	bapi_ui_destroy(ui);
	return result ? 0 : 1;
}
