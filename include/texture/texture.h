#pragma once

#include "bapi_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BAPI_MAX_CACHED_TEXTURES 64

bapi_texture_t bapi_texture_load(const char* filepath);
void bapi_texture_destroy(bapi_texture_t texture);

void bapi_texture_render(bapi_texture_t texture, float x, float y);
void bapi_texture_render_ex(bapi_texture_t texture, float x, float y, float w, float h);
void bapi_texture_get_size(bapi_texture_t texture, int* w, int* h);

void bapi_texture_cache_clear(void);

bapi_texture_t bapi_texture_from_file(const char* filepath, int* out_w, int* out_h);

#ifdef __cplusplus
}
#endif
