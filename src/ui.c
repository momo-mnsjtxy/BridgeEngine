#include "BridgeEngine.h"
#include "internal/scene/ui_internal.h"
#include <stdlib.h>
#include <string.h>

static char *ui_strdup(const char *value)
{
	if (!value) return NULL;
	size_t length = strlen(value) + 1;
	char  *copy	  = malloc(length);
	if (copy) memcpy(copy, value, length);
	return copy;
}

static int ui_reserve(void **items, int *capacity, int count, size_t item_size)
{
	if (count < *capacity) return 0;
	int	  new_capacity = *capacity ? *capacity * 2 : 8;
	void *new_items	   = realloc(*items, (size_t)new_capacity * item_size);
	if (!new_items) return -1;
	*items	  = new_items;
	*capacity = new_capacity;
	return 0;
}

void bapi_ui_component_destroy(bapi_ui_component_t component)
{
	if (!component) return;
	for (int i = 0; i < component->child_count; i++)
		bapi_ui_component_destroy(component->children[i]);
	free(component->children);
	free(component->id);
	free(component->text);
	free(component->src);
	if (component->texture) bapi_texture_destroy(component->texture);
	if (component->video) bapi_video_free(component->video);
	free(component);
}

static int ui_tree_has_id(bapi_ui_component_t component, const char *id)
{
	if (!component || !id) return 0;
	if (strcmp(component->id, id) == 0) return 1;
	for (int i = 0; i < component->child_count; i++)
		if (ui_tree_has_id(component->children[i], id)) return 1;
	return 0;
}

static int ui_tree_conflicts(bapi_ui_component_t candidate, bapi_ui_component_t existing)
{
	if (!candidate || !existing) return 0;
	if (ui_tree_has_id(existing, candidate->id)) return 1;
	for (int i = 0; i < candidate->child_count; i++)
		if (ui_tree_conflicts(candidate->children[i], existing)) return 1;
	return 0;
}

static int ui_tree_count_id(bapi_ui_component_t component, const char *id)
{
	int count = 0;
	if (!component || !id) return 0;
	if (strcmp(component->id, id) == 0) count++;
	for (int i = 0; i < component->child_count; i++)
		count += ui_tree_count_id(component->children[i], id);
	return count;
}

static int ui_tree_has_duplicate(bapi_ui_component_t component, bapi_ui_component_t root)
{
	if (!component) return 0;
	if (component != root && ui_tree_count_id(root, component->id) > 1) return 1;
	for (int i = 0; i < component->child_count; i++)
		if (ui_tree_has_duplicate(component->children[i], root)) return 1;
	return 0;
}

bapi_ui_t bapi_ui_create(void)
{
	return calloc(1, sizeof(struct bapi_ui_internal));
}

bapi_ui_component_t bapi_ui_component_create(bapi_ui_component_type_t type, const char *id)
{
	if (!id || !id[0]) return NULL;
	bapi_ui_component_t component = calloc(1, sizeof(*component));
	if (!component) return NULL;
	component->type			   = type;
	component->id			   = ui_strdup(id);
	component->visible		   = 1;
	component->enabled		   = 1;
	component->max_value	   = 1.0f;
	component->step			   = 1.0f;
	component->selected_index  = -1;
	component->max_text_length = 256;
	component->color		   = bapi_color(255, 255, 255, 255);
	component->color2		   = bapi_color(0, 0, 0, 255);
	component->color3		   = component->color2;
	component->text_color	   = component->color;
	component->text_size	   = 18.0f;
	if (!component->id) {
		free(component);
		return NULL;
	}
	return component;
}

int bapi_ui_component_add_child(bapi_ui_component_t parent, bapi_ui_component_t child)
{
	/* A component with a parent is already owned by a tree; mounting it again
	 * (same parent or another) would make bapi_ui_component_destroy free it
	 * twice. */
	if (!parent || !child || child->parent != NULL ||
		ui_reserve((void **)&parent->children, &parent->child_capacity, parent->child_count,
				   sizeof(*parent->children)) != 0)
		return -1;
	child->parent							= parent;
	parent->children[parent->child_count++] = child;
	return 0;
}

