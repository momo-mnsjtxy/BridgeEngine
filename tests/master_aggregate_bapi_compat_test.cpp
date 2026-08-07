#define BAPI_LOG_ENABLED
#include "master_aggregate_bapi_compat_members.h"
#include "master_aggregate_bapi_compat_signatures.h"
#include <BridgeEngine.h>

#include <cstdint>
#include <type_traits>

#define CHECK_SIGNATURE(symbol, type)                                                              \
	static_assert(std::is_same<decltype(&symbol), type>::value, #symbol);
#define CHECK_MEMBER(type, member, expected)                                                       \
	static_assert(std::is_same<decltype(((type *)0)->member), expected>::value, #type "." #member);
#define CHECK_TYPE(type, expected) static_assert(std::is_same<type, expected>::value, #type);
MASTER_AGGREGATE_BAPI_SIGNATURES(CHECK_SIGNATURE)
MASTER_AGGREGATE_BAPI_MEMBERS(CHECK_MEMBER)

CHECK_TYPE(bapi_window_t, struct bapi_window_internal *);
CHECK_TYPE(bapi_renderer_t, struct bapi_renderer_internal *);
CHECK_TYPE(bapi_texture_t, struct bapi_texture_internal *);
CHECK_TYPE(bapi_sound_t, struct bapi_sound_internal *);
CHECK_TYPE(bapi_video_t, struct bapi_video_internal *);
CHECK_TYPE(bapi_scene_t, struct bapi_scene_internal *);
CHECK_TYPE(bapi_scene_manager_t, struct bapi_scene_manager_internal *);
CHECK_TYPE(bapi_level_t, struct bapi_level_internal *);
CHECK_TYPE(bapi_level_manager_t, struct bapi_level_manager_internal *);
CHECK_TYPE(bapi_ui_t, struct bapi_ui_internal *);
CHECK_TYPE(bapi_ui_component_t, struct bapi_ui_component_internal *);
CHECK_TYPE(bapi_scene_on_enter_fn, void (*)(bapi_scene_t));
CHECK_TYPE(bapi_scene_on_exit_fn, void (*)(bapi_scene_t));
CHECK_TYPE(bapi_scene_on_update_fn, void (*)(bapi_scene_t, float));
CHECK_TYPE(bapi_scene_on_render_fn, void (*)(bapi_scene_t));
CHECK_TYPE(bapi_level_on_load_fn, void (*)(bapi_level_t));
CHECK_TYPE(bapi_level_on_unload_fn, void (*)(bapi_level_t));
CHECK_TYPE(bapi_level_on_update_fn, void (*)(bapi_level_t, float));
CHECK_TYPE(bapi_level_on_render_fn, void (*)(bapi_level_t));

static_assert(std::is_same<decltype(&bapi_event_get_type), int (*)(const bapi_event_t *)>::value,
			  "bapi_event_get_type");
static_assert(
	std::is_same<decltype(&bapi_event_get_key_code), uint8_t (*)(const bapi_event_t *)>::value,
	"bapi_event_get_key_code");
static_assert(std::is_same<decltype(&bapi_color_from_hex), bapi_color_t (*)(uint32_t)>::value,
			  "bapi_color_from_hex");
static_assert(std::is_same<decltype(&bapi_scene_create),
						   bapi_scene_t (*)(const char *, bapi_scene_callbacks_t)>::value,
			  "bapi_scene_create");
static_assert(std::is_same<decltype(&bapi_level_create),
						   bapi_level_t (*)(const char *, int, bapi_level_callbacks_t)>::value,
			  "bapi_level_create");
static_assert(std::is_same<decltype(&bapi_ui_load_from_xml), bapi_ui_t (*)(const char *)>::value,
			  "bapi_ui_load_from_xml");
static_assert(
	std::is_same<decltype(&bapi_ui_update), void (*)(bapi_ui_t, const bapi_event_t *)>::value,
	"bapi_ui_update");
static_assert(std::is_same<decltype(&bapi_ui_was_clicked), int (*)(bapi_ui_t, const char *)>::value,
			  "bapi_ui_was_clicked");
static_assert(
	std::is_same<decltype(&bapi_ui_find), bapi_ui_component_t (*)(bapi_ui_t, const char *)>::value,
	"bapi_ui_find");
static_assert(std::is_same<decltype(&bapi_ui_component_set_text),
						   int (*)(bapi_ui_component_t, const char *)>::value,
			  "bapi_ui_component_set_text");
static_assert(std::is_same<decltype(&bapi_ui_component_set_value),
						   int (*)(bapi_ui_component_t, float)>::value,
			  "bapi_ui_component_set_value");
static_assert(std::is_same<decltype(&bapi_ui_layout), void (*)(bapi_ui_t)>::value,
			  "bapi_ui_layout");
static_assert(std::is_same<decltype(&bapi_ui_component_set_scroll_offset),
						   int (*)(bapi_ui_component_t, float)>::value,
			  "bapi_ui_component_set_scroll_offset");
static_assert(std::is_same<decltype(&bridgeengine_get_version_number), int (*)(void)>::value,
			  "bridgeengine_get_version_number");
static_assert(BAPI_EVENT_QUIT == 0 && BAPI_EVENT_KEY_DOWN == 1 &&
				  BAPI_EVENT_MOUSE_BUTTON_DOWN == 2 && BAPI_EVENT_MOUSE_BUTTON_UP == 3 &&
				  BAPI_EVENT_MOUSE_MOTION == 4,
			  "BAPI event values");
static_assert(KEY_ESC == 128 && KEY_BACKSPACE == 129 && KEY_TAB == 130 && KEY_ENTER == 131 &&
				  KEY_CAPS == 132 && KEY_SHIFT == 133 && KEY_CTRL == 134 && KEY_ALT == 135 &&
				  KEY_SPACE == 136 && KEY_F1 == 137 && KEY_F12 == 148 && KEY_NUML == 149 &&
				  KEY_SCROLL == 150,
			  "special key values");
static_assert(BAPI_BUTTON_LEFT == 1, "BAPI_BUTTON_LEFT");
static_assert(BAPI_LOG_LEVEL_DEBUG == 0 && BAPI_LOG_LEVEL_INFO == 1 && BAPI_LOG_LEVEL_WARN == 2 &&
				  BAPI_LOG_LEVEL_ERROR == 3 && BAPI_LOG_LEVEL_CRITICAL == 4 &&
				  BAPI_LOG_LEVEL_NONE == 5,
			  "BAPI log values");

int main()
{
	bapi_color_t color = bapi_color(1, 2, 3, 4);
	BAPI_LOG_DEBUG("master aggregate compatibility");
	BAPI_LOG_INFO("master aggregate compatibility");
	BAPI_LOG_WARN("master aggregate compatibility");
	BAPI_LOG_ERROR("master aggregate compatibility");
	BAPI_LOG_CRITICAL("master aggregate compatibility");
	BAPI_LOG_ASSERT(true, "master aggregate compatibility");
	if (false) BAPI_LOG_INIT_DEFAULT();
	return BRIDGEENGINE_MAJOR < 0 || BRIDGEENGINE_MINOR < 0 || BRIDGEENGINE_PATCH < 0 ||
		   BRIDGEENGINE_VERSION[0] == '\0' || bridgeengine_get_version()[0] == '\0' ||
		   bridgeengine_get_version_number() < 0 || color.a != 4;
}
