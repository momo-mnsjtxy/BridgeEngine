#include "BridgeEngine.h"
#include <stdbool.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
	(void)argc;
	(void)argv;

	if (bapi_engine_init("Text Example", 800, 600) != 0) {
		printf("Failed to initialize engine\n");
		return 1;
	}

	bapi_text_init();

	bool		 running = true;
	bapi_event_t event;

	while (running) {
		while (bapi_poll_event(&event)) {
			int type = bapi_event_get_type(&event);
			if (type == BAPI_EVENT_QUIT) {
				running = false;
			}
		}

		bapi_render_clear();

		bapi_draw_text("Red Text", 50, 50, 32, bapi_color(255, 0, 0, 255));
		bapi_draw_text("Green Text", 50, 100, 32, bapi_color(0, 255, 0, 255));
		bapi_draw_text("Blue Text", 50, 150, 32, bapi_color(0, 0, 255, 255));
		bapi_draw_text("White Text", 50, 200, 32, bapi_color(255, 255, 255, 255));
		bapi_draw_text("中文显示测试", 50, 250, 32, bapi_color(255, 255, 0, 255));

		bapi_render_present();
		bapi_delay(16);
	}
	bapi_text_cleanup();
	bapi_engine_quit();

	return 0;
}
