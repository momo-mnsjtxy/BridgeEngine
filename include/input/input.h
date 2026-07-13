#pragma once

#include "bapi_types.h"
#include "bapi_internal.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void bapi_input_init(void);
void bapi_input_cleanup(void);
void bapi_input_update(void);
void bapi_input_handle_event(const bapi_event_t* event);

int bapi_is_key_down(uint8_t key);
int bapi_is_key_pressed(uint8_t key);
int bapi_is_key_released(uint8_t key);

int bapi_is_mouse_button_down(int button);
int bapi_is_mouse_button_pressed(int button);
int bapi_is_mouse_button_released(int button);

float bapi_get_mouse_x(void);
float bapi_get_mouse_y(void);
void bapi_get_mouse_position(float* x, float* y);

#ifdef __cplusplus
}
#endif
