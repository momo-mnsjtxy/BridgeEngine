#pragma once

#include "platform/platform_types.h"
#include <stdint.h>

typedef enum {
	BAPI_EVENT_QUIT = 0,
	BAPI_EVENT_KEY_DOWN,
	BAPI_EVENT_KEY_UP,
	BAPI_EVENT_MOUSE_BUTTON_DOWN,
	BAPI_EVENT_MOUSE_BUTTON_UP,
	BAPI_EVENT_MOUSE_MOTION,
	BAPI_EVENT_UNKNOWN,
} bapi_event_type_t;

#define BAPI_BUTTON_LEFT 1

struct bapi_event_internal {
	bapi_event_type_t type;
	union {
		struct {
			uint32_t key;
		} key;
		struct {
			float x;
			float y;
			int	  button;
		} button;
		struct {
			float x;
			float y;
		} motion;
	} data;
};

typedef struct bapi_event_internal bapi_event_t;

#ifdef __cplusplus
extern "C" {
#endif

plat_renderer_t bapi_internal_get_renderer(void);
uint32_t		bapi_get_ticks(void);

#ifdef __cplusplus
}
#endif
