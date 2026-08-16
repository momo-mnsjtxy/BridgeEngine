#include <BridgeEngine.h>
#include <rz_lib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

static int fails = 0;

static int expect(int condition, const char *message)
{
	if (!condition) {
		fprintf(stderr, "FAIL: %s\n", message);
		fails++;
	}
	return condition;
}

static int test_mkdir(const char *dir)
{
#ifdef _WIN32
	return _mkdir(dir);
#else
	return mkdir(dir, 0755);
#endif
}

static int test_chdir(const char *dir)
{
#ifdef _WIN32
	return _chdir(dir);
#else
	return chdir(dir);
#endif
}

static char *test_strdup(const char *text)
{
	size_t len  = strlen(text) + 1;
	char  *copy = (char *)malloc(len);
	if (copy) memcpy(copy, text, len);
	return copy;
}

static void index_free_all(rz_index_t *index)
{
	rz_file_t *node = index->head;
	while (node) {
		rz_file_t *next = node->next;
		free(node->filename);
		free(node);
		node = next;
	}
	index->head = NULL;
	index->tail = NULL;
}

static rz_file_t *make_entry(const char *name)
{
	rz_file_t *file = (rz_file_t *)calloc(1, sizeof(*file));
	if (!file) return NULL;
	file->filename = test_strdup(name);
	if (!file->filename) {
		free(file);
		return NULL;
	}
	return file;
}

static int write_test_file(const char *name, const unsigned char *data, size_t len)
{
	FILE *f = fopen(name, "wb");
	if (!f) return -1;
	if (len > 0 && fwrite(data, 1, len, f) != len) {
		fclose(f);
		return -1;
	}
	fclose(f);
	return 0;
}

/* Hand-crafted empty pack: 48-byte header, zero entries, index right after
 * the header. All multi-byte values little-endian. */
static int write_empty_pack(const char *path)
{
	unsigned char h[48];
	memset(h, 0, sizeof(h));
	h[0] = 'R';
	h[1] = 'Z';
	h[2] = RZ_VERSION_MAJOR;
	h[3] = 0;
	h[4] = RZ_VERSION_MINOR;
	h[5] = 0;
	h[14] = 48; /* comp_size == whole archive size */
	h[30] = 48; /* index_offset */
	FILE *f = fopen(path, "wb");
	if (!f) return -1;
	int ok = fwrite(h, 1, sizeof(h), f) == sizeof(h);
	fclose(f);
	return ok ? 0 : -1;
}

static void test_pack_a(void)
{
	static const char hello_content[] = "Hello, RZip pack!\n";
	static const unsigned char binary_content[96] = {
		0x00, 0x01, 0x02, 0x7F, 0x80, 0xFF, 0x00, 0x00, 0x41, 0x42, 0x43, 0x00,
		0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	};
	/* remaining bytes zero-filled by the initializer */

	if (write_test_file("hello.txt", (const unsigned char *)hello_content,
						strlen(hello_content)) != 0 ||
		write_test_file("empty.dat", NULL, 0) != 0 ||
		write_test_file("data/binary.bin", binary_content, sizeof(binary_content)) != 0) {
		expect(0, "could not write source files");
		return;
	}

	rz_index_t index = {NULL, NULL};
	rz_file_t *e1 = make_entry("hello.txt");
	rz_file_t *e2 = make_entry("data/binary.bin");
	rz_file_t *e3 = make_entry("empty.dat");
	expect(e1 && e2 && e3 && rz_add_index(e1, &index) && rz_add_index(e2, &index) &&
			   rz_add_index(e3, &index),
		   "index nodes linked");
	expect(rz_create("pack_a.rz", &index, 0) == 0, "rz_create pack_a.rz");
	index_free_all(&index);

	/* open failure cases */
	expect(bapi_pack_open(NULL) == NULL, "open(NULL) fails");
	expect(bapi_pack_open("missing.rz") == NULL, "open(missing) fails");
	expect(bapi_pack_open("garbage.bin") == NULL, "open(garbage) fails");

	bapi_pack_t pack = bapi_pack_open("pack_a.rz");
	expect(pack != NULL, "open pack_a.rz");
	if (!pack) return;

	/* count / enumerate */
	expect(bapi_pack_file_count(pack) == 3, "file_count == 3");
	expect(strcmp(bapi_pack_file_name(pack, 0), "hello.txt") == 0, "name(0)");
	expect(strcmp(bapi_pack_file_name(pack, 1), "data/binary.bin") == 0, "name(1)");
	expect(strcmp(bapi_pack_file_name(pack, 2), "empty.dat") == 0, "name(2)");
	expect(bapi_pack_file_name(pack, -1) == NULL, "name(-1) is NULL");
	expect(bapi_pack_file_name(pack, 3) == NULL, "name(3) is NULL");

	/* find */
	expect(bapi_pack_find_file(pack, "hello.txt") == 0, "find hello.txt");
	expect(bapi_pack_find_file(pack, "data/binary.bin") == 1, "find data/binary.bin");
	expect(bapi_pack_find_file(pack, "empty.dat") == 2, "find empty.dat");
	expect(bapi_pack_find_file(pack, "nope.txt") == -1, "find nope.txt");
	expect(bapi_pack_find_file(pack, NULL) == -1, "find NULL name");

	/* size */
	expect(bapi_pack_file_size(pack, "hello.txt") == (int64_t)strlen(hello_content),
		   "size hello.txt");
	expect(bapi_pack_file_size(pack, "data/binary.bin") == (int64_t)sizeof(binary_content),
		   "size data/binary.bin");
	expect(bapi_pack_file_size(pack, "empty.dat") == 0, "size empty.dat");
	expect(bapi_pack_file_size(pack, "nope.txt") == -1, "size nope.txt");
	expect(bapi_pack_file_size(pack, NULL) == -1, "size NULL name");

	/* read into caller buffer */
	char buffer[128];
	memset(buffer, 0, sizeof(buffer));
	expect(bapi_pack_read_file(pack, "hello.txt", buffer, sizeof(buffer)) ==
			   strlen(hello_content),
		   "read hello.txt byte count");
	expect(memcmp(buffer, hello_content, strlen(hello_content)) == 0, "read hello.txt content");

	unsigned char bin[128];
	expect(bapi_pack_read_file(pack, "data/binary.bin", bin, sizeof(bin)) ==
			   sizeof(binary_content),
		   "read binary byte count");
	expect(memcmp(bin, binary_content, sizeof(binary_content)) == 0, "read binary content");

	char tiny[4];
	expect(bapi_pack_read_file(pack, "hello.txt", tiny, sizeof(tiny)) == sizeof(tiny),
		   "truncated read returns buffer size");
	expect(memcmp(tiny, "Hell", 4) == 0, "truncated read prefix matches");

	expect(bapi_pack_read_file(pack, "hello.txt", buffer, 0) == 0, "read with size 0");
	expect(bapi_pack_read_file(pack, "hello.txt", NULL, sizeof(buffer)) == 0, "read NULL buffer");
	expect(bapi_pack_read_file(pack, "nope.txt", buffer, sizeof(buffer)) == 0, "read nope.txt");
	expect(bapi_pack_read_file(pack, NULL, buffer, sizeof(buffer)) == 0, "read NULL name");
	expect(bapi_pack_read_file(pack, "empty.dat", buffer, sizeof(buffer)) == 0, "read empty file");

	/* read alloc */
	size_t sz = 0;
	unsigned char *alloc = bapi_pack_read_file_alloc(pack, "hello.txt", &sz);
	expect(alloc != NULL && sz == strlen(hello_content), "alloc hello.txt size");
	expect(alloc && memcmp(alloc, hello_content, strlen(hello_content)) == 0,
		   "alloc hello.txt content");
	free(alloc);

	alloc = bapi_pack_read_file_alloc(pack, "empty.dat", &sz);
	expect(alloc != NULL && sz == 0, "alloc empty.dat gives non-NULL empty buffer");
	free(alloc);

	sz = 0xDEAD;
	expect(bapi_pack_read_file_alloc(pack, "nope.txt", &sz) == NULL, "alloc nope.txt fails");
	expect(sz == 0xDEAD, "out_size untouched on failure");
	expect(bapi_pack_read_file_alloc(pack, NULL, &sz) == NULL, "alloc NULL name fails");

	alloc = bapi_pack_read_file_alloc(pack, "hello.txt", NULL);
	expect(alloc != NULL, "alloc with NULL out_size still works");
	free(alloc);

	bapi_pack_close(pack);
	bapi_pack_close(NULL);
}

