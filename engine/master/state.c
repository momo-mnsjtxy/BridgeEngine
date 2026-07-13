#include "engine/state.h"
#include <string.h>

static bapi_engine_state_t g_engine_state;

bapi_engine_state_t *bapi_engine_state(void)
{
	return &g_engine_state;
}

void bapi_engine_state_reset(void)
{
	memset(&g_engine_state, 0, sizeof(g_engine_state));
}
