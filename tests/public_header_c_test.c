#include <BridgeEngine.h>

int main(void)
{
	bapi_event_t event = {0};
	return event.type != BAPI_EVENT_QUIT;
}