static void test_pack_duplicates(void)
{
	static const char content_a[] = "first";
	if (write_test_file("dup.txt", (const unsigned char *)content_a, strlen(content_a)) != 0) {
		expect(0, "could not write dup source");
		return;
	}

	rz_index_t index = {NULL, NULL};
	rz_file_t *e1 = make_entry("dup.txt");
	rz_file_t *e2 = make_entry("dup.txt");
	expect(e1 && e2 && rz_add_index(e1, &index) && rz_add_index(e2, &index),
		   "duplicate index nodes linked");
	expect(rz_create("pack_b.rz", &index, 0) == 0, "rz_create pack_b.rz");
	index_free_all(&index);

	bapi_pack_t pack = bapi_pack_open("pack_b.rz");
	expect(pack != NULL, "open pack_b.rz");
	if (!pack) return;
	expect(bapi_pack_file_count(pack) == 2, "dup pack has 2 entries");
	expect(bapi_pack_find_file(pack, "dup.txt") == 0, "find returns first duplicate");
	char buffer[16];
	size_t got = bapi_pack_read_file(pack, "dup.txt", buffer, sizeof(buffer));
	expect(got == strlen(content_a) && memcmp(buffer, content_a, got) == 0,
		   "first duplicate content read");
	bapi_pack_close(pack);
}

static void test_empty_pack(void)
{
	expect(write_empty_pack("empty_pack.rz") == 0, "hand-crafted empty pack written");
	bapi_pack_t pack = bapi_pack_open("empty_pack.rz");
	expect(pack != NULL, "open empty pack");
	if (!pack) return;
	expect(bapi_pack_file_count(pack) == 0, "empty pack file_count == 0");
	expect(bapi_pack_file_name(pack, 0) == NULL, "empty pack name(0) NULL");
	expect(bapi_pack_find_file(pack, "x") == -1, "empty pack find fails");
	expect(bapi_pack_file_size(pack, "x") == -1, "empty pack size fails");
	bapi_pack_close(pack);
}

int main(int argc, char **argv)
{
	char dir[1024];
	if (argc < 2) {
		fprintf(stderr, "usage: %s <build-dir>\n", argv[0]);
		return 1;
	}
	snprintf(dir, sizeof(dir), "%s/packtest", argv[1]);
	test_mkdir(dir);
	snprintf(dir, sizeof(dir), "%s/packtest/data", argv[1]);
	test_mkdir(dir);

	snprintf(dir, sizeof(dir), "%s/packtest", argv[1]);
	if (test_chdir(dir) != 0) {
		fprintf(stderr, "cannot chdir to %s\n", dir);
		return 1;
	}

	if (write_test_file("garbage.bin", (const unsigned char *)"not a pack at all", 18) != 0) {
		fprintf(stderr, "cannot write garbage.bin\n");
		return 1;
	}

	test_pack_a();
	test_pack_duplicates();
	test_empty_pack();

	return fails ? 1 : 0;
}
