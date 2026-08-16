# RZip

A minimal, cross-platform archive tool written in C99. It packages files into
`.rz` archives with CRC-32 integrity checking. Compression is **not**
implemented yet — files are stored verbatim, so `comp_size == dcom_size`.

The on-disk format is defined in `rzip-format.md` and `rz.h`: a 48-byte
header, the file data, and a file index (a chain of per-file metadata records)
at the end of the archive. All multi-byte values are little-endian.

## Build

Requires CMake >= 3.15 and any C99 compiler (GCC, Clang, MinGW, MSVC).

```sh
cmake -S . -B build
cmake --build build
```

Windows (MSVC):

```powershell
cmake -S . -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
```

This produces the `rzlib` static library and the `rz` executable
(`rz.exe` on Windows) in `build/`.

## Usage

```
Usage: rz <command> [options]

Commands:
  -c <archive> <file>...   Create an archive from files (stored, not compressed)
  -x <archive> [dir]       Extract an archive into dir (default: current directory)
  -l <archive>             List the contents of an archive
  -t <archive>             Test the integrity of an archive
  -h, --help               Show this help
  -v, --version            Show the version
```

Examples:

```sh
rz -c backup.rz a.txt b.bin          # store a.txt and b.bin in backup.rz
rz -l backup.rz                      # list contents (sizes, CRC-32, mtime)
rz -t backup.rz                      # verify structure and all CRC-32 checksums
rz -x backup.rz restore/             # extract into restore/
rz -x backup.rz                      # extract into the current directory
```

Entry names keep the path they were given (e.g. `rz -c a.rz sub/c.txt`
stores `sub/c.txt` and extraction recreates the `sub/` directory).
Directory creation works on Windows and POSIX alike.

## Library

Link against `rzlib` and include `rz_lib.h`:

```c
struct rz_index index = { NULL, NULL };
rz_file_t *f = calloc(1, sizeof *f);
f->filename = strdup("hello.txt");
rz_add_index(f, &index);

rz_create("out.rz", &index, 0);      /* reads hello.txt, fills in sizes/CRC */

rz_t rz = rz_open("out.rz");         /* parse header + index */
rz_decompress_index(rz.index, "outdir", 0);
rz_close(&rz);                       /* free everything */
```

Key functions:

| Function | Description |
|---|---|
| `rz_create(path, index, flags)` | Write an archive from the files named in the index |
| `rz_open(path)` / `rz_close(&rz)` | Open / release an archive |
| `rz_check(path)` | Structural validity (1 = valid) |
| `rz_test(path)` | Structure + CRC-32 of every file (0 = ok) |
| `rz_decompress(...)` / `rz_decompress_index(...)` / `rz_decompress_file(...)` | Extract |
| `rz_decompress_file_by_name(name, target, flags)` | Extract one file, searched in all open archives |
| `rz_read_file(file, offset, count)` | Raw access to a file's stored data (returns a malloc'd buffer) |

`rz_add_index` / `rz_rm_index` / `rz_rm_index_by_name` manage the metadata
chain and return NULL on failure. Getters (`rz_get_file_name`, ...) expose
individual metadata fields.

## Format notes

- Header: 48 bytes — `RZ` magic, version, total sizes, file count, flags,
  index offset; unused bytes are zero.
- Index: at the end of the file, one record per file
  (`filename_length u16 | filename | dcom_size u64 | comp_size u64 |
  data_offset u64 | checksum u32 | timestamp u64`), little-endian.
- `index_offset` is a 32-bit field, so archives larger than 4 GB are not
  supported by this format version.
- `flags` is reserved; pass 0.

## Limitations & security

- Store-only: no compression, no encryption, no symlinks, no permissions.
- Extraction rejects absolute paths, Windows drive letters and `..` path
  components, but the library is not a security boundary — only extract
  archives you trust.
- The library keeps an archive's stream open until `rz_close()`; metadata
  nodes are only valid while their archive is open.
