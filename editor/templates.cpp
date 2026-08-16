#include "editor.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#ifdef _WIN32
#include <io.h>
#endif

// Template / preset library: save the selected components (roots of the
// selection) to a UI-XML file, and load templates back by cloning their roots
// into the current document. Template files live in "<project>/templates/".

static const char *kTemplateDir = "templates";

std::string EditorTemplateDir(const EditorState &state)
{
	std::string root;
	std::string path = state.project_path;
	if (!path.empty()) {
		size_t slash = path.find_last_of("/\\");
		root = slash == std::string::npos ? std::string() : path.substr(0, slash);
	}
	if (root.empty()) root = ".";
	return root + "/" + kTemplateDir;
}

bool EditorSaveTemplate(EditorState &state, const char *filepath)
{
	if (!state.ui || !filepath || !filepath[0]) return false;

	// collect the top-level components of the current selection
	std::vector<bapi_ui_component_t> sources;
	for (bapi_ui_component_t comp : state.selection_list) {
		if (!comp) continue;
		// skip components whose parent is also selected (keep topmost only)
		bool has_parent = false;
		for (bapi_ui_component_t p = bapi_ui_component_get_parent(comp); p;
			 p = bapi_ui_component_get_parent(p)) {
			if (std::find(state.selection_list.begin(), state.selection_list.end(), p) !=
				state.selection_list.end()) {
				has_parent = true;
				break;
			}
		}
		if (!has_parent) sources.push_back(comp);
	}
	if (sources.empty()) return false;

	bapi_ui_t tmp = bapi_ui_create();
	if (!tmp) return false;
	for (bapi_ui_component_t comp : sources) {
		bapi_ui_component_t clone = bapi_ui_component_clone(comp);
		if (clone) bapi_ui_add_root(tmp, clone);
	}
	int rc = bapi_ui_save_to_xml(tmp, filepath);
	bapi_ui_destroy(tmp);
	return rc == 0;
}

bool EditorLoadTemplate(EditorState &state, const char *filepath)
{
	if (!state.ui || !filepath || !filepath[0]) return false;
	bapi_ui_t tmp = bapi_ui_load_from_xml(filepath);
	if (!tmp) return false;

	// Clone every root of the template into the current document.
	bapi_ui_component_t parent = nullptr;
	if (state.selection && EditorIsContainerType(bapi_ui_component_get_type(state.selection)))
		parent = state.selection;

	std::vector<bapi_ui_component_t> created;
	int root_count = bapi_ui_get_root_count(tmp);
	for (int i = 0; i < root_count; i++) {
		bapi_ui_component_t root = bapi_ui_get_root(tmp, i);
		if (!root) continue;
		const char *base = bapi_ui_component_get_id(root);
		char new_id[128];
		int	 suffix = 1;
		for (;;) {
			snprintf(new_id, sizeof(new_id), "%s_tpl%d", base ? base : "tpl", suffix);
			if (bapi_ui_find(state.ui, new_id) == nullptr) break;
			suffix++;
		}
		// EditorInsertClone clones the (off-tree) template root into the doc.
		bapi_ui_component_t clone = EditorInsertClone(state, root, parent, new_id);
		if (clone) created.push_back(clone);
	}
	bapi_ui_destroy(tmp);
	if (!created.empty()) {
		state.selection_list = created;
		state.selection		 = created.back();
	}
	return !created.empty();
}

std::vector<std::string> EditorListTemplates(const EditorState &state)
{
	std::vector<std::string> result;
	std::string				 dir = EditorTemplateDir(state);
	std::string				 pattern = dir + "\\*.xml";
	// Use _findfirst on Windows; non-Windows returns empty (editor is desktop).
#ifdef _WIN32
	struct _finddata_t fd;
	intptr_t		   handle = _findfirst(pattern.c_str(), &fd);
	if (handle == -1) return result;
	do {
		if (!(fd.attrib & _A_SUBDIR)) result.push_back(std::string(dir) + "\\" + fd.name);
	} while (_findnext(handle, &fd) == 0);
	_findclose(handle);
#endif
	std::sort(result.begin(), result.end());
	return result;
}
