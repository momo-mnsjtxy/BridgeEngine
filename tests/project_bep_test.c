#include "BridgeEngine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures = 0;

#define CHECK(cond, msg)                                                                           \
	do {                                                                                           \
		if (!(cond)) {                                                                             \
			fprintf(stderr, "FAIL: %s (line %d)\n", msg, __LINE__);                                \
			failures++;                                                                            \
		} else {                                                                                   \
			printf("ok:   %s\n", msg);                                                             \
		}                                                                                          \
	} while (0)

int main(int argc, char *argv[])
{
	const char *out_dir = argc > 1 ? argv[1] : ".";
	char		bep_path[1024];
	snprintf(bep_path, sizeof(bep_path), "%s/roundtrip_test.bep", out_dir);

	FILE *file = fopen(bep_path, "wb");
	if (!file) {
		fprintf(stderr, "cannot write %s\n", bep_path);
		return 1;
	}
	fprintf(file, "; BridgeEngine project file\n");
	fprintf(file, "name = RoundtripDemo\n");
	fprintf(file, "engine = G:/BridgeEngine\n");
	fprintf(file, "\n");
	fprintf(file, "[documents]\n");
	fprintf(file, "ui/menu.xml\n");
	fprintf(file, "ui/game.xml\n");
	fprintf(file, "\n");
	fprintf(file, "# a commented line and a section we ignore\n");
	fprintf(file, "[other]\n");
	fprintf(file, "ignored = true\n");
	fclose(file);

	// NULL / missing file
	bapi_project_t missing = bapi_project_load_from_bep(bep_path);
	CHECK(missing != NULL, "loads existing project file");

	bapi_project_t none = bapi_project_load_from_bep(NULL);
	CHECK(none == NULL, "NULL path rejected");

	bapi_project_t bogus = bapi_project_load_from_bep("no_such_file.bep");
	CHECK(bogus == NULL, "missing file rejected");

	// Contents
	CHECK(missing != NULL, "project handle non-null");
	if (missing) {
		CHECK(strcmp(bapi_project_get_name(missing), "RoundtripDemo") == 0,
			  "name parsed");
		CHECK(strcmp(bapi_project_get_engine_dir(missing), "G:/BridgeEngine") == 0,
			  "engine dir parsed");
		CHECK(bapi_project_get_document_count(missing) == 2, "document count is 2");
		CHECK(strcmp(bapi_project_get_document_path(missing, 0), "ui/menu.xml") == 0,
			  "first document path");
		CHECK(strcmp(bapi_project_get_document_path(missing, 1), "ui/game.xml") == 0,
			  "second document path");
		CHECK(bapi_project_get_document_path(missing, 2) == NULL, "out-of-range index NULL");
		CHECK(bapi_project_get_document_path(missing, -1) == NULL, "negative index NULL");
		bapi_project_destroy(missing);
	}

	// Bounds checks on a NULL handle are safe no-ops.
	CHECK(bapi_project_get_document_count(NULL) == 0, "count on NULL is 0");
	CHECK(bapi_project_get_name(NULL) == NULL, "name on NULL is NULL");

	// destroy(NULL) is safe
	bapi_project_destroy(NULL);

	remove(bep_path);

	printf(failures ? "\n%d FAILURE(S)\n" : "\nALL PASSED\n", failures);
	return failures ? 1 : 0;
}
