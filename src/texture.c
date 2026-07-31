#include "BridgeEngine.h"
#include "internal/bapi_internal.h"
#include "internal/engine/texture_internal.h"
#include "internal/platform/platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bapi_texture_t g_texture_cache[BAPI_MAX_CACHED_TEXTURES];
static bapi_texture_t g_allocated_textures;

static char *bapi_texture_copy_key(const char *filepath)
{
	size_t length = strlen(filepath) + 1;
	char  *copy	  = malloc(length);
	if (copy) memcpy(copy, filepath, length);
	return copy;
}

static void bapi_texture_remove_from_cache(bapi_texture_t texture)
{
	for (int index = 0; index < BAPI_MAX_CACHED_TEXTURES; index++) {
		if (g_texture_cache[index] == texture) {
			g_texture_cache[index] = NULL;
			return;
		}
	}
}

static void bapi_texture_remove_from_allocated(bapi_texture_t texture)
{
	bapi_texture_t *current = &g_allocated_textures;
	while (*current != NULL) {
		if (*current == texture) {
			*current = texture->next_allocated;
			return;
		}
		current = &(*current)->next_allocated;
	}
}

static void bapi_texture_free(bapi_texture_t texture)
{
	const plat_interface_t *plat = plat_get();
	bapi_texture_remove_from_cache(texture);
	bapi_texture_remove_from_allocated(texture);
	if (plat != NULL && texture->platform_texture != NULL) {
		plat->texture.destroy_texture(texture->platform_texture);
	}
	free(texture->cache_key);
	free(texture);
}

static void bapi_texture_release(bapi_texture_t texture)
{
	if (!texture || --texture->reference_count > 0) return;
	bapi_texture_free(texture);
}

bapi_texture_t bapi_texture_load(const char *filepath)
{
	if (!filepath || !filepath[0]) return NULL;

	const plat_interface_t *plat = plat_get();
	if (!plat) return NULL;

	plat_surface_t *surface = plat->texture.load_image(filepath);
	if (!surface) {
		printf("[TEXTURE] Failed to load: %s\n", filepath);
		return NULL;
	}

	plat_texture_t plat_tex =
		plat->texture.create_texture_from_surface(bapi_internal_get_renderer(), surface);
	plat->texture.destroy_surface(surface);

	if (!plat_tex) {
		printf("[TEXTURE] Failed to create texture from: %s\n", filepath);
		return NULL;
	}

	bapi_texture_t tex = calloc(1, sizeof(struct bapi_texture_internal));
	if (!tex) {
		plat->texture.destroy_texture(plat_tex);
		return NULL;
	}
	tex->platform_texture = plat_tex;
	tex->reference_count  = 1;
	if (plat->texture.get_texture_size(plat_tex, &tex->width, &tex->height) != 0 ||
		tex->width <= 0 || tex->height <= 0) {
		plat->texture.destroy_texture(plat_tex);
		free(tex);
		return NULL;
	}
	tex->next_allocated = g_allocated_textures;
	g_allocated_textures = tex;
	return tex;
}

void bapi_texture_destroy(bapi_texture_t texture)
{
	bapi_texture_release(texture);
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
	plat->renderer.render_texture(bapi_internal_get_renderer(), texture->platform_texture, x, y, w,
								  h);
}

void bapi_texture_get_size(bapi_texture_t texture, int *w, int *h)
{
	if (w) *w = texture ? texture->width : 0;
	if (h) *h = texture ? texture->height : 0;
}

void bapi_texture_cache_clear(void)
{
	for (int index = 0; index < BAPI_MAX_CACHED_TEXTURES; index++) {
		bapi_texture_t texture = g_texture_cache[index];
		g_texture_cache[index] = NULL;
		bapi_texture_release(texture);
	}
}

void bapi_texture_cleanup(void)
{
	memset(g_texture_cache, 0, sizeof(g_texture_cache));
	while (g_allocated_textures != NULL) {
		bapi_texture_free(g_allocated_textures);
	}
}

bapi_texture_t bapi_texture_from_file(const char *filepath, int *out_w, int *out_h)
{
	if (out_w) *out_w = 0;
	if (out_h) *out_h = 0;
	if (!filepath || !filepath[0]) return NULL;

	int free_slot = -1;
	for (int index = 0; index < BAPI_MAX_CACHED_TEXTURES; index++) {
		bapi_texture_t texture = g_texture_cache[index];
		if (!texture) {
			if (free_slot < 0) free_slot = index;
			continue;
		}
		if (strcmp(texture->cache_key, filepath) == 0) {
			texture->reference_count++;
			if (out_w) *out_w = texture->width;
			if (out_h) *out_h = texture->height;
			return texture;
		}
	}

	bapi_texture_t texture = bapi_texture_load(filepath);
	if (!texture) return NULL;
	if (free_slot >= 0) {
		texture->cache_key = bapi_texture_copy_key(filepath);
		if (texture->cache_key) {
			g_texture_cache[free_slot] = texture;
			texture->reference_count++;
		}
	}
	if (out_w) *out_w = texture->width;
	if (out_h) *out_h = texture->height;
	return texture;
}
