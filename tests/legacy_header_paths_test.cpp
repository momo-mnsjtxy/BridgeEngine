#include <audio/audio.h>
#include <bapi.h>
#include <bapi_types.h>
#include <button/button.h>
#include <level/level.h>
#include <log/log.h>
#include <master/init.h>
#include <master/master.h>
#include <mouse_drawing.h>
#include <publics.h>
#include <render/create.h>
#include <render/draw.h>
#include <render/render.h>
#include <scene/scene.h>
#include <text/text.h>
#include <video/video.h>
#include <xml/xml_loader.h>

int main()
{
	bapi_event_t event = {};
	return event.type == BAPI_EVENT_QUIT && NULL == nullptr ? 0 : 1;
}
