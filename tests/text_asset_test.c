#include "BridgeEngine.h"
#include "internal/bapi_internal.h"
#include "internal/engine/render_context.h"
#include "internal/platform/platform.h"
#include <stdio.h>
#include <string.h>

struct plat_font {
	int unused;
};

static struct plat_font fake_font;
static int				text_initialized;
static int				open_attempt_count;
static int				close_font_count;
static const char	*available_font_path;

bool bapi_runtime_is_initialized(void)
{
	return true;
}

bool bapi_runtime_is_text_initialized(void)
{
	return text_initialized;
}

void bapi_runtime_set_text_initialized(bool initialized)
{
	text_initialized = initialized;
}

plat_renderer_t bapi_internal_get_renderer(void)
{
	return NULL;
}

static plat_font_t open_font(const char *filepath, float size)
{
	(void)size;
	open_attempt_count++;
	return strcmp(filepath, available_font_path) == 0 ? &fake_font : NULL;
}

static void close_font(plat_font_t font)
{
	if (font) close_font_count++;
}

static int get_string_size(plat_font_t font, const char *text, int length, int *width, int *height)
{
	(void)font;
	(void)text;
	(void)length;
	*width	= 20;
	*height = 30;
	return 0;
}

static int expect(int condition, const char *message)
{
	if (!condition) fprintf(stderr, "%s\n", message);
	return condition;
}

int main(void)
{
	const plat_interface_t platform = {
		.text =
			{
				.open_font		 = open_font,
				.close_font		 = close_font,
				.get_string_size = get_string_size,
			},
	};
	if (plat_init(&platform) != 0) return 1;

	available_font_path = "examples/assets/text/font.ttf";
	bapi_text_init();
	float width	 = 0;
	float height = 0;
	bapi_get_text_size("text", 16, &width, &height);
	int result =
		expect(width == 20 && height == 30, "fallback font renders text metrics") &&
		expect(open_attempt_count == 2, "source-tree font path follows runtime asset path");
	bapi_text_cleanup();
	result = result && expect(close_font_count == 1, "cleanup closes fallback font");

	open_attempt_count = 0;
	close_font_count	= 0;
	available_font_path = "text/font.ttf";
	bapi_text_init();
	bapi_get_text_size("text", 16, &width, &height);
	result = result && expect(width == 20 && height == 30, "legacy font renders text metrics") &&
			 expect(open_attempt_count == 3, "legacy font path follows current fallbacks");
	bapi_text_cleanup();
	result = result && expect(close_font_count == 1, "cleanup closes legacy font");
	plat_shutdown();
	return result ? 0 : 1;
}
