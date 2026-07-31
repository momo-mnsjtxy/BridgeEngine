#pragma once

#include "internal/platform/platform_types.h"
#include <stdint.h>

#include "BridgeEngine.h"

#ifdef __cplusplus
extern "C" {
#endif

plat_renderer_t bapi_internal_get_renderer(void);
uint32_t		bapi_get_ticks(void);

#ifdef __cplusplus
}
#endif
