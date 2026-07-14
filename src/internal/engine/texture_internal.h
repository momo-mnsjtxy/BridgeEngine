#pragma once

#include "BridgeEngine.h"
#include "internal/platform/platform_types.h"

struct bapi_texture_internal {
	plat_texture_t platform_texture;
	int			   width;
	int			   height;
	int			   reference_count;
	char		  *cache_key;
};
