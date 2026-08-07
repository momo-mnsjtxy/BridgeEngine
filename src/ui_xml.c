#include "BridgeEngine.h"
#include "internal/scene/ui_internal.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	char *name;
	char *value;
} ui_attr_t;

typedef struct {
	char	 *name;
	ui_attr_t attrs[32];
	int		  attr_count;
	int		  self_closing;
	int		  closing;
} ui_tag_t;

typedef struct {
	const char *cursor;
	const char *base_dir;
	bapi_ui_t	ui;
	int			failed;
} ui_parser_t;

static char *ui_strdup(const char *value)
{
	if (!value) return NULL;
	size_t length = strlen(value) + 1;
	char  *copy	  = malloc(length);
	if (copy) memcpy(copy, value, length);
	return copy;
}

static char *copy_range(const char *start, const char *end)
{
	if (!start || !end || end < start) return NULL;
	size_t length = (size_t)(end - start);
	char  *copy	  = malloc(length + 1);
	if (copy) {
		memcpy(copy, start, length);
		copy[length] = '\0';
	}
	return copy;
}

static void destroy_tag(ui_tag_t *tag)
{
	if (!tag) return;
	free(tag->name);
	for (int i = 0; i < tag->attr_count; i++) {
		free(tag->attrs[i].name);
		free(tag->attrs[i].value);
	}
	memset(tag, 0, sizeof(*tag));
}

static const char *skip_space(const char *cursor)
{
	while (*cursor && isspace((unsigned char)*cursor)) cursor++;
	return cursor;
}

static int is_name_char(int c)
{
	return isalnum((unsigned char)c) || c == '_' || c == '-' || c == ':';
}

static char *decode_entities(const char *start, const char *end)
{
	char *value = malloc((size_t)(end - start) + 1);
	char *out;
	if (!value) return NULL;
	out = value;
	while (start < end) {
		if (*start == '&' && (size_t)(end - start) >= 5 && memcmp(start, "&amp;", 5) == 0) {
			*out++ = '&';
			start += 5;
		} else if (*start == '&' && (size_t)(end - start) >= 4 && memcmp(start, "&lt;", 4) == 0) {
			*out++ = '<';
			start += 4;
		} else if (*start == '&' && (size_t)(end - start) >= 4 && memcmp(start, "&gt;", 4) == 0) {
			*out++ = '>';
			start += 4;
		} else if (*start == '&' && (size_t)(end - start) >= 6 && memcmp(start, "&quot;", 6) == 0) {
			*out++ = '"';
			start += 6;
		} else if (*start == '&' && (size_t)(end - start) >= 6 && memcmp(start, "&apos;", 6) == 0) {
			*out++ = '\'';
			start += 6;
		} else {
			*out++ = *start++;
		}
	}
	*out = '\0';
	return value;
}

static const char *find_tag_end(const char *cursor)
{
	int quoted = 0;
	while (*cursor) {
		if (*cursor == '"')
			quoted = !quoted;
		else if (*cursor == '>' && !quoted)
			return cursor;
		cursor++;
	}
	return NULL;
}

static int parse_tag(const char *start, const char *end, ui_tag_t *tag)
{
	const char *cursor = start + 1;
	memset(tag, 0, sizeof(*tag));
	if (!start || !end || start >= end || *start != '<') return -1;
	if (*cursor == '/') {
		tag->closing = 1;
		cursor++;
	}
	cursor				   = skip_space(cursor);
	const char *name_start = cursor;
	while (cursor < end && is_name_char(*cursor)) cursor++;
	if (cursor == name_start) return -1;
	tag->name = copy_range(name_start, cursor);
	if (!tag->name) return -1;
	while (cursor < end) {
		cursor = skip_space(cursor);
		if (cursor >= end) break;
		if (*cursor == '/') {
			tag->self_closing = 1;
			cursor++;
			continue;
		}
		if (tag->attr_count >= 32) return -1;
		const char *attr_start = cursor;
		while (cursor < end && is_name_char(*cursor)) cursor++;
		if (cursor == attr_start) return -1;
		char *attr_name = copy_range(attr_start, cursor);
		if (!attr_name) return -1;
		cursor = skip_space(cursor);
		if (cursor >= end || *cursor++ != '=') {
			free(attr_name);
			return -1;
		}
		cursor = skip_space(cursor);
		if (cursor >= end || *cursor++ != '"') {
			free(attr_name);
			return -1;
		}
		const char *value_start = cursor;
		while (cursor < end && *cursor != '"') cursor++;
		if (cursor >= end) {
			free(attr_name);
			return -1;
		}
		char *attr_value = decode_entities(value_start, cursor);
		if (!attr_value) {
			free(attr_name);
			return -1;
		}
		tag->attrs[tag->attr_count].name  = attr_name;
		tag->attrs[tag->attr_count].value = attr_value;
		tag->attr_count++;
		cursor++;
	}
	return 0;
}

