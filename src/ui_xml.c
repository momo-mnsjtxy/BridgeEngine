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
	int			depth;
} ui_parser_t;

/* Bounds component nesting so deeply nested XML cannot overflow the stack. */
#define MAX_UI_XML_DEPTH 256

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
	/* Reject values outside int range before casting; casting an out-of-range
	 * float (e.g. infinity from a huge digit string) is undefined behavior. */
	if (value < -2147483648.0f || value >= 2147483648.0f) return -1;
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
		component->src_raw = ui_strdup(src);
		if (!component->src_raw) return -1;
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
	for (;;) {
		parser->cursor = strchr(parser->cursor, '<');
		if (!parser->cursor) return 0;
		if (strncmp(parser->cursor, "<!--", 4) == 0) {
			const char *end = strstr(parser->cursor + 4, "-->");
			if (!end) return -1;
			parser->cursor = end + 3;
			continue;
		}
		if (strncmp(parser->cursor, "<?", 2) == 0) {
			const char *end = strstr(parser->cursor + 2, "?>");
			if (!end) return -1;
			parser->cursor = end + 2;
			continue;
		}
		break;
	}
	const char *end = find_tag_end(parser->cursor + 1);
	if (!end) return -1;
	if (parse_tag(parser->cursor, end, tag) != 0) {
		destroy_tag(tag);
		return -1;
	}
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
		if (parser->depth >= MAX_UI_XML_DEPTH) {
			parser->failed = 1;
			destroy_tag(&tag);
			return component;
		}
		parser->depth++;
		bapi_ui_component_t child = parse_component(parser, &tag);
		parser->depth--;
		destroy_tag(&tag);
		if (!child || bapi_ui_component_add_child(component, child) != 0) {
			bapi_ui_component_destroy(child);
			parser->failed = 1;
			return component;
		}
	}
	return component;
}

