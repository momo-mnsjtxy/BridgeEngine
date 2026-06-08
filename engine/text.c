#include "text/text.h"
#include "master/init.h"
#include "bapi_internal.h"
#include "platform/platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_FONTS 8
#define FONT_PATH "text/font.ttf"

typedef struct {
	plat_font_t font;
	float size;
	int in_use;
} cached_font_t;

static cached_font_t g_font_cache[MAX_FONTS] = {0};
static int g_text_initialized = 0;

static plat_font_t get_or_load_font(float size)
{
	const plat_interface_t* plat = plat_get();

	for (int i = 0; i < MAX_FONTS; i++) {
		if (g_font_cache[i].in_use && g_font_cache[i].size == size) {
			return g_font_cache[i].font;
		}
	}

	for (int i = 0; i < MAX_FONTS; i++) {
		if (!g_font_cache[i].in_use) {
			g_font_cache[i].font = plat->open_font(FONT_PATH, size);
			if (g_font_cache[i].font) {
				g_font_cache[i].size = size;
				g_font_cache[i].in_use = 1;
				return g_font_cache[i].font;
			}
			return NULL;
		}
	}

	return NULL;
}

void bapi_text_init(void)
{
	if (g_text_initialized) return;
	memset(g_font_cache, 0, sizeof(g_font_cache));
	g_text_initialized = 1;
}

void bapi_text_cleanup(void)
{
	const plat_interface_t* plat = plat_get();
	for (int i = 0; i < MAX_FONTS; i++) {
		if (g_font_cache[i].in_use && g_font_cache[i].font) {
			plat->close_font(g_font_cache[i].font);
			g_font_cache[i].font = NULL;
			g_font_cache[i].in_use = 0;
		}
	}
	g_text_initialized = 0;
}

void bapi_draw_text(const char* text, float x, float y, float size, bapi_color_t color)
{
	if (!text || !text[0]) return;

	const plat_interface_t* plat = plat_get();

	plat_font_t font = get_or_load_font(size);
	if (!font) {
		return;
	}

	plat_surface_t* surface = plat->render_text_blended(font, text, (int)strlen(text),
							    color.r, color.g, color.b, color.a);
	if (!surface) {
		return;
	}

	plat_texture_t texture = plat->create_texture_from_surface(bapi_internal_renderer, surface);
	if (!texture) {
		plat->destroy_surface(surface);
		return;
	}

	
	int surf_w = 0, surf_h = 0;
	plat->get_string_size(font, text, (int)strlen(text), &surf_w, &surf_h);

	plat->render_texture(bapi_internal_renderer, texture, x, y, (float)surf_w, (float)surf_h);

	plat->destroy_texture(texture);
	plat->destroy_surface(surface);
}

void bapi_get_text_size(const char* text, float size, float* width, float* height)
{
	if (width) *width = 0;
	if (height) *height = 0;

	if (!text || !text[0]) return;

	const plat_interface_t* plat = plat_get();

	plat_font_t font = get_or_load_font(size);
	if (!font) return;

	int w = 0, h = 0;
	plat->get_string_size(font, text, (int)strlen(text), &w, &h);

	if (width) *width = (float)w;
	if (height) *height = (float)h;
}