static const char *get_attr(const ui_tag_t *tag, const char *name)
{
	for (int i = 0; i < tag->attr_count; i++)
		if (strcmp(tag->attrs[i].name, name) == 0) return tag->attrs[i].value;
	return NULL;
}

static int parse_float(const ui_tag_t *tag, const char *name, float *out)
{
	const char *value  = get_attr(tag, name);
	int			sign   = 1;
	float		result = 0;
	int			digits = 0;
	if (!value || !value[0]) return -1;
	if (*value == '-' || *value == '+') {
		if (*value++ == '-') sign = -1;
	}
	while (isdigit((unsigned char)*value)) {
		result = result * 10.0f + (float)(*value++ - '0');
		digits++;
	}
	if (*value == '.') {
		float place = 0.1f;
		value++;
		while (isdigit((unsigned char)*value)) {
			result += (float)(*value++ - '0') * place;
			place *= 0.1f;
			digits++;
		}
	}
	if (!digits || *value) return -1;
	*out = (float)sign * result;
	return 0;
}

static int parse_int(const ui_tag_t *tag, const char *name, int *out)
{
	float value;
	if (parse_float(tag, name, &value) != 0) return -1;
	*out = (int)value;
	return value == (float)*out ? 0 : -1;
}

static int parse_bool(const ui_tag_t *tag, const char *name, int fallback)
{
	const char *value = get_attr(tag, name);
	if (!value) return fallback;
	return strcmp(value, "true") == 0 || strcmp(value, "1") == 0 || strcmp(value, "yes") == 0;
}

static int hex_digit(int c)
{
	if (c >= '0' && c <= '9') return c - '0';
	if (c >= 'a' && c <= 'f') return c - 'a' + 10;
	if (c >= 'A' && c <= 'F') return c - 'A' + 10;
	return -1;
}

static int parse_color(const ui_tag_t *tag, const char *name, bapi_color_t *out,
					   bapi_color_t fallback)
{
	const char *value	 = get_attr(tag, name);
	uint8_t		bytes[4] = {fallback.r, fallback.g, fallback.b, fallback.a};
	if (!value) {
		*out = fallback;
		return 0;
	}
	if (*value == '#') value++;
	size_t length = strlen(value);
	if (length != 6 && length != 8) return -1;
	for (size_t i = 0; i < length / 2; i++) {
		int hi = hex_digit(value[i * 2]);
		int lo = hex_digit(value[i * 2 + 1]);
		if (hi < 0 || lo < 0) return -1;
		bytes[i] = (uint8_t)((hi << 4) | lo);
	}
	*out = bapi_color(bytes[0], bytes[1], bytes[2], bytes[3]);
	return 0;
}

static bapi_ui_component_type_t component_type(const char *name)
{
#define UI_TYPE(tag, value)                                                                        \
	if (strcmp(name, tag) == 0) return value
	UI_TYPE("rect", BAPI_UI_COMPONENT_RECT);
	UI_TYPE("label", BAPI_UI_COMPONENT_LABEL);
	UI_TYPE("button", BAPI_UI_COMPONENT_BUTTON);
	UI_TYPE("image", BAPI_UI_COMPONENT_IMAGE);
	UI_TYPE("line", BAPI_UI_COMPONENT_LINE);
	UI_TYPE("circle", BAPI_UI_COMPONENT_CIRCLE);
	UI_TYPE("polygon", BAPI_UI_COMPONENT_POLYGON);
	UI_TYPE("border", BAPI_UI_COMPONENT_BORDER);
	UI_TYPE("progress", BAPI_UI_COMPONENT_PROGRESS);
	UI_TYPE("separator", BAPI_UI_COMPONENT_SEPARATOR);
	UI_TYPE("panel", BAPI_UI_COMPONENT_PANEL);
	UI_TYPE("container", BAPI_UI_COMPONENT_CONTAINER);
	UI_TYPE("row", BAPI_UI_COMPONENT_ROW);
	UI_TYPE("column", BAPI_UI_COMPONENT_COLUMN);
	UI_TYPE("grid", BAPI_UI_COMPONENT_GRID);
	UI_TYPE("checkbox", BAPI_UI_COMPONENT_CHECKBOX);
	UI_TYPE("radio", BAPI_UI_COMPONENT_RADIO);
	UI_TYPE("toggle", BAPI_UI_COMPONENT_TOGGLE);
	UI_TYPE("slider", BAPI_UI_COMPONENT_SLIDER);
	UI_TYPE("input", BAPI_UI_COMPONENT_INPUT);
	UI_TYPE("select", BAPI_UI_COMPONENT_SELECT);
	UI_TYPE("dropdown", BAPI_UI_COMPONENT_SELECT);
	UI_TYPE("list", BAPI_UI_COMPONENT_LIST);
	UI_TYPE("scroll", BAPI_UI_COMPONENT_SCROLL);
	UI_TYPE("tab", BAPI_UI_COMPONENT_TAB);
	UI_TYPE("video", BAPI_UI_COMPONENT_VIDEO);
	UI_TYPE("canvas", BAPI_UI_COMPONENT_CANVAS);
	UI_TYPE("nine_patch", BAPI_UI_COMPONENT_NINE_PATCH);
	UI_TYPE("animation", BAPI_UI_COMPONENT_ANIMATION);
	UI_TYPE("tooltip", BAPI_UI_COMPONENT_TOOLTIP);
	UI_TYPE("modal", BAPI_UI_COMPONENT_MODAL);
	UI_TYPE("popup", BAPI_UI_COMPONENT_POPUP);
#undef UI_TYPE
	return (bapi_ui_component_type_t)-1;
}

