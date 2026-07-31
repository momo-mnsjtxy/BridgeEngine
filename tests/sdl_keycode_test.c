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
		expect(bapi_sdl_keycode_convert(0x40000039u + 13u) == ' ', "special table gap maps unchanged") &&
		expect(bapi_sdl_keycode_convert(0x40000039u - 1u) == 0, "non-ASCII below special base is unknown") &&
		expect(bapi_sdl_keycode_convert(0x40000039u + 0x90u) == 0, "special range end is unknown") &&
		expect(bapi_sdl_keycode_convert(0x1f642u) == 0, "Unicode is unknown") &&
		expect(bapi_sdl_keycode_convert(UINT32_MAX) == 0, "large keycode is unknown") ? 0 : 1;
}
