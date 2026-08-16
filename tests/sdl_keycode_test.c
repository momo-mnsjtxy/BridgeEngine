#include "internal/platform/platform_types.h"
#include "internal/platform/sdl_keycode.h"
#include <stdio.h>

static int expect(int condition, const char *message)
{
	if (!condition) fprintf(stderr, "%s\n", message);
	return condition;
}

int main(void)
{
	return expect(bapi_sdl_keycode_convert('a') == 'a', "ASCII maps unchanged") &&
		expect(bapi_sdl_keycode_convert(0x4000003au) == PLAT_KEY_F1, "F1 maps unchanged") &&
		expect(bapi_sdl_keycode_convert(0x40000039u - 1u) == 0, "non-ASCII below special base is unknown") &&
		expect(bapi_sdl_keycode_convert(0x40000039u + 0xAEu) == 0, "special range end is unknown") &&
		expect(bapi_sdl_keycode_convert(0x1f642u) == 0, "Unicode is unknown") &&
		expect(bapi_sdl_keycode_convert(UINT32_MAX) == 0, "large keycode is unknown") &&
		expect(bapi_sdl_keycode_convert(8u) == PLAT_KEY_BACKSPACE, "backspace maps to KEY_BACKSPACE") &&
		expect(bapi_sdl_keycode_convert(13u) == PLAT_KEY_ENTER, "return maps to KEY_ENTER") &&
		expect(bapi_sdl_keycode_convert(9u) == PLAT_KEY_TAB, "tab maps to KEY_TAB") &&
		expect(bapi_sdl_keycode_convert(27u) == PLAT_KEY_ESC, "escape maps to KEY_ESC") &&
		expect(bapi_sdl_keycode_convert(0x7fu) == 0, "delete is unknown") &&
		expect(bapi_sdl_keycode_convert(0x400000e0u) == PLAT_KEY_CTRL, "LCTRL maps to KEY_CTRL") &&
		expect(bapi_sdl_keycode_convert(0x400000e4u) == PLAT_KEY_CTRL, "RCTRL maps to KEY_CTRL") &&
		expect(bapi_sdl_keycode_convert(0x400000e1u) == PLAT_KEY_SHIFT, "LSHIFT maps to KEY_SHIFT") &&
		expect(bapi_sdl_keycode_convert(0x400000e6u) == PLAT_KEY_ALT, "RALT maps to KEY_ALT") &&
		expect(bapi_sdl_keycode_convert(0x40000053u) == PLAT_KEY_NUML, "numlock maps to KEY_NUML") &&
		expect(bapi_sdl_keycode_convert(0x40000054u) == '/', "KP_DIVIDE maps to slash") &&
		expect(bapi_sdl_keycode_convert(0x40000058u) == PLAT_KEY_ENTER, "KP_ENTER maps to KEY_ENTER") &&
		expect(bapi_sdl_keycode_convert(0x40000059u) == '1', "KP_1 maps to digit") &&
		expect(bapi_sdl_keycode_convert(0x40000062u) == '0', "KP_0 maps to digit") &&
		expect(bapi_sdl_keycode_convert(0x40000063u) == '.', "KP_PERIOD maps to dot") &&
		expect(bapi_sdl_keycode_convert(0x40000050u) == 0, "left arrow is unknown, not space") &&
		expect(bapi_sdl_keycode_convert(0x4000004cu) == 0, "special table gap is unknown") ? 0 : 1;
}
