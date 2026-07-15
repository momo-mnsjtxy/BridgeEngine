#include "internal/bapi_internal.h"
#include "internal/engine/texture_internal.h"
#include "internal/platform/platform.h"
#include "BridgeEngine.h"
#include <stdio.h>
#include <stdlib.h>

struct plat_renderer {
	int unused;
};

struct plat_texture {
	int width;
	int height;
};

struct plat_surface {
	int width;
	int height;
};

static struct plat_renderer fake_renderer;
static float				rendered_width;
static float				rendered_height;
static int				destroyed_texture_count;

plat_renderer_t bapi_internal_get_renderer(void)
{
	return &fake_renderer;
}

static plat_surface_t *load_image(const char *filepath)
{
	if (!filepath) return NULL;
	plat_surface_t *surface = malloc(sizeof(*surface));
	if (surface) {
		surface->width	= 320;
		surface->height = 180;
	}
	return surface;
}

static void destroy_surface(plat_surface_t *surface)
{
	free(surface);
}

static plat_texture_t create_texture_from_surface(plat_renderer_t renderer, plat_surface_t *surface)
{
	if (!renderer || !surface) return NULL;
	plat_texture_t texture = malloc(sizeof(*texture));
	if (texture) {
		texture->width	= surface->width;
		texture->height = surface->height;
	}
	return texture;
}

static void destroy_texture(plat_texture_t texture)
{
	destroyed_texture_count++;
	free(texture);
}

static int get_texture_size(plat_texture_t texture, int *width, int *height)
{
	if (!texture || !width || !height) return -1;
	*width	= texture->width;
	*height = texture->height;
	return 0;
}

static void render_texture(plat_renderer_t renderer, plat_texture_t texture, float x, float y,
						   float width, float height)
{
	(void)renderer;
	(void)texture;
	(void)x;
	(void)y;
	rendered_width	= width;
	rendered_height = height;
}

static int expect(int condition, const char *message)
{
	if (!condition) fprintf(stderr, "%s\n", message);
	return condition;
}

int main(void)
{
	const plat_interface_t platform = {
		.renderer = {.render_texture = render_texture},
		.texture =
			{
				.create_texture_from_surface = create_texture_from_surface,
				.get_texture_size			 = get_texture_size,
				.destroy_texture			 = destroy_texture,
				.load_image					 = load_image,
				.destroy_surface			 = destroy_surface,
			},
	};
	if (plat_init(&platform) != 0) return 1;
	bapi_texture_t texture = bapi_texture_load("test-image");
	int			   width   = 0;
	int			   height  = 0;
	bapi_texture_get_size(texture, &width, &height);
	bapi_texture_render(texture, 0, 0);
	bapi_texture_t cached_first	 = bapi_texture_from_file("cached-image", NULL, NULL);
	bapi_texture_t cached_second = bapi_texture_from_file("cached-image", NULL, NULL);
	int			   result = expect(texture != NULL, "texture loads") &&
							expect(texture->cache_key == NULL, "uncached texture has no cache key") &&
							expect(width == 320 && height == 180, "texture reports source size") &&
							expect(rendered_width == 320 && rendered_height == 180,
								   "default render uses source size") &&
							expect(cached_first == cached_second, "cached loads reuse a texture") &&
							expect(cached_first != NULL, "cached texture loads");
	bapi_texture_destroy(texture);
	bapi_texture_destroy(cached_first);
	bapi_texture_cache_clear();
	bapi_texture_render(cached_second, 0, 0);
	result = result && expect(rendered_width == 320 && rendered_height == 180,
							  "cache clear preserves caller texture");
	bapi_texture_t cached_after_clear = bapi_texture_from_file("cached-image", NULL, NULL);
	result =
		result && expect(cached_after_clear != cached_second, "cache clear removes cached entry");
	bapi_texture_destroy(cached_second);
	bapi_texture_destroy(cached_after_clear);
	bapi_texture_cache_clear();
	result = result && expect(destroyed_texture_count == 3, "all texture backends are destroyed once");
	plat_shutdown();
	return result ? 0 : 1;
}
