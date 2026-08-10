#include "project_file.h"

#include "editor.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <vector>

namespace {

static const char *kRecentProjectFile = "recent_projects.txt";

static void trim(std::string &value)
{
	const char *spaces = " \t";
	size_t first = value.find_first_not_of(spaces);
	if (first == std::string::npos) {
		value.clear();
		return;
	}
	size_t last = value.find_last_not_of(spaces);
	value = value.substr(first, last - first + 1);
}

static std::string parent_dir(const std::string &path)
{
	size_t slash = path.find_last_of("\\/");
	return slash == std::string::npos ? "." : path.substr(0, slash);
}

static std::string join_path(const std::string &base, const std::string &relative)
{
	if (relative.empty()) return base;
	if (base.empty()) return relative;
	char last = base.back();
	if (last == '\\' || last == '/') return base + relative;
	return base + "\\" + relative;
}

struct ProjectConfig {
	std::string name;
	std::string engine;
	std::vector<std::string> documents;
};

static bool parse_project(const char *path, ProjectConfig &config, std::string &error)
{
	FILE *file = std::fopen(path, "rb");
	if (!file) {
		error = std::string("Cannot open project file: ") + path;
		return false;
	}

	char line[1024];
	bool in_documents = false;
	while (std::fgets(line, sizeof(line), file)) {
		std::string value(line);
		while (!value.empty() && (value.back() == '\n' || value.back() == '\r')) value.pop_back();
		trim(value);
		if (value.empty() || value[0] == '#' || value[0] == ';') continue;
		if (value.front() == '[' && value.back() == ']') {
			in_documents = value == "[documents]";
			continue;
		}
		if (in_documents) {
			config.documents.push_back(value);
			continue;
		}

		size_t equals = value.find('=');
		if (equals == std::string::npos) continue;
		std::string key = value.substr(0, equals);
		std::string data = value.substr(equals + 1);
		trim(key);
		trim(data);
		if (key == "name") config.name = data;
		else if (key == "engine") config.engine = data;
	}
	std::fclose(file);

	if (config.documents.empty()) {
		error = "Project file contains no documents.";
		return false;
	}
	return true;
}

} // namespace

void EditorPushRecentProject(EditorState &state, const char *path)
{
	if (!path || !path[0]) return;
	std::string value(path);
	auto it = std::find(state.recent_projects.begin(), state.recent_projects.end(), value);
	if (it != state.recent_projects.end()) state.recent_projects.erase(it);
	state.recent_projects.insert(state.recent_projects.begin(), value);
	if (state.recent_projects.size() > 8) state.recent_projects.resize(8);
	EditorSaveRecentProjects(state);
}

void EditorLoadRecentProjects(EditorState &state)
{
	FILE *file = std::fopen(kRecentProjectFile, "rb");
	if (!file) return;
	char line[1024];
	while (std::fgets(line, sizeof(line), file)) {
		std::string value(line);
		while (!value.empty() && (value.back() == '\n' || value.back() == '\r')) value.pop_back();
		trim(value);
		if (!value.empty()) state.recent_projects.push_back(value);
	}
	std::fclose(file);
	if (state.recent_projects.size() > 8) state.recent_projects.resize(8);
}

void EditorSaveRecentProjects(EditorState &state)
{
	FILE *file = std::fopen(kRecentProjectFile, "wb");
	if (!file) return;
	for (const std::string &path : state.recent_projects)
		std::fprintf(file, "%s\n", path.c_str());
	std::fclose(file);
}

bool EditorOpenProject(EditorState &state, const char *path, std::string &error_out)
{
	if (!path || !path[0]) {
		error_out = "Project path is empty.";
		return false;
	}

	ProjectConfig config;
	if (!parse_project(path, config, error_out)) return false;
	for (int i = 0; i < (int)state.documents.size(); i++) {
		if (EditorDocumentIsDirty(state, i)) {
			error_out = "Save or discard changes in all open documents before opening a project.";
			return false;
		}
	}

	std::string root = parent_dir(path);
	// EditorCloseDocument deliberately creates an untitled replacement after the
	// final tab closes. Keep one document, replace it with the first project XML,
	// then discard that now-parked untitled document.
	while (state.documents.size() > 1) EditorCloseDocument(state, 0);

	bool loaded_any = false;
	for (const std::string &relative : config.documents) {
		std::string document_path = join_path(root, relative);
		if (EditorLoadFile(state, document_path.c_str())) {
			if (!loaded_any && state.documents.size() > 1) EditorCloseDocument(state, 0);
			loaded_any = true;
		} else if (error_out.empty()) {
			error_out = "Failed to open project document: " + document_path;
		}
	}

	if (!loaded_any) {
		error_out = error_out.empty() ? "No project documents could be opened." : error_out;
		return false;
	}

	state.project_path = path;
	state.show_welcome = false;
	EditorPushRecentProject(state, path);
	return true;
}