static char *path_dirname(const char *filepath)
{
	const char *slash	  = strrchr(filepath, '/');
	const char *backslash = strrchr(filepath, '\\');
	const char *last	  = slash;
	if (!last || (backslash && backslash > last)) last = backslash;
	return last ? copy_range(filepath, last + 1) : ui_strdup("");
}

static int is_absolute_path(const char *path)
{
	return path && (path[0] == '/' || path[0] == '\\' ||
					(isalpha((unsigned char)path[0]) && path[1] == ':'));
}

static char *join_path(const char *base, const char *path)
{
	if (is_absolute_path(path) || !base[0]) return ui_strdup(path);
	size_t base_len = strlen(base), path_len = strlen(path);
	int	   separator = base_len && base[base_len - 1] != '/' && base[base_len - 1] != '\\';
	char  *result	 = malloc(base_len + (size_t)separator + path_len + 1);
	if (!result) return NULL;
	memcpy(result, base, base_len);
	if (separator) result[base_len++] = '/';
	memcpy(result + base_len, path, path_len + 1);
	return result;
}

static int apply_attributes(bapi_ui_component_t component, const ui_tag_t *tag,
							const char *base_dir)
{
	const char *id = get_attr(tag, "id");
	float		value;
	if (!id || parse_float(tag, "x", &component->rect.x) != 0 ||
		parse_float(tag, "y", &component->rect.y) != 0)
		return -1;
	if (parse_float(tag, "w", &component->rect.w) != 0) component->rect.w = 0;
	if (parse_float(tag, "h", &component->rect.h) != 0) component->rect.h = 0;
	component->visible			 = parse_bool(tag, "visible", 1);
	component->enabled			 = parse_bool(tag, "enabled", 1);
	component->relative_position = parse_bool(tag, "relative", 0);
	component->text				 = ui_strdup(get_attr(tag, "text") ? get_attr(tag, "text") : "");
	if (!component->text) return -1;
	if (parse_float(tag, "size", &value) == 0) component->text_size = value;
	if (parse_float(tag, "text_size", &value) == 0) component->text_size = value;
	if (parse_float(tag, "value", &component->value) != 0)
		component->value = component->type == BAPI_UI_COMPONENT_PROGRESS ? 1.0f : component->value;
	if (parse_float(tag, "min", &component->min_value) != 0) component->min_value = 0;
	if (parse_float(tag, "max", &component->max_value) != 0) component->max_value = 1;
	if (parse_float(tag, "step", &component->step) != 0) component->step = 1;
	parse_int(tag, "sides", &component->sides);
	parse_int(tag, "columns", &component->columns);
	parse_int(tag, "rows", &component->rows);
	parse_float(tag, "radius", &component->radius);
	if (parse_color(tag, "color", &component->color, component->color) != 0 ||
		parse_color(tag, "color2", &component->color2, component->color2) != 0 ||
		parse_color(tag, "normal", &component->color, component->color) != 0 ||
		parse_color(tag, "hover", &component->color2, component->color2) != 0 ||
		parse_color(tag, "click", &component->color3, component->color3) != 0 ||
		parse_color(tag, "text_color", &component->text_color, component->text_color) != 0)
		return -1;
	component->checked = parse_bool(tag, "checked", 0);
	if (parse_int(tag, "items", &component->item_count) != 0) component->item_count = 0;
	if (parse_int(tag, "max_length", &component->max_text_length) != 0)
		component->max_text_length = 256;
	component->local_rect = component->rect;
	const char *src		  = get_attr(tag, "src");
	if (src) {
		char *path = join_path(base_dir, src);
		if (!path) return -1;
		component->src = ui_strdup(path);
		if (component->type == BAPI_UI_COMPONENT_VIDEO) {
#ifdef USE_BACKEND_XJ380
			component->visible = 0;
			component->enabled = 0;
#else
			component->video = bapi_video_load(path);
			if (!component->video) {
				free(path);
				return -1;
			}
#endif
		} else {
			component->texture = bapi_texture_from_file(path, NULL, NULL);
		}
		free(path);
		if (!component->src || (component->type != BAPI_UI_COMPONENT_VIDEO && !component->texture))
			return -1;
	}
	return 0;
}

