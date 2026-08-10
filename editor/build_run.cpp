#include "build_run.h"

#include "editor.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include <cstdio>
#include <cstring>
#include <string>

static void append_log(EditorState *state, const std::string &text)
{
	std::lock_guard<std::mutex> lock(state->build_log_mutex);
	state->build_log += text;
}

static int run_command(EditorState *state, const char *command)
{
	append_log(state, "> ");
	append_log(state, command);
	append_log(state, "\n");

	FILE *pipe = _popen(command, "r");
	if (!pipe) {
		append_log(state, "failed to start process\n");
		return -1;
	}
	char buffer[1024];
	while (std::fgets(buffer, sizeof(buffer), pipe)) append_log(state, buffer);
	int exit_code = _pclose(pipe);

	char tail[64];
	std::snprintf(tail, sizeof(tail), "\n[exit code %d]\n", exit_code);
	append_log(state, tail);
	return exit_code;
}

static std::string project_root(const EditorState &state)
{
	if (state.project_path.empty()) return "";
	size_t slash = state.project_path.find_last_of("\\/");
	return slash == std::string::npos ? state.project_path : state.project_path.substr(0, slash);
}

static std::string find_exe_recursive(const std::string &dir, const std::string &exe_name)
{
	WIN32_FIND_DATAA fd;
	std::string pattern = dir + "\\*";
	HANDLE handle		= FindFirstFileA(pattern.c_str(), &fd);
	if (handle == INVALID_HANDLE_VALUE) return "";
	do {
		std::string path = dir + "\\" + fd.cFileName;
		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			if (fd.cFileName[0] == '.') continue;
			std::string sub = find_exe_recursive(path, exe_name);
			if (!sub.empty()) {
				FindClose(handle);
				return sub;
			}
		} else if (_stricmp(fd.cFileName, exe_name.c_str()) == 0) {
			FindClose(handle);
			return path;
		}
	} while (FindNextFileA(handle, &fd) != 0);
	FindClose(handle);
	return "";
}

static void build_worker(EditorState *state, std::string root)
{
	append_log(state, "=== configure ===\n");
	std::string cache_path = root + "\\build\\CMakeCache.txt";
	if (GetFileAttributesA(cache_path.c_str()) == INVALID_FILE_ATTRIBUTES) {
		std::string command = "cmake -S \"" + root + "\" -B \"" + root + "\\build\"";
		const char *vcpkg_root = std::getenv("VCPKG_ROOT");
		if (vcpkg_root && vcpkg_root[0]) {
			command += " -DCMAKE_TOOLCHAIN_FILE=";
			command += vcpkg_root;
			command += "/scripts/buildsystems/vcpkg.cmake";
			command += " -DVCPKG_TARGET_TRIPLET=x64-windows";
		}
		command += " 2>&1";
		run_command(state, command.c_str());
	}

	append_log(state, "=== build ===\n");
	std::string command = "cmake --build \"" + root + "\\build\" --config Debug 2>&1";
	int build_exit		 = run_command(state, command.c_str());
	state->build_succeeded = build_exit == 0;
	state->build_running   = false;
}

void EditorBuildProject(EditorState &state)
{
	if (state.build_running) return;
	std::string root = project_root(state);
	if (root.empty()) return;

	state.build_succeeded = false;
	state.build_running	  = true;
	{
		std::lock_guard<std::mutex> lock(state.build_log_mutex);
		state.build_log.clear();
	}
	state.build_thread = std::make_unique<std::thread>(build_worker, &state, root);
}

void EditorUpdateBuildThread(EditorState &state)
{
	if (state.build_thread && state.build_thread->joinable() && !state.build_running) {
		state.build_thread->join();
		state.build_thread.reset();
	}
}

bool EditorCanBuild(const EditorState &state)
{
	return !state.project_path.empty() && !state.build_running;
}

bool EditorCanRun(const EditorState &state)
{
	return !state.project_path.empty() && !state.build_running && state.build_succeeded;
}

void EditorRunProject(EditorState &state)
{
	std::string root = project_root(state);
	if (root.empty()) return;

	// Executable name = project name (cmake target / exe name).
	std::string base = state.project_path;
	size_t slash	 = base.find_last_of("\\/");
	base = slash == std::string::npos ? base : base.substr(slash + 1);
	if (base.size() > 4 && base.compare(base.size() - 4, 4, ".bep") == 0) base.resize(base.size() - 4);

	std::string exe = find_exe_recursive(root + "\\build", base + ".exe");
	if (exe.empty()) {
		append_log(&state, "run: executable not found in the build directory\n");
		return;
	}
	state.build_exe_path = exe;

	// main.c loads assets relative to the working directory, so run from the
	// executable's folder.
	std::string exe_dir = exe;
	size_t exe_slash	= exe_dir.find_last_of("\\/");
	exe_dir = exe_slash == std::string::npos ? "." : exe_dir.substr(0, exe_slash);

	STARTUPINFOA si;
	PROCESS_INFORMATION pi;
	std::memset(&si, 0, sizeof(si));
	std::memset(&pi, 0, sizeof(pi));
	si.cb = sizeof(si);
	if (CreateProcessA(NULL, const_cast<char *>(exe.c_str()), NULL, NULL, FALSE, 0, NULL,
					   exe_dir.c_str(), &si, &pi)) {
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
		append_log(&state, "run: started " + exe + "\n");
	} else {
		append_log(&state, "run: failed to start " + exe + "\n");
	}
}
