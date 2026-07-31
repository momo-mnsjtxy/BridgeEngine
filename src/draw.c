#include "BridgeEngine.h"
#include <stdlib.h>

void bapi_engine_render_drawpixel(int x, int y, int color)
{
	bapi_color_t c = bapi_color_from_hex((uint32_t)color);
	bapi_draw_pixel((float)x, (float)y, c);
}

void bapi_engine_render_fillrect(int ax, int ay, int width, int height, int color)
{
	bapi_color_t c = bapi_color_from_hex((uint32_t)color);
	bapi_fill_rect((float)ax, (float)ay, (float)width, (float)height, c);
}

void bapi_engine_render_draw_triangle(int x1, int y1, int x2, int y2, int x3, int y3, int color)
{
	bapi_color_t c = bapi_color_from_hex((uint32_t)color);
	bapi_draw_triangle((float)x1, (float)y1, (float)x2, (float)y2, (float)x3, (float)y3, c);
}