int bapi_ui_add_root(bapi_ui_t ui, bapi_ui_component_t component)
{
	if (!ui || !component || component->parent != NULL ||
		ui_tree_has_duplicate(component, component))
		return -1;
	for (int i = 0; i < ui->root_count; i++)
		if (ui_tree_conflicts(component, ui->roots[i])) return -1;
	if (ui_reserve((void **)&ui->roots, &ui->root_capacity, ui->root_count, sizeof(*ui->roots)) !=
		0)
		return -1;
	component->parent			= NULL;
	ui->roots[ui->root_count++] = component;
	return 0;
}

void bapi_ui_destroy(bapi_ui_t ui)
{
	if (!ui) return;
	for (int i = 0; i < ui->root_count; i++) bapi_ui_component_destroy(ui->roots[i]);
	free(ui->roots);
	free(ui);
}

static bapi_ui_component_t ui_find_node(bapi_ui_component_t component, const char *id)
{
	if (!component || !id) return NULL;
	if (strcmp(component->id, id) == 0) return component;
	for (int i = 0; i < component->child_count; i++) {
		bapi_ui_component_t found = ui_find_node(component->children[i], id);
		if (found) return found;
	}
	return NULL;
}

bapi_ui_component_t bapi_ui_find(bapi_ui_t ui, const char *id)
{
	if (!ui || !id) return NULL;
	for (int i = 0; i < ui->root_count; i++) {
		bapi_ui_component_t found = ui_find_node(ui->roots[i], id);
		if (found) return found;
	}
	return NULL;
}

static int ui_is_layout(bapi_ui_component_t component)
{
	return component->type == BAPI_UI_COMPONENT_ROW ||
		   component->type == BAPI_UI_COMPONENT_COLUMN || component->type == BAPI_UI_COMPONENT_GRID;
}

static void ui_layout_node(bapi_ui_component_t component)
{
	if (ui_is_layout(component)) {
		float cursor =
			component->type == BAPI_UI_COMPONENT_ROW ? component->rect.x : component->rect.y;
		int columns = component->columns > 0 ? component->columns : 1;
		for (int i = 0; i < component->child_count; i++) {
			bapi_ui_component_t child = component->children[i];
			child->rect				  = child->local_rect;
			if (child->relative_position) {
				child->rect.x += component->rect.x;
				child->rect.y += component->rect.y;
				if (component->type == BAPI_UI_COMPONENT_SCROLL)
					child->rect.y -= component->scroll_offset;
			}
			if (component->type == BAPI_UI_COMPONENT_ROW) {
				child->rect.x = cursor;
				child->rect.y = component->rect.y;
				cursor += child->rect.w + component->step;
			} else if (component->type == BAPI_UI_COMPONENT_COLUMN) {
				child->rect.x = component->rect.x;
				child->rect.y = cursor;
				cursor += child->rect.h + component->step;
			} else {
				child->rect.x =
					component->rect.x + (float)(i % columns) * (child->rect.w + component->step);
				child->rect.y =
					component->rect.y + (float)(i / columns) * (child->rect.h + component->step);
			}
			ui_layout_node(child);
		}
	} else {
		for (int i = 0; i < component->child_count; i++) {
			bapi_ui_component_t child = component->children[i];
			child->rect				  = child->local_rect;
			if (child->relative_position) {
				child->rect.x += component->rect.x;
				child->rect.y += component->rect.y;
				if (component->type == BAPI_UI_COMPONENT_SCROLL)
					child->rect.y -= component->scroll_offset;
			}
			ui_layout_node(child);
		}
	}
}

void bapi_ui_layout(bapi_ui_t ui)
{
	if (!ui) return;
	for (int i = 0; i < ui->root_count; i++) {
		ui->roots[i]->rect = ui->roots[i]->local_rect;
		ui_layout_node(ui->roots[i]);
	}
}

