#ifndef INCLUDE_RZ_LIB_H_
#define INCLUDE_RZ_LIB_H_

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include "rz.h"

/* RZip .rz file */
int rz_check(const char *path);		// Check structural validity: 1 = valid, 0 = invalid or unreadable
rz_t rz_open(const char *path);		// Open a .rz file and get the structure.
									// Returns rz_t with header == NULL on failure.
									// The archive stays open (metadata nodes point at its stream);
									// call rz_close() when done.
void rz_close(rz_t *rz);			// Release an archive opened with rz_open: frees memory and closes the stream
int rz_create(const char *path, rz_index_t *index, uint32_t flags);	// Create a .rz file.
									// Reads every file named in the index, computes sizes/CRC-32/
									// timestamps, and fills them into the metadata nodes.
									// Returns 0 on success, -1 on failure. flags is reserved (pass 0).
int *rz_add_index(rz_file_t *file, rz_index_t *index);					// Append a metadata node to the index.
									// Returns the file on success, NULL on failure.
int *rz_rm_index(rz_file_t *file, rz_index_t *index);					// Unlink a metadata node from the index.
									// Returns the file (caller still owns it) or NULL if not linked.
int *rz_rm_index_by_name(const char *name, rz_index_t *index);			// Unlink the first metadata node whose
									// filename matches. Returns the removed file or NULL if not found.
int rz_decompress(const char *file, const char *targetd, uint32_t flags);		// Decompress a .rz file to a target directory.
int rz_decompress_index(rz_index_t *index, const char *targetd, uint32_t flags);// Decompress every file of an opened index.
int rz_decompress_file(rz_file_t *file, const char *target, uint32_t flags);	// Decompress one file to a target path.
int rz_decompress_file_by_name(const char *name, const char *target, uint32_t flags); // Decompress one file by name,
									// looked up in every archive currently open with rz_open().
int rz_test(const char *path);		// Full integrity test (structure + CRC-32 of every file):
									// 0 = ok, -1 = invalid or corrupt

uint16_t rz_get_file_length(rz_file_t *file);
char *rz_get_file_name(rz_file_t *file);
uint64_t rz_get_file_decomp_size(rz_file_t *file);
uint64_t rz_get_file_comp_size(rz_file_t *file);
uint32_t rz_get_file_check_sum(rz_file_t *file);
uint64_t rz_get_file_timestamp(rz_file_t *file);

/* Read raw file data from the archive, starting `offset` bytes into the file's data.
 * Returns a malloc'd buffer of `count` bytes (free it), or NULL on failure. */
uint8_t *rz_read_file(rz_file_t *file, long offset, int count);

#endif // INCLUDE_RZ_LIB_H
