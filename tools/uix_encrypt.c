// Build-time UI encryptor.
//
// Usage: uix_encrypt <key> <input_dir> <output_dir> [<input_dir2> <output_dir2> ...]
//
// Recursively encrypts every *.xml under each input directory into a matching
// file in the output directory. The extension is kept: only the content is
// encrypted (BUIX magic), so .bep paths and the runtime loader work unchanged.
// Non-XML files are copied as-is. Returns 0 on success.

#include "BridgeEngine.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#include <io.h>
#include <windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

// Create a directory (and any missing parents). Returns 0 on success.
static int make_dir(const char *path)
{
	char tmp[4096];
	snprintf(tmp, sizeof(tmp), "%s", path);
	size_t len = strlen(tmp);
	// first pass: ensure the path ends cleanly, then walk separators
	for (size_t i = 0; i < len; i++) {
		if (tmp[i] == '\\') tmp[i] = '/';
	}
	if (len > 0 && tmp[len - 1] == '/') tmp[len - 1] = '\0';
	for (char *p = tmp + 1; *p; p++) {
		if (*p == '/') {
			*p = '\0';
#ifdef _WIN32
			CreateDirectoryA(tmp, NULL);
#else
			mkdir(tmp, 0755);
#endif
			*p = '/';
		}
	}
#ifdef _WIN32
	CreateDirectoryA(tmp, NULL);
	return GetLastError() == ERROR_ALREADY_EXISTS ? 0 : 0;
#else
	mkdir(tmp, 0755);
	return 0;
#endif
}

// Recursively walk `in_dir`, writing encrypted .uix / copied files into
// `out_dir`. Returns number of failures.
static int process_dir(const char *key, const char *in_dir, const char *out_dir)
{
	if (make_dir(out_dir) != 0) {
		fprintf(stderr, "uix_encrypt: cannot create %s\n", out_dir);
		return 1;
	}

	int failures = 0;
#ifdef _WIN32
	char pattern[MAX_PATH];
	snprintf(pattern, sizeof(pattern), "%s\\*", in_dir);
	struct _finddata_t fd;
	intptr_t		   handle = _findfirst(pattern, &fd);
	if (handle == -1) return 0;
	do {
		if (strcmp(fd.name, ".") == 0 || strcmp(fd.name, "..") == 0) continue;
		char in_path[MAX_PATH], out_path[MAX_PATH];
		snprintf(in_path, sizeof(in_path), "%s\\%s", in_dir, fd.name);
		snprintf(out_path, sizeof(out_path), "%s\\%s", out_dir, fd.name);
		if (fd.attrib & _A_SUBDIR) {
			failures += process_dir(key, in_path, out_path);
			continue;
		}
		// Encrypt *.xml in place (content encrypted, extension kept) so that
		// .bep document paths and the runtime loader stay unchanged; the loader
		// detects the BUIX magic by content. Copy everything else as-is.
		size_t len = strlen(fd.name);
		int	   is_xml = len > 4 && (strcmp(fd.name + len - 4, ".xml") == 0 ||
									strcmp(fd.name + len - 4, ".XML") == 0);
		if (is_xml) {
			if (bapi_uix_encrypt_file(in_path, out_path, key) != 0) {
				fprintf(stderr, "uix_encrypt: failed to encrypt %s\n", in_path);
				failures++;
			}
		} else if (!CopyFileA(in_path, out_path, FALSE)) {
			fprintf(stderr, "uix_encrypt: failed to copy %s\n", in_path);
			failures++;
		}
	} while (_findnext(handle, &fd) == 0);
	_findclose(handle);
#else
	DIR *dir = opendir(in_dir);
	if (!dir) return 1;
	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL) {
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
		char in_path[1024], out_path[1024];
		snprintf(in_path, sizeof(in_path), "%s/%s", in_dir, entry->d_name);
		snprintf(out_path, sizeof(out_path), "%s/%s", out_dir, entry->d_name);
		struct stat st;
		if (stat(in_path, &st) == 0 && S_ISDIR(st.st_mode)) {
			failures += process_dir(key, in_path, out_path);
			continue;
		}
		size_t len = strlen(entry->d_name);
		int	   is_xml = len > 4 && (strcmp(entry->d_name + len - 4, ".xml") == 0 ||
									strcmp(entry->d_name + len - 4, ".XML") == 0);
		if (is_xml) {
			if (bapi_uix_encrypt_file(in_path, out_path, key) != 0) {
				fprintf(stderr, "uix_encrypt: failed to encrypt %s\n", in_path);
				failures++;
			}
		} else {
			FILE *src = fopen(in_path, "rb");
			FILE *dst = fopen(out_path, "wb");
			if (src && dst) {
				char buf[8192];
				size_t n;
				while ((n = fread(buf, 1, sizeof(buf), src)) > 0)
					fwrite(buf, 1, n, dst);
			} else {
				fprintf(stderr, "uix_encrypt: failed to copy %s\n", in_path);
				failures++;
			}
			if (src) fclose(src);
			if (dst) fclose(dst);
		}
	}
	closedir(dir);
#endif
	return failures;
}

int main(int argc, char **argv)
{
	// Fixed prefix: program, key, then pairs of (in_dir, out_dir).
	if (argc < 4 || (argc - 4) % 2 != 0) {
		fprintf(stderr, "usage: uix_encrypt <key> <in_dir> <out_dir> [<in_dir2> <out_dir2> ...]\n");
		return 1;
	}
	const char *key = argv[1];
	int			total = 0;
	for (int i = 2; i + 1 < argc; i += 2) total += process_dir(key, argv[i], argv[i + 1]);
	return total > 0 ? 1 : 0;
}