static int ui_is_interactive(bapi_ui_component_t component)
{
	switch (component->type) {
	case BAPI_UI_COMPONENT_BUTTON:
	case BAPI_UI_COMPONENT_CHECKBOX:
	case BAPI_UI_COMPONENT_RADIO:
	case BAPI_UI_COMPONENT_TOGGLE:
	case BAPI_UI_COMPONENT_SLIDER:
	case BAPI_UI_COMPONENT_INPUT:
	case BAPI_UI_COMPONENT_SELECT:
	case BAPI_UI_COMPONENT_LIST:
	case BAPI_UI_COMPONENT_TAB:
		return 1;
	default:
		return 0;
	}
}

static int ui_contains(bapi_ui_component_t component, float x, float y)
{
	return x >= component->rect.x && y >= component->rect.y &&
		   x <= component->rect.x + component->rect.w && y <= component->rect.y + component->rect.h;
}

static int ui_point_in_scroll_ancestors(bapi_ui_component_t component, float x, float y)
{
	for (bapi_ui_component_t parent = component->parent; parent; parent = parent->parent) {
		if (parent->type == BAPI_UI_COMPONENT_SCROLL && !ui_contains(parent, x, y)) return 0;
	}
	return 1;
}

static int ui_rect_intersects(bapi_rect_t a, bapi_rect_t b)
{
	return a.x < b.x + b.w && a.x + a.w > b.x && a.y < b.y + b.h && a.y + a.h > b.y;
}

static bapi_ui_component_t ui_hit_test(bapi_ui_component_t node, float x, float y,
									   int parent_visible)
{
	if (!node || !parent_visible || !node->visible) return NULL;
	for (int i = node->child_count - 1; i >= 0; i--) {
		bapi_ui_component_t hit = ui_hit_test(node->children[i], x, y, 1);
		if (hit) return hit;
	}
	return node->enabled && ui_is_interactive(node) && ui_contains(node, x, y) &&
				   ui_point_in_scroll_ancestors(node, x, y)
			   ? node
			   : NULL;
}

static bapi_ui_component_t ui_hit_test_document(bapi_ui_t ui, float x, float y)
{
	for (int i = ui->root_count - 1; i >= 0; i--) {
		bapi_ui_component_t hit = ui_hit_test(ui->roots[i], x, y, 1);
		if (hit) return hit;
	}
	return NULL;
}

static void ui_reset_transient(bapi_ui_component_t node)
{
	if (!node) return;
	node->clicked = 0;
	node->hovered = 0;
	for (int i = 0; i < node->child_count; i++) ui_reset_transient(node->children[i]);
}

static void ui_set_focus(bapi_ui_t ui, bapi_ui_component_t component)
{
	if (ui->focused && ui->focused != component) ui->focused->focused = 0;
	ui->focused = component;
	if (component) component->focused = 1;
}

static void ui_focus_next_node(bapi_ui_component_t node, bapi_ui_component_t current,
							   bapi_ui_component_t *first, bapi_ui_component_t *next,
							   int *seen_current)
{
	if (!node || !node->visible || !node->enabled) return;
	if (ui_is_interactive(node)) {
		if (!*first) *first = node;
		if (*seen_current && !*next) *next = node;
		if (node == current) *seen_current = 1;
	}
	for (int i = 0; i < node->child_count; i++)
		ui_focus_next_node(node->children[i], current, first, next, seen_current);
}

static void ui_focus_next(bapi_ui_t ui)
{
	bapi_ui_component_t first		 = NULL;
	bapi_ui_component_t next		 = NULL;
	int					seen_current = ui->focused == NULL;
	for (int i = 0; i < ui->root_count; i++)
		ui_focus_next_node(ui->roots[i], ui->focused, &first, &next, &seen_current);
	ui_set_focus(ui, next ? next : first);
}

