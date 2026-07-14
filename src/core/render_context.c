#include "internal/engine/render_context.h"
#include <string.h>

typedef struct {
	plat_window_t	window;
	plat_renderer_t renderer;
	bool			initialized;
	bool			text_initialized;
} bapi_runtime_t;

static bapi_runtime_t g_runtime;

extern void bapi_text_cleanup(void);

static void bapi_runtime_release_window_and_renderer(const plat_interface_t *platform)
{
	if (g_runtime.renderer) {
		platform->renderer.destroy_renderer(g_runtime.renderer);
		g_runtime.renderer = NULL;
	}
	if (g_runtime.window) {
		platform->window.destroy_window(g_runtime.window);
		g_runtime.window = NULL;
	}
}

int bapi_runtime_start(const plat_interface_t *platform, const char *title, int width, int height)
{
	if (g_runtime.initialized) return 0;
	if (!platform || plat_init(platform) != 0) return 1;

	const plat_interface_t *active_platform = plat_get();
	if (!active_platform || active_platform->core.init(PLAT_INIT_VIDEO | PLAT_INIT_AUDIO) != 0) {
		plat_shutdown();
		return 1;
	}

	g_runtime.window = active_platform->window.create_window(title, width, height);
	if (!g_runtime.window) {
		active_platform->core.quit();
		plat_shutdown();
		return 1;
	}

	g_runtime.renderer = active_platform->renderer.create_renderer(g_runtime.window);
	if (!g_runtime.renderer) {
		bapi_runtime_release_window_and_renderer(active_platform);
		active_platform->core.quit();
		plat_shutdown();
		return 1;
	}

	active_platform->renderer.set_render_draw_color(g_runtime.renderer, 0, 0, 0, 255);
	active_platform->renderer.render_clear(g_runtime.renderer);
	active_platform->renderer.set_render_draw_blend_mode(g_runtime.renderer, PLAT_BLENDMODE_BLEND);
	active_platform->renderer.render_present(g_runtime.renderer);

	if (active_platform->text.init_ttf() != 0) {
		bapi_runtime_release_window_and_renderer(active_platform);
		active_platform->core.quit();
		plat_shutdown();
		return 1;
	}

	g_runtime.initialized = true;
	return 0;
}

void bapi_runtime_stop(void)
{
	if (!g_runtime.initialized) return;

	const plat_interface_t *platform = plat_get();
	if (platform) {
		bapi_text_cleanup();
		platform->text.quit_ttf();
		bapi_runtime_release_window_and_renderer(platform);
		platform->core.quit();
	}
	memset(&g_runtime, 0, sizeof(g_runtime));
	plat_shutdown();
}

bool bapi_runtime_is_initialized(void)
{
	return g_runtime.initialized;
}

plat_window_t bapi_runtime_window(void)
{
	return g_runtime.window;
}

plat_renderer_t bapi_runtime_renderer(void)
{
	return g_runtime.renderer;
}

bool bapi_runtime_is_text_initialized(void)
{
	return g_runtime.text_initialized;
}

void bapi_runtime_set_text_initialized(bool initialized)
{
	g_runtime.text_initialized = initialized;
}
