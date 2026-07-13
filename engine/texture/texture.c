#include "texture/texture.h"
#include "bapi_internal.h"
#include "master/init.h"
#include "platform/platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bapi_texture_t bapi_texture_load(const char *filepath)
{
	if (!filepath || !filepath[0]) return NULL;

	const plat_interface_t *plat = plat_get();
	if (!plat) return NULL;

	plat_surface_t *surface = plat->load_image(filepath);
	if (!surface) {
		printf("[TEXTURE] Failed to load: %s\n", filepath);
		return NULL;
	}

	plat_texture_t plat_tex =
		plat->create_texture_from_surface(bapi_internal_get_renderer(), surface);
	plat->destroy_surface(surface);

	if (!plat_tex) {
		printf("[TEXTURE] Failed to create texture from: %s\n", filepath);
		return NULL;
	}

	bapi_texture_t tex = malloc(sizeof(struct bapi_texture_internal));
	if (!tex) {
		plat->destroy_texture(plat_tex);
		return NULL;
	}
	tex->plat_texture = plat_tex;
	return tex;
}

void bapi_texture_destroy(bapi_texture_t texture)
{
	if (!texture) return;
	const plat_interface_t *plat = plat_get();
	if (plat) {
		plat->destroy_texture(texture->plat_texture);
	}
	free(texture);
}

void bapi_texture_render(bapi_texture_t texture, float x, float y)
{
	if (!texture) return;
	int w = 0, h = 0;
	bapi_texture_get_size(texture, &w, &h);
	bapi_texture_render_ex(texture, x, y, (float)w, (float)h);
}

void bapi_texture_render_ex(bapi_texture_t texture, float x, float y, float w, float h)
{
	if (!texture) return;
	const plat_interface_t *plat = plat_get();
	if (!plat) return;
	plat->render_texture(bapi_internal_get_renderer(), texture->plat_texture, x, y, w, h);
}

void bapi_texture_get_size(bapi_texture_t texture, int *w, int *h)
{
	if (!texture) return;
	if (w) *w = 0;
	if (h) *h = 0;
}