static void ui_append_input(bapi_ui_component_t input, uint32_t key)
{
	if (!input || input->type != BAPI_UI_COMPONENT_INPUT) return;
	if (key == KEY_BACKSPACE) {
		size_t length = input->text ? strlen(input->text) : 0;
		if (length) input->text[length - 1] = '\0';
		return;
	}
	if (key < 32 || key > 126) return;
	size_t length = input->text ? strlen(input->text) : 0;
	if ((int)length >= input->max_text_length) return;
	char *text = realloc(input->text, length + 2);
	if (!text) return;
	text[length]	 = (char)key;
	text[length + 1] = '\0';
	input->text		 = text;
}

static float ui_value_ratio(bapi_ui_component_t component)
{
	float range = component->max_value - component->min_value;
	if (range <= 0) return 0;
	float ratio = (component->value - component->min_value) / range;
	if (ratio < 0) return 0;
	if (ratio > 1) return 1;
	return ratio;
}

static void ui_activate(bapi_ui_t ui, bapi_ui_component_t component, float pointer_x)
{
	component->clicked = 1;
	ui_set_focus(ui, component);
	if (component->type == BAPI_UI_COMPONENT_CHECKBOX ||
		component->type == BAPI_UI_COMPONENT_TOGGLE) {
		component->checked = !component->checked;
	} else if (component->type == BAPI_UI_COMPONENT_RADIO) {
		if (component->parent) {
			for (int i = 0; i < component->parent->child_count; i++) {
				bapi_ui_component_t sibling = component->parent->children[i];
				if (sibling->type == BAPI_UI_COMPONENT_RADIO) sibling->checked = 0;
			}
		}
		component->checked = 1;
	} else if (component->type == BAPI_UI_COMPONENT_SLIDER) {
		float ratio =
			component->rect.w > 0 ? (pointer_x - component->rect.x) / component->rect.w : 0;
		bapi_ui_component_set_value(component,
									component->min_value +
										ratio * (component->max_value - component->min_value));
	} else if (component->type == BAPI_UI_COMPONENT_SELECT ||
			   component->type == BAPI_UI_COMPONENT_LIST ||
			   component->type == BAPI_UI_COMPONENT_TAB) {
		component->selected_index =
			component->item_count > 0 ? (component->selected_index + 1) % component->item_count : 0;
	}
}

void bapi_ui_update(bapi_ui_t ui, const bapi_event_t *event)
{
	if (!ui || !event) return;
	for (int i = 0; i < ui->root_count; i++) ui_reset_transient(ui->roots[i]);
	if (event->type == BAPI_EVENT_KEY_DOWN) {
		if (event->data.key.key == KEY_TAB)
			ui_focus_next(ui);
		else if (event->data.key.key == KEY_ESC)
			ui_set_focus(ui, NULL);
		else if (event->data.key.key == KEY_ENTER && ui->focused)
			ui_activate(ui, ui->focused, ui->focused->rect.x);
		else
			ui_append_input(ui->focused, event->data.key.key);
		return;
	}
	if (event->type != BAPI_EVENT_MOUSE_MOTION && event->type != BAPI_EVENT_MOUSE_BUTTON_DOWN &&
		event->type != BAPI_EVENT_MOUSE_BUTTON_UP)
		return;
	float x = event->type == BAPI_EVENT_MOUSE_MOTION ? event->data.motion.x : event->data.button.x;
	float y = event->type == BAPI_EVENT_MOUSE_MOTION ? event->data.motion.y : event->data.button.y;
	bapi_ui_component_t hit = ui_hit_test_document(ui, x, y);
	ui->hovered				= hit;
	if (hit) hit->hovered = 1;
	if (event->type == BAPI_EVENT_MOUSE_BUTTON_DOWN) {
		ui->pressed = hit;
		if (hit) hit->pressed = 1;
	} else if (event->type == BAPI_EVENT_MOUSE_BUTTON_UP) {
		if (ui->pressed && ui->pressed == hit) ui_activate(ui, hit, x);
		if (ui->pressed) ui->pressed->pressed = 0;
		ui->pressed = NULL;
	}
}

