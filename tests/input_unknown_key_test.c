#include "BridgeEngine.h"
#include "internal/platform/platform.h"
#include <stdio.h>

int bapi_event_get_type(const bapi_event_t *event)
{
	return event->type;
}

uint8_t bapi_event_get_key_code(const bapi_event_t *event)
{
	return (uint8_t)event->data.key.key;
}

int bapi_event_get_mouse_button(const bapi_event_t *event)
{
	return event->data.button.button;
}

int bapi_event_get_motion_x(const bapi_event_t *event)
{
	return (int)event->data.motion.x;
}

int bapi_event_get_motion_y(const bapi_event_t *event)
{
	return (int)event->data.motion.y;
}

const plat_interface_t *plat_get(void)
{
	return NULL;
}

int main(void)
{
	bapi_event_t known = {.type = BAPI_EVENT_KEY_DOWN, .data.key.key = 'a'};
	bapi_event_t unknown = {.type = BAPI_EVENT_KEY_DOWN, .data.key.key = 0};
	bapi_input_init();
	bapi_input_handle_event(&known);
	bapi_input_handle_event(&unknown);
	if (!bapi_is_key_down('a') || bapi_is_key_down(0)) {
		fprintf(stderr, "unknown key changed input state\n");
		return 1;
	}
	return 0;
}
