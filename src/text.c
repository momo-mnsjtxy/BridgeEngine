#include "BridgeEngine.h"
#include "internal/bapi_internal.h"
#include "internal/engine/render_context.h"
#include "internal/platform/platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_FONTS		  8
#define FONT_PATH_RUNTIME "assets/text/font.ttf"
#define FONT_PATH_SOURCE  "examples/assets/text/font.ttf"

typedef struct {
	plat_font_t font;
	float		size;
	int			in_use;
} cached_font_t;

static cached_font_t g_font_cache[MAX_FONTS] = {0};

static plat_font_t get_or_load_font(float size)
{
	if (!bapi_runtime_is_text_initialized()) bapi_text_init();
	if (!bapi_runtime_is_text_initialized()) return NULL;

	const plat_interface_t *plat = plat_get();

	for (int i = 0; i < MAX_FONTS; i++) {
		if (g_font_cache[i].in_use && g_font_cache[i].size == size) {
			return g_font_cache[i].font;
		}
	}

	for (int i = 0; i < MAX_FONTS; i++) {
		if (!g_font_cache[i].in_use) {
			const char *font_paths[] = {FONT_PATH_RUNTIME, FONT_PATH_SOURCE};
			for (size_t path_index = 0; path_index < sizeof(font_paths) / sizeof(font_paths[0]);
				 path_index++) {
				g_font_cache[i].font = plat->text.open_font(font_paths[path_index], size);
				if (g_font_cache[i].font) {
					g_font_cache[i].size   = size;
					g_font_cache[i].in_use = 1;
					return g_font_cache[i].font;
				}
			}
			return NULL;
		}
	}

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
