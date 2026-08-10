#include "project_templates.h"

#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <cctype>
#include <cstdio>
#include <cstring>
#include <set>
#include <string>
#include <vector>

// Compiled in by CMake: absolute path to the engine source root.
#ifndef BRIDGEENGINE_SOURCE_DIR
#define BRIDGEENGINE_SOURCE_DIR ""
#endif

namespace {

std::string replace_all(std::string s, const std::string &from, const std::string &to)
{
	if (from.empty()) return s;
	size_t pos = 0;
	while ((pos = s.find(from, pos)) != std::string::npos) {
		s.replace(pos, from.size(), to);
		pos += to.size();
	}
	return s;
}

std::string forward_slashes(std::string s)
{
	for (char &c : s)
		if (c == '\\') c = '/';
	return s;
}

std::string template_dir()
{
	std::string base = BRIDGEENGINE_SOURCE_DIR;
	for (char &c : base)
		if (c == '/') c = '\\';
	if (!base.empty() && base.back() != '\\') base += '\\';
	return base + "templates\\project";
}

bool valid_project_name(const std::string &name)
{
	if (name.empty()) return false;
	if (std::isdigit((unsigned char)name[0])) return false;
	for (char c : name) {
		if (!(std::isalnum((unsigned char)c) || c == '_')) return false;
	}
	return true;
}

bool make_dirs(const std::string &dir)
{
	std::vector<std::string> parts;
	std::string cur;
	for (char c : dir) {
		if (c == '\\' || c == '/') {
			parts.push_back(cur);
			cur.clear();
		} else {
			cur += c;
		}
	}
	if (!cur.empty()) parts.push_back(cur);

	std::string path;
	size_t first = 0;
	if (dir.size() >= 2 && dir[1] == ':') {
		path = dir.substr(0, 2); // drive prefix "C:"
		first = 1; // parts[0] is the drive, already consumed
	}
	for (size_t i = first; i < parts.size(); i++) {
		const std::string &part = parts[i];
		if (part.empty()) continue;
		if (!path.empty() && path.back() != '\\') path += '\\';
		path += part;
		if (GetFileAttributesA(path.c_str()) == INVALID_FILE_ATTRIBUTES) {
			if (!CreateDirectoryA(path.c_str(), NULL)) return false;
		}
	}
	return true;
}

bool read_file(const std::string &path, std::string &out)
{
	FILE *f = std::fopen(path.c_str(), "rb");
	if (!f) return false;
	std::fseek(f, 0, SEEK_END);
	long size = std::ftell(f);
	std::fseek(f, 0, SEEK_SET);
	out.resize(size > 0 ? (size_t)size : 0);
	if (size > 0 && std::fread(&out[0], 1, (size_t)size, f) != (size_t)size) {
		std::fclose(f);
		return false;
	}
	std::fclose(f);
	return true;
}

bool write_file(const std::string &path, const std::string &data)
{
	FILE *f = std::fopen(path.c_str(), "wb");
	if (!f) return false;
	bool ok = std::fwrite(data.data(), 1, data.size(), f) == data.size();
	std::fclose(f);
	return ok;
}

// Files whose contents get placeholder substitution after copying. Binary
// assets (e.g. the font) must never be touched.
const std::set<std::string> &text_files()
{
	static const std::set<std::string> files = {
		"CMakeLists.txt",
		"main.c",
		"project.bep",
		"ui\\menu.xml",
		"ui\\game.xml",
		"ui\\settings.xml",
	};
	return files;
}

struct Placeholders {
	std::string project_name;
	std::string engine_dir; // forward-slashed; empty means find_package
};

bool copy_tree(const std::string &from_dir, const std::string &to_dir, const std::string &rel_prefix,
			   const Placeholders &ph)
{
	if (!make_dirs(to_dir)) return false;

	WIN32_FIND_DATAA fd;
	std::string pattern = from_dir + "\\*";
	HANDLE h		   = FindFirstFileA(pattern.c_str(), &fd);
	if (h == INVALID_HANDLE_VALUE) return false;
	do {
		if (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0) continue;

		std::string from = from_dir + "\\" + fd.cFileName;
		std::string to	 = to_dir + "\\" + fd.cFileName;
		if (strcmp(fd.cFileName, "project.bep") == 0)
			to = to_dir + "\\" + ph.project_name + ".bep";
		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			if (!copy_tree(from, to, rel_prefix + fd.cFileName + "\\", ph)) {
				FindClose(h);
				return false;
			}
			continue;
		}

		if (!CopyFileA(from.c_str(), to.c_str(), FALSE)) {
			FindClose(h);
			return false;
		}

		// Substitute placeholders in text files, identified by their path
		// relative to the template root.
		std::string rel = rel_prefix + fd.cFileName;
		if (text_files().count(rel) == 0) continue;

		std::string content;
		if (!read_file(to, content)) {
			FindClose(h);
			return false;
		}
		content = replace_all(content, "__PROJECT_NAME__", ph.project_name);
		content = replace_all(content, "__EXEC_NAME__", ph.project_name);
		content = replace_all(content, "__ENGINE_DIR__", ph.engine_dir);
		if (!write_file(to, content)) {
			FindClose(h);
			return false;
		}
	} while (FindNextFileA(h, &fd) != 0);
	FindClose(h);
	return true;
}

} // namespace

bool EditorCreateProject(const std::string &project_name, const std::string &parent_dir,
						 const std::string &engine_dir, std::string &error_out,
						 std::string *out_bep_path)
{
	if (!valid_project_name(project_name)) {
		error_out =
			"Project name must be a C identifier (letters, digits, underscore, not starting with a digit).";
		return false;
	}
	if (parent_dir.empty()) {
		error_out = "Choose a parent directory for the project.";
		return false;
	}

	std::string root = parent_dir;
	if (root.back() != '\\' && root.back() != '/') root += '\\';
	root += project_name;

	if (GetFileAttributesA(root.c_str()) != INVALID_FILE_ATTRIBUTES) {
		error_out = "Directory already exists: " + root;
		return false;
	}

	std::string tpl = template_dir();
	if (GetFileAttributesA(tpl.c_str()) == INVALID_FILE_ATTRIBUTES) {
		error_out = "Project template not found: " + tpl;
		return false;
	}

	Placeholders ph;
	ph.project_name = project_name;
	ph.engine_dir	= engine_dir.empty() ? "" : forward_slashes(engine_dir);

	if (!copy_tree(tpl, root, "", ph)) {
		error_out = "Failed to copy the project template.";
		return false;
	}
	if (out_bep_path) *out_bep_path = root + "\\" + project_name + ".bep";
	return true;
}
