#include "../editor.h"
#include "../i18n.h"

// Undo history panel: shows the undo and redo stacks as a list of operations.
// Clicking an entry undoes/redoes up to that point in one go.

void EditorUndoPanel(EditorState &state)
{
	ImGui::Begin(L("History"));

	// Jumping is implemented by repeatedly calling Undo/Redo. Because each call
	// marks the doc dirty, jumping to an older state is still "one user action"
	// from the user's perspective.
	auto jump_to = [&](int target_depth) {
		while ((int)state.undo_stack.size() > target_depth) EditorUndo(state);
		while ((int)state.undo_stack.size() < target_depth) EditorRedo(state);
	};

	int undo_count = (int)state.undo_stack.size();
	int redo_count = (int)state.redo_stack.size();

	if (undo_count == 0 && redo_count == 0) {
		ImGui::TextDisabled("%s", L("No history yet."));
		ImGui::End();
		return;
	}

	if (ImGui::BeginChild("undo_list")) {
		// redo stack, top of stack = most recent redo, shown first
		for (int i = redo_count - 1; i >= 0; i--) {
			std::string label = std::string(">> ") + state.redo_stack[i]->Name();
			if (i == redo_count - 1) label += " [redo]";
			ImGui::PushID(1000 + i);
			if (ImGui::Selectable(label.c_str())) {
				int target = undo_count + (redo_count - i);
				jump_to(target);
			}
			ImGui::PopID();
		}

		// current state marker
		ImGui::Separator();

		// undo stack, bottom of stack = oldest, shown last; index 0 is current
		for (int i = undo_count - 1; i >= 0; i--) {
			std::string label = std::string(state.undo_stack[i]->Name());
			if (i == undo_count - 1) label += " (undo)";
			ImGui::PushID(2000 + i);
			if (ImGui::Selectable(label.c_str())) {
				jump_to(i); // undo back to depth i
			}
			ImGui::PopID();
		}
	}
	ImGui::EndChild();

	ImGui::End();
}