static bapi_ui_t parse_ui_buffer(const char *xml, const char *base_dir)
{
	bapi_ui_t	ui			= bapi_ui_create();
	ui_parser_t parser		= {xml, base_dir, ui, 0, 0};
	ui_tag_t	root		= {0};
	int			root_parsed = 0;
	if (!ui || next_tag(&parser, &root) != 1 || root.closing ||
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
	return ui;
}

bapi_ui_t bapi_ui_load_from_xml(const char *filepath)
{
	if (!filepath || !filepath[0]) return NULL;

	FILE *file;
	long  size;
	char *xml;
	if (!(file = fopen(filepath, "rb"))) return NULL;
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
	char	   *base_dir = path_dirname(filepath);
	bapi_ui_t	ui		   = base_dir ? parse_ui_buffer(xml, base_dir) : NULL;
	free(base_dir);
	free(xml);
	return ui;
}

static const char *ui_tag_name(bapi_ui_component_type_t type)
{
	switch (type) {
	case BAPI_UI_COMPONENT_RECT:
		return "rect";
	case BAPI_UI_COMPONENT_LABEL:
		return "label";
	case BAPI_UI_COMPONENT_BUTTON:
		return "button";
	case BAPI_UI_COMPONENT_IMAGE:
		return "image";
	case BAPI_UI_COMPONENT_LINE:
		return "line";
	case BAPI_UI_COMPONENT_CIRCLE:
		return "circle";
	case BAPI_UI_COMPONENT_POLYGON:
		return "polygon";
	case BAPI_UI_COMPONENT_BORDER:
		return "border";
	case BAPI_UI_COMPONENT_PROGRESS:
		return "progress";
	case BAPI_UI_COMPONENT_SEPARATOR:
		return "separator";
	case BAPI_UI_COMPONENT_PANEL:
		return "panel";
	case BAPI_UI_COMPONENT_CONTAINER:
		return "container";
	case BAPI_UI_COMPONENT_ROW:
		return "row";
	case BAPI_UI_COMPONENT_COLUMN:
		return "column";
	case BAPI_UI_COMPONENT_GRID:
		return "grid";
	case BAPI_UI_COMPONENT_CHECKBOX:
		return "checkbox";
	case BAPI_UI_COMPONENT_RADIO:
		return "radio";
	case BAPI_UI_COMPONENT_TOGGLE:
		return "toggle";
	case BAPI_UI_COMPONENT_SLIDER:
		return "slider";
	case BAPI_UI_COMPONENT_INPUT:
		return "input";
	case BAPI_UI_COMPONENT_SELECT:
		return "select";
	case BAPI_UI_COMPONENT_LIST:
		return "list";
	case BAPI_UI_COMPONENT_SCROLL:
		return "scroll";
	case BAPI_UI_COMPONENT_TAB:
		return "tab";
	case BAPI_UI_COMPONENT_VIDEO:
		return "video";
	case BAPI_UI_COMPONENT_CANVAS:
		return "canvas";
	case BAPI_UI_COMPONENT_NINE_PATCH:
		return "nine_patch";
	case BAPI_UI_COMPONENT_ANIMATION:
		return "animation";
	case BAPI_UI_COMPONENT_TOOLTIP:
		return "tooltip";
	case BAPI_UI_COMPONENT_MODAL:
		return "modal";
	case BAPI_UI_COMPONENT_POPUP:
		return "popup";
	default:
		return NULL;
	}
}

typedef struct {
	char   *buf;
	size_t  len;
	size_t  cap;
	int	  indent;
	int	  failed;
} ui_writer_t;

static void ui_write_reserve(ui_writer_t *w, size_t extra)
{
	if (w->failed || w->len + extra + 1 <= w->cap) return;
	size_t new_cap = w->cap ? w->cap * 2 : 4096;
	while (new_cap < w->len + extra + 1) new_cap *= 2;
	char *nbuf = realloc(w->buf, new_cap);
	if (!nbuf) {
		w->failed = 1;
		return;
	}
	w->buf = nbuf;
	w->cap = new_cap;
}

static void ui_write_raw(ui_writer_t *w, const char *s, size_t n)
{
	ui_write_reserve(w, n);
	if (w->failed) return;
	memcpy(w->buf + w->len, s, n);
	w->len += n;
	w->buf[w->len] = '\0';
}

static void ui_write_str(ui_writer_t *w, const char *s)
{
	if (s) ui_write_raw(w, s, strlen(s));
}

static void ui_write_fmt(ui_writer_t *w, const char *fmt, ...)
{
	char	 small[512];
	va_list ap;
	va_start(ap, fmt);
	va_list copy;
	va_copy(copy, ap);
	int need = vsnprintf(small, sizeof(small), fmt, ap);
	va_end(ap);
	if (need < 0) {
		va_end(copy);
		w->failed = 1;
		return;
	}
	if ((size_t)need < sizeof(small)) {
		ui_write_raw(w, small, (size_t)need);
	} else {
		char *big = malloc((size_t)need + 1);
		if (!big) {
			va_end(copy);
			w->failed = 1;
			return;
		}
		vsnprintf(big, (size_t)need + 1, fmt, copy);
		va_end(copy);
		ui_write_raw(w, big, (size_t)need);
		free(big);
	}
}

static void ui_write_indent(ui_writer_t *w)
{
	for (int i = 0; i < w->indent; i++) ui_write_str(w, "  ");
}

static void ui_write_attr_encoded(ui_writer_t *w, const char *name, const char *value)
{
	ui_write_str(w, " ");
	ui_write_str(w, name);
	ui_write_str(w, "=\"");
	for (const char *cursor = value; *cursor; cursor++) {
		switch (*cursor) {
		case '&':
			ui_write_str(w, "&amp;");
			break;
		case '<':
			ui_write_str(w, "&lt;");
			break;
		case '>':
			ui_write_str(w, "&gt;");
			break;
		case '"':
			ui_write_str(w, "&quot;");
			break;
		case '\'':
			ui_write_str(w, "&apos;");
			break;
		default:
			ui_write_raw(w, cursor, 1);
			break;
		}
	}
	ui_write_str(w, "\"");
}

static void ui_write_float_attr(ui_writer_t *w, const char *name, float value)
{
	if (value == (float)(int)value) {
		ui_write_fmt(w, " %s=\"%d\"", name, (int)value);
		return;
	}
	char buffer[64];
	snprintf(buffer, sizeof(buffer), "%.6f", (double)value);
	size_t len = strlen(buffer);
	while (len > 0 && buffer[len - 1] == '0') len--;
	if (len > 0 && buffer[len - 1] == '.') len--;
	buffer[len] = '\0';
	ui_write_fmt(w, " %s=\"%s\"", name, buffer);
}

static void ui_write_int_attr(ui_writer_t *w, const char *name, int value)
{
	ui_write_fmt(w, " %s=\"%d\"", name, value);
}

static void ui_write_color_attr(ui_writer_t *w, const char *name, bapi_color_t color)
{
	ui_write_fmt(w, " %s=\"#%02X%02X%02X%02X\"", name, color.r, color.g, color.b, color.a);
}

static int color_equals(bapi_color_t a, bapi_color_t b)
{
	return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

static void ui_write_component(ui_writer_t *w, bapi_ui_component_t component)
{
	const char *tag = ui_tag_name(component->type);
	if (!tag || !component->id) {
		w->failed = 1;
		return;
	}
	ui_write_indent(w);
	ui_write_fmt(w, "<%s", tag);
	ui_write_fmt(w, " id=\"%s\"", component->id);
	ui_write_float_attr(w, "x", component->local_rect.x);
	ui_write_float_attr(w, "y", component->local_rect.y);
	ui_write_float_attr(w, "w", component->local_rect.w);
	ui_write_float_attr(w, "h", component->local_rect.h);
	if (!component->visible) ui_write_str(w, " visible=\"false\"");
	if (!component->enabled) ui_write_str(w, " enabled=\"false\"");
	if (component->relative_position) ui_write_str(w, " relative=\"true\"");
	if (component->text && component->text[0]) ui_write_attr_encoded(w, "text", component->text);
	if (component->text_size != 18.0f)
		ui_write_float_attr(w, component->type == BAPI_UI_COMPONENT_BUTTON ? "text_size" : "size",
							component->text_size);
	if (component->radius != 0.0f) ui_write_float_attr(w, "radius", component->radius);
	if (component->sides != 0) ui_write_int_attr(w, "sides", component->sides);
	if (component->columns != 0) ui_write_int_attr(w, "columns", component->columns);
	if (component->rows != 0) ui_write_int_attr(w, "rows", component->rows);
	if (component->item_count != 0) ui_write_int_attr(w, "items", component->item_count);
	if (component->max_text_length != 256)
		ui_write_int_attr(w, "max_length", component->max_text_length);
	if (component->checked) ui_write_str(w, " checked=\"true\"");
	if (component->type == BAPI_UI_COMPONENT_PROGRESS ||
		component->type == BAPI_UI_COMPONENT_SLIDER) {
		ui_write_float_attr(w, "value", component->value);
		if (component->min_value != 0.0f) ui_write_float_attr(w, "min", component->min_value);
		if (component->max_value != 1.0f) ui_write_float_attr(w, "max", component->max_value);
		if (component->step != 1.0f) ui_write_float_attr(w, "step", component->step);
	}
	if (component->type == BAPI_UI_COMPONENT_ROW || component->type == BAPI_UI_COMPONENT_COLUMN ||
		component->type == BAPI_UI_COMPONENT_GRID) {
		if (component->step != 1.0f) ui_write_float_attr(w, "step", component->step);
	}
	if (component->type == BAPI_UI_COMPONENT_BUTTON) {
		if (!color_equals(component->color, bapi_color(255, 255, 255, 255)))
			ui_write_color_attr(w, "normal", component->color);
		if (!color_equals(component->color2, bapi_color(0, 0, 0, 255)))
			ui_write_color_attr(w, "hover", component->color2);
		if (!color_equals(component->color3, bapi_color(0, 0, 0, 255)))
			ui_write_color_attr(w, "click", component->color3);
		if (!color_equals(component->text_color, bapi_color(255, 255, 255, 255)))
			ui_write_color_attr(w, "text_color", component->text_color);
	} else {
		if (!color_equals(component->color, bapi_color(255, 255, 255, 255)))
			ui_write_color_attr(w, "color", component->color);
		if (!color_equals(component->color2, bapi_color(0, 0, 0, 255)))
			ui_write_color_attr(w, "color2", component->color2);
	}
	if (component->src_raw && component->src_raw[0])
		ui_write_attr_encoded(w, "src", component->src_raw);
	if (component->child_count > 0) {
		ui_write_str(w, ">\n");
		w->indent++;
		for (int i = 0; i < component->child_count; i++)
			ui_write_component(w, component->children[i]);
		w->indent--;
		ui_write_indent(w);
		ui_write_fmt(w, "</%s>\n", tag);
	} else {
		ui_write_str(w, " />\n");
	}
}

// Serialize a UI into a freshly malloc'd NUL-terminated XML string.
static char *ui_serialize_to_memory(bapi_ui_t ui)
{
	if (!ui) return NULL;
	ui_writer_t w = {NULL, 0, 0, 0, 0};
	ui_write_str(&w, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<ui>\n");
	if (!w.failed)
		for (int i = 0; i < ui->root_count; i++) ui_write_component(&w, ui->roots[i]);
	if (!w.failed) ui_write_str(&w, "</ui>\n");
	if (w.failed || !w.buf) {
		free(w.buf);
		return NULL;
	}
	return w.buf;
}

int bapi_ui_save_to_xml(bapi_ui_t ui, const char *filepath)
{
	if (!ui || !filepath) return -1;
	char *xml = ui_serialize_to_memory(ui);
	if (!xml) return -1;
	FILE *file = fopen(filepath, "wb");
	if (!file) {
		free(xml);
		return -1;
	}
	size_t len = strlen(xml);
	size_t written = fwrite(xml, 1, len, file);
	fclose(file);
	free(xml);
	return written == len ? 0 : -1;
}

// ---------------------------------------------------------------------------
// .uix encrypted UI documents
//
// The byte-level encryption/decryption lives in src/ui_crypt.c; here we only
// bridge the UI object to those helpers. Layout and behaviour are documented
// in BridgeEngine.h next to bapi_ui_load_from_file / bapi_ui_save_to_file.
// ---------------------------------------------------------------------------

int bapi_ui_save_to_file(bapi_ui_t ui, const char *filepath, const char *key)
{
	if (!ui || !filepath || !filepath[0]) return -1;
	if (!key || !key[0]) return bapi_ui_save_to_xml(ui, filepath);

	char *xml = ui_serialize_to_memory(ui);
	if (!xml) return -1;
	size_t len = strlen(xml);

	unsigned char *payload;
	size_t		   payload_len;
	if (bapi_uix_encrypt_mem(key, (const unsigned char *)xml, len, &payload, &payload_len) != 0) {
		free(xml);
		return -1;
	}
	free(xml);

	FILE *f = fopen(filepath, "wb");
	if (!f) {
		free(payload);
		return -1;
	}
	int ok = fwrite(payload, 1, payload_len, f) == payload_len;
	fclose(f);
	free(payload);
	return ok ? 0 : -1;
}

// Parse UI from raw file bytes (may be plain XML or encrypted .uix), resolving
// src-relative paths against `base_dir`. Returns a ui or NULL.
static bapi_ui_t load_ui_from_bytes(const unsigned char *data, size_t size, const char *key,
									const char *base_dir)
{
	if (!data || size == 0) return NULL;

	// Plain XML: only allowed when no key is expected.
	if (!bapi_uix_is_encrypted_mem(data, size)) {
		if (key && key[0]) return NULL;
		char *xml = malloc(size + 1);
		if (!xml) return NULL;
		memcpy(xml, data, size);
		xml[size] = '\0';
		bapi_ui_t ui = base_dir ? parse_ui_buffer(xml, base_dir) : NULL;
		free(xml);
		return ui;
	}
	if (!key || !key[0]) {
		return NULL; // encrypted file requires a key
	}

	unsigned char *plain;
	size_t		   plain_len;
	if (bapi_uix_decrypt_mem(key, data, size, &plain, &plain_len) != 0) {
		return NULL; // wrong key or tampered file
	}

	char *xml = malloc(plain_len + 1);
	if (!xml) {
		free(plain);
		return NULL;
	}
	memcpy(xml, plain, plain_len);
	xml[plain_len] = '\0';
	free(plain);

	bapi_ui_t ui = base_dir ? parse_ui_buffer(xml, base_dir) : NULL;
	free(xml);
	return ui;
}

bapi_ui_t bapi_ui_load_from_file(const char *filepath, const char *key)
{
	if (!filepath || !filepath[0]) return NULL;
	char *base_dir = path_dirname(filepath);

	FILE *file = fopen(filepath, "rb");
	if (!file) {
		free(base_dir);
		return NULL;
	}
	fseek(file, 0, SEEK_END);
	long size = ftell(file);
	fseek(file, 0, SEEK_SET);
	if (size < 0) {
		fclose(file);
		free(base_dir);
		return NULL;
	}
	unsigned char *data = malloc((size_t)size ? (size_t)size : 1);
	if (!data) {
		fclose(file);
		free(base_dir);
		return NULL;
	}
	if (size > 0 && fread(data, 1, (size_t)size, file) != (size_t)size) {
		free(data);
		fclose(file);
		free(base_dir);
		return NULL;
	}
	fclose(file);

	bapi_ui_t ui = load_ui_from_bytes(data, (size_t)size, key, base_dir);
	free(data);
	free(base_dir);
	return ui;
}
