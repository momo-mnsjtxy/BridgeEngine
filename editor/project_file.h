#pragma once

#include <string>

struct EditorState;

bool EditorOpenProject(EditorState &state, const char *path, std::string &error_out);
void EditorPushRecentProject(EditorState &state, const char *path);
void EditorLoadRecentProjects(EditorState &state);
void EditorSaveRecentProjects(EditorState &state);
