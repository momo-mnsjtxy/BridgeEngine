#include <BridgeEngine.h>

int main()
{
	bapi_color_t color = bapi_color(1, 2, 3, 4);
	return color.r == 1 ? 0 : 1;
}
