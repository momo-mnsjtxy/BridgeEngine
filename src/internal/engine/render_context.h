#pragma once

#include "internal/platform/platform.h"
#include <stdbool.h>

int	 bapi_runtime_start(const plat_interface_t *platform, const char *title, int width, int height);
void bapi_runtime_stop(void);

bool			bapi_runtime_is_initialized(void);
plat_window_t	bapi_runtime_window(void);
plat_renderer_t bapi_runtime_renderer(void);

bool bapi_runtime_is_text_initialized(void);
void bapi_runtime_set_text_initialized(bool initialized);
