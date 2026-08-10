#include "platform_dialogs.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <commdlg.h>
#include <shlobj.h>
#include <cstdio>
#include <cstring>

static std::string run_dialog(bool is_open, const char *filter, const char *default_value)
{
	OPENFILENAMEA ofn;
	std::memset(&ofn, 0, sizeof(ofn));

	char buffer[MAX_PATH] = {};
	if (default_value) {
		std::snprintf(buffer, sizeof(buffer), "%s", default_value);
	}

	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = NULL;
	ofn.lpstrFilter = filter;
	ofn.lpstrFile = buffer;
	ofn.nMaxFile = sizeof(buffer);
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;
	if (!is_open) ofn.Flags |= OFN_OVERWRITEPROMPT;
	ofn.lpstrDefExt = "xml";
	ofn.lpstrTitle = is_open ? "Open UI Document" : "Save UI Document";

	if ((is_open ? GetOpenFileNameA(&ofn) : GetSaveFileNameA(&ofn)) == 0) return "";
	return std::string(buffer);
}

std::string EditorBrowseForFolder(const char *title)
{
	BROWSEINFOA bi;
	std::memset(&bi, 0, sizeof(bi));
	bi.lpszTitle = title;
	bi.ulFlags	  = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

	LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);
	if (!pidl) return "";
	char path[MAX_PATH] = {};
	if (!SHGetPathFromIDListA(pidl, path)) {
		CoTaskMemFree(pidl);
		return "";
	}
	CoTaskMemFree(pidl);
	return std::string(path);
}

#else

static std::string run_dialog(bool is_open, const char *filter, const char *default_value)
{
	(void)is_open;
	(void)filter;
	(void)default_value;
	return "";
}

std::string EditorBrowseForFolder(const char *title)
{
	(void)title;
	return "";
}

#endif

std::string EditorOpenFileDialog(const char *filter, const char *default_dir)
{
	return run_dialog(true, filter, default_dir);
}

std::string EditorSaveFileDialog(const char *filter, const char *default_name)
{
	return run_dialog(false, filter, default_name);
}
