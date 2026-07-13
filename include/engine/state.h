#pragma once

#include "platform/platform.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	plat_window_t	window;
	plat_renderer_t renderer;
	bool			initialized;
	int				text_initialized;
} bapi_engine_state_t;

bapi_engine_state_t *bapi_engine_state(void);
void				 bapi_engine_state_reset(void);

#ifdef __cplusplus
}
#endif
