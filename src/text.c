#include "BridgeEngine.h"
#include "internal/bapi_internal.h"
#include "internal/engine/render_context.h"
#include "internal/platform/platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_FONTS		  64
#define FONT_PATH_RUNTIME "assets/text/font.ttf"
#define FONT_PATH_SOURCE  "examples/assets/text/font.ttf"
#define FONT_PATH_LEGACY  "text/font.ttf"

typedef struct {
	plat_font_t font;
	float		size;
	int			in_use;
	int			last_used;
} cached_font_t;

static cached_font_t g_font_cache[MAX_FONTS] = {0};
static int			g_font_clock = 0;

// Quantize to integer pixels so zooming reuses cached fonts instead of
// creating a new font for every fractional size.
static float font_quantize_size(float size)
{
	float q = (float)(int)(size + 0.5f);
	return q < 1.0f ? 1.0f : q;
}

static plat_font_t get_or_load_font(float size)
{
	if (!bapi_runtime_is_text_initialized()) bapi_text_init();
	if (!bapi_runtime_is_text_initialized()) return NULL;

	const plat_interface_t *plat = plat_get();

	size = font_quantize_size(size);

	for (int i = 0; i < MAX_FONTS; i++) {
		if (g_font_cache[i].in_use && g_font_cache[i].size == size) {
			g_font_cache[i].last_used = ++g_font_clock;
			return g_font_cache[i].font;
		}
	}

	// prefer a free slot; otherwise reuse the least-recently-used one so the
	// cache can never silently starve rendering (which made all text vanish
	// when zooming changed every requested size at once)
	int slot = -1;
	for (int i = 0; i < MAX_FONTS; i++) {
		if (!g_font_cache[i].in_use) {
			slot = i;
			break;
		}
	}
	if (slot < 0) {
		int oldest = 0;
		for (int i = 1; i < MAX_FONTS; i++) {
			if (g_font_cache[i].last_used < g_font_cache[oldest].last_used) oldest = i;
		}
		slot = oldest;
		if (g_font_cache[slot].font) {
			plat->text.close_font(g_font_cache[slot].font);
			g_font_cache[slot].font = NULL;
		}
	}

	const char *font_paths[] = {FONT_PATH_RUNTIME, FONT_PATH_SOURCE, FONT_PATH_LEGACY};
	for (size_t path_index = 0; path_index < sizeof(font_paths) / sizeof(font_paths[0]);
		 path_index++) {
		g_font_cache[slot].font = plat->text.open_font(font_paths[path_index], size);
		if (g_font_cache[slot].font) {
			g_font_cache[slot].size		= size;
			g_font_cache[slot].in_use	= 1;
			g_font_cache[slot].last_used = ++g_font_clock;
			return g_font_cache[slot].font;
		}
	}

	g_font_cache[slot].in_use = 0;
	return NULL;
}

void bapi_text_init(void)
{
	if (!bapi_runtime_is_initialized() || bapi_runtime_is_text_initialized()) return;
	memset(g_font_cache, 0, sizeof(g_font_cache));
	bapi_runtime_set_text_initialized(true);
}

void bapi_text_cleanup(void)
{
	const plat_interface_t *plat = plat_get();
	if (!bapi_runtime_is_text_initialized()) return;
	for (int i = 0; i < MAX_FONTS; i++) {
		if (plat && g_font_cache[i].in_use && g_font_cache[i].font) {
			plat->text.close_font(g_font_cache[i].font);
			g_font_cache[i].font   = NULL;
			g_font_cache[i].in_use = 0;
		}
	}
	bapi_runtime_set_text_initialized(false);
}

void bapi_draw_text(const char *text, float x, float y, float size, bapi_color_t color)
{
	if (!text || !text[0]) return;
	if (!bapi_runtime_is_initialized()) return;

	const plat_interface_t *plat	 = plat_get();
	plat_renderer_t			renderer = bapi_internal_get_renderer();

	plat_font_t font = get_or_load_font(size);
	if (!font) {
		return;
	}

	plat_surface_t *surface = plat->text.render_text_blended(font, text, (int)strlen(text), color.r,
															 color.g, color.b, color.a);
	if (!surface) {
		return;
	}

	plat_texture_t texture = plat->texture.create_texture_from_surface(renderer, surface);
	if (!texture) {
		plat->texture.destroy_surface(surface);
		return;
	}

	int surf_w = 0, surf_h = 0;
	plat->text.get_string_size(font, text, (int)strlen(text), &surf_w, &surf_h);

	plat->renderer.render_texture(renderer, texture, x, y, (float)surf_w, (float)surf_h);

	plat->texture.destroy_texture(texture);
	plat->texture.destroy_surface(surface);
}

void bapi_get_text_size(const char *text, float size, float *width, float *height)
{
	if (width) *width = 0;
	if (height) *height = 0;

	if (!text || !text[0]) return;
	if (!bapi_runtime_is_initialized()) return;

	const plat_interface_t *plat = plat_get();

	plat_font_t font = get_or_load_font(size);
	if (!font) return;

	int w = 0, h = 0;
	plat->text.get_string_size(font, text, (int)strlen(text), &w, &h);

	if (width) *width = (float)w;
	if (height) *height = (float)h;
}
