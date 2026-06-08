






#include "video/video.h"
#include <stdlib.h>

int bapi_video_init(void)
{
	return 0;
}

void bapi_video_cleanup(void)
{
}

bapi_video_t bapi_video_load(const char *filepath)
{
	(void)filepath;
	return NULL;
}

void bapi_video_free(bapi_video_t video)
{
	(void)video;
}

int bapi_video_play(bapi_video_t video)
{
	(void)video;
	return 1;
}

void bapi_video_pause(bapi_video_t video)
{
	(void)video;
}

void bapi_video_stop(bapi_video_t video)
{
	(void)video;
}

void bapi_video_render(bapi_video_t video, int x, int y, int w, int h)
{
	(void)video;
	(void)x;
	(void)y;
	(void)w;
	(void)h;
}

void bapi_video_render_fit(bapi_video_t video, int area_x, int area_y, int area_w, int area_h)
{
	(void)video;
	(void)area_x;
	(void)area_y;
	(void)area_w;
	(void)area_h;
}

void bapi_video_render_center(bapi_video_t video, int window_w, int window_h)
{
	(void)video;
	(void)window_w;
	(void)window_h;
}

void bapi_video_set_loop(bapi_video_t video, int loop)
{
	(void)video;
	(void)loop;
}

void bapi_video_set_volume(bapi_video_t video, float volume)
{
	(void)video;
	(void)volume;
}

int bapi_video_is_playing(bapi_video_t video)
{
	(void)video;
	return 0;
}

void bapi_video_get_size(bapi_video_t video, int *w, int *h)
{
	(void)video;
	if (w)
		*w = 0;
	if (h)
		*h = 0;
}

void bapi_video_update(void)
{
}
