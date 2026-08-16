#ifndef INCLUDE_RZ_H_
#define INCLUDE_RZ_H_

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>		// FILE * (internal archive stream, not part of the on-disk format)

#define RZ_HEADER_MAGIC	 "RZ"

#define RZ_VERSION_MAJOR 0
#define RZ_VERSION_MINOR 1
#define RZ_VERSION		 "0.1"

/*	NOTE
 *	As compression is not currently implemented, the 'compressed_size' elements would be same as
 *	the 'decompressed_size' elements.
 */

/* All the data should be access in little-endian */
/* File index should be put at the end of file */

/* Compressed file metadata */
struct rz_file_metadata
{
	uint16_t filename_length;		// Length of file name						2 bytes
	char *filename;					// File name (UTF-8)						variable
	uint64_t dcom_size;				// Original size of decompressed data		8 bytes
	uint64_t comp_size;				// Size of compressed data					8 bytes
	uint64_t data_offset;			// Offset of the start of data				8 bytes
	uint32_t checksum;				// CRC-32 checksum							4 bytes
	uint64_t timestamp;				// UNIX timestamp							8 bytes

	struct rz_file_metadata *prev;	// Previous file
	struct rz_file_metadata *next;	// Next file
	FILE *stream;					// (internal) archive stream, set by rz_open
};

/* File index */
struct rz_index
{
	struct rz_file_metadata *head;	// The first file
	struct rz_file_metadata *tail;	// The last file
};

/* RZip file header */
struct rz_header
{
	char magic[2];					// RZ file magic "RZ"						2 bytes
	uint16_t version_major;			// Major version							2 bytes
	uint16_t version_minor;			// Minor version							2 bytes
	uint64_t dcom_size;				// Original size of decompressed data		8 bytes
	uint64_t comp_size;				// Size of compressed data (RZ file size)	8 bytes
	uint32_t file_count;			// Total file count							4 bytes
	uint32_t flags;					// Optional flags							4 bytes
	uint32_t index_offset;			// Offset of file index						4 bytes

	/* Header fills to 48 bytes */
};

struct rz
{
	struct rz_header *header;
	struct rz_index *index;
	FILE *stream;					// (internal) archive stream, set by rz_open
};

typedef struct rz_file_metadata rz_file_t;
typedef struct rz_index rz_index_t;
typedef struct rz rz_t;

#endif // INCLUDE_RZ_H_
