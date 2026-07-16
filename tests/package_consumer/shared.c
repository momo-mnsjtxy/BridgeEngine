#include <BridgeEngine.h>

int main(void)
{
	bapi_color_t color = bapi_color(1, 2, 3, 4);
	return color.a == 4 ? 0 : 1;
}