static int next_tag(ui_parser_t *parser, ui_tag_t *tag)
{
	parser->cursor = strchr(parser->cursor, '<');
	if (!parser->cursor) return 0;
	if (strncmp(parser->cursor, "<!--", 4) == 0) {
		const char *end = strstr(parser->cursor + 4, "-->");
		if (!end) return -1;
		parser->cursor = end + 3;
		return next_tag(parser, tag);
	}
	if (strncmp(parser->cursor, "<?", 2) == 0) {
		const char *end = strstr(parser->cursor + 2, "?>");
		if (!end) return -1;
		parser->cursor = end + 2;
		return next_tag(parser, tag);
	}
	const char *end = find_tag_end(parser->cursor + 1);
	if (!end || parse_tag(parser->cursor, end, tag) != 0) return -1;
	parser->cursor = end + 1;
	return 1;
}

static bapi_ui_component_t parse_component(ui_parser_t *parser, ui_tag_t *opening)
{
	bapi_ui_component_type_t type = component_type(opening->name);
	if ((int)type < 0 || (!opening->self_closing && opening->closing)) return NULL;
	bapi_ui_component_t component = bapi_ui_component_create(type, get_attr(opening, "id"));
	if (!component || apply_attributes(component, opening, parser->base_dir) != 0) {
		bapi_ui_component_destroy(component);
		return NULL;
	}
	if (opening->self_closing) return component;
	while (!parser->failed) {
		ui_tag_t tag;
		int		 result = next_tag(parser, &tag);
		if (result <= 0) {
			parser->failed = 1;
			return component;
		}
		if (tag.closing) {
			int matches = strcmp(tag.name, opening->name) == 0;
			destroy_tag(&tag);
			if (!matches) parser->failed = 1;
			return component;
		}
		bapi_ui_component_t child = parse_component(parser, &tag);
		destroy_tag(&tag);
		if (!child || bapi_ui_component_add_child(component, child) != 0) {
			bapi_ui_component_destroy(child);
			parser->failed = 1;
			return component;
		}
	}
	return component;
}

bapi_ui_t bapi_ui_load_from_xml(const char *filepath)
{
	FILE *file;
	long  size;
	char *xml;
	if (!filepath || !filepath[0] || !(file = fopen(filepath, "rb"))) return NULL;
	fseek(file, 0, SEEK_END);
	size = ftell(file);
	fseek(file, 0, SEEK_SET);
	if (size < 0 || !(xml = malloc((size_t)size + 1))) {
		fclose(file);
		return NULL;
	}
	if (fread(xml, 1, (size_t)size, file) != (size_t)size) {
		fclose(file);
		free(xml);
		return NULL;
	}
	xml[size] = '\0';
	fclose(file);
	char	   *base_dir	= path_dirname(filepath);
	bapi_ui_t	ui			= bapi_ui_create();
	ui_parser_t parser		= {xml, base_dir, ui, 0};
	ui_tag_t	root		= {0};
	int			root_parsed = 0;
	if (!base_dir || !ui || next_tag(&parser, &root) != 1 || root.closing ||
		strcmp(root.name, "ui") != 0)
		parser.failed = 1;
	else
		root_parsed = 1;
	if (!parser.failed && root.self_closing) root.closing = 1;
	while (!parser.failed && !root.closing) {
		ui_tag_t tag;
		int		 result = next_tag(&parser, &tag);
		if (result != 1) {
			parser.failed = 1;
			break;
		}
		if (tag.closing) {
			if (strcmp(tag.name, "ui") != 0) parser.failed = 1;
			destroy_tag(&tag);
			root.closing = 1;
			break;
		}
		bapi_ui_component_t component = parse_component(&parser, &tag);
		destroy_tag(&tag);
		if (!component || bapi_ui_add_root(ui, component) != 0) {
			bapi_ui_component_destroy(component);
			parser.failed = 1;
			break;
		}
	}
	int root_closed = root_parsed && root.closing;
	destroy_tag(&root);
	if (!root_closed) parser.failed = 1;
	if (ui) bapi_ui_layout(ui);
	if (parser.failed) {
		bapi_ui_destroy(ui);
		ui = NULL;
	}
	free(base_dir);
	free(xml);
	return ui;
}
