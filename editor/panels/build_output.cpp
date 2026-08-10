#include "../build_run.h"
#include "../editor.h"
#include "../i18n.h"

void EditorBuildOutputPanel(EditorState &state)
{
	if (!ImGui::Begin(L("Build Output"))) {
		ImGui::End();
		return;
	}

	if (state.build_running) {
		ImGui::TextColored(ImVec4(0.9f, 0.8f, 0.2f, 1.0f), "%s", L("build.running"));
	} else if (state.build_succeeded) {
		ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.4f, 1.0f), "%s", L("build.success"));
	} else if (!state.build_log.empty()) {
		ImGui::TextColored(ImVec4(0.9f, 0.3f, 0.3f, 1.0f), "%s", L("build.failed"));
	}
	ImGui::SameLine();
	if (ImGui::Button(L("Clear"), ImVec2(0, 0))) {
		std::lock_guard<std::mutex> lock(state.build_log_mutex);
		state.build_log.clear();
	}

	ImGui::Separator();

	std::string text;
	{
		std::lock_guard<std::mutex> lock(state.build_log_mutex);
		text = state.build_log;
	}
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 2));
	ImGui::BeginChild("build_log_scroll", ImVec2(0, 0), false,
					  ImGuiWindowFlags_HorizontalScrollbar);
	ImGui::TextUnformatted(text.empty() ? " " : text.c_str());
	if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) ImGui::SetScrollHereY(1.0f);
	ImGui::EndChild();
	ImGui::PopStyleVar();

	ImGui::End();
}
