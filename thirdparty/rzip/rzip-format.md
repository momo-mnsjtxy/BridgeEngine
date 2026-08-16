The definition of RZip format is written in `rz.h`.

```C
struct rz_file_metadata
{
	uint16_t filename_length;
	char *filename;
	uint64_t dcom_size;
	uint64_t comp_size;
	long data_offset;
	uint32_t checksum;
	uint64_t timestamp;

	struct rz_file_metadata *prev;
	struct rz_file_metadata *next;
};

struct rz_index
{
	struct rz_file_metadata *head;
	struct rz_file_metadata *tail;
};

struct rz_header
{
	char magic[2];
	uint16_t version_major;
	uint16_t version_minor;
	uint64_t dcom_size;
	uint64_t comp_size;
	uint32_t file_count;
	uint32_t flags;
	uint32_t index_offset;
};
```

The header totally takes 48 bytes at the start of a RZip file. Bytes are set to `00` except used bytes.
File index is put at the end of file. File index stores the metadata of all the files. RZip manages them with a chainlist.
All the data should be access in little-endian.
