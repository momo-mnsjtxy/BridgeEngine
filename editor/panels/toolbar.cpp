#include "../editor.h"
#include "../i18n.h"
#include "../platform_dialogs.h"

#include <imgui_internal.h>

static const char *kUiFileFilter = "UI Documents (*.xml)\0*.xml\0All Files (*.*)\0*.*\0";

void EditorToolbarPanel(EditorState &state)
{
	if (ImGui::BeginViewportSideBar("##Toolbar", ImGui::GetMainViewport(),
									ImGuiDir_Up, 36.0f,
									ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoResize)) {
		if (ImGui::Button(L("New"))) {
			EditorNewDocument(state);
			state.show_welcome = false;
		}
		ImGui::SameLine();
		if (ImGui::Button(L("Open"))) {
			std::string path = EditorOpenFileDialog(kUiFileFilter, "assets/ui/demo.xml");
			if (!path.empty() && !EditorLoadFile(state, path.c_str())) EditorNewDocument(state);
		}
		ImGui::SameLine();
		if (ImGui::Button(L("Save"))) {
			if (state.filepath.empty()) {
				std::string path = EditorSaveFileDialog(kUiFileFilter, "editor_out.xml");
				if (!path.empty()) EditorSaveFile(state, path.c_str());
			} else {
				EditorSaveFile(state, state.filepath.c_str());
			}
		}
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
		ImGui::SameLine();
		if (ImGui::Button(L("Undo"))) EditorUndo(state);
		ImGui::SameLine();
		if (ImGui::Button(L("Redo"))) EditorRedo(state);
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
		ImGui::SameLine();
		if (ImGui::Button(L("Duplicate"))) EditorDuplicateSelection(state);
		ImGui::SameLine();
		if (ImGui::Button(L("Delete Selection"))) EditorRemoveComponents(state, state.selection_list);
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
		ImGui::SameLine();
		if (ImGui::Checkbox(L("Grid"), &state.show_grid)) {
		}
		ImGui::SameLine();
		if (ImGui::Checkbox(L("Snap"), &state.snap_to_grid)) {
		}
		ImGui::SameLine();
		ImGui::Text(L("Scale: %.2f"), state.view_scale);
		ImGui::End();
	}
}