static void ui_render_node(bapi_ui_component_t node, int parent_visible)
{
	if (!node || !parent_visible || !node->visible) return;
	int			 interactive = ui_is_interactive(node);
	bapi_color_t color		 = interactive && node->pressed
								   ? node->color3
								   : (interactive && node->hovered ? node->color2 : node->color);
	switch (node->type) {
	case BAPI_UI_COMPONENT_RECT:
	case BAPI_UI_COMPONENT_PANEL:
	case BAPI_UI_COMPONENT_CONTAINER:
	case BAPI_UI_COMPONENT_MODAL:
	case BAPI_UI_COMPONENT_POPUP:
		bapi_fill_rect(node->rect.x, node->rect.y, node->rect.w, node->rect.h, color);
		break;
	case BAPI_UI_COMPONENT_BORDER:
	case BAPI_UI_COMPONENT_SEPARATOR:
		bapi_draw_rect(node->rect.x, node->rect.y, node->rect.w, node->rect.h, color);
		break;
	case BAPI_UI_COMPONENT_LABEL:
	case BAPI_UI_COMPONENT_TOOLTIP:
	case BAPI_UI_COMPONENT_INPUT:
	case BAPI_UI_COMPONENT_SELECT:
	case BAPI_UI_COMPONENT_LIST:
	case BAPI_UI_COMPONENT_TAB:
		bapi_draw_text(node->text ? node->text : "", node->rect.x, node->rect.y, node->text_size,
					   color);
		if (node->focused)
			bapi_draw_rect(node->rect.x, node->rect.y, node->rect.w, node->rect.h, node->color2);
		break;
	case BAPI_UI_COMPONENT_BUTTON:
		bapi_fill_rect(node->rect.x, node->rect.y, node->rect.w, node->rect.h, color);
		bapi_draw_rect(node->rect.x, node->rect.y, node->rect.w, node->rect.h, node->color2);
		if (node->text)
			bapi_draw_text(node->text, node->rect.x + 8, node->rect.y + 8, node->text_size,
						   node->text_color);
		break;
	case BAPI_UI_COMPONENT_IMAGE:
	case BAPI_UI_COMPONENT_NINE_PATCH:
	case BAPI_UI_COMPONENT_ANIMATION:
		bapi_texture_render_ex(node->texture, node->rect.x, node->rect.y, node->rect.w,
							   node->rect.h);
		break;
	case BAPI_UI_COMPONENT_LINE:
		bapi_draw_line(node->rect.x, node->rect.y, node->rect.x + node->rect.w,
					   node->rect.y + node->rect.h, color);
		break;
	case BAPI_UI_COMPONENT_CIRCLE:
		(node->checked ? bapi_fill_circle : bapi_draw_circle)(node->rect.x, node->rect.y,
															  node->radius, color);
		break;
	case BAPI_UI_COMPONENT_POLYGON:
		bapi_draw_polygon(node->rect.x, node->rect.y, node->radius, node->sides, color);
		break;
	case BAPI_UI_COMPONENT_PROGRESS:
		bapi_fill_rect(node->rect.x, node->rect.y, node->rect.w, node->rect.h, node->color2);
		bapi_fill_rect(node->rect.x, node->rect.y, node->rect.w * ui_value_ratio(node),
					   node->rect.h, node->color);
		break;
	case BAPI_UI_COMPONENT_CHECKBOX:
	case BAPI_UI_COMPONENT_RADIO:
	case BAPI_UI_COMPONENT_TOGGLE:
		bapi_fill_rect(node->rect.x, node->rect.y, node->rect.w, node->rect.h, color);
		bapi_draw_rect(node->rect.x, node->rect.y, node->rect.w, node->rect.h, node->color2);
		if (node->checked)
			bapi_fill_rect(node->rect.x + 4, node->rect.y + 4, node->rect.w - 8, node->rect.h - 8,
						   node->color2);
		break;
	case BAPI_UI_COMPONENT_SLIDER:
		bapi_fill_rect(node->rect.x, node->rect.y, node->rect.w, node->rect.h, node->color2);
		bapi_fill_circle(node->rect.x + node->rect.w * ui_value_ratio(node),
						 node->rect.y + node->rect.h / 2, node->rect.h / 2, node->color);
		break;
	case BAPI_UI_COMPONENT_VIDEO:
		bapi_video_render(node->video, (int)node->rect.x, (int)node->rect.y, (int)node->rect.w,
						  (int)node->rect.h);
		break;
	case BAPI_UI_COMPONENT_CANVAS:
	case BAPI_UI_COMPONENT_SCROLL:
	case BAPI_UI_COMPONENT_ROW:
	case BAPI_UI_COMPONENT_COLUMN:
	case BAPI_UI_COMPONENT_GRID:
		break;
	}
	for (int i = 0; i < node->child_count; i++) {
		bapi_ui_component_t child = node->children[i];
		if (node->type == BAPI_UI_COMPONENT_SCROLL && !ui_rect_intersects(node->rect, child->rect))
			continue;
		ui_render_node(child, 1);
	}
}

