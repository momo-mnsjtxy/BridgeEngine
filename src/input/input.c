#include "BridgeEngine.h"
#include "internal/platform/platform.h"
#include <string.h>

#define MAX_KEYS 256

static struct {
	uint8_t keys[MAX_KEYS];
	uint8_t keys_prev[MAX_KEYS];
	int		mouse_buttons;
	int		mouse_buttons_prev;
	float	mouse_x;
	float	mouse_y;
	int		initialized;
} g_input = {0};

void bapi_input_init(void)
{
	memset(&g_input, 0, sizeof(g_input));
	g_input.initialized = 1;
}

void bapi_input_cleanup(void)
{
	memset(&g_input, 0, sizeof(g_input));
}

void bapi_input_update(void)
{
	memcpy(g_input.keys_prev, g_input.keys, MAX_KEYS);
	g_input.mouse_buttons_prev = g_input.mouse_buttons;

	const plat_interface_t *plat = plat_get();
	if (plat) {
		plat->window.get_mouse_state(&g_input.mouse_x, &g_input.mouse_y);
	}
}

void bapi_input_handle_event(const bapi_event_t *event)
{
	if (!g_input.initialized) return;

	int type = bapi_event_get_type(event);

	switch (type) {
	case BAPI_EVENT_KEY_DOWN: {
		uint8_t key		  = bapi_event_get_key_code(event);
		if (key != 0) g_input.keys[key] = 1;
		break;
	}
	case BAPI_EVENT_KEY_UP: {
		uint8_t key		  = bapi_event_get_key_code(event);
		if (key != 0) g_input.keys[key] = 0;
		break;
	}
	case BAPI_EVENT_MOUSE_BUTTON_DOWN:
		g_input.mouse_buttons |= 1 << bapi_event_get_mouse_button(event);
		break;
	case BAPI_EVENT_MOUSE_BUTTON_UP:
		g_input.mouse_buttons &= ~(1 << bapi_event_get_mouse_button(event));
		break;
	case BAPI_EVENT_MOUSE_MOTION:
		g_input.mouse_x = (float)bapi_event_get_motion_x(event);
		g_input.mouse_y = (float)bapi_event_get_motion_y(event);
		break;
	}
}

int bapi_is_key_down(uint8_t key)
{
	return g_input.keys[key];
}

int bapi_is_key_pressed(uint8_t key)
{
	return g_input.keys[key] && !g_input.keys_prev[key];
}

int bapi_is_key_released(uint8_t key)
{
	return !g_input.keys[key] && g_input.keys_prev[key];
}

int bapi_is_mouse_button_down(int button)
{
	if (button < 0 || button >= 31) return 0;
	return (g_input.mouse_buttons >> button) & 1;
}

int bapi_is_mouse_button_pressed(int button)
{
	if (button < 0 || button >= 31) return 0;
	return ((g_input.mouse_buttons >> button) & 1) &&
		   !((g_input.mouse_buttons_prev >> button) & 1);
}

int bapi_is_mouse_button_released(int button)
{
	if (button < 0 || button >= 31) return 0;
	return !((g_input.mouse_buttons >> button) & 1) &&
		   ((g_input.mouse_buttons_prev >> button) & 1);
}

float bapi_get_mouse_x(void)
{
	return g_input.mouse_x;
}

float bapi_get_mouse_y(void)
{
	return g_input.mouse_y;
}

void bapi_get_mouse_position(float *x, float *y)
{
	if (x) *x = g_input.mouse_x;
	if (y) *y = g_input.mouse_y;
}
