#pragma once

#include "BridgeEngine.h"

struct bapi_ui_component_internal {
	bapi_ui_component_type_t			type;
	char							   *id;
	char							   *text;
	char							   *src;
	char							   *src_raw;
	bapi_rect_t							rect;
	bapi_rect_t							local_rect;
	bapi_color_t						color;
	bapi_color_t						color2;
	bapi_color_t						color3;
	bapi_color_t						text_color;
	float								text_size;
	float								radius;
	float								value;
	float								min_value;
	float								max_value;
	float								step;
	float								scroll_offset;
	int									sides;
	int									columns;
	int									rows;
	int									visible;
	int									enabled;
	int									relative_position;
	int									hovered;
	int									pressed;
	int									clicked;
	int									checked;
	int									focused;
	int									selected_index;
	int									item_count;
	int									max_text_length;
	bapi_texture_t						texture;
	bapi_video_t						video;
	struct bapi_ui_component_internal  *parent;
	struct bapi_ui_component_internal **children;
	int									child_count;
	int									child_capacity;
};

struct bapi_ui_internal {
	bapi_ui_component_t *roots;
	int					 root_count;
	int					 root_capacity;
	bapi_ui_component_t	 focused;
	bapi_ui_component_t	 hovered;
	bapi_ui_component_t	 pressed;
};

bapi_ui_t			bapi_ui_create(void);
bapi_ui_component_t bapi_ui_component_create(bapi_ui_component_type_t type, const char *id);
void				bapi_ui_component_destroy(bapi_ui_component_t component);
int bapi_ui_component_add_child(bapi_ui_component_t parent, bapi_ui_component_t child);
int bapi_ui_add_root(bapi_ui_t ui, bapi_ui_component_t component);
