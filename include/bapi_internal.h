#pragma once

#include "platform/platform_types.h"

#define BAPI_EVENT_QUIT PLAT_EVENT_QUIT
#define BAPI_EVENT_KEY_DOWN PLAT_EVENT_KEY_DOWN
#define BAPI_EVENT_MOUSE_BUTTON_DOWN PLAT_EVENT_MOUSE_BUTTON_DOWN
#define BAPI_EVENT_MOUSE_BUTTON_UP PLAT_EVENT_MOUSE_BUTTON_UP
#define BAPI_EVENT_MOUSE_MOTION PLAT_EVENT_MOUSE_MOTION

#define BAPI_BUTTON_LEFT PLAT_BUTTON_LEFT

struct bapi_event_internal {
	plat_event_t event;
};

typedef struct bapi_event_internal bapi_event_t;

extern plat_renderer_t bapi_internal_renderer;

#ifdef __cplusplus
extern "C" {
#endif

uint32_t bapi_get_ticks(void);

#ifdef __cplusplus
}
#endif
