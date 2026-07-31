#include "internal/platform/platform.h"
#include <stddef.h>

static plat_interface_t g_platform;
static int				g_platform_initialized = 0;

int plat_init(const plat_interface_t *interface)
{
	if (interface == NULL) return 1;
	g_platform			   = *interface;
	g_platform_initialized = 1;
	return 0;
}

const plat_interface_t *plat_get(void)
{
	return g_platform_initialized ? &g_platform : NULL;
}

void plat_shutdown(void)
{
	g_platform_initialized = 0;
}
