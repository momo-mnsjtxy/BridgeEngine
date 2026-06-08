#pragma once

#include <stdint.h>
#include "platform/platform_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct bapi_window_internal* bapi_window_t;
typedef struct bapi_renderer_internal* bapi_renderer_t;
typedef struct bapi_texture_internal* bapi_texture_t;

struct bapi_texture_internal {
	plat_texture_t plat_texture;
};

typedef struct {
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
} bapi_color_t;

typedef struct {
	float x;
	float y;
	float w;
	float h;
} bapi_rect_t;

typedef struct {
	bapi_rect_t rect;
	bapi_color_t normal_color;
	bapi_color_t hover_color;
	bapi_color_t click_color;
	const char* text;
	bapi_color_t text_color;
	float text_size;
	float text_width;
	float text_height;
	int is_clicked;
	int is_hovered;
} bapi_button_t;


enum special_key_code {
	KEY_ESC = PLAT_KEY_ESC,
	KEY_BACKSPACE = PLAT_KEY_BACKSPACE,
	KEY_TAB = PLAT_KEY_TAB,
	KEY_ENTER = PLAT_KEY_ENTER,
	KEY_CAPS = PLAT_KEY_CAPS,
	KEY_SHIFT = PLAT_KEY_SHIFT,
	KEY_CTRL = PLAT_KEY_CTRL,
	KEY_ALT = PLAT_KEY_ALT,
	KEY_SPACE = PLAT_KEY_SPACE,
	KEY_F1 = PLAT_KEY_F1,
	KEY_F2 = PLAT_KEY_F2,
	KEY_F3 = PLAT_KEY_F3,
	KEY_F4 = PLAT_KEY_F4,
	KEY_F5 = PLAT_KEY_F5,
	KEY_F6 = PLAT_KEY_F6,
	KEY_F7 = PLAT_KEY_F7,
	KEY_F8 = PLAT_KEY_F8,
	KEY_F9 = PLAT_KEY_F9,
	KEY_F10 = PLAT_KEY_F10,
	KEY_F11 = PLAT_KEY_F11,
	KEY_F12 = PLAT_KEY_F12,
	KEY_NUML = PLAT_KEY_NUML,
	KEY_SCROLL = PLAT_KEY_SCROLL
};

#ifdef __cplusplus
}
#endif
