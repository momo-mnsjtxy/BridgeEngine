#include "../editor.h"
#include "../i18n.h"

#include <cstdio>
#include <cstring>

static bool is_selected(const EditorState &state, bapi_ui_component_t component)
{
	for (bapi_ui_component_t comp : state.selection_list)
		if (comp == component) return true;
	return false;
}

static void handle_drop_target(EditorState &state, bapi_ui_component_t component)
{
	if (!ImGui::BeginDragDropTarget()) return;
	if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("BAPI_UI_COMPONENT")) {
		bapi_ui_component_t dragged = nullptr;
		std::memcpy(&dragged, payload->Data, sizeof(dragged));
		if (dragged) EditorReparentComponent(state, dragged, component);
	}
	ImGui::EndDragDropTarget();
}

static void tree_node(EditorState &state, bapi_ui_component_t component, int depth)
{
	if (!component) return;
	const char *id = EditorComponentId(component);
	const char *type = EditorComponentTypeName(bapi_ui_component_get_type(component));
	int			child_count = bapi_ui_component_get_child_count(component);

	char label[160];
	if (child_count > 0)
		snprintf(label, sizeof(label), "%s (%s) [%d]", id ? id : "?", type, child_count);
	else
		snprintf(label, sizeof(label), "%s (%s)", id ? id : "?", type);

	ImGui::PushID(component);
	bool selected = is_selected(state, component);

	if (child_count > 0) {
		bool open = ImGui::TreeNodeEx(label,
									  ImGuiTreeNodeFlags_OpenOnArrow |
										  ImGuiTreeNodeFlags_OpenOnDoubleClick |
										  (selected ? ImGuiTreeNodeFlags_Selected : 0));
		if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
			if (ImGui::GetIO().KeyCtrl)
				EditorToggleSelectComponent(state, component);
			else
				EditorSelectComponent(state, component);
		}
		// drag source: reparent this node
		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
			ImGui::SetDragDropPayload("BAPI_UI_COMPONENT", &component, sizeof(component));
			ImGui::Text(L("Reparent %s"), id ? id : "?");
			ImGui::EndDragDropSource();
		}
		handle_drop_target(state, component);
		if (open) {
			for (int i = 0; i < child_count; i++) {
				bapi_ui_component_t child = bapi_ui_component_get_child(component, i);
				tree_node(state, child, depth + 1);
			}
			ImGui::TreePop();
		}
	} else {
		if (ImGui::Selectable(label, selected)) {
			if (ImGui::GetIO().KeyCtrl)
				EditorToggleSelectComponent(state, component);
			else
				EditorSelectComponent(state, component);
		}
		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
			ImGui::SetDragDropPayload("BAPI_UI_COMPONENT", &component, sizeof(component));
			ImGui::Text(L("Reparent %s"), id ? id : "?");
			ImGui::EndDragDropSource();
		}
		handle_drop_target(state, component);
	}
	ImGui::PopID();
}

void EditorTreePanel(EditorState &state)
{
	ImGui::Begin(L("Hierarchy"));
	if (ImGui::BeginChild("tree")) {
		int root_count = state.ui ? bapi_ui_get_root_count(state.ui) : 0;
		if (root_count == 0) {
			ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Empty");
		} else {
			for (int i = 0; i < root_count; i++) {
				bapi_ui_component_t root = bapi_ui_get_root(state.ui, i);
				tree_node(state, root, 0);
			}
		}
		// drop onto empty area reparents to the UI root list
		if (ImGui::BeginDragDropTarget()) {
			if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("BAPI_UI_COMPONENT")) {
				bapi_ui_component_t dragged = nullptr;
				std::memcpy(&dragged, payload->Data, sizeof(dragged));
				if (dragged) EditorReparentComponent(state, dragged, nullptr);
			}
			ImGui::EndDragDropTarget();
		}
	}
	ImGui::EndChild();
	ImGui::End();
}