void bapi_ui_render(bapi_ui_t ui)
{
	if (!ui) return;
	for (int i = 0; i < ui->root_count; i++) ui_render_node(ui->roots[i], 1);
}

int bapi_ui_was_clicked(bapi_ui_t ui, const char *id)
{
	bapi_ui_component_t component = bapi_ui_find(ui, id);
	return component ? component->clicked : 0;
}

bapi_ui_component_type_t bapi_ui_component_get_type(bapi_ui_component_t component)
{
	return component ? component->type : BAPI_UI_COMPONENT_RECT;
}

void bapi_ui_component_get_rect(bapi_ui_component_t component, bapi_rect_t *out_rect)
{
	if (out_rect) *out_rect = component ? component->rect : (bapi_rect_t){0, 0, 0, 0};
}

int bapi_ui_component_is_visible(bapi_ui_component_t component)
{
	return component ? component->visible : 0;
}
void bapi_ui_component_set_visible(bapi_ui_component_t component, int visible)
{
	if (component) component->visible = visible != 0;
}
int bapi_ui_component_is_enabled(bapi_ui_component_t component)
{
	return component ? component->enabled : 0;
}
void bapi_ui_component_set_enabled(bapi_ui_component_t component, int enabled)
{
	if (component) component->enabled = enabled != 0;
}
const char *bapi_ui_component_get_text(bapi_ui_component_t component)
{
	return component ? component->text : NULL;
}

int bapi_ui_component_set_text(bapi_ui_component_t component, const char *text)
{
	if (!component || !text) return -1;
	char *copy = ui_strdup(text);
	if (!copy) return -1;
	free(component->text);
	component->text = copy;
	return 0;
}

float bapi_ui_component_get_value(bapi_ui_component_t component)
{
	return component ? component->value : 0;
}

int bapi_ui_component_set_value(bapi_ui_component_t component, float value)
{
	if (!component) return -1;
	if (value < component->min_value) value = component->min_value;
	if (value > component->max_value) value = component->max_value;
	component->value = value;
	return 0;
}

int bapi_ui_component_is_checked(bapi_ui_component_t component)
{
	return component ? component->checked : 0;
}
int bapi_ui_component_is_focused(bapi_ui_component_t component)
{
	return component ? component->focused : 0;
}
int bapi_ui_component_get_selected_index(bapi_ui_component_t component)
{
	return component ? component->selected_index : -1;
}

int bapi_ui_component_set_selected_index(bapi_ui_component_t component, int index)
{
	if (!component || index < 0 || (component->item_count > 0 && index >= component->item_count))
		return -1;
	component->selected_index = index;
	return 0;
}

float bapi_ui_component_get_scroll_offset(bapi_ui_component_t component)
{
	return component ? component->scroll_offset : 0;
}

int bapi_ui_component_set_scroll_offset(bapi_ui_component_t component, float offset)
{
	if (!component || component->type != BAPI_UI_COMPONENT_SCROLL || offset < 0) return -1;
	component->scroll_offset = offset;
	return 0;
}
