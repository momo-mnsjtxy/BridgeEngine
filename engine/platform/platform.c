#include "platform/platform.h"
#include <stddef.h>

static plat_interface_t g_plat;
static int g_plat_initialized = 0;

int plat_init(const plat_interface_t* interface)
{
	if (interface == NULL) {
		return 1;
	}
	g_plat = *interface;
	g_plat_initialized = 1;
	return 0;
}

const plat_interface_t* plat_get(void)
{
	if (!g_plat_initialized) {
		return NULL;
	}
	return &g_plat;
}

void plat_shutdown(void)
{
	g_plat_initialized = 0;
}
