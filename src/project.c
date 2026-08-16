#include "BridgeEngine.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct bapi_project_internal {
	char  *name;
	char  *engine;
	char **documents;
	int	   document_count;
};

// Strip leading/trailing spaces and tabs from value.
static char *trim(char *value)
{
	char *start = value;
	char *end;

	while (*start == ' ' || *start == '\t') start++;
	end = start + strlen(start);
	while (end > start && (end[-1] == ' ' || end[-1] == '\t')) end--;
	*end = '\0';
	return start;
}

// Portable strdup (not a C standard function; avoids feature-test macros).
static char *copy_str(const char *value)
{
	if (!value) return NULL;
	size_t len = strlen(value);
	char  *copy = malloc(len + 1);
	if (copy) memcpy(copy, value, len + 1);
	return copy;
}

// Parse .bep contents held in a NUL-terminated buffer. Returns a project or
// NULL. `buf` is borrowed (not freed); document strings are duplicated.
static bapi_project_t parse_bep_buffer(const char *buf)
{
	bapi_project_t project = (bapi_project_t)calloc(1, sizeof(struct bapi_project_internal));
	if (!project) return NULL;

	const char *cursor = buf;
	int			in_documents = 0;
	while (cursor && *cursor) {
		const char *line_end = strchr(cursor, '\n');
		size_t		length   = line_end ? (size_t)(line_end - cursor) : strlen(cursor);
		if (length > 0 && cursor[length - 1] == '\r') length--;

		char *line = malloc(length + 1);
		if (!line) {
			bapi_project_destroy(project);
			return NULL;
		}
		memcpy(line, cursor, length);
		line[length] = '\0';
		char *value = trim(line);
		if (*value && *value != '#' && *value != ';') {
			if (value[0] == '[' && value[strlen(value) - 1] == ']') {
				in_documents = (strcmp(value, "[documents]") == 0);
			} else if (in_documents) {
				char **grown = (char **)realloc(
					project->documents, (size_t)(project->document_count + 1) * sizeof(char *));
				if (!grown) {
					free(line);
					bapi_project_destroy(project);
					return NULL;
				}
				project->documents = grown;
				project->documents[project->document_count] = copy_str(value);
				if (!project->documents[project->document_count]) {
					free(line);
					bapi_project_destroy(project);
					return NULL;
				}
				project->document_count++;
			} else {
				char *equals = strchr(value, '=');
				if (equals) {
					*equals = '\0';
					char *key	= trim(value);
					char *data	= trim(equals + 1);
					if (strcmp(key, "name") == 0) {
						free(project->name);
						project->name = copy_str(data);
					} else if (strcmp(key, "engine") == 0) {
						free(project->engine);
						project->engine = copy_str(data);
					}
				}
			}
		}
		free(line);
		cursor = line_end ? line_end + 1 : NULL;
	}
	return project;
}

bapi_project_t bapi_project_load_from_bep(const char *filepath)
{
	if (!filepath || !filepath[0]) return NULL;

	FILE *file = fopen(filepath, "rb");
	if (!file) return NULL;
	fseek(file, 0, SEEK_END);
	long size = ftell(file);
	fseek(file, 0, SEEK_SET);
	if (size < 0) {
		fclose(file);
		return NULL;
	}
	char *buf = malloc((size_t)size + 1);
	if (!buf) {
		fclose(file);
		return NULL;
	}
	if (size > 0 && fread(buf, 1, (size_t)size, file) != (size_t)size) {
		free(buf);
		fclose(file);
		return NULL;
	}
	buf[size] = '\0';
	fclose(file);

	bapi_project_t project = parse_bep_buffer(buf);
	free(buf);
	return project;
}

void bapi_project_destroy(bapi_project_t project)
{
	if (!project) return;
	free(project->name);
	free(project->engine);
	for (int i = 0; i < project->document_count; i++) free(project->documents[i]);
	free(project->documents);
	memset(project, 0, sizeof(struct bapi_project_internal));
	free(project);
}

const char *bapi_project_get_name(bapi_project_t project)
{
	return project ? project->name : NULL;
}

const char *bapi_project_get_engine_dir(bapi_project_t project)
{
	return project ? project->engine : NULL;
}

int bapi_project_get_document_count(bapi_project_t project)
{
	return project ? project->document_count : 0;
}

const char *bapi_project_get_document_path(bapi_project_t project, int index)
{
	if (!project || index < 0 || index >= project->document_count) return NULL;
	return project->documents[index];
}
