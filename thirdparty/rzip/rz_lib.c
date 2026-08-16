/* RZip library
 *
 * On-disk layout (all multi-byte values little-endian):
 *   [ 48-byte header ][ file 0 data ][ file 1 data ] ... [ file index ... ]
 *
 * The file index is a chain of entries, one per file, each:
 *   filename_length (u16) | filename (UTF-8) | dcom_size (u64) | comp_size (u64)
 *   | data_offset (u64) | checksum (u32, CRC-32) | timestamp (u64)
 *
 * Compression is not implemented yet: data is stored verbatim, so comp_size
 * equals dcom_size and "decompression" is a copy with CRC-32 verification.
 */

#include "rz_lib.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#ifdef _WIN32
#include <direct.h>		/* _mkdir */
#include <sys/stat.h>	/* _stat64 */
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

/* ------------------------------------------------------------------ */
/* Little-endian encode/decode helpers                                 */
/* ------------------------------------------------------------------ */

static void le16_encode(uint8_t *p, uint16_t v) { p[0] = (uint8_t)(v & 0xFF); p[1] = (uint8_t)(v >> 8); }
static void le32_encode(uint8_t *p, uint32_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24); }
static void le64_encode(uint8_t *p, uint64_t v)
{
	for (int i = 0; i < 8; i++) p[i] = (uint8_t)(v >> (8 * i));
}
static uint16_t le16_decode(const uint8_t *p) { return (uint16_t)(p[0] | ((uint16_t)p[1] << 8)); }
static uint32_t le32_decode(const uint8_t *p)
{
	return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static uint64_t le64_decode(const uint8_t *p)
{
	uint64_t v = 0;
	for (int i = 7; i >= 0; i--) v = (v << 8) | p[i];
	return v;
}

static int le16_write(FILE *f, uint16_t v) { uint8_t b[2]; le16_encode(b, v); return fwrite(b, 1, 2, f) == 2 ? 0 : -1; }
static int le32_write(FILE *f, uint32_t v) { uint8_t b[4]; le32_encode(b, v); return fwrite(b, 1, 4, f) == 4 ? 0 : -1; }
static int le64_write(FILE *f, uint64_t v) { uint8_t b[8]; le64_encode(b, v); return fwrite(b, 1, 8, f) == 8 ? 0 : -1; }

/* 64-bit stream position, so large archives work on Windows (LLP64) too */
#ifdef _WIN32
#define rz_fseek64 _fseeki64
#define rz_ftell64 _ftelli64
#else
#define rz_fseek64 fseeko
#define rz_ftell64 ftello
#endif

/* ------------------------------------------------------------------ */
/* CRC-32 (IEEE 802.3, reflected polynomial 0xEDB88320)                */
/* ------------------------------------------------------------------ */

static uint32_t crc_table[256];
static int crc_table_ready = 0;

static void crc_table_init(void)
{
	for (uint32_t i = 0; i < 256; i++) {
		uint32_t c = i;
		for (int k = 0; k < 8; k++)
			c = (c & 1u) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
		crc_table[i] = c;
	}
	crc_table_ready = 1;
}

/* Incremental CRC-32: start with crc32_init(), feed data, finish with crc32_fin() */
static uint32_t crc32_init(void) { return 0xFFFFFFFFu; }
static uint32_t crc32_add(uint32_t crc, const uint8_t *buf, size_t len)
{
	if (!crc_table_ready) crc_table_init();
	for (size_t i = 0; i < len; i++)
		crc = crc_table[(crc ^ buf[i]) & 0xFFu] ^ (crc >> 8);
	return crc;
}
static uint32_t crc32_fin(uint32_t crc) { return ~crc; }

/* ------------------------------------------------------------------ */
/* Small cross-platform helpers                                        */
/* ------------------------------------------------------------------ */

static char *rz_strdup(const char *s)
{
	size_t n = strlen(s) + 1;
	char *d = malloc(n);
	if (d) memcpy(d, s, n);
	return d;
}

/* File modification time as a UNIX timestamp (seconds since the epoch) */
static uint64_t rz_mtime(const char *path)
{
#ifdef _WIN32
	struct _stat64 st;
	if (_stat64(path, &st) == 0) return (uint64_t)st.st_mtime;
#else
	struct stat st;
	if (stat(path, &st) == 0) return (uint64_t)st.st_mtime;
#endif
	return (uint64_t)time(NULL);
}

/* Create a directory and all missing parents. Returns 0 on success. */
static int rz_mkdirs(const char *path)
{
	if (!path || path[0] == '\0') return -1;
	if (!strcmp(path, ".") || !strcmp(path, "/")) return 0;

	char tmp[4096];
	size_t len = strlen(path);
	if (len >= sizeof tmp) return -1;
	memcpy(tmp, path, len + 1);

	/* strip trailing separators */
	while (len > 0 && (tmp[len - 1] == '/' || tmp[len - 1] == '\\'))
		tmp[--len] = '\0';
	if (len == 0) return 0;

	/* create each intermediate component */
	for (size_t i = 0; i < len; i++) {
		if (tmp[i] == '/' || tmp[i] == '\\') {
			tmp[i] = '\0';
#ifdef _WIN32
			_mkdir(tmp);	/* errors ignored: the component may already exist */
#else
			mkdir(tmp, 0755);
#endif
			tmp[i] = '/';
		}
	}
	/* create the final component */
#ifdef _WIN32
	if (_mkdir(tmp) != 0 && errno != EEXIST) return -1;
#else
	if (mkdir(tmp, 0755) != 0 && errno != EEXIST) return -1;
#endif
	return 0;
}

/* Parent directory of a path, e.g. "out/sub/a.txt" -> "out/sub" */
static void rz_parent_dir(const char *path, char *out, size_t outsz)
{
	if (outsz == 0) return;
	size_t len = strlen(path);
	if (len >= outsz) len = outsz - 1;
	memcpy(out, path, len);
	out[len] = '\0';
	char *slash = strrchr(out, '/');
	char *bs = strrchr(out, '\\');
	char *sep = (bs && (!slash || bs > slash)) ? bs : slash;
	if (sep) {
		if (sep != out) *sep = '\0';
		else out[1] = '\0';	/* "/name" -> parent "/" */
	} else {
		out[0] = '.';
		out[1] = '\0';
	}
}

/* Refuse names that could escape the extraction target directory */
static int rz_name_is_safe(const char *name)
{
	if (!name || name[0] == '\0') return 0;
	if (name[0] == '/' || name[0] == '\\') return 0;
#ifdef _WIN32
	if (strchr(name, ':') != NULL) return 0;	/* drive letters, alternate streams */
#endif
	const char *p = name;
	for (;;) {
		const char *slash = strchr(p, '/');
		const char *bs = strchr(p, '\\');
		const char *end = NULL;
		if (slash && (!bs || slash < bs)) end = slash; else end = bs;
		size_t clen = end ? (size_t)(end - p) : strlen(p);
		if (clen == 2 && p[0] == '.' && p[1] == '.') return 0;	/* ".." escapes */
		if (!end) break;
		p = end + 1;
	}
	return 1;
}

/* ------------------------------------------------------------------ */
/* Header and index parsing                                            */
/* ------------------------------------------------------------------ */

/* Parsed header plus the actual file size, with validation flags */
struct rz_header_info
{
	int ok;
	uint16_t version_major;
	uint16_t version_minor;
	uint64_t dcom_size;
	uint64_t comp_size;
	uint32_t file_count;
	uint32_t flags;
	uint32_t index_offset;
	long long file_size;
};

static void header_info_read(FILE *f, struct rz_header_info *hi)
{
	memset(hi, 0, sizeof *hi);
	if (rz_fseek64(f, 0, SEEK_END) != 0) return;
	hi->file_size = rz_ftell64(f);
	if (hi->file_size < 48) return;
	if (rz_fseek64(f, 0, SEEK_SET) != 0) return;

	uint8_t h[48];
	if (fread(h, 1, 48, f) != 48) return;
	if (h[0] != 'R' || h[1] != 'Z') return;
	hi->version_major = le16_decode(h + 2);
	hi->version_minor = le16_decode(h + 4);
	if (hi->version_major != RZ_VERSION_MAJOR) return;	/* incompatible format */

	/* NOTE: the on-disk header is packed (no C-struct padding).
	 * The struct's alignment padding is *not* part of the format. */
	hi->dcom_size = le64_decode(h + 6);
	hi->comp_size = le64_decode(h + 14);
	hi->file_count = le32_decode(h + 22);
	hi->flags = le32_decode(h + 26);
	hi->index_offset = le32_decode(h + 30);

	if ((uint64_t)hi->file_size != hi->comp_size) return;	/* comp_size == archive size */
	if (hi->index_offset < 48) return;
	hi->ok = 1;
}

static int header_write(FILE *f, const struct rz_header *h)
{
	uint8_t b[48];
	memset(b, 0, sizeof b);
	b[0] = 'R';
	b[1] = 'Z';
	le16_encode(b + 2, h->version_major);
	le16_encode(b + 4, h->version_minor);
	/* packed on disk: no struct-alignment padding */
	le64_encode(b + 6, h->dcom_size);
	le64_encode(b + 14, h->comp_size);
	le32_encode(b + 22, h->file_count);
	le32_encode(b + 26, h->flags);
	le32_encode(b + 30, h->index_offset);
	return fwrite(b, 1, 48, f) == 48 ? 0 : -1;
}

/* One index entry on disk (packed, no C-struct padding), total = 38 + nlen:
 * nlen(2) | filename(nlen) | dcom(8) | comp(8) | data_offset(8) | crc(4) | ts(8) */
#define RZ_INDEX_ENTRY_FIXED 38		/* entry size minus the filename */

/* Read one index entry located at `off`. Returns NULL if it is invalid. */
static rz_file_t *node_read(FILE *f, uint64_t off, long long file_size, uint64_t index_offset)
{
	if (off + RZ_INDEX_ENTRY_FIXED > (uint64_t)file_size) return NULL;
	if (rz_fseek64(f, (int64_t)off, SEEK_SET) != 0) return NULL;

	uint8_t nl[2];
	if (fread(nl, 1, 2, f) != 2) return NULL;
	uint16_t nlen = le16_decode(nl);
	if (nlen == 0) return NULL;
	if (off + RZ_INDEX_ENTRY_FIXED + nlen > (uint64_t)file_size) return NULL;

	/* the filename comes right after the length, before the fixed fields */
	char *name = malloc((size_t)nlen + 1);
	if (!name) return NULL;
	if (fread(name, 1, nlen, f) != nlen) { free(name); return NULL; }
	name[nlen] = '\0';

	uint8_t b[36];
	if (fread(b, 1, 36, f) != 36) { free(name); return NULL; }
	uint64_t dcom = le64_decode(b);
	uint64_t comp = le64_decode(b + 8);
	uint64_t doff = le64_decode(b + 16);
	uint32_t crc = le32_decode(b + 24);
	uint64_t ts = le64_decode(b + 28);

	/* data must lie inside the archive, before the index */
	if (doff < 48 || doff + comp > index_offset || doff + comp > (uint64_t)file_size) {
		free(name);
		return NULL;
	}

	rz_file_t *n = calloc(1, sizeof *n);
	if (!n) { free(name); return NULL; }
	n->filename_length = nlen;
	n->filename = name;
	n->dcom_size = dcom;
	n->comp_size = comp;
	n->data_offset = doff;
	n->checksum = crc;
	n->timestamp = ts;
	return n;
}

static int index_entry_write(FILE *f, const rz_file_t *n)
{
	if (le16_write(f, n->filename_length) != 0) return -1;
	if (fwrite(n->filename, 1, n->filename_length, f) != n->filename_length) return -1;
	if (le64_write(f, n->dcom_size) != 0) return -1;
	if (le64_write(f, n->comp_size) != 0) return -1;
	if (le64_write(f, n->data_offset) != 0) return -1;
	if (le32_write(f, n->checksum) != 0) return -1;
	if (le64_write(f, n->timestamp) != 0) return -1;
	return 0;
}

static void index_free(struct rz_index *index)
{
	if (!index) return;
	for (rz_file_t *n = index->head; n;) {
		rz_file_t *next = n->next;
		free(n->filename);
		free(n);
		n = next;
	}
	free(index);
}

/* Read the whole index chain; sets *ok = 1 on success. */
static struct rz_index *index_read(FILE *f, const struct rz_header_info *hi, int *ok)
{
	*ok = 0;
	struct rz_index *index = calloc(1, sizeof *index);
	if (!index) return NULL;

	uint64_t off = hi->index_offset;
	for (uint32_t i = 0; i < hi->file_count; i++) {
		rz_file_t *n = node_read(f, off, hi->file_size, hi->index_offset);
		if (!n) { index_free(index); return NULL; }
		if (index->tail) index->tail->next = n; else index->head = n;
		n->prev = index->tail;
		index->tail = n;
		off += RZ_INDEX_ENTRY_FIXED + (uint64_t)n->filename_length;
	}
	if (off > (uint64_t)hi->file_size) { index_free(index); return NULL; }
	*ok = 1;
	return index;
}

/* ------------------------------------------------------------------ */
/* Registry of open archives (for the by-name operations)              */
/* ------------------------------------------------------------------ */

struct rz_archive
{
	char *path;			/* archive path */
	rz_t rz;			/* header/index/stream of the open archive */
	struct rz_archive *next;
};

static struct rz_archive *archives = NULL;

static void archive_register(const char *path, rz_t rz)
{
	struct rz_archive *a = calloc(1, sizeof *a);
	if (!a) return;		/* by-name operations just won't find this archive */
	a->path = rz_strdup(path);
	a->rz = rz;
	a->next = archives;
	archives = a;
}

static struct rz_archive *archive_find(rz_t rz)
{
	for (struct rz_archive *a = archives; a; a = a->next)
		if (a->rz.header == rz.header) return a;
	return NULL;
}

static void archive_unregister(struct rz_archive *a)
{
	struct rz_archive **p = &archives;
	while (*p && *p != a) p = &(*p)->next;
	if (*p) *p = a->next;
	free(a->path);
	free(a);
}

/* ------------------------------------------------------------------ */
/* Public API                                                          */
/* ------------------------------------------------------------------ */

int rz_check(const char *path)
{
	FILE *f = fopen(path, "rb");
	if (!f) return 0;
	struct rz_header_info hi;
	header_info_read(f, &hi);
	if (!hi.ok) { fclose(f); return 0; }
	int ok = 0;
	struct rz_index *index = index_read(f, &hi, &ok);
	index_free(index);
	fclose(f);
	return ok ? 1 : 0;
}

rz_t rz_open(const char *path)
{
	rz_t rz = { NULL, NULL, NULL };
	FILE *f = fopen(path, "rb");
	if (!f) return rz;

	struct rz_header_info hi;
	header_info_read(f, &hi);
	if (!hi.ok) { fclose(f); return rz; }

	int ok = 0;
	struct rz_index *index = index_read(f, &hi, &ok);
	if (!ok) { fclose(f); index_free(index); return rz; }

	struct rz_header *header = malloc(sizeof *header);
	if (!header) { fclose(f); index_free(index); return rz; }
	header->magic[0] = 'R';
	header->magic[1] = 'Z';
	header->version_major = hi.version_major;
	header->version_minor = hi.version_minor;
	header->dcom_size = hi.dcom_size;
	header->comp_size = hi.comp_size;
	header->file_count = hi.file_count;
	header->flags = hi.flags;
	header->index_offset = hi.index_offset;

	for (rz_file_t *n = index->head; n; n = n->next)
		n->stream = f;

	rz.header = header;
	rz.index = index;
	rz.stream = f;
	archive_register(path, rz);
	return rz;
}

void rz_close(rz_t *rz)
{
	if (!rz || !rz->header) return;
	struct rz_archive *a = archive_find(*rz);
	if (a) archive_unregister(a);
	if (rz->stream) fclose(rz->stream);
	index_free(rz->index);
	free(rz->header);
	rz->header = NULL;
	rz->index = NULL;
	rz->stream = NULL;
}

int rz_create(const char *path, rz_index_t *index, uint32_t flags)
{
	if (!index || !index->head) return -1;

	FILE *f = fopen(path, "wb");
	if (!f) return -1;

	struct rz_header h;
	memset(&h, 0, sizeof h);
	h.magic[0] = 'R';
	h.magic[1] = 'Z';
	h.version_major = RZ_VERSION_MAJOR;
	h.version_minor = RZ_VERSION_MINOR;
	h.flags = flags;

	/* placeholder header, rewritten at the end */
	uint8_t zero[48];
	memset(zero, 0, sizeof zero);
	zero[0] = 'R';
	zero[1] = 'Z';
	le16_encode(zero + 2, h.version_major);
	le16_encode(zero + 4, h.version_minor);
	if (fwrite(zero, 1, 48, f) != 48) { fclose(f); remove(path); return -1; }

	int fail = 0;
	uint64_t total_dcom = 0;
	uint32_t count = 0;
	uint8_t buf[65536];

	for (rz_file_t *n = index->head; n; n = n->next) {
		size_t nlen = strlen(n->filename);
		if (nlen == 0 || nlen > UINT16_MAX) { fail = 1; break; }
		n->filename_length = (uint16_t)nlen;

		FILE *in = fopen(n->filename, "rb");
		if (!in) { fail = 1; break; }
		n->data_offset = (uint64_t)rz_ftell64(f);
		uint32_t crc = crc32_init();
		uint64_t sz = 0;
		size_t r;
		while ((r = fread(buf, 1, sizeof buf, in)) > 0) {
			if (fwrite(buf, 1, r, f) != r) { fclose(in); fail = 1; break; }
			crc = crc32_add(crc, buf, r);
			sz += r;
		}
		if (ferror(in)) fail = 1;
		fclose(in);
		if (fail) break;

		n->dcom_size = sz;
		n->comp_size = sz;		/* no compression: data stored verbatim */
		n->checksum = crc32_fin(crc);
		n->timestamp = rz_mtime(n->filename);
		n->stream = NULL;
		total_dcom += sz;
		count++;
	}
	if (fail) { fclose(f); remove(path); return -1; }

	h.dcom_size = total_dcom;
	h.file_count = count;

	uint64_t index_off = (uint64_t)rz_ftell64(f);
	if (index_off > UINT32_MAX) { fclose(f); remove(path); return -1; }
	h.index_offset = (uint32_t)index_off;

	for (rz_file_t *n = index->head; n; n = n->next) {
		if (index_entry_write(f, n) != 0) { fclose(f); remove(path); return -1; }
	}
	h.comp_size = (uint64_t)rz_ftell64(f);

	if (rz_fseek64(f, 0, SEEK_SET) != 0) { fclose(f); remove(path); return -1; }
	if (header_write(f, &h) != 0) { fclose(f); remove(path); return -1; }
	if (fclose(f) != 0) { remove(path); return -1; }
	return 0;
}

/* These functions keep the `int *` return type declared in rz_lib.h:
 * NULL means failure, any non-NULL value means success. */
int *rz_add_index(rz_file_t *file, rz_index_t *index)
{
	if (!file || !index) return NULL;
	file->prev = index->tail;
	file->next = NULL;
	if (index->tail) index->tail->next = file; else index->head = file;
	index->tail = file;
	return (int *)(void *)file;
}

int *rz_rm_index(rz_file_t *file, rz_index_t *index)
{
	if (!file || !index) return NULL;
	if (file->prev) file->prev->next = file->next; else index->head = file->next;
	if (file->next) file->next->prev = file->prev; else index->tail = file->prev;
	file->prev = NULL;
	file->next = NULL;
	return (int *)(void *)file;		/* the caller still owns the node */
}

int *rz_rm_index_by_name(const char *name, rz_index_t *index)
{
	if (!name || !index) return NULL;
	for (rz_file_t *n = index->head; n; n = n->next) {
		if (strcmp(n->filename, name) == 0)
			return rz_rm_index(n, index);
	}
	return NULL;
}

int rz_decompress_file(rz_file_t *file, const char *target, uint32_t flags)
{
	(void)flags;	/* reserved */
	if (!file || !target || !file->stream) return -1;
	if (file->comp_size != file->dcom_size) return -1;	/* compressed data not supported yet */
	if (!rz_name_is_safe(file->filename)) return -1;

	char parent[4096];
	rz_parent_dir(target, parent, sizeof parent);
	if (rz_mkdirs(parent) != 0) return -1;

	FILE *out = fopen(target, "wb");
	if (!out) return -1;
	if (rz_fseek64(file->stream, (int64_t)file->data_offset, SEEK_SET) != 0) {
		fclose(out);
		remove(target);
		return -1;
	}

	int rc = 0;
	uint64_t left = file->comp_size;
	uint32_t crc = crc32_init();
	uint8_t buf[65536];
	while (left > 0) {
		size_t want = left > sizeof buf ? sizeof buf : (size_t)left;
		size_t got = fread(buf, 1, want, file->stream);
		if (got == 0) { rc = -1; break; }
		if (fwrite(buf, 1, got, out) != got) { rc = -1; break; }
		crc = crc32_add(crc, buf, got);
		left -= got;
	}
	if (fclose(out) != 0 && rc == 0) rc = -1;
	if (rc == 0 && crc32_fin(crc) != file->checksum) rc = -1;	/* checksum mismatch */
	if (rc != 0) {
		remove(target);
		return -1;
	}
	return 0;
}

int rz_decompress_index(rz_index_t *index, const char *targetd, uint32_t flags)
{
	if (!index || !targetd || targetd[0] == '\0') return -1;
	if (rz_mkdirs(targetd) != 0) return -1;
	int rc = 0;
	for (rz_file_t *n = index->head; n; n = n->next) {
		char target[4096];
		int len = snprintf(target, sizeof target, "%s/%s", targetd, n->filename);
		if (len < 0 || (size_t)len >= sizeof target) { rc = -1; continue; }
		if (rz_decompress_file(n, target, flags) != 0) rc = -1;
	}
	return rc;
}

int rz_decompress(const char *file, const char *targetd, uint32_t flags)
{
	rz_t rz = rz_open(file);
	if (!rz.header) return -1;
	int rc = rz_decompress_index(rz.index, targetd, flags);
	rz_close(&rz);
	return rc;
}

int rz_decompress_file_by_name(const char *name, const char *target, uint32_t flags)
{
	if (!name) return -1;
	for (struct rz_archive *a = archives; a; a = a->next) {
		for (rz_file_t *n = a->rz.index->head; n; n = n->next) {
			if (strcmp(n->filename, name) == 0)
				return rz_decompress_file(n, target, flags);
		}
	}
	return -1;
}

int rz_test(const char *path)
{
	FILE *f = fopen(path, "rb");
	if (!f) return -1;

	struct rz_header_info hi;
	header_info_read(f, &hi);
	if (!hi.ok) { fclose(f); return -1; }
	int ok = 0;
	struct rz_index *index = index_read(f, &hi, &ok);
	if (!ok) { fclose(f); index_free(index); return -1; }

	int bad = 0;
	uint8_t buf[65536];
	for (rz_file_t *n = index->head; n && !bad; n = n->next) {
		if (rz_fseek64(f, (int64_t)n->data_offset, SEEK_SET) != 0) { bad = 1; break; }
		uint64_t left = n->comp_size;
		uint32_t crc = crc32_init();
		while (left > 0) {
			size_t want = left > sizeof buf ? sizeof buf : (size_t)left;
			size_t got = fread(buf, 1, want, f);
			if (got == 0) { bad = 1; break; }
			crc = crc32_add(crc, buf, got);
			left -= got;
		}
		if (!bad && crc32_fin(crc) != n->checksum) bad = 1;
	}
	index_free(index);
	fclose(f);
	return bad ? -1 : 0;
}

uint16_t rz_get_file_length(rz_file_t *file) { return file ? file->filename_length : 0; }
char *rz_get_file_name(rz_file_t *file) { return file ? file->filename : NULL; }
uint64_t rz_get_file_decomp_size(rz_file_t *file) { return file ? file->dcom_size : 0; }
uint64_t rz_get_file_comp_size(rz_file_t *file) { return file ? file->comp_size : 0; }
uint32_t rz_get_file_check_sum(rz_file_t *file) { return file ? file->checksum : 0; }
uint64_t rz_get_file_timestamp(rz_file_t *file) { return file ? file->timestamp : 0; }

uint8_t *rz_read_file(rz_file_t *file, long offset, int count)
{
	if (!file || !file->stream || count < 0) return NULL;
	uint8_t *buf = malloc(count > 0 ? (size_t)count : 1);
	if (!buf) return NULL;
	if (count == 0) return buf;
	if (rz_fseek64(file->stream, (int64_t)file->data_offset + (int64_t)offset, SEEK_SET) != 0) {
		free(buf);
		return NULL;
	}
	if (fread(buf, 1, (size_t)count, file->stream) != (size_t)count) {
		free(buf);
		return NULL;
	}
	return buf;
}
