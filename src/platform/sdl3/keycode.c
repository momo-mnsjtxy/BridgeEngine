#include "internal/platform/platform_types.h"
#include "internal/platform/sdl_keycode.h"

/*
 * SDL3 keycodes (SDL_keycode.h). Only the values are referenced here so this
 * translation unit (and its test) stays free of an SDL3 include dependency:
 *   SDLK_CAPSLOCK    = 0x40000039 (base)
 *   SDLK_NUMLOCKCLEAR= 0x40000053 (offset 26)
 *   SDLK_KP_DIVIDE   = 0x40000054 (27) ... SDLK_KP_0 = 0x40000062 (41)
 *   SDLK_KP_PERIOD   = 0x40000063 (42)
 *   SDLK_RETURN2     = 0x4000009e (101)
 *   SDLK_LCTRL       = 0x400000e0 (167) / RCTRL 171 / LSHIFT 168 / RSHIFT 172
 *   SDLK_LALT        = 0x400000e2 (169) / RALT 173 (highest mapped offset)
 * SDLK_RETURN (0x0d), SDLK_BACKSPACE (0x08) and SDLK_DELETE (0x7f) are below
 * the special range.
 */
#define SDL_SPECIAL_KEY_BASE 0x40000039u
#define SDL_SPECIAL_KEY_COUNT 0xAEu

uint8_t bapi_sdl_keycode_convert(uint32_t keycode)
{
	if (keycode < 0x80u) {
		if (keycode == 8u) return PLAT_KEY_BACKSPACE;
		if (keycode == 9u) return PLAT_KEY_TAB;
		if (keycode == 13u) return PLAT_KEY_ENTER;
		if (keycode == 27u) return PLAT_KEY_ESC;
		if ((keycode >= 32u && keycode <= 64u) || (keycode >= 91u && keycode <= 126u)) {
			return (uint8_t)keycode;
		}
		return 0;
	}
	if (keycode < SDL_SPECIAL_KEY_BASE || keycode - SDL_SPECIAL_KEY_BASE >= SDL_SPECIAL_KEY_COUNT) {
		return 0;
	}
	switch (keycode - SDL_SPECIAL_KEY_BASE) {
	case 0: return PLAT_KEY_CAPS;
	case 14: return PLAT_KEY_SCROLL;
	case 26: return PLAT_KEY_NUML;
	case 27: return '/';
	case 28: return '*';
	case 29: return '-';
	case 30: return '+';
	case 31: return PLAT_KEY_ENTER;
	case 42: return '.';
	case 101: return PLAT_KEY_ENTER;
	case 167: case 171: return PLAT_KEY_CTRL;
	case 168: case 172: return PLAT_KEY_SHIFT;
	case 169: case 173: return PLAT_KEY_ALT;
	default:
		if (keycode - SDL_SPECIAL_KEY_BASE >= 1u && keycode - SDL_SPECIAL_KEY_BASE <= 12u) {
			return (uint8_t)(PLAT_KEY_F1 + keycode - SDL_SPECIAL_KEY_BASE - 1u);
		}
		if (keycode - SDL_SPECIAL_KEY_BASE >= 32u && keycode - SDL_SPECIAL_KEY_BASE <= 40u) {
			return (uint8_t)('1' + keycode - SDL_SPECIAL_KEY_BASE - 32u);
		}
		if (keycode - SDL_SPECIAL_KEY_BASE == 41u) return '0';
		return 0;
	}
}
