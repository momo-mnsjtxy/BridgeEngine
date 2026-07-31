#include "internal/platform/platform_types.h"
#include "internal/platform/sdl_keycode.h"

#define SDL_SPECIAL_KEY_BASE 0x40000039u
#define SDL_SPECIAL_KEY_COUNT 0x90u

uint8_t bapi_sdl_keycode_convert(uint32_t keycode)
{
	if (keycode < 0x80u) {
		if (keycode == 8u) return '\b';
		if (keycode == 9u) return PLAT_KEY_TAB;
		if (keycode == 27u) return PLAT_KEY_ESC;
		if ((keycode >= 32u && keycode <= 64u) || (keycode >= 91u && keycode <= 126u)) {
			return (uint8_t)keycode;
		}
		return ' ';
	}
	if (keycode < SDL_SPECIAL_KEY_BASE || keycode - SDL_SPECIAL_KEY_BASE >= SDL_SPECIAL_KEY_COUNT) {
		return 0;
	}
	switch (keycode - SDL_SPECIAL_KEY_BASE) {
	case 0: return PLAT_KEY_CAPS;
	case 14: return PLAT_KEY_SCROLL;
	case 27: return PLAT_KEY_NUML;
	case 28: return '/';
	case 29: return '*';
	case 30: return '-';
	case 31: return '+';
	case 32: return '\n';
	case 134: case 138: return PLAT_KEY_CTRL;
	case 135: case 139: return PLAT_KEY_SHIFT;
	case 136: case 140: return PLAT_KEY_ALT;
	default:
		if (keycode - SDL_SPECIAL_KEY_BASE >= 1u && keycode - SDL_SPECIAL_KEY_BASE <= 12u) {
			return (uint8_t)(PLAT_KEY_F1 + keycode - SDL_SPECIAL_KEY_BASE - 1u);
		}
		return ' ';
	}
}
